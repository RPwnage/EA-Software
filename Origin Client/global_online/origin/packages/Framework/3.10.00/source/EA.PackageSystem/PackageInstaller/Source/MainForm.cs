using System;
using System.Collections;
using System.Diagnostics;
using System.IO;
using System.Reflection;
using System.Windows.Forms;

using NAnt.Core.PackageCore;
using EA.PackageSystem.PackageCore;

namespace EA.PackageSystem.PackageInstaller
{

    public class MainForm : System.Windows.Forms.Form
    {

        private System.Windows.Forms.Panel BannerPanel;
        private System.Windows.Forms.PictureBox LogoPictureBox;
        private System.Windows.Forms.Label TitleLabel;
        private System.Windows.Forms.Label TitleSummaryLabel;
        private System.Windows.Forms.GroupBox TopMargin;
        private System.Windows.Forms.Label BrandingLabel;
        private System.Windows.Forms.GroupBox BottomMargin;
        private System.Windows.Forms.Button QuitButton;
        private System.Windows.Forms.Button Install;
        private System.Windows.Forms.Button PackageManager;
        private EA.PackageSystem.PackageInstaller.PackageInstallControl InstallControl;
        private EA.PackageSystem.PackageInstaller.PackageListControl ListControl;

        /// <summary>Required designer variable.</summary>
        private System.ComponentModel.Container components = null;

        EA.PackageSystem.PackageCore.PackageInstaller _installer = new EA.PackageSystem.PackageCore.PackageInstaller();

        public string PackageName = null;
        public string PackageVersion = null;

