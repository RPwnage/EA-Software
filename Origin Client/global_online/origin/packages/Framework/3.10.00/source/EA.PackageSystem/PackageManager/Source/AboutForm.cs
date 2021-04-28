using System;
using System.Collections;
using System.ComponentModel;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Reflection;
using System.Windows.Forms;

using NAnt.Core.PackageCore;

namespace EA.PackageSystem.PackageManager
{
    public class AboutForm : System.Windows.Forms.Form
    {
        private System.Windows.Forms.Button OkButton;
        private System.Windows.Forms.Label CopyrightLabel;
        private System.Windows.Forms.PictureBox LogoPictureBox;
        private System.Windows.Forms.Label TitleLabel;
        private System.Windows.Forms.Label VersionLabel;
        private System.Windows.Forms.RichTextBox CreditsBox;

        /// <summary>Required designer variable.</summary>
        private System.ComponentModel.Container components = null;

        public AboutForm()
        {
            // Required for Windows Form Designer support
            InitializeComponent();

            Release r = PackageMap.Instance.GetFrameworkRelease();
            if (r != null)
            {
                VersionLabel.Text = r.FullName;
            }
            else
            {
                VersionLabel.Text = "";
            }

            Stream creditsStream = ResourceLoader.Instance.OpenStream("credits.rtf");
            CreditsBox.LoadFile(creditsStream, RichTextBoxStreamType.RichText);
            creditsStream.Close();
        }

        /// <summary>Clean up any resources being used.</summary>
        protected override void Dispose(bool disposing)
        {
            if (disposing)
            {
                if (components != null)
                {
                    components.Dispose();
                }
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
            System.Resources.ResourceManager resources = new System.Resources.ResourceManager(typeof(AboutForm));
            this.LogoPictureBox = new System.Windows.Forms.PictureBox();
            this.TitleLabel = new System.Windows.Forms.Label();
            this.VersionLabel = new System.Windows.Forms.Label();
            this.OkButton = new System.Windows.Forms.Button();
            this.CopyrightLabel = new System.Windows.Forms.Label();
            this.CreditsBox = new System.Windows.Forms.RichTextBox();
            this.SuspendLayout();
            // 
            // LogoPictureBox
            // 
            this.LogoPictureBox.Image = ((System.Drawing.Bitmap)(resources.GetObject("LogoPictureBox.Image")));
            this.LogoPictureBox.Location = new System.Drawing.Point(17, 11);
            this.LogoPictureBox.Name = "LogoPictureBox";
            this.LogoPictureBox.Size = new System.Drawing.Size(48, 48);
            this.LogoPictureBox.TabIndex = 0;
            this.LogoPictureBox.TabStop = false;
            // 
            // TitleLabel
            // 
            this.TitleLabel.Font = new System.Drawing.Font("Tahoma", 14.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((System.Byte)(0)));
            this.TitleLabel.Location = new System.Drawing.Point(72, 16);
            this.TitleLabel.Name = "TitleLabel";
            this.TitleLabel.Size = new System.Drawing.Size(200, 24);
            this.TitleLabel.TabIndex = 1;
            this.TitleLabel.Text = "Package Manager";
            // 
            // VersionLabel
            // 
            this.VersionLabel.Location = new System.Drawing.Point(75, 41);
            this.VersionLabel.Name = "VersionLabel";
            this.VersionLabel.Size = new System.Drawing.Size(197, 16);
            this.VersionLabel.TabIndex = 1;
            this.VersionLabel.Text = "Version x.y.z";
            // 
            // OkButton
            // 
            this.OkButton.Anchor = System.Windows.Forms.AnchorStyles.Bottom;
            this.OkButton.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.OkButton.Location = new System.Drawing.Point(111, 222);
            this.OkButton.Name = "OkButton";
            this.OkButton.TabIndex = 2;
            this.OkButton.Text = "OK";
            this.OkButton.Click += new System.EventHandler(this.OkButton_Click);
            // 
            // CopyrightLabel
            // 
            this.CopyrightLabel.Anchor = System.Windows.Forms.AnchorStyles.Bottom;
            this.CopyrightLabel.Location = new System.Drawing.Point(68, 198);
            this.CopyrightLabel.Name = "CopyrightLabel";
            this.CopyrightLabel.Size = new System.Drawing.Size(160, 16);
            this.CopyrightLabel.TabIndex = 1;
            this.CopyrightLabel.Text = "© 2003 Electronic Arts Inc.";
            this.CopyrightLabel.TextAlign = System.Drawing.ContentAlignment.TopCenter;
            // 
            // CreditsBox
            // 
            this.CreditsBox.Anchor = (((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                | System.Windows.Forms.AnchorStyles.Left)
                | System.Windows.Forms.AnchorStyles.Right);
            this.CreditsBox.Location = new System.Drawing.Point(0, 72);
            this.CreditsBox.Name = "CreditsBox";
            this.CreditsBox.ReadOnly = true;
            this.CreditsBox.Size = new System.Drawing.Size(296, 110);
            this.CreditsBox.TabIndex = 3;
            this.CreditsBox.Text = "";
            // 
            // AboutForm
            // 
            this.AcceptButton = this.OkButton;
            this.AutoScaleBaseSize = new System.Drawing.Size(5, 14);
            this.ClientSize = new System.Drawing.Size(296, 262);
            this.ControlBox = false;
            this.Controls.AddRange(new System.Windows.Forms.Control[] {
                                                                          this.CreditsBox,
                                                                          this.OkButton,
                                                                          this.TitleLabel,
                                                                          this.LogoPictureBox,
                                                                          this.VersionLabel,
                                                                          this.CopyrightLabel});
            this.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((System.Byte)(0)));
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.MinimumSize = new System.Drawing.Size(296, 264);
            this.Name = "AboutForm";
            this.ShowInTaskbar = false;
            this.SizeGripStyle = System.Windows.Forms.SizeGripStyle.Hide;
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
            this.Text = "About Package Manager";
            this.ResumeLayout(false);

        }
        #endregion

        private void OkButton_Click(object sender, System.EventArgs e)
        {
            Close();
        }
    }
}
