namespace Iridinite.Wirefox.Battleships
{
    partial class FormConnect
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.lblTitle = new System.Windows.Forms.Label();
            this.cmdConnect = new System.Windows.Forms.Button();
            this.fraConnect = new System.Windows.Forms.GroupBox();
            this.txtIP = new System.Windows.Forms.TextBox();
            this.lblIP = new System.Windows.Forms.Label();
            this.fraHost = new System.Windows.Forms.GroupBox();
            this.lblHostStatus = new System.Windows.Forms.Label();
            this.cmdHost = new System.Windows.Forms.Button();
            this.cmdExit = new System.Windows.Forms.Button();
            this.lklGithub = new System.Windows.Forms.LinkLabel();
            this.fraConnect.SuspendLayout();
            this.fraHost.SuspendLayout();
            this.SuspendLayout();
            // 
            // lblTitle
            // 
            this.lblTitle.AutoSize = true;
            this.lblTitle.Location = new System.Drawing.Point(16, 16);
            this.lblTitle.Name = "lblTitle";
            this.lblTitle.Size = new System.Drawing.Size(184, 26);
            this.lblTitle.TabIndex = 0;
            this.lblTitle.Text = "A Wirefox demo game application.\r\nCopyright (C) Mika Molenkamp, 2019.";
            // 
            // cmdConnect
            // 
            this.cmdConnect.Location = new System.Drawing.Point(16, 80);
            this.cmdConnect.Name = "cmdConnect";
            this.cmdConnect.Size = new System.Drawing.Size(232, 24);
            this.cmdConnect.TabIndex = 1;
            this.cmdConnect.Text = "Connect";
            this.cmdConnect.UseVisualStyleBackColor = true;
            this.cmdConnect.Click += new System.EventHandler(this.cmdConnect_Click);
            // 
            // fraConnect
            // 
            this.fraConnect.Controls.Add(this.txtIP);
            this.fraConnect.Controls.Add(this.lblIP);
            this.fraConnect.Controls.Add(this.cmdConnect);
            this.fraConnect.Location = new System.Drawing.Point(16, 88);
            this.fraConnect.Name = "fraConnect";
            this.fraConnect.Size = new System.Drawing.Size(264, 120);
            this.fraConnect.TabIndex = 2;
            this.fraConnect.TabStop = false;
            this.fraConnect.Text = "Connect to a Lobby";
            // 
            // txtIP
            // 
            this.txtIP.Location = new System.Drawing.Point(16, 40);
            this.txtIP.Name = "txtIP";
            this.txtIP.Size = new System.Drawing.Size(232, 20);
            this.txtIP.TabIndex = 3;
            this.txtIP.Text = "localhost";
            // 
            // lblIP
            // 
            this.lblIP.AutoSize = true;
            this.lblIP.Location = new System.Drawing.Point(16, 24);
            this.lblIP.Name = "lblIP";
            this.lblIP.Size = new System.Drawing.Size(124, 13);
            this.lblIP.TabIndex = 2;
            this.lblIP.Text = "IP Address or Hostname:";
            // 
            // fraHost
            // 
            this.fraHost.Controls.Add(this.lblHostStatus);
            this.fraHost.Controls.Add(this.cmdHost);
            this.fraHost.Location = new System.Drawing.Point(16, 224);
            this.fraHost.Name = "fraHost";
            this.fraHost.Size = new System.Drawing.Size(264, 96);
            this.fraHost.TabIndex = 4;
            this.fraHost.TabStop = false;
            this.fraHost.Text = "Host a Lobby";
            // 
            // lblHostStatus
            // 
            this.lblHostStatus.AutoSize = true;
            this.lblHostStatus.Location = new System.Drawing.Point(16, 24);
            this.lblHostStatus.Name = "lblHostStatus";
            this.lblHostStatus.Size = new System.Drawing.Size(183, 13);
            this.lblHostStatus.TabIndex = 3;
            this.lblHostStatus.Text = "One other player can connect to you.";
            // 
            // cmdHost
            // 
            this.cmdHost.Location = new System.Drawing.Point(16, 56);
            this.cmdHost.Name = "cmdHost";
            this.cmdHost.Size = new System.Drawing.Size(232, 24);
            this.cmdHost.TabIndex = 1;
            this.cmdHost.Text = "Start Server";
            this.cmdHost.UseVisualStyleBackColor = true;
            this.cmdHost.Click += new System.EventHandler(this.cmdHost_Click);
            // 
            // cmdExit
            // 
            this.cmdExit.Location = new System.Drawing.Point(168, 352);
            this.cmdExit.Name = "cmdExit";
            this.cmdExit.Size = new System.Drawing.Size(112, 24);
            this.cmdExit.TabIndex = 2;
            this.cmdExit.Text = "Exit";
            this.cmdExit.UseVisualStyleBackColor = true;
            this.cmdExit.Click += new System.EventHandler(this.cmdExit_Click);
            // 
            // lklGithub
            // 
            this.lklGithub.AutoSize = true;
            this.lklGithub.Location = new System.Drawing.Point(16, 56);
            this.lklGithub.Name = "lklGithub";
            this.lklGithub.Size = new System.Drawing.Size(119, 13);
            this.lklGithub.TabIndex = 5;
            this.lklGithub.TabStop = true;
            this.lklGithub.Text = "Visit Wirefox on GitHub!";
            this.lklGithub.LinkClicked += new System.Windows.Forms.LinkLabelLinkClickedEventHandler(this.lklGithub_LinkClicked);
            // 
            // FormConnect
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(297, 393);
            this.Controls.Add(this.lklGithub);
            this.Controls.Add(this.cmdExit);
            this.Controls.Add(this.fraHost);
            this.Controls.Add(this.fraConnect);
            this.Controls.Add(this.lblTitle);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
            this.MaximizeBox = false;
            this.Name = "FormConnect";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
            this.Text = "Battleships#";
            this.fraConnect.ResumeLayout(false);
            this.fraConnect.PerformLayout();
            this.fraHost.ResumeLayout(false);
            this.fraHost.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Label lblTitle;
        private System.Windows.Forms.Button cmdConnect;
        private System.Windows.Forms.GroupBox fraConnect;
        private System.Windows.Forms.TextBox txtIP;
        private System.Windows.Forms.Label lblIP;
        private System.Windows.Forms.GroupBox fraHost;
        private System.Windows.Forms.Button cmdHost;
        private System.Windows.Forms.Button cmdExit;
        private System.Windows.Forms.Label lblHostStatus;
        private System.Windows.Forms.LinkLabel lklGithub;
    }
}

