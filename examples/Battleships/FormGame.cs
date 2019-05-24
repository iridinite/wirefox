using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Drawing;
using System.Text;
using System.Threading;
using System.Windows.Forms;

namespace Iridinite.Wirefox.Battleships {

    /// <summary>
    /// Represents the form that displays the game form and accepts player input.
    /// </summary>
    public partial class FormGame : Form {

        /// <summary>
        /// Indicates the state the game is in.
        /// </summary>
        private enum GamePhase {
            PlacingShips,
            WaitForOpponent,
            OurTurn,
            OpponentTurn,
            Ended
        }

        private static readonly Pen m_CellBorderPen = new Pen(Color.DarkSlateBlue);
        private static readonly Pen m_CellShipEdgePen = new Pen(Color.DarkSlateGray, 2f);
        private static readonly Pen m_CellHighlightPen = new Pen(Color.Black, 2f);

        private const int CellSizeMain = 32;
        private const int CellSizeTracking = 16;

        private static readonly string[] m_GridLetters = {"A", "B", "C", "D", "E", "F", "G", "H", "I", "J"};

        private static readonly Font m_GridLabelFont = new Font(FontFamily.GenericSansSerif, 12f, FontStyle.Regular);
        private static readonly Brush m_GridLabelBrush = new SolidBrush(Color.Black);
        private static readonly Brush m_CellBrushEmpty = new SolidBrush(Color.LightBlue);
        private static readonly Brush m_CellBrushShip = new SolidBrush(Color.SteelBlue);
        private static readonly Brush m_CellBrushError = new SolidBrush(Color.IndianRed);

        // We cache these references ourselves, because Resources getters are SUPER slow
        private static readonly Image m_SpriteCellHit = Properties.Resources.Sprite_CellHit;
        private static readonly Image m_SpriteCellMiss = Properties.Resources.Sprite_CellMiss;

        private readonly Dictionary<PacketCommand, MessageReceivedHandler> m_packetHandlers;

        private GamePhase m_Phase;
        private BoardPosition m_MousePosition;

        private Ship m_Placing;
        private ShipType m_PlacingType;
        private bool m_ExpectDisconnect = false;

        public FormGame() {
            InitializeComponent();

            m_packetHandlers = new Dictionary<PacketCommand, MessageReceivedHandler> {
                { PacketCommand.NOTIFY_DISCONNECTED, OnDisconnect },
                { PacketCommand.NOTIFY_CONNECTION_LOST, OnDisconnect },
                { (PacketCommand) GameCommand.OpponentDisconnected, OnDisconnectOpponent },
                { (PacketCommand) GameCommand.Chat, OnChat },
                { (PacketCommand) GameCommand.PlaceShip, OnPlaceShipReply },
                { (PacketCommand) GameCommand.YourTurn, OnOurTurn },
                { (PacketCommand) GameCommand.OpponentTurn, OnOpponentTurn },
                { (PacketCommand) GameCommand.ShootResult, OnShootResult },
                { (PacketCommand) GameCommand.ShootIncoming, OnShootIncoming },
                { (PacketCommand) GameCommand.GameWin, OnGameWon },
                { (PacketCommand) GameCommand.GameLose, OnGameLost }
            };

            SetPhase(GamePhase.PlacingShips);
        }

        private void FormGame_Load(object sender, System.EventArgs e) {
            Client.SetMessageHandler(OnMessageReceived);
        }

        private void FormGame_FormClosed(object sender, FormClosedEventArgs e) {
            ThreadPool.QueueUserWorkItem(delegate {
                // stop networking
                Server.Stop();
                Client.Stop();

                // reset and re-show the welcome dialog
                FormConnect.Instance.Invoke(new Action(() => {
                    FormConnect.Instance.ResetUI();
                    FormConnect.Instance.Show();
                }));
            });
        }

