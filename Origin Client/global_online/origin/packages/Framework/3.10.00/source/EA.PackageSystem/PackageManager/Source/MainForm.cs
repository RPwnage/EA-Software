using System;
using System.Linq;
using System.Diagnostics;
using System.Drawing;
using System.Collections;
using System.Collections.Generic;
using System.ComponentModel;
using System.Text;
using System.IO;
using System.Windows.Forms;

using EA.Common;
using EA.Common.CommandBars;
using EA.Common.CommandHandling;

using NAnt.Core.PackageCore;
using EA.PackageSystem.PackageCore.Services;
using Release = NAnt.Core.PackageCore.Release;

namespace EA.PackageSystem.PackageManager
{
    public class MainForm : System.Windows.Forms.Form
    {
        private static readonly Color AlternateRowColor = Color.FromArgb(237, 243, 254);

        private System.Windows.Forms.StatusBar StatusBar;
        private System.Windows.Forms.Panel ToolbarPanel;
        private System.Windows.Forms.Panel ViewPanel;
        private System.Windows.Forms.Button BrowseButton;
        private System.Windows.Forms.Button ClearSearchButton;

        private System.Windows.Forms.ListView ListView;
        private System.Windows.Forms.Panel DetailsPanel;
        private System.Windows.Forms.Splitter Splitter;
        private System.Windows.Forms.StatusBarPanel PackageRootPanel;
        private System.Windows.Forms.StatusBarPanel StatusPanel;
        private System.Windows.Forms.StatusBarPanel PadPanel1;
        private System.Windows.Forms.StatusBarPanel PadPanel2;
        private System.Windows.Forms.Label SearchLabel;
        private System.ComponentModel.IContainer components = null;
        private System.Windows.Forms.Panel SearchBoxPanel;
        public System.Windows.Forms.StatusBarPanel BackgroundActionPanel;

        private EA.Common.WebBrowser WebBrowser;
        private MyTextBox SearchTextBox;