        public MainForm()
        {
            InitializeComponent();

            InstallControl.Visible = false;
            ListControl.Visible = true;
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
            System.Resources.ResourceManager resources = new System.Resources.ResourceManager(typeof(MainForm));
            this.BannerPanel = new System.Windows.Forms.Panel();
            this.TitleSummaryLabel = new System.Windows.Forms.Label();
            this.TitleLabel = new System.Windows.Forms.Label();
            this.LogoPictureBox = new System.Windows.Forms.PictureBox();
            this.QuitButton = new System.Windows.Forms.Button();
            this.Install = new System.Windows.Forms.Button();
            this.TopMargin = new System.Windows.Forms.GroupBox();
            this.BrandingLabel = new System.Windows.Forms.Label();
            this.BottomMargin = new System.Windows.Forms.GroupBox();
            this.InstallControl = new EA.PackageSystem.PackageInstaller.PackageInstallControl();
            this.ListControl = new EA.PackageSystem.PackageInstaller.PackageListControl();
            this.PackageManager = new System.Windows.Forms.Button();
            this.BannerPanel.SuspendLayout();
            this.SuspendLayout();
            // 
            // BannerPanel
            // 
            this.BannerPanel.BackColor = System.Drawing.Color.White;
            this.BannerPanel.Controls.AddRange(new System.Windows.Forms.Control[] {
                                                                                      this.TitleSummaryLabel,
                                                                                      this.TitleLabel,
                                                                                      this.LogoPictureBox});
            this.BannerPanel.Dock = System.Windows.Forms.DockStyle.Top;
            this.BannerPanel.Name = "BannerPanel";
            this.BannerPanel.Size = new System.Drawing.Size(496, 56);
            this.BannerPanel.TabIndex = 1;
            // 
            // TitleSummaryLabel
            // 
            this.TitleSummaryLabel.Anchor = ((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                | System.Windows.Forms.AnchorStyles.Right);
            this.TitleSummaryLabel.Location = new System.Drawing.Point(24, 30);
            this.TitleSummaryLabel.Name = "TitleSummaryLabel";
            this.TitleSummaryLabel.Size = new System.Drawing.Size(406, 16);
            this.TitleSummaryLabel.TabIndex = 4;
            this.TitleSummaryLabel.Text = "Select packages to install.";
            // 
            // TitleLabel
            // 
            this.TitleLabel.Anchor = ((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                | System.Windows.Forms.AnchorStyles.Right);
            this.TitleLabel.Font = new System.Drawing.Font("Tahoma", 9.75F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((System.Byte)(0)));
            this.TitleLabel.Location = new System.Drawing.Point(16, 8);
            this.TitleLabel.Name = "TitleLabel";
            this.TitleLabel.Size = new System.Drawing.Size(414, 16);
            this.TitleLabel.TabIndex = 3;
            this.TitleLabel.Text = "Install ${FullPackageName}";
            // 
            // LogoPictureBox
            // 
            this.LogoPictureBox.Anchor = (System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right);
            this.LogoPictureBox.Image = ((System.Drawing.Bitmap)(resources.GetObject("LogoPictureBox.Image")));
            this.LogoPictureBox.Location = new System.Drawing.Point(438, 6);
            this.LogoPictureBox.Name = "LogoPictureBox";
            this.LogoPictureBox.Size = new System.Drawing.Size(48, 48);
            this.LogoPictureBox.TabIndex = 2;
            this.LogoPictureBox.TabStop = false;
            // 
            // QuitButton
            // 
            this.QuitButton.Anchor = (System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right);
            this.QuitButton.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.QuitButton.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.QuitButton.Location = new System.Drawing.Point(408, 326);
            this.QuitButton.Name = "QuitButton";
            this.QuitButton.TabIndex = 2;
            this.QuitButton.Text = "Cancel";
            this.QuitButton.Click += new System.EventHandler(this.QuitButton_Click);
            // 
            // Install
            // 
            this.Install.Anchor = (System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right);
            this.Install.DialogResult = System.Windows.Forms.DialogResult.OK;
            this.Install.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.Install.Location = new System.Drawing.Point(326, 326);
            this.Install.Name = "Install";
            this.Install.TabIndex = 2;
            this.Install.Text = "Install";
            this.Install.Click += new System.EventHandler(this.Install_Click);
            // 
            // TopMargin
            // 
            this.TopMargin.Anchor = ((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                | System.Windows.Forms.AnchorStyles.Right);
            this.TopMargin.ForeColor = System.Drawing.SystemColors.GrayText;
            this.TopMargin.Location = new System.Drawing.Point(-8, 50);
            this.TopMargin.Name = "TopMargin";
            this.TopMargin.Size = new System.Drawing.Size(510, 8);
            this.TopMargin.TabIndex = 4;
            this.TopMargin.TabStop = false;
            // 
            // BrandingLabel
            // 
            this.BrandingLabel.Anchor = (System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left);
            this.BrandingLabel.BackColor = System.Drawing.Color.Transparent;
            this.BrandingLabel.Enabled = false;
            this.BrandingLabel.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.BrandingLabel.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((System.Byte)(0)));
            this.BrandingLabel.ForeColor = System.Drawing.SystemColors.ControlText;
            this.BrandingLabel.Location = new System.Drawing.Point(6, 305);
            this.BrandingLabel.Name = "BrandingLabel";
            this.BrandingLabel.Size = new System.Drawing.Size(144, 17);
            this.BrandingLabel.TabIndex = 5;
            this.BrandingLabel.Text = "packages.ea.com";
            string webServices2URL = EA.PackageSystem.PackageCore.Services.WebServices2URL.GetWebServices2URL();
            if (webServices2URL != null)
            {
                Uri uri = new Uri(webServices2URL);
                this.BrandingLabel.Text = uri.Host;
            }
            // 
            // BottomMargin
            // 
            this.BottomMargin.Anchor = ((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)
                | System.Windows.Forms.AnchorStyles.Right);
            this.BottomMargin.Location = new System.Drawing.Point(148, 304);
            this.BottomMargin.Name = "BottomMargin";
            this.BottomMargin.Size = new System.Drawing.Size(340, 8);
            this.BottomMargin.TabIndex = 6;
            this.BottomMargin.TabStop = false;
            // 
            // InstallControl
            // 
            this.InstallControl.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((System.Byte)(0)));
            this.InstallControl.Location = new System.Drawing.Point(0, 64);
            this.InstallControl.Name = "InstallControl";
            this.InstallControl.Size = new System.Drawing.Size(496, 240);
            this.InstallControl.TabIndex = 7;
            this.InstallControl.Visible = false;
            // 
            // ListControl
            // 
            this.ListControl.Location = new System.Drawing.Point(0, 64);
            this.ListControl.Name = "ListControl";
            this.ListControl.Size = new System.Drawing.Size(496, 232);
            this.ListControl.TabIndex = 8;
            // 
            // PackageManager
            // 
            this.PackageManager.Anchor = (System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right);
            this.PackageManager.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.PackageManager.Enabled = false;
            this.PackageManager.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.PackageManager.Location = new System.Drawing.Point(8, 326);
            this.PackageManager.Name = "PackageManager";
            this.PackageManager.Size = new System.Drawing.Size(160, 23);
            this.PackageManager.TabIndex = 2;
            this.PackageManager.Text = "Open Package Manager";
            this.PackageManager.Click += new System.EventHandler(this.PackageManager_Click);
            // 
            // MainForm
            // 
            this.AcceptButton = this.Install;
            this.AutoScaleBaseSize = new System.Drawing.Size(5, 14);
            this.CancelButton = this.QuitButton;
            this.ClientSize = new System.Drawing.Size(496, 360);
            this.Controls.AddRange(new System.Windows.Forms.Control[] {
                                                                          this.BottomMargin,
                                                                          this.BrandingLabel,
                                                                          this.QuitButton,
                                                                          this.BannerPanel,
                                                                          this.Install,
                                                                          this.TopMargin,
                                                                          this.ListControl,
                                                                          this.InstallControl,
                                                                          this.PackageManager});
            this.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((System.Byte)(0)));
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "MainForm";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
            this.Text = "Package Installer";
            this.Load += new System.EventHandler(this.MainForm_Load);
            this.BannerPanel.ResumeLayout(false);
            this.ResumeLayout(false);

        }
        #endregion