        private void cmdDisconnect_Click(object sender, EventArgs e) {
            if (m_ExpectDisconnect) {
                Close();
                return;
            }

            cmdDisconnect.Text = "Disconnecting...";
            cmdDisconnect.Enabled = false;
            m_ExpectDisconnect = true;
            Client.Disconnect();
        }

        #region Board Rendering

        private void picMainBoard_Paint(object sender, PaintEventArgs e) {
            var g = e.Graphics;
            DrawGrid(g, CellSizeMain);

            // Draw the board contents
            if (m_Phase == GamePhase.OurTurn)
                DrawTrackingBoard(g, CellSizeMain);
            else
                DrawOwnBoard(g, CellSizeMain);

            // Draw highlight rectangle showing which cell the user is hovering over
            if (m_MousePosition.x > -1 && m_Phase != GamePhase.PlacingShips) {
                g.DrawRectangle(m_CellHighlightPen,
                    CellSizeMain + m_MousePosition.x * CellSizeMain,
                    CellSizeMain + m_MousePosition.y * CellSizeMain,
                    CellSizeMain,
                    CellSizeMain);
            }
        }

        private void picTrackingBoard_Paint(object sender, PaintEventArgs e) {
            var g = e.Graphics;
            DrawGrid(g, CellSizeTracking, false);

            // Draw the secondary board contents
            if (m_Phase == GamePhase.OurTurn)
                DrawOwnBoard(g, CellSizeTracking);
            else
                DrawTrackingBoard(g, CellSizeTracking);
        }

        private void DrawGrid(Graphics g, int cellSize, bool drawLabels = true) {
            // Flat color background
            g.FillRectangle(m_CellBrushEmpty, new Rectangle(
                cellSize,
                cellSize,
                Board.Dimensions * cellSize,
                Board.Dimensions * cellSize));

            // Pen grid separating the cells
            // Please don't mind the forest of coordinate calculations :)
            for (int x = 0; x <= Board.Dimensions; x++) {
                if (drawLabels && x < Board.Dimensions) // because we draw one more line at the edge, to close off the grid
                    g.DrawString((x + 1).ToString(), m_GridLabelFont, m_GridLabelBrush, (x + 1) * cellSize + cellSize * 0.25f, cellSize * 0.25f);
                g.DrawLine(m_CellBorderPen, (x + 1) * cellSize, cellSize, (x + 1) * cellSize, (Board.Dimensions + 1) * cellSize);
            }

            for (int y = 0; y <= Board.Dimensions; y++) {
                if (drawLabels && y < Board.Dimensions)
                    g.DrawString(m_GridLetters[y], m_GridLabelFont, m_GridLabelBrush, cellSize * 0.25f, (y + 1) * cellSize + cellSize * 0.25f);
                g.DrawLine(m_CellBorderPen, cellSize, (y + 1) * cellSize, (Board.Dimensions + 1) * cellSize, (y + 1) * cellSize);
            }
        }

        private void DrawShip(Graphics g, Ship ship, Brush backColor, int cellSize) {
            var area = new Rectangle(ship.Position.x, ship.Position.y, 1, 1);
            if (ship.Orientation == ShipOrientation.Horizontal)
                area.Width = ship.GetShipLength();
            else
                area.Height = ship.GetShipLength();

            var pixelArea = new Rectangle(
                area.X * cellSize + cellSize,
                area.Y * cellSize + cellSize,
                area.Width * cellSize,
                area.Height * cellSize);

            g.FillRectangle(backColor, pixelArea);
            g.DrawRectangle(m_CellShipEdgePen, pixelArea);
        }

        private void DrawShots(Graphics g, Board b, int cellSize) {
            foreach (var pair in b.Shots) {
                var pixelRect = new Rectangle(
                    cellSize + pair.Key.x * cellSize,
                    cellSize + pair.Key.y * cellSize,
                    cellSize,
                    cellSize);

                //g.FillRectangle(m_CellBrushError, pixelRect);
                g.DrawImage(pair.Value == ShotResult.Hit ? m_SpriteCellHit : m_SpriteCellMiss,
                    pixelRect,
                    new Rectangle(0, 0, 32, 32),
                    GraphicsUnit.Pixel);
            }
        }

