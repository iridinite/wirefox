using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Text;
using System.Threading;

namespace Iridinite.Wirefox.Battleships {

    internal static class Server {

        private class Player {
            public PeerID NetworkID { get; set; } = PeerID.Invalid;
            public Board Board { get; } = new Board();
            public bool IsPlayerOne { get; set; }
        }

        public const ushort GamePort = 55010;

        private static Peer peer;
        private static Player player1, player2;
        private static Random rng = new Random();
        private static bool whoseTurn;
        private static bool shotLaunchedThisTurn;
        private static bool gameStarted;

        private static Thread worker;
        private static volatile bool workerStop = true;
        private static readonly Dictionary<PacketCommand, MessageReceivedHandler> packetHandlers;

        private static readonly byte[] EmptyPayload = new byte[0];

        static Server() {
            // Register handlers for each of the possible PacketCommands.
            // This is a bit more elegant than a massive switch case, I think.
            packetHandlers = new Dictionary<PacketCommand, MessageReceivedHandler> {
                {PacketCommand.NOTIFY_CONNECTION_INCOMING, OnPlayerConnect},
                {PacketCommand.NOTIFY_CONNECTION_LOST, OnPlayerDisconnect},
                {PacketCommand.NOTIFY_DISCONNECTED, OnPlayerDisconnect},
                {(PacketCommand) GameCommand.PlaceShip, OnPlayerPlaceShip},
                {(PacketCommand) GameCommand.Shoot, OnPlayerShoot},
                {(PacketCommand) GameCommand.Chat, OnPlayerChat}
            };
        }

        public static void Initialize() {
            if (IsActive()) return;

            workerStop = false;
            gameStarted = false;
            shotLaunchedThisTurn = false;
            player1 = new Player { IsPlayerOne = true };
            player2 = new Player { IsPlayerOne = false };

            peer = new Peer(2);
            peer.Bind(SocketProtocol.IPv4, GamePort);
            peer.SetMaxIncomingPeers(2);

            // unfortunately we don't have a proper game loop like a 3D renderer does, so we'll use a thread instead
            worker = new Thread(ThreadWorker) {
                IsBackground = true
            };
            worker.Start();
        }

        public static bool IsActive() {
            return !workerStop;
        }

        public static void Stop() {
            if (!IsActive()) return;

            workerStop = true;
            worker.Join();
            worker = null;

            peer.Dispose();
            peer = null;
        }

        private static Player GetPlayerForPeerID(PeerID who) {
            if (player1.NetworkID == who) return player1;
            if (player2.NetworkID == who) return player2;
            return null;
        }

        private static Player GetOtherPlayer(Player first) {
            return first.NetworkID == player1.NetworkID ? player2 : player1;
        }

        private static void ThreadWorker() {
            while (!workerStop) {
                var recv = peer.Receive();
                while (recv != null) {
                    // Technically the check isn't needed, but this prevents a malicious packet from throwing
                    if (packetHandlers.ContainsKey(recv.GetCommand()))
                        packetHandlers[recv.GetCommand()].Invoke(recv);

                    // Keep checking until the queue is empty
                    recv = peer.Receive();
                }

                Thread.Sleep(1);
            }
        }

        private static void OnPlayerConnect(Packet recv) {
            var remoteID = recv.GetSender();
            if (player1.NetworkID == PeerID.Invalid) {
                player1.NetworkID = remoteID;
                return;
            }

            player2.NetworkID = remoteID;

            // We now have two players! Begin the game session.
            using (var beginSessionCmd = new Packet((PacketCommand) GameCommand.BeginSession, EmptyPayload)) {
                peer.Send(beginSessionCmd, player1.NetworkID, PacketOptions.RELIABLE);
                peer.Send(beginSessionCmd, player2.NetworkID, PacketOptions.RELIABLE);
            }
        }

        private static void BeginNextTurn() {
            shotLaunchedThisTurn = false;
            whoseTurn = !whoseTurn;

            using (var packet = new Packet((PacketCommand) GameCommand.YourTurn, EmptyPayload)) {
                peer.Send(packet, whoseTurn ? player1.NetworkID : player2.NetworkID, PacketOptions.RELIABLE);
            }

            using (var packet = new Packet((PacketCommand) GameCommand.OpponentTurn, EmptyPayload)) {
                peer.Send(packet, whoseTurn ? player2.NetworkID : player1.NetworkID, PacketOptions.RELIABLE);
            }
        }

        private static void OnPlayerDisconnect(Packet recv) { }

        private static void OnPlayerChat(Packet recv) {
            var player = GetPlayerForPeerID(recv.GetSender());
            if (player == null) return;

            // forward the chat message to the other player
            peer.Send(recv, GetOtherPlayer(player).NetworkID, PacketOptions.RELIABLE);
        }

