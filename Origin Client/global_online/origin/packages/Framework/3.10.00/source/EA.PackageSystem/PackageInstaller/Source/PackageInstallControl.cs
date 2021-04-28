using System;
using System.Collections;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Windows.Forms;

namespace EA.PackageSystem.PackageInstaller {
    
    public class PackageInstallControl : System.Windows.Forms.UserControl {
        public System.Windows.Forms.Label StatusLabel;
        public System.Windows.Forms.ProgressBar ProgressBar;
        public System.Windows.Forms.TextBox TaskList;

        /// <summary> Required designer variable.</summary>
        private System.ComponentModel.Container components = null;

        public PackageInstallControl() {
            // This call is required by the Windows.Forms Form Designer.
            InitializeComponent();
        }

        /// <summary>Clean up any resources being used.</summary>
        protected override void Dispose(bool disposing) {
            if (disposing) {
                if (components != null) {
                    components.Dispose();
                }
            }
            base.Dispose(disposing);
        }

		#region Component Designer generated code
        /// <summary> 
        /// Required method for Designer support - do not modify 
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent() {
            this.StatusLabel = new System.Windows.Forms.Label();
            this.ProgressBar = new System.Windows.Forms.ProgressBar();
            this.TaskList = new System.Windows.Forms.TextBox();
            this.SuspendLayout();
            // 
            // StatusLabel
            // 
            this.StatusLabel.Anchor = ((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
                | System.Windows.Forms.AnchorStyles.Right);
            this.StatusLabel.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((System.Byte)(0)));
            this.StatusLabel.Location = new System.Drawing.Point(8, 8);
            this.StatusLabel.Name = "StatusLabel";
            this.StatusLabel.Size = new System.Drawing.Size(360, 16);
            this.StatusLabel.TabIndex = 9;
            this.StatusLabel.Text = "Installing";
            // 
            // ProgressBar
            // 
            this.ProgressBar.Anchor = ((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
                | System.Windows.Forms.AnchorStyles.Right);
            this.ProgressBar.Location = new System.Drawing.Point(8, 24);
            this.ProgressBar.Name = "ProgressBar";
            this.ProgressBar.Size = new System.Drawing.Size(360, 20);
            this.ProgressBar.TabIndex = 10;
            this.ProgressBar.Value = 66;
            // 
            // TaskList
            // 
            this.TaskList.Anchor = (((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
                | System.Windows.Forms.AnchorStyles.Left) 
                | System.Windows.Forms.AnchorStyles.Right);
            this.TaskList.BackColor = System.Drawing.SystemColors.Window;
            this.TaskList.Location = new System.Drawing.Point(8, 48);
            this.TaskList.Multiline = true;
            this.TaskList.Name = "TaskList";
            this.TaskList.ReadOnly = true;
            this.TaskList.Size = new System.Drawing.Size(360, 152);
            this.TaskList.TabIndex = 11;
            this.TaskList.Text = "";
            // 
            // PackageInstallControl
            // 
            this.Controls.AddRange(new System.Windows.Forms.Control[] {
                                                                          this.TaskList,
                                                                          this.ProgressBar,
                                                                          this.StatusLabel});
            this.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((System.Byte)(0)));
            this.Name = "PackageInstallControl";
            this.Size = new System.Drawing.Size(376, 208);
            this.ResumeLayout(false);

        }
		#endregion
    }
}
