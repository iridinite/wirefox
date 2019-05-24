using System.IO;
using System.Text;
using System.Threading;
using System.Windows.Forms;

namespace Iridinite.Wirefox.Battleships {

    internal delegate void MessageReceivedHandler(Packet recv);

    internal static class Client {

        private static Peer peer;
        private static PeerID serverID;

        private static Thread worker;
        private static volatile bool workerStop = true;

        private static event MessageReceivedHandler OnMessageReceived;

        /// <summary>
        /// Represents the local player's game board.
        /// </summary>
        public static Board Board { get; private set; }

        /// <summary>
        /// Represents the tracking board: what the local player knows of the opponent's game board.
        /// </summary>
        public static Board TrackingBoard { get; private set; }

        public static void Initialize() {
            if (IsActive()) return;

            Board = new Board();
            TrackingBoard = new Board();

            workerStop = false;
            peer = new Peer(1);
            peer.Bind(SocketProtocol.IPv4, 0);

            // unfortunately we don't have a proper game loop like a 3D renderer does, so we'll use a thread instead
            worker = new Thread(ThreadWorker) {
                IsBackground = true
            };
            worker.Start();
        }

        public static void Stop() {
            if (!IsActive()) return;

            OnMessageReceived = null;

            workerStop = true;
            worker.Join();
            worker = null;

            peer.Dispose();
            peer = null;
        }

        public static bool IsActive() {
            return !workerStop;
        }

        public static void SetMessageHandler(MessageReceivedHandler callback) {
            // we use this event thing, because the callbacks need to be ran on the UI thread, making this a bit trickier
            OnMessageReceived = null;
            OnMessageReceived += callback;
        }

        private static void ThreadWorker() {
            while (!workerStop) {
                var recv = peer.Receive();
                while (recv != null) {
                    // store the server's PeerID, so we can pass it to Send() later
                    if (recv.GetCommand() == PacketCommand.NOTIFY_CONNECT_SUCCESS)
                        serverID = recv.GetSender();

                    OnMessageReceived?.Invoke(recv);

                    // Keep checking until the queue is empty
                    recv = peer.Receive();
                }

                Thread.Sleep(1);
            }
        }

        public static bool Connect(string host) {
            var ret = peer.Connect(host, Server.GamePort);
            if (ret != ConnectAttemptResult.OK)
                MessageBox.Show("Could not connect to " + host + ": " + ret.ToString(), ":(", MessageBoxButtons.OK, MessageBoxIcon.Warning);

            return ret == ConnectAttemptResult.OK;
        }

        public static void Disconnect() {
            peer.Disconnect(serverID);
        }

        public static ulong GetStat(PeerStatID stat) {
            return peer.GetStat(serverID, stat);
        }

        public static void SendPlaceShip(Ship newShip) {
            using (var ms = new MemoryStream()) {
                using (var writer = new BinaryWriter(ms, Encoding.UTF8)) {
                    writer.Write((byte) newShip.Type);
                    writer.Write((byte) newShip.Orientation);
                    writer.Write((byte) newShip.Position.x);
                    writer.Write((byte) newShip.Position.y);

                    using (var packet = new Packet((PacketCommand) GameCommand.PlaceShip, ms.ToArray())) {
                        peer.Send(packet, serverID, PacketOptions.RELIABLE);
                    }
                }
            }
        }

        public static void SendShootRequest(BoardPosition at) {
            using (var ms = new MemoryStream()) {
                using (var writer = new BinaryWriter(ms, Encoding.UTF8)) {
                    writer.Write((byte) at.x);
                    writer.Write((byte) at.y);

                    using (var packet = new Packet((PacketCommand) GameCommand.Shoot, ms.ToArray())) {
                        peer.Send(packet, serverID, PacketOptions.RELIABLE);
                    }
                }
            }
        }

        public static void SendChat(string message) {
            using (var ms = new MemoryStream()) {
                using (var outstream = new BinaryWriter(ms, Encoding.UTF8)) {
                    outstream.Write(message);

                    using (var packet = new Packet((PacketCommand) GameCommand.Chat, ms.ToArray())) {
                        peer.Send(packet, serverID, PacketOptions.RELIABLE);
                    }
                }
            }
        }

    }

}