        public MainForm()
        {
            // Required for Windows Form Designer support
            InitializeComponent();

            // create the main menu and toolbars
            Menu = CommandBarFactory.Instance.CreateMainMenuFromResource(
                "EA.PackageSystem.PackageManager.Resources.MainMenu.xml",
                "MainMenu", new EventHandler(Menu_CommandClick));

            // create web browser control
            WebBrowser = new EA.Common.WebBrowser();
            WebBrowser.Dock = DockStyle.Fill;
            WebBrowser.Parent = DetailsPanel;
            WebBrowser.ContainingControl = this;
            WebBrowser.BeforeNavigate += new BeforeNavigateEventHandler(WebBrowser_OnBeforeNavigate);
            WebBrowser.HandleCreated += new EventHandler(WebBrowser_OnHandleCreated);

            // create search box
            SearchTextBox = new MyTextBox();
            SearchTextBox.Dock = DockStyle.Fill;
            SearchTextBox.Parent = SearchBoxPanel;
            SearchTextBox.TextChanged += new EventHandler(SearchTextBox_TextChanged);

            PackageMap.Init(null);

            PackageMap.Instance.Releases.OrderBy(r => r);
            PackageRootPanel.Text = PackageMap.Instance.PackageRoots.ToString();


            InitializeReleaseList();
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
            this.StatusBar = new System.Windows.Forms.StatusBar();
            this.StatusPanel = new System.Windows.Forms.StatusBarPanel();
            this.BackgroundActionPanel = new System.Windows.Forms.StatusBarPanel();
            this.PadPanel1 = new System.Windows.Forms.StatusBarPanel();
            this.PackageRootPanel = new System.Windows.Forms.StatusBarPanel();
            this.PadPanel2 = new System.Windows.Forms.StatusBarPanel();
            this.ToolbarPanel = new System.Windows.Forms.Panel();
            this.SearchBoxPanel = new System.Windows.Forms.Panel();
            this.BrowseButton = new System.Windows.Forms.Button();
            this.ClearSearchButton = new System.Windows.Forms.Button();
            this.SearchLabel = new System.Windows.Forms.Label();
            this.ViewPanel = new System.Windows.Forms.Panel();
            this.DetailsPanel = new System.Windows.Forms.Panel();
            this.Splitter = new System.Windows.Forms.Splitter();
            this.ListView = new System.Windows.Forms.ListView();
            ((System.ComponentModel.ISupportInitialize)(this.StatusPanel)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.BackgroundActionPanel)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.PadPanel1)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.PackageRootPanel)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.PadPanel2)).BeginInit();
            this.ToolbarPanel.SuspendLayout();
            this.ViewPanel.SuspendLayout();
            this.SuspendLayout();
            // 
            // StatusBar
            // 
            this.StatusBar.Location = new System.Drawing.Point(0, 544);
            this.StatusBar.Name = "StatusBar";
            this.StatusBar.Panels.AddRange(new System.Windows.Forms.StatusBarPanel[] {
                                                                                         this.StatusPanel,
                                                                                         this.BackgroundActionPanel,
                                                                                         this.PadPanel1,
                                                                                         this.PackageRootPanel,
                                                                                         this.PadPanel2});
            this.StatusBar.ShowPanels = true;
            this.StatusBar.Size = new System.Drawing.Size(624, 22);
            this.StatusBar.TabIndex = 1;
            this.StatusBar.Text = "StatusBar";
            // 
            // StatusPanel
            // 
            this.StatusPanel.AutoSize = System.Windows.Forms.StatusBarPanelAutoSize.Spring;
            this.StatusPanel.BorderStyle = System.Windows.Forms.StatusBarPanelBorderStyle.None;
            this.StatusPanel.MinWidth = 100;
            this.StatusPanel.Text = "140 Packages (Filtered)";
            this.StatusPanel.Width = 248;
            // 
            // BackgroundActionPanel
            // 
            this.BackgroundActionPanel.AutoSize = System.Windows.Forms.StatusBarPanelAutoSize.Contents;
            this.BackgroundActionPanel.BorderStyle = System.Windows.Forms.StatusBarPanelBorderStyle.None;
            this.BackgroundActionPanel.MinWidth = 200;
            this.BackgroundActionPanel.Text = "Ready";
            this.BackgroundActionPanel.ToolTipText = "Background activity";
            this.BackgroundActionPanel.Width = 200;
            // 
            // PadPanel1
            // 
            this.PadPanel1.BorderStyle = System.Windows.Forms.StatusBarPanelBorderStyle.None;
            this.PadPanel1.Width = 30;
            // 
            // PackageRootPanel
            // 
            this.PackageRootPanel.Alignment = System.Windows.Forms.HorizontalAlignment.Right;
            this.PackageRootPanel.AutoSize = System.Windows.Forms.StatusBarPanelAutoSize.Contents;
            this.PackageRootPanel.BorderStyle = System.Windows.Forms.StatusBarPanelBorderStyle.None;
            this.PackageRootPanel.MinWidth = 100;
            this.PackageRootPanel.Text = "D:\\packages";
            this.PackageRootPanel.ToolTipText = "Package root path";
            // 
            // PadPanel2
            // 
            this.PadPanel2.BorderStyle = System.Windows.Forms.StatusBarPanelBorderStyle.None;
            this.PadPanel2.Width = 30;
            // 
            // ToolbarPanel
            // 
            this.ToolbarPanel.Controls.AddRange(new System.Windows.Forms.Control[] {
                                                                                       this.SearchBoxPanel,
                                                                                       this.BrowseButton,
                                                                                       this.ClearSearchButton,
                                                                                       this.SearchLabel});
            this.ToolbarPanel.Dock = System.Windows.Forms.DockStyle.Top;
            this.ToolbarPanel.Name = "ToolbarPanel";
            this.ToolbarPanel.Size = new System.Drawing.Size(624, 40);
            this.ToolbarPanel.TabIndex = 0;
            // 
            // SearchBoxPanel
            // 
            this.SearchBoxPanel.Anchor = (System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right);
            this.SearchBoxPanel.Location = new System.Drawing.Point(400, 9);
            this.SearchBoxPanel.Name = "SearchBoxPanel";
            this.SearchBoxPanel.Size = new System.Drawing.Size(128, 21);
            this.SearchBoxPanel.TabIndex = 4;
            // 
            // BrowseButton
            // 
            this.BrowseButton.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.BrowseButton.Location = new System.Drawing.Point(8, 8);
            this.BrowseButton.Name = "BrowseButton";
            this.BrowseButton.TabIndex = 0;
            this.BrowseButton.Text = "&Open";
            this.BrowseButton.Click += new System.EventHandler(this.BrowseButton_Click);
            // 
            // ClearSearchButton
            // 
            this.ClearSearchButton.Anchor = (System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right);
            this.ClearSearchButton.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.ClearSearchButton.Location = new System.Drawing.Point(536, 8);
            this.ClearSearchButton.Name = "ClearSearchButton";
            this.ClearSearchButton.TabIndex = 5;
            this.ClearSearchButton.Text = "&Clear";
            this.ClearSearchButton.Click += new System.EventHandler(this.ClearSearchButton_Click);
            // 
            // SearchLabel
            // 
            this.SearchLabel.Anchor = (System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right);
            this.SearchLabel.Location = new System.Drawing.Point(357, 12);
            this.SearchLabel.Name = "SearchLabel";
            this.SearchLabel.Size = new System.Drawing.Size(45, 16);
            this.SearchLabel.TabIndex = 3;
            this.SearchLabel.Text = "&Search:";
            // 
            // ViewPanel
            // 
            this.ViewPanel.Controls.AddRange(new System.Windows.Forms.Control[] {
                                                                                    this.DetailsPanel,
                                                                                    this.Splitter,
                                                                                    this.ListView});
            this.ViewPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this.ViewPanel.Location = new System.Drawing.Point(0, 40);
            this.ViewPanel.Name = "ViewPanel";
            this.ViewPanel.Size = new System.Drawing.Size(624, 504);
            this.ViewPanel.TabIndex = 2;
            // 
            // DetailsPanel
            // 
            this.DetailsPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this.DetailsPanel.Location = new System.Drawing.Point(0, 187);
            this.DetailsPanel.Name = "DetailsPanel";
            this.DetailsPanel.Size = new System.Drawing.Size(624, 317);
            this.DetailsPanel.TabIndex = 2;
            // 
            // Splitter
            // 
            this.Splitter.Dock = System.Windows.Forms.DockStyle.Top;
            this.Splitter.Location = new System.Drawing.Point(0, 184);
            this.Splitter.Name = "Splitter";
            this.Splitter.Size = new System.Drawing.Size(624, 3);
            this.Splitter.TabIndex = 1;
            this.Splitter.TabStop = false;
            // 
            // ListView
            // 
            this.ListView.AllowColumnReorder = true;
            this.ListView.Dock = System.Windows.Forms.DockStyle.Top;
            this.ListView.FullRowSelect = true;
            this.ListView.HideSelection = false;
            this.ListView.Name = "ListView";
            this.ListView.Size = new System.Drawing.Size(624, 184);
            this.ListView.TabIndex = 0;
            this.ListView.View = System.Windows.Forms.View.Details;
            this.ListView.ColumnClick += new System.Windows.Forms.ColumnClickEventHandler(this.ListView_ColumnClick);
            this.ListView.SelectedIndexChanged += new System.EventHandler(this.ListView_SelectedIndexChanged);
            // 
            // MainForm
            // 
            this.AutoScaleBaseSize = new System.Drawing.Size(5, 14);
            this.ClientSize = new System.Drawing.Size(624, 566);
            this.Controls.AddRange(new System.Windows.Forms.Control[] {
                                                                          this.ViewPanel,
                                                                          this.ToolbarPanel,
                                                                          this.StatusBar});
            this.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((System.Byte)(0)));
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.MinimumSize = new System.Drawing.Size(536, 192);
            this.Name = "MainForm";
            this.Text = "Package Manager";
            ((System.ComponentModel.ISupportInitialize)(this.StatusPanel)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.BackgroundActionPanel)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.PadPanel1)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.PackageRootPanel)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.PadPanel2)).EndInit();
            this.ToolbarPanel.ResumeLayout(false);
            this.ViewPanel.ResumeLayout(false);
            this.ResumeLayout(false);

        }
        #endregion

        public string PackageRootPanelText
        {
            get { return PackageRootPanel.Text; }
            set { PackageRootPanel.Text = value; }
        }

        void InitializeReleaseList()
        {
            ListView.BeginUpdate();
            ListView.Items.Clear();
            ListView.Columns.Clear();

            ListView.Columns.Add("Package", 175, HorizontalAlignment.Left);
            ListView.Columns.Add("Size", 50, HorizontalAlignment.Right);
            ListView.Columns.Add("Use Count", 75, HorizontalAlignment.Right);
            ListView.Columns.Add("Last Used", 100, HorizontalAlignment.Left);
            ListView.Columns.Add("Status", 100, HorizontalAlignment.Left);
            ListView.Columns.Add("License", 100, HorizontalAlignment.Left);
            ListView.Columns.Add("Contact", 100, HorizontalAlignment.Left);
            ListView.Columns.Add("Summary", 211, HorizontalAlignment.Left);

            ListView.EndUpdate();
        }

        ListViewItem CreateListItem(Release release)
        {
            ReleaseDetails details = null;
            if (release.Tag != null)
            {
                details = release.Tag as ReleaseDetails;
            }
            if (details == null)
            {
                details = new ReleaseDetails(release);
            }
            release.Tag = details;

            ListViewItem item = new ListViewItem();
            item.Text = release.FullName;

            string sizeText = "";
            string useCountText = "";
            string lastUsedText = "";
            string statusText = "";
            string licenseText = "";
            string contactText = "";
            string summaryText = "";

            if (release.Summary != null)
            {
                summaryText = release.Summary;
            }

            if (release.License != null)
            {
                licenseText = release.License;
            }

            if (release.ContactName != null)
            {
                contactText = release.ContactName;
            }

            if (details.Size > 0)
            {
                sizeText = details.GetSizeString();
            }

            item.SubItems.Add(sizeText);
            item.SubItems.Add(useCountText);
            item.SubItems.Add(lastUsedText);
            item.SubItems.Add(statusText);
            item.SubItems.Add(licenseText);
            item.SubItems.Add(contactText);
            item.SubItems.Add(summaryText);

            item.Tag = release;
            return item;
        }

        bool PassesFilter(Release release)
        {
            string filter = SearchTextBox.Text.ToLower();

            if (filter == "")
            {
                return true;
            }

            bool passes = true;
            foreach (string token in filter.Split())
            {
                if ((release.FullName.ToLower().IndexOf(token) != -1) ||
                    (release.Summary != null && release.Summary.ToLower().IndexOf(token) != -1) ||
                    (release.ContactName != null && release.ContactName.ToLower().IndexOf(token) != -1) ||
                    (release.ContactEmail != null && release.ContactEmail.ToLower().IndexOf(token) != -1) ||
                    (release.Status.ToString().ToLower().IndexOf(token) != -1) ||
                    (release.License != null && release.License.ToLower().IndexOf(token) != -1))
                {
                }
                else
                {
                    passes = false;
                }
            }
            return passes;
        }

        public void RefreshAll()
        {
            RefreshReleaseList();
            RefreshDetails();
        }

        void RefreshReleaseList()
        {
            Release lastFocusedRelease = GetFocusedRelease();

            ListView.BeginUpdate();
            ListView.Items.Clear();
            foreach (Release release in PackageMap.Instance.Releases)
            {
                if (PassesFilter(release))
                {
                    ListView.Items.Add(CreateListItem(release));
                }
            }
            RefreshRowColors();
            ListView.EndUpdate();

            if (ListView.Items.Count > 0)
            {
                if (lastFocusedRelease != null)
                {
                    SetFocusedRelease(lastFocusedRelease.FullName);
                }

                if (GetFocusedRelease() == null)
                {
                    SetFocusedRelease(ListView.Items[0].Text);
                }
            }

            RefreshStatusBar();
        }

        void RefreshRowColors()
        {
            int index = 0;
            foreach (ListViewItem item in ListView.Items)
            {
                if (index % 2 == 0)
                {
                    item.BackColor = Color.White;
                }
                else
                {
                    item.BackColor = AlternateRowColor;
                }
                index++;
            }
        }

        void RefreshStatusBar()
        {
            string filterText = "";
            if (SearchTextBox.Text.Length > 0)
            {
                filterText = " (Filtered)";
            }
            StatusPanel.Text = String.Format("{0} packages{1}", ListView.Items.Count, filterText);
        }

        void RefreshDetails()
        {
            Release release = GetFocusedRelease();
            if (release != null)
            {
                WebBrowser.Html = ((Details)(release.Tag)).ToHtml();
            }
            else
            {
                WebBrowser.Html = BlankDetails.Instance.ToHtml();
            }
        }

        public Release GetFocusedRelease()
        {
            if (ListView.FocusedItem != null)
            {
                return (Release)ListView.FocusedItem.Tag;
            }
            else
            {
                return null;
            }
        }

        public ICollection<Release> GetSelectedReleases()
        {
            var selected = new List<Release>();
            foreach (ListViewItem item in ListView.SelectedItems)
            {
                selected.Add((Release)item.Tag);
            }
            return selected;
        }

        public void SetFocusedRelease(string fullPackageName)
        {
            foreach (ListViewItem item in ListView.Items)
            {
                if (String.Compare(item.Text, fullPackageName, true) == 0)
                {
                    ListView.SelectedItems.Clear();
                    item.Focused = true;
                    item.Selected = true;
                    item.EnsureVisible();
                    break;
                }
            }
        }

        public void SelectAllReleases()
        {
            foreach (ListViewItem item in ListView.Items)
            {
                item.Selected = true;
            }
        }

        private void SearchTextBox_TextChanged(object sender, System.EventArgs e)
        {
            RefreshReleaseList();
            RefreshDetails();
        }

        void WebBrowser_OnHandleCreated(object sender, EventArgs e)
        {
            RefreshAll();
        }

        void WebBrowser_OnBeforeNavigate(object sender, BeforeNavigateEventArgs e)
        {
            const string processPrefix = "start://";
            const string commandPrefix = "command://";
            const string packagePrefix = "package://";

            if (e.Url.StartsWith(processPrefix))
            {
                string fileName = e.Url.Substring(processPrefix.Length);
                try
                {
                    Process.Start(fileName);
                }
                catch (Exception ex)
                {
                    string msg = String.Format("Could not open '{0}' because:\n{1}", fileName, ex.Message);
                    App.Instance.ShowError(msg);
                }
                e.Cancel = true;

            }
            else if (e.Url.StartsWith(commandPrefix))
            {
                string command = e.Url.Substring(commandPrefix.Length, e.Url.Length - 1 - commandPrefix.Length);
                CommandFactory.Instance.ExecuteCommand(command);
                e.Cancel = true;

            }
            else if (e.Url.StartsWith(packagePrefix))
            {
                string release = e.Url.Substring(packagePrefix.Length);
                if (release.Length > 0)
                {
                    char c = release[release.Length - 1];
                    if (c == Path.AltDirectorySeparatorChar || c == Path.DirectorySeparatorChar)
                        release = release.Substring(0, release.Length - 1);
                }
                SearchTextBox.Text = "";
                SetFocusedRelease(release);
                e.Cancel = true;
            }
        }

        void ListView_SelectedIndexChanged(object sender, System.EventArgs e)
        {
            RefreshDetails();
        }

        void ListView_ColumnClick(object sender, ColumnClickEventArgs e)
        {
            var s = (ListViewItemComparer)ListView.ListViewItemSorter ?? new ListViewItemComparer();
            s.FlipSortOrderIfColumnIs((PackageColumn)e.Column);
            ListView.ListViewItemSorter = s;
            ListView.Sort();

            // make sure if any item is focused it stays visible, otherwise the user loses their position
            if (ListView.FocusedItem != null)
                ListView.EnsureVisible(ListView.FocusedItem.Index);

            // recolor alternate colored rows
            RefreshRowColors();
        }

        public void Menu_CommandClick(object s, EventArgs e)
        {
            CommandMenuItem item = s as CommandMenuItem;
            if (item != null)
            {
                CommandFactory.Instance.ExecuteCommand(item.Name);
            }
        }

        private void BrowseButton_Click(object sender, System.EventArgs e)
        {
            CommandFactory.Instance.ExecuteCommand("Package.Browse");
        }

        private void UpdateButton_Click(object sender, System.EventArgs e)
        {
            CommandFactory.Instance.ExecuteCommand("Package.Update");
        }

        private void DeleteButton_Click(object sender, System.EventArgs e)
        {
            CommandFactory.Instance.ExecuteCommand("Package.Delete");
        }

        private void ClearSearchButton_Click(object sender, System.EventArgs e)
        {
            SearchTextBox.Text = "";
        }

        private class ListViewItemComparer : IComparer
        {
            // public PackageColumn Column { get; set; }

            public ListViewItemComparer(PackageColumn column = PackageColumn.Name, SortOrder sortOrder = SortOrder.Ascending)
            {
                _packageSorter = new PackageSorter(column, (PackageSortOrder)(int)_packageSorter.SortOrder);
            }

            public void FlipSortOrderIfColumnIs(PackageColumn c)
            {
                // 0 -> 0
                // 1 -> 2
                // 2 -> 1
                //
                // y = (3 - x) % 3

                if (_packageSorter.Column == c)
                    _packageSorter.SortOrder = (PackageSortOrder)((3 - (int)_packageSorter.SortOrder) % 3);
            }

            public int Compare(object x, object y)
            {
                return _packageSorter.Compare((Release)((ListViewItem)x).Tag, (Release)((ListViewItem)y).Tag);
            }

            private PackageSorter _packageSorter;
        }
    }
}
