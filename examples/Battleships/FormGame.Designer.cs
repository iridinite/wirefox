namespace Iridinite.Wirefox.Battleships
{
    partial class FormGame
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
            this.picMainBoard = new System.Windows.Forms.PictureBox();
            this.picTrackingBoard = new System.Windows.Forms.PictureBox();
            this.lblGameStatus = new System.Windows.Forms.Label();
            this.txtChatEntry = new System.Windows.Forms.TextBox();
            this.txtChatLog = new System.Windows.Forms.TextBox();
            this.cmdChat = new System.Windows.Forms.Button();
            this.lblTrackingGridPurpose = new System.Windows.Forms.Label();
            ((System.ComponentModel.ISupportInitialize)(this.picMainBoard)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.picTrackingBoard)).BeginInit();
            this.SuspendLayout();
            // 
            // picMainBoard
            // 
            this.picMainBoard.Location = new System.Drawing.Point(200, 48);
            this.picMainBoard.Name = "picMainBoard";
            this.picMainBoard.Size = new System.Drawing.Size(400, 400);
            this.picMainBoard.TabIndex = 0;
            this.picMainBoard.TabStop = false;
            this.picMainBoard.Click += new System.EventHandler(this.picMainBoard_Click);
            this.picMainBoard.Paint += new System.Windows.Forms.PaintEventHandler(this.picMainBoard_Paint);
            this.picMainBoard.MouseLeave += new System.EventHandler(this.picMainBoard_MouseLeave);
            this.picMainBoard.MouseMove += new System.Windows.Forms.MouseEventHandler(this.picMainBoard_MouseMove);
            // 
            // picTrackingBoard
            // 
            this.picTrackingBoard.Location = new System.Drawing.Point(608, 48);
            this.picTrackingBoard.Name = "picTrackingBoard";
            this.picTrackingBoard.Size = new System.Drawing.Size(200, 200);
            this.picTrackingBoard.TabIndex = 1;
            this.picTrackingBoard.TabStop = false;
            this.picTrackingBoard.Paint += new System.Windows.Forms.PaintEventHandler(this.picTrackingBoard_Paint);
            // 
            // lblGameStatus
            // 
            this.lblGameStatus.AutoSize = true;
            this.lblGameStatus.Font = new System.Drawing.Font("Microsoft Sans Serif", 11.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.lblGameStatus.Location = new System.Drawing.Point(200, 16);
            this.lblGameStatus.Name = "lblGameStatus";
            this.lblGameStatus.Size = new System.Drawing.Size(105, 18);
            this.lblGameStatus.TabIndex = 2;
            this.lblGameStatus.Text = "Game status...";
            // 
            // txtChatEntry
            // 
            this.txtChatEntry.Location = new System.Drawing.Point(16, 248);
            this.txtChatEntry.Name = "txtChatEntry";
            this.txtChatEntry.Size = new System.Drawing.Size(168, 20);
            this.txtChatEntry.TabIndex = 3;
            this.txtChatEntry.KeyDown += new System.Windows.Forms.KeyEventHandler(this.txtChatEntry_KeyDown);
            // 
            // txtChatLog
            // 
            this.txtChatLog.Location = new System.Drawing.Point(16, 48);
            this.txtChatLog.Multiline = true;
            this.txtChatLog.Name = "txtChatLog";
            this.txtChatLog.ReadOnly = true;
            this.txtChatLog.Size = new System.Drawing.Size(168, 192);
            this.txtChatLog.TabIndex = 4;
            // 
            // cmdChat
            // 
            this.cmdChat.Location = new System.Drawing.Point(104, 272);
            this.cmdChat.Name = "cmdChat";
            this.cmdChat.Size = new System.Drawing.Size(80, 24);
            this.cmdChat.TabIndex = 5;
            this.cmdChat.Text = "Chat";
            this.cmdChat.UseVisualStyleBackColor = true;
            this.cmdChat.Click += new System.EventHandler(this.cmdChat_Click);
            // 
            // lblTrackingGridPurpose
            // 
            this.lblTrackingGridPurpose.AutoSize = true;
            this.lblTrackingGridPurpose.Font = new System.Drawing.Font("Microsoft Sans Serif", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.lblTrackingGridPurpose.Location = new System.Drawing.Point(616, 32);
            this.lblTrackingGridPurpose.Name = "lblTrackingGridPurpose";
            this.lblTrackingGridPurpose.Size = new System.Drawing.Size(89, 16);
            this.lblTrackingGridPurpose.TabIndex = 6;
            this.lblTrackingGridPurpose.Text = "Tracking Grid";
            // 
            // FormGame
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(841, 465);
            this.Controls.Add(this.lblTrackingGridPurpose);
            this.Controls.Add(this.cmdChat);
            this.Controls.Add(this.txtChatLog);
            this.Controls.Add(this.txtChatEntry);
            this.Controls.Add(this.lblGameStatus);
            this.Controls.Add(this.picTrackingBoard);
            this.Controls.Add(this.picMainBoard);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedSingle;
            this.MaximizeBox = false;
            this.Name = "FormGame";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
            this.Text = "Battleships#";
            this.FormClosed += new System.Windows.Forms.FormClosedEventHandler(this.FormGame_FormClosed);
            this.Load += new System.EventHandler(this.FormGame_Load);
            ((System.ComponentModel.ISupportInitialize)(this.picMainBoard)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.picTrackingBoard)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.PictureBox picMainBoard;
        private System.Windows.Forms.PictureBox picTrackingBoard;
        private System.Windows.Forms.Label lblGameStatus;
        private System.Windows.Forms.TextBox txtChatEntry;
        private System.Windows.Forms.TextBox txtChatLog;
        private System.Windows.Forms.Button cmdChat;
        private System.Windows.Forms.Label lblTrackingGridPurpose;
    }
}