        private void Install_Click(object sender, System.EventArgs e)
        {
            Install.Enabled = false;
            QuitButton.Text = "Cancel";

            InstallControl.Visible = !InstallControl.Visible;
            ListControl.Visible = !ListControl.Visible;

            // remove uncheck releases from the installer releases
            foreach (ListViewItem item in ListControl.PackageListView.Items)
            {
                if (!item.Checked)
                {
                    Release release = (Release)item.Tag;
                    _installer.Releases.Remove(release);
                }
            }

            InstallControl.ProgressBar.Minimum = 0;
            InstallControl.ProgressBar.Maximum = 100;

            // hook up events and do the install
            _installer.UpdateProgress += new InstallProgressEventHandler(UpdateProgress);

            try
            {
                _installer.Install();
                InstallControl.ProgressBar.Value = InstallControl.ProgressBar.Maximum;
                InstallControl.StatusLabel.Text = "Install completed";
                InstallControl.TaskList.Text += InstallControl.StatusLabel.Text + Environment.NewLine;

            }
            catch (Exception exception)
            {
                if (_installer.Break)
                {
                    InstallControl.ProgressBar.Value = InstallControl.ProgressBar.Maximum;
                    InstallControl.StatusLabel.Text = "Install canceled";
                    InstallControl.TaskList.Text += InstallControl.StatusLabel.Text + Environment.NewLine;
                }
                else
                {
                    InstallControl.ProgressBar.Value = InstallControl.ProgressBar.Maximum;
                    InstallControl.StatusLabel.Font = new System.Drawing.Font(InstallControl.StatusLabel.Font,
                        InstallControl.StatusLabel.Font.Style | System.Drawing.FontStyle.Bold);
                    InstallControl.StatusLabel.Text = "Install error";
                    InstallControl.TaskList.Text += InstallControl.StatusLabel.Text + Environment.NewLine;
                    App.Instance.ShowError(exception.Message, MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
                }
            }

            _installer.UpdateProgress -= new InstallProgressEventHandler(UpdateProgress);
            QuitButton.Text = "Close";
        }

        Release _lastRelease = null;

        void UpdateProgress(object sender, InstallProgressEventArgs e)
        {
            InstallControl.StatusLabel.Text = e.Message;
            if (_lastRelease != e.Release)
            {
                string msg = String.Format("Installing {0}-{1}", e.Release.Name, e.Release.Version);
                InstallControl.TaskList.Text += msg + Environment.NewLine;
                _lastRelease = e.Release;
            }

            InstallControl.ProgressBar.Value = Math.Max(0,e.GlobalProgress);
            Application.DoEvents();
        }

        private void MainForm_Load(object sender, System.EventArgs e)
        {
            try
            {
                // make the main window visible so the user isn't left hanging
                TitleLabel.Text = String.Format("Installing {0}-{1}", PackageName, PackageVersion);
                PackageManager.Enabled = false;
                Install.Enabled = false;
                QuitButton.Enabled = false;
                QuitButton.Text = "Close";
                Show();
                Cursor.Current = Cursors.WaitCursor;
                Application.DoEvents();

                // get required releases
                _installer.Releases.Add(PackageName, PackageVersion, true);
                Cursor.Current = Cursors.Arrow;

                PackageManager.Enabled = true;
                Install.Enabled = true;
                QuitButton.Enabled = true;
                ListControl.StatusLabel.Text = "The following packages will be installed:";

                foreach (Release release in _installer.Releases)
                {
                    if (PackageMap.Instance.Releases.FindByNameAndVersion(release.Name, release.Version) == null)
                    {
                        ListViewItem item = new ListViewItem(String.Format("{0}-{1}", release.Name, release.Version));
                        item.Checked = true;
                        item.Tag = release;
                        ListControl.PackageListView.Items.Add(item);
                    }
                }

                if (ListControl.PackageListView.Items.Count == 0)
                {
                    ListControl.StatusLabel.Text = "All required packages are already installed.";
                    Install.Enabled = false;
                }


            }
            catch (Exception ex)
            {
                string msg = String.Format("There was a problem accessing the Package Server.\n\n{0}", ex.Message);
                App.Instance.ShowError(msg, MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
                Close();
            }
        }

        private void QuitButton_Click(object sender, System.EventArgs e)
        {
            if (InstallControl.Visible)
            {
                if (QuitButton.Text == "Close")
                {
                    Close();
                }
                else
                {
                    _installer.Break = true;
                    QuitButton.Text = "Close";
                }

            }
            else
            {
                Close();
            }
        }

        private void PackageManager_Click(object sender, System.EventArgs e)
        {
            string path = Path.Combine(Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location), "PackageManager.exe");
            try
            {
                Process.Start(path);
                Close();
            }
            catch (Exception ex)
            {
                string msg = String.Format("Could not open '{0}' because:\n{1}", path, ex.Message);
                App.Instance.ShowError(msg, MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
            }
        }
    }
}