        private void DrawOwnBoard(Graphics g, int cellSize) {
            // Draw all ships already on the board
            foreach (var ship in Client.Board.Ships.Values)
                DrawShip(g, ship, m_CellBrushShip, cellSize);

            // Draw the opponent's shots
            DrawShots(g, Client.Board, cellSize);

            // If a temporary placement ship should be drawn...
            if (m_Phase == GamePhase.PlacingShips && m_Placing != null && m_MousePosition.x > -1)
                DrawShip(g, m_Placing, Client.Board.CanPlaceShip(m_Placing) ? m_CellBrushShip : m_CellBrushError, CellSizeMain);
        }

        private void DrawTrackingBoard(Graphics g, int cellSize) {
            // Draw all ships on the board (they will be placed/revealed if we sink one)
            foreach (var ship in Client.TrackingBoard.Ships.Values)
                DrawShip(g, ship, m_CellBrushShip, cellSize);

            DrawShots(g, Client.TrackingBoard, cellSize);
        }

        #endregion

        #region Board Interaction

        private void picMainBoard_MouseMove(object sender, MouseEventArgs e) {
            // Update mouse position relative to the board
            int cx = (e.X - CellSizeMain) / CellSizeMain;
            int cy = (e.Y - CellSizeMain) / CellSizeMain;
            m_MousePosition = e.X < CellSizeMain || e.Y < CellSizeMain || cx < 0 || cy < 0 || cx >= Board.Dimensions || cy >= Board.Dimensions
                ? new BoardPosition(-1, -1)
                : new BoardPosition(cx, cy);

            if (m_Placing != null)
                m_Placing.Position = m_MousePosition;

            picMainBoard.Invalidate();
        }

        private void picMainBoard_MouseLeave(object sender, System.EventArgs e) {
            m_MousePosition = new BoardPosition(-1, -1);
        }

        private void picMainBoard_Click(object sender, System.EventArgs e) {
            switch (m_Phase) {
                case GamePhase.PlacingShips:
                    if (!Client.Board.CanPlaceShip(m_Placing)) break;

                    //Client.Board.SetShip(m_PlacingType, m_Placing);
                    picMainBoard.Enabled = false;
                    lblGameStatus.Text = "Hold on...";
                    Client.SendPlaceShip(m_Placing);

                    break;

                case GamePhase.OurTurn:
                    if (!m_MousePosition.IsWithinBoard()) break;

                    picMainBoard.Enabled = false;
                    lblGameStatus.Text = "Hold on...";
                    Client.SendShootRequest(m_MousePosition);
                    break;
            }
        }

        private void BeginPlacingShip() {
            m_Placing = new Ship(m_PlacingType);
            lblGameStatus.Text = $"Place your ships! Now placing {m_PlacingType} ({m_Placing.GetShipLength()} cells wide)";
        }

        #endregion

        #region Network Callbacks

        private void SetPhase(GamePhase newPhase) {
            m_Phase = newPhase;

            switch (m_Phase) {
                case GamePhase.PlacingShips:
                    m_PlacingType = ShipType.Carrier;
                    BeginPlacingShip();
                    break;

                case GamePhase.WaitForOpponent:
                    m_Placing = null;
                    lblGameStatus.Text = "Waiting for opponent.";
                    break;

                case GamePhase.OurTurn:
                    lblGameStatus.Text = "It's your turn! Click one cell to shoot it.";
                    lblTrackingGridPurpose.Text = "Your Fleet";
                    picMainBoard.Enabled = true;
                    picMainBoard.Invalidate();
                    picTrackingBoard.Invalidate();
                    break;

                case GamePhase.OpponentTurn:
                    lblGameStatus.Text = "Opponent's turn.";
                    lblTrackingGridPurpose.Text = "Enemy Fleet";
                    picMainBoard.Invalidate();
                    picTrackingBoard.Invalidate();
                    break;
            }
        }

