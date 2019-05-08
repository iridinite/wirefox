using System;
using System.Diagnostics;
using System.Windows.Forms;

namespace Iridinite.Wirefox.Battleships {

    public partial class FormConnect : Form {

        public static FormConnect Instance { get; private set; }

        public FormConnect() {
            InitializeComponent();

            Instance = this;
            DialogResult = DialogResult.Cancel;
        }

        public void ResetUI() {
            fraConnect.Enabled = true;
            fraHost.Enabled = true;
            lblHostStatus.Text = "One other player can connect to you.";
            cmdHost.Enabled = true;
            cmdConnect.Text = "Connect";
        }

        private void cmdHost_Click(object sender, EventArgs e) {
            //if (Server.IsActive()) {
            //    Server.Stop();
            //    Client.Stop();
            //    fraConnect.Enabled = true;
            //    return;
            //}

            fraConnect.Enabled = false;

            Server.Initialize();
            Client.Initialize();
            Client.SetMessageHandler(OnMessageReceived);
            if (!Client.Connect("localhost")) {
                fraConnect.Enabled = true;
                fraHost.Enabled = true;
                return;
            }

            lblHostStatus.Text = "Waiting for another player...";
            cmdHost.Enabled = false;
        }

        private void cmdConnect_Click(object sender, EventArgs e) {
            fraConnect.Enabled = false;
            fraHost.Enabled = false;
            cmdConnect.Text = "Connecting...";

            Client.Initialize();
            Client.SetMessageHandler(OnMessageReceived);
            if (!Client.Connect(txtIP.Text)) {
                fraConnect.Enabled = true;
                fraHost.Enabled = true;
                cmdConnect.Text = "Connect";
            }
        }

        private void cmdExit_Click(object sender, EventArgs e) {
            Server.Stop();
            Client.Stop();

            DialogResult = DialogResult.Cancel;
            Close();
        }

        private void OnMessageReceived(Packet recv) {
            // need to be on UI thread
            if (this.InvokeRequired) {
                this.Invoke(new Action(delegate { OnMessageReceived(recv); }));
                return;
            }

            switch (recv.GetCommand()) {
                case PacketCommand.NOTIFY_CONNECT_SUCCESS:
                    // Connection successful, wait for notification to begin
                    cmdConnect.Text = "Waiting for opponent...";
                    break;
                case PacketCommand.NOTIFY_CONNECT_FAILED:
                    // Connection failed, reset form
                    ConnectResult reason;
                    using (var reader = recv.GetStream()) {
                        reason = (ConnectResult) reader.ReadByte();
                    }

                    fraConnect.Enabled = true;
                    fraHost.Enabled = true;
                    cmdConnect.Text = "Connect";
                    MessageBox.Show("Error: " + Peer.ConnectResultToString(reason), ":(", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
                    break;

                case PacketCommand.NOTIFY_CONNECTION_LOST:
                case PacketCommand.NOTIFY_DISCONNECTED:
                    fraConnect.Enabled = true;
                    fraHost.Enabled = true;
                    MessageBox.Show("Lost connection.", ":(", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
                    break;

                case (PacketCommand) GameCommand.BeginSession:
                    new FormGame().Show(this);
                    this.Hide();
                    break;
            }
        }

        private void lklGithub_LinkClicked(object sender, LinkLabelLinkClickedEventArgs e) {
            Process.Start("https://github.com/iridinite/wirefox");
        }

    }

}
