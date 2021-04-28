using System;
using System.Collections;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Windows.Forms;

namespace EA.PackageSystem.PackageInstaller {

    public class PackageListControl : System.Windows.Forms.UserControl {
        public System.Windows.Forms.ListView PackageListView;
        public System.Windows.Forms.Label StatusLabel;
        private System.Windows.Forms.ColumnHeader PackageColumn;
        
        /// <summary>Required designer variable.</summary>
        private System.ComponentModel.Container components = null;

        public PackageListControl() {
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
            this.PackageListView = new System.Windows.Forms.ListView();
            this.PackageColumn = new System.Windows.Forms.ColumnHeader();
            this.StatusLabel = new System.Windows.Forms.Label();
            this.SuspendLayout();
            // 
            // PackageListView
            // 
            this.PackageListView.Anchor = (((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
                | System.Windows.Forms.AnchorStyles.Left) 
                | System.Windows.Forms.AnchorStyles.Right);
            this.PackageListView.CheckBoxes = true;
            this.PackageListView.Columns.AddRange(new System.Windows.Forms.ColumnHeader[] {
                                                                                              this.PackageColumn});
            this.PackageListView.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((System.Byte)(0)));
            this.PackageListView.FullRowSelect = true;
            this.PackageListView.HeaderStyle = System.Windows.Forms.ColumnHeaderStyle.Nonclickable;
            this.PackageListView.Location = new System.Drawing.Point(8, 37);
            this.PackageListView.Name = "PackageListView";
            this.PackageListView.Size = new System.Drawing.Size(360, 168);
            this.PackageListView.TabIndex = 9;
            this.PackageListView.View = System.Windows.Forms.View.Details;
            // 
            // PackageColumn
            // 
            this.PackageColumn.Text = "Package";
            this.PackageColumn.Width = 250;
            // 
            // StatusLabel
            // 
            this.StatusLabel.Anchor = ((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
                | System.Windows.Forms.AnchorStyles.Right);
            this.StatusLabel.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((System.Byte)(0)));
            this.StatusLabel.Location = new System.Drawing.Point(8, 8);
            this.StatusLabel.Name = "StatusLabel";
            this.StatusLabel.Size = new System.Drawing.Size(360, 16);
            this.StatusLabel.TabIndex = 8;
            this.StatusLabel.Text = "Checking for required packages...";
            // 
            // PackageListControl
            // 
            this.Controls.AddRange(new System.Windows.Forms.Control[] {
                                                                          this.PackageListView,
                                                                          this.StatusLabel});
            this.Name = "PackageListControl";
            this.Size = new System.Drawing.Size(376, 208);
            this.ResumeLayout(false);

        }
		#endregion
    }
}