        private void UpdateStats() {
            var sb = new StringBuilder();
            sb.AppendFormat("Byte Totals: {0:##,##0} sent / {1:##,##0} recv",
                Client.GetStat(PeerStatID.BYTES_SENT),
                Client.GetStat(PeerStatID.BYTES_RECEIVED));
            sb.AppendLine();
            sb.AppendFormat("Bytes in Flight: {0:##,##0}",
                Client.GetStat(PeerStatID.BYTES_IN_FLIGHT));
            sb.AppendLine();
            sb.AppendFormat("Packets queued: {0:##,##0} ({1:##,##0} waiting)",
                Client.GetStat(PeerStatID.PACKETS_QUEUED),
                Client.GetStat(PeerStatID.PACKETS_IN_QUEUE));
            sb.AppendLine();
            sb.AppendFormat("Packets: {0:##,##0} sent / {1:##,##0} recv / {2:##,##0} lost",
                Client.GetStat(PeerStatID.PACKETS_SENT),
                Client.GetStat(PeerStatID.PACKETS_RECEIVED),
                Client.GetStat(PeerStatID.PACKETS_LOST));
            sb.AppendLine();
            sb.AppendFormat("Datagrams: {0:##,##0} sent / {1:##,##0} recv",
                Client.GetStat(PeerStatID.PACKETS_SENT),
                Client.GetStat(PeerStatID.PACKETS_RECEIVED));
            sb.AppendLine();
            sb.AppendFormat("CWND: {0:##,##0}",
                Client.GetStat(PeerStatID.CWND));

            lblStats.Text = sb.ToString();
        }

        private void OnMessageReceived(Packet recv) {
            // need to be on UI thread
            if (this.InvokeRequired) {
                this.Invoke(new Action(delegate { OnMessageReceived(recv); }));
                return;
            }

            if (m_packetHandlers.ContainsKey(recv.GetCommand()))
                m_packetHandlers[recv.GetCommand()].Invoke(recv);

            // visualize connection debug info
            UpdateStats();
        }

        private void OnChat(Packet recv) {
            using (var instream = recv.GetStream()) {
                var message = instream.ReadString();

                txtChatLog.Text += $"[Opponent] {message}{Environment.NewLine}";
                txtChatLog.Select(txtChatLog.TextLength, 0);
                txtChatLog.ScrollToCaret();
            }
        }

        private void OnPlaceShipReply(Packet recv) {
            ShipType recvType;
            bool recvAccepted;
            using (var instream = recv.GetStream()) {
                recvType = (ShipType) instream.ReadByte();
                recvAccepted = instream.ReadBoolean();
            }

            Debug.Assert(recvType == m_PlacingType);
            picMainBoard.Enabled = true;

            if (!recvAccepted) {
                // the server rejected our request for some reason
                return;
            }

            // the server approved our request, so update the local board with the placed ship
            Client.Board.PlaceShip(m_PlacingType, m_Placing);

            if (m_PlacingType == ShipType.Destroyer) {
                // All ships placed
                SetPhase(GamePhase.WaitForOpponent);
            } else {
                m_PlacingType++;
                BeginPlacingShip();
            }
        }

        private void OnOurTurn(Packet recv) {
            SetPhase(GamePhase.OurTurn);
        }

        private void OnOpponentTurn(Packet recv) {
            SetPhase(GamePhase.OpponentTurn);
        }