        private static void OnPlayerPlaceShip(Packet recv) {
            if (gameStarted) return;

            var player = GetPlayerForPeerID(recv.GetSender());
            if (player == null) return;

            using (var instream = recv.GetStream()) {
                // reassemble the ship's settings from the stream data
                var type = (ShipType) instream.ReadByte();
                var ori = (ShipOrientation) instream.ReadByte();
                var pos = new BoardPosition(instream.ReadByte(), instream.ReadByte());
                var ship = new Ship(type) {
                    Position = pos,
                    Orientation = ori
                };

                bool acceptable = false;
                if (player.Board.CanPlaceShip(ship)) {
                    // the player sent a valid request, so honor it
                    player.Board.PlaceShip(type, ship);
                    acceptable = true;
                }

                // send out the response, with the ship type in the payload
                using (var ms = new MemoryStream()) {
                    using (var outstream = new BinaryWriter(ms, Encoding.UTF8)) {
                        outstream.Write((byte) type);
                        outstream.Write(acceptable);

                        using (var packet = new Packet((PacketCommand) GameCommand.PlaceShip, ms.ToArray()))
                            peer.Send(packet, player.NetworkID, PacketOptions.RELIABLE);
                    }
                }
            }

            // all ships placed?
            bool p1done = player1.Board.Ships.Count == 5;
            bool p2done = player2.Board.Ships.Count == 5;
            if (p1done && p2done) {
                // instruct both players that the game is now starting
                using (var packet = new Packet((PacketCommand) GameCommand.BeginGame, EmptyPayload)) {
                    peer.Send(packet, player1.NetworkID, PacketOptions.RELIABLE);
                    peer.Send(packet, player2.NetworkID, PacketOptions.RELIABLE);
                }

                whoseTurn = rng.Next() % 2 == 0;
                gameStarted = true;
                BeginNextTurn();
            }
        }

        private static void OnPlayerShoot(Packet recv) {
            // trivial safety measure
            if (!gameStarted) return;

            // only the current player is allowed to place shots
            var player = GetPlayerForPeerID(recv.GetSender());
            if (player == null || player.IsPlayerOne != whoseTurn) return;

            // to prevent a player from firing multiple shots per turn
            if (shotLaunchedThisTurn) return;
            shotLaunchedThisTurn = true;

            var opponent = GetOtherPlayer(player); // whoseTurn ? player2 : player1;
            Debug.Assert(player != opponent);

            using (var instream = recv.GetStream()) {
                var acceptable = true;
                var pos = new BoardPosition(instream.ReadByte(), instream.ReadByte());
                ShotResult result = 0;

                // Reply to player who placed the shot
                using (var ms = new MemoryStream()) {
                    using (var outstream = new BinaryWriter(ms, Encoding.UTF8)) {
                        outstream.Write((byte) pos.x);
                        outstream.Write((byte) pos.y);

                        if (!pos.IsWithinBoard() || opponent.Board.Shots.ContainsKey(pos)) {
                            // this shot is invalid, it's either outside the board, or already been shot at before
                            shotLaunchedThisTurn = false; // permit retry
                            acceptable = false;
                            outstream.Write(false);
                        } else {
                            // shot position is acceptable; actually place it and determine result
                            result = opponent.Board.DoIncomingShotAt(pos, out var hitWhat);
                            opponent.Board.SetShotResultAt(pos, result);

                            outstream.Write(true);
                            outstream.Write((byte) result);

                            // if shot hit a ship, also indicate whether the ship was sunk
                            if (result == ShotResult.Hit) {
                                var ship = opponent.Board.Ships[hitWhat];
                                var sunk = ship.IsSunk();
                                outstream.Write(sunk);

                                // and only if actually sunk, do we tell the client what ship it was (minor security measure)
                                if (sunk) {
                                    outstream.Write((byte) hitWhat);
                                    outstream.Write((byte) ship.Position.x);
                                    outstream.Write((byte) ship.Position.y);
                                    outstream.Write((byte) ship.Orientation);
                                }
                            }
                        }

                        using (var packet = new Packet((PacketCommand) GameCommand.ShootResult, ms.ToArray()))
                            peer.Send(packet, player.NetworkID, PacketOptions.RELIABLE);
                    }
                }

                // Inform other player of the shot
                if (!acceptable) return;
                using (var ms = new MemoryStream()) {
                    using (var outstream = new BinaryWriter(ms, Encoding.UTF8)) {
                        outstream.Write((byte) pos.x);
                        outstream.Write((byte) pos.y);
                        outstream.Write((byte) result);

                        using (var packet = new Packet((PacketCommand) GameCommand.ShootIncoming, ms.ToArray()))
                            peer.Send(packet, opponent.NetworkID, PacketOptions.RELIABLE);
                    }
                }
            }

            // this is a very cheapskate way of pausing the game. I want both players to have some time to view the shot's
            // results, before the game switches to the next turn. And handling it clientside (in winforms) is too painful.
            Thread.Sleep(2000);

            // check if the opponent is now screwed
            if (opponent.Board.GetAllShipsSunk()) {
                // game over
                using (var packet = new Packet((PacketCommand) GameCommand.GameWin, EmptyPayload))
                    peer.Send(packet, player.NetworkID, PacketOptions.RELIABLE);
                using (var packet = new Packet((PacketCommand) GameCommand.GameLose, EmptyPayload))
                    peer.Send(packet, opponent.NetworkID, PacketOptions.RELIABLE);

                gameStarted = false;
            } else {
                BeginNextTurn();
            }
        }

    }

}