        private void OnShootResult(Packet recv) {
            using (var instream = recv.GetStream()) {
                var pos = new BoardPosition(instream.ReadByte(), instream.ReadByte());
                var acceptable = instream.ReadBoolean();

                picMainBoard.Enabled = true;
                if (!acceptable) return;

                var result = (ShotResult) instream.ReadByte();
                Client.TrackingBoard.SetShotResultAt(pos, result);

                string sunkString = String.Empty;
                if (result == ShotResult.Hit) {
                    var sunk = instream.ReadBoolean();
                    if (sunk) {
                        // the server now tells us what ship we actually managed to sink
                        var sunkWhat = (ShipType) instream.ReadByte();
                        sunkString = $", and sunk their {sunkWhat}!";

                        // place it on the tracking board, so that the player can see it
                        var ship = new Ship(sunkWhat) {
                            Position = new BoardPosition(instream.ReadByte(), instream.ReadByte()),
                            Orientation = (ShipOrientation) instream.ReadByte()
                        };
                        Client.TrackingBoard.PlaceShip(sunkWhat, ship);
                    }
                }

                lblGameStatus.Text = $"Your shot at {m_GridLetters[pos.y]}{pos.x + 1} {(result == ShotResult.Hit ? "was a hit" : "missed")}{sunkString}!";
                picMainBoard.Invalidate();
                picTrackingBoard.Invalidate();
            }
        }

        private void OnShootIncoming(Packet recv) {
            using (var instream = recv.GetStream()) {
                var pos = new BoardPosition(instream.ReadByte(), instream.ReadByte());
                var result = (ShotResult) instream.ReadByte();

                //picMainBoard.Enabled = true;

                var hit = Client.Board.DoIncomingShotAt(pos, out var hitWhat);
                var ship = Client.Board.Ships[hitWhat];
                var sunk = ship.IsSunk();
                Client.Board.SetShotResultAt(pos, result);
                Debug.Assert(hit == result); // client and server should agree on the hit

                lblGameStatus.Text =
                    $"Your opponent fired at {m_GridLetters[pos.y]}{pos.x + 1} and " +
                    $"{(result == ShotResult.Hit ? $"{(sunk ? "sunk" : "hit")} your {hitWhat}" : "missed")}!";
                picMainBoard.Invalidate();
                picTrackingBoard.Invalidate();
            }
        }

        private void OnGameWon(Packet recv) {
            lblGameStatus.Text = "A winner is you! Whoooo!";
            SetPhase(GamePhase.Ended);
        }

        private void OnGameLost(Packet recv) {
            lblGameStatus.Text = "All your ships are gone, you lost :(";
            SetPhase(GamePhase.Ended);
        }

        private void OnDisconnect(Packet recv) {
            if (m_ExpectDisconnect) {
                Close();
                return;
            }

            cmdChat.Enabled = false;
            cmdDisconnect.Text = "Exit";
            cmdDisconnect.Enabled = true;
            if (m_Phase != GamePhase.Ended) // leave "opponent disconnect text" if present
                lblGameStatus.Text = "Lost connection with the server.";
            m_ExpectDisconnect = true;
            SetPhase(GamePhase.Ended);
        }

        private void OnDisconnectOpponent(Packet recv) {
            cmdChat.Enabled = false;
            cmdDisconnect.Text = "Exit";
            cmdDisconnect.Enabled = true;
            lblGameStatus.Text = "Your opponent disconnected.";
            SetPhase(GamePhase.Ended);
        }

        #endregion

        #region Chat

        private void txtChatEntry_KeyDown(object sender, KeyEventArgs e) {
            if (e.KeyCode == Keys.Enter) {
                // submit chat message
                cmdChat_Click(sender, e);
                e.Handled = true;
                e.SuppressKeyPress = true;
            }
        }

        private void cmdChat_Click(object sender, EventArgs e) {
            if (String.IsNullOrWhiteSpace(txtChatEntry.Text)) {
                txtChatEntry.Text = String.Empty;
                return;
            }

            string message = txtChatEntry.Text.Trim();
            txtChatLog.Text += $"[You] {message}{Environment.NewLine}";
            txtChatLog.Select(txtChatLog.TextLength, 0);
            txtChatLog.ScrollToCaret();
            Client.SendChat(message);

            txtChatEntry.Text = String.Empty;
        }

        #endregion

        private void tmrStats_Tick(object sender, EventArgs e) {
            if (!Client.IsActive()) return;

            UpdateStats();
        }

    }

}
