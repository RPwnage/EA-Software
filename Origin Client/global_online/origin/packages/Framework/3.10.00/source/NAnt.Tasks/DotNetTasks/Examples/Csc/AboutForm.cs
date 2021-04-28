using System;
using System.Drawing;
using System.Collections;
using System.ComponentModel;
using System.Diagnostics;
using System.Reflection;
using System.Windows.Forms;

namespace About
{
	/// <summary>
	/// Summary description for AboutForm.
	/// </summary>
	public class AboutForm : System.Windows.Forms.Form
	{
        static void Main() {
            AboutForm form = new AboutForm();
            form.Show();
        }

		private System.Windows.Forms.Button buttonOk;
		private System.Windows.Forms.LinkLabel linkLabel1;
		private System.Windows.Forms.LinkLabel linkLabel2;
		private System.Windows.Forms.PictureBox pictureBox1;
		private System.Windows.Forms.ContextMenu m_contextMenu;
		private System.Windows.Forms.MenuItem m_miCopy;
		private System.Windows.Forms.Label label5;
		private System.Windows.Forms.ListView m_listView;
		private System.Windows.Forms.Label label3;
		private System.Windows.Forms.Label label2;
		/// <summary>
		/// Required designer variable.
		/// </summary>
		private System.ComponentModel.Container components = null;

		public AboutForm()
		{
			//
			// Required for Windows Form Designer support
			//
			InitializeComponent();

			StartPosition = FormStartPosition.CenterParent;

			// Create a new link using the Add method of the LinkCollection class.
			linkLabel1.Links.Add(0, linkLabel1.Text.Length, "http://apollo.eac.ad.ea.com");
			linkLabel2.Links.Add(0, linkLabel2.Text.Length, "mailto:wwframeworkdev@ea.com");
		}

		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		protected override void Dispose( bool disposing )
		{
			if( disposing )
			{
				if(components != null)
				{
					components.Dispose();
				}
			}
			base.Dispose( disposing );
		}

		#region Windows Form Designer generated code
		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		private void InitializeComponent()
		{
            System.Resources.ResourceManager resources = new System.Resources.ResourceManager(typeof(AboutForm));
            this.buttonOk = new System.Windows.Forms.Button();
            this.linkLabel1 = new System.Windows.Forms.LinkLabel();
            this.linkLabel2 = new System.Windows.Forms.LinkLabel();
            this.pictureBox1 = new System.Windows.Forms.PictureBox();
            this.m_contextMenu = new System.Windows.Forms.ContextMenu();
            this.m_miCopy = new System.Windows.Forms.MenuItem();
            this.label5 = new System.Windows.Forms.Label();
            this.m_listView = new System.Windows.Forms.ListView();
            this.label3 = new System.Windows.Forms.Label();
            this.label2 = new System.Windows.Forms.Label();
            this.SuspendLayout();
            // 
            // buttonOk
            // 
            this.buttonOk.Anchor = System.Windows.Forms.AnchorStyles.Bottom;
            this.buttonOk.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.buttonOk.Location = new System.Drawing.Point(165, 264);
            this.buttonOk.Name = "buttonOk";
            this.buttonOk.TabIndex = 0;
            this.buttonOk.Text = "Ok";
            // 
            // linkLabel1
            // 
            this.linkLabel1.Anchor = (System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right);
            this.linkLabel1.Location = new System.Drawing.Point(248, 8);
            this.linkLabel1.Name = "linkLabel1";
            this.linkLabel1.Size = new System.Drawing.Size(152, 23);
            this.linkLabel1.TabIndex = 2;
            this.linkLabel1.TabStop = true;
            this.linkLabel1.Text = "http://apollo.eac.ad.ea.com";
            this.linkLabel1.LinkClicked += new System.Windows.Forms.LinkLabelLinkClickedEventHandler(this.linkLabel_LinkClicked);
            // 
            // linkLabel2
            // 
            this.linkLabel2.Anchor = (System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right);
            this.linkLabel2.Location = new System.Drawing.Point(248, 24);
            this.linkLabel2.Name = "linkLabel2";
            this.linkLabel2.Size = new System.Drawing.Size(152, 23);
            this.linkLabel2.TabIndex = 3;
            this.linkLabel2.TabStop = true;
            this.linkLabel2.Text = "wwframeworkdev@ea.com";
            this.linkLabel2.LinkClicked += new System.Windows.Forms.LinkLabelLinkClickedEventHandler(this.linkLabel_LinkClicked);
            // 
            // pictureBox1
            // 
            this.pictureBox1.Image = ((System.Drawing.Bitmap)(resources.GetObject("pictureBox1.Image")));
            this.pictureBox1.Location = new System.Drawing.Point(16, 16);
            this.pictureBox1.Name = "pictureBox1";
            this.pictureBox1.Size = new System.Drawing.Size(16, 16);
            this.pictureBox1.TabIndex = 4;
            this.pictureBox1.TabStop = false;
            // 
            // m_contextMenu
            // 
            this.m_contextMenu.MenuItems.AddRange(new System.Windows.Forms.MenuItem[] {
                                                                                          this.m_miCopy});
            // 
            // m_miCopy
            // 
            this.m_miCopy.Index = 0;
            this.m_miCopy.Text = "Copy";
            this.m_miCopy.Click += new System.EventHandler(this.m_miCopy_Click);
            // 
            // label5
            // 
            this.label5.Location = new System.Drawing.Point(10, 47);
            this.label5.Name = "label5";
            this.label5.Size = new System.Drawing.Size(176, 16);
            this.label5.TabIndex = 9;
            this.label5.Text = "Program Assemblies:";
            // 
            // m_listView
            // 
            this.m_listView.Anchor = (((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
                | System.Windows.Forms.AnchorStyles.Left) 
                | System.Windows.Forms.AnchorStyles.Right);
            this.m_listView.FullRowSelect = true;
            this.m_listView.HeaderStyle = System.Windows.Forms.ColumnHeaderStyle.Nonclickable;
            this.m_listView.Location = new System.Drawing.Point(10, 63);
            this.m_listView.Name = "m_listView";
            this.m_listView.Size = new System.Drawing.Size(384, 184);
            this.m_listView.TabIndex = 8;
            this.m_listView.View = System.Windows.Forms.View.Details;
            this.m_listView.KeyDown += new System.Windows.Forms.KeyEventHandler(this.m_listView_KeyDown);
            this.m_listView.MouseUp += new System.Windows.Forms.MouseEventHandler(this.m_listView_MouseUp);
            // 
            // label3
            // 
            this.label3.Anchor = (System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right);
            this.label3.Location = new System.Drawing.Point(192, 24);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(40, 23);
            this.label3.TabIndex = 11;
            this.label3.Text = "Email:";
            // 
            // label2
            // 
            this.label2.Anchor = (System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right);
            this.label2.Location = new System.Drawing.Point(192, 8);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(48, 23);
            this.label2.TabIndex = 10;
            this.label2.Text = "Website:";
            // 
            // AboutForm
            // 
            this.AcceptButton = this.buttonOk;
            this.AutoScaleBaseSize = new System.Drawing.Size(5, 13);
            this.CancelButton = this.buttonOk;
            this.ClientSize = new System.Drawing.Size(404, 295);
            this.Controls.AddRange(new System.Windows.Forms.Control[] {
                                                                          this.label3,
                                                                          this.label2,
                                                                          this.label5,
                                                                          this.m_listView,
                                                                          this.pictureBox1,
                                                                          this.linkLabel2,
                                                                          this.linkLabel1,
                                                                          this.buttonOk});
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.MinimumSize = new System.Drawing.Size(412, 322);
            this.Name = "AboutForm";
            this.ShowInTaskbar = false;
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
            this.Load += new System.EventHandler(this.AboutForm_Load);
            this.ResumeLayout(false);

        }
		#endregion

		private void AddColumn(string text, int position)
		{
			ColumnHeader header = new ColumnHeader();
			header.Text = text;
			m_listView.Columns.Add(header);
		}

		private void AboutForm_Load(object sender, System.EventArgs e)
		{
			AddColumn("Name", 0);
			AddColumn("Version", 1);
			AddColumn("Location", 2);
			
			//add this application
			Assembly appAssembly = Assembly.GetExecutingAssembly();
			AssemblyName appAssemblyName = appAssembly.GetName();
			
			m_listView.Items.Add(appAssemblyName.Name);
			m_listView.Items[ m_listView.Items.Count-1 ].SubItems.Add(appAssemblyName.Version.ToString());
			m_listView.Items[ m_listView.Items.Count-1 ].SubItems.Add(appAssembly.Location);
			

			AssemblyName [] assemblyNames = Assembly.GetExecutingAssembly().GetReferencedAssemblies();					
			foreach(AssemblyName assemblyName in assemblyNames)
			{
				Assembly assembly = Assembly.Load(assemblyName);

				m_listView.Items.Add(assemblyName.Name);				
				m_listView.Items[ m_listView.Items.Count-1 ].SubItems.Add(assemblyName.Version.ToString());
				m_listView.Items[ m_listView.Items.Count-1 ].SubItems.Add(assembly.Location);
			}

			foreach(ColumnHeader header in m_listView.Columns)
			{
				header.Width = -1;
			}
		}

		private void m_miCopy_Click(object sender, System.EventArgs e)
		{
			string text = string.Empty;

			foreach(ListViewItem lvi in this.m_listView.SelectedItems)
			{
				text += lvi.SubItems[0].Text + ", " + lvi.SubItems[1].Text + ", " + lvi.SubItems[2].Text + Environment.NewLine;
			}

			Clipboard.SetDataObject(text, true);		
		}

		private void m_listView_MouseUp(object sender, System.Windows.Forms.MouseEventArgs e)
		{
			if (e.Button == MouseButtons.Right)
			{
				m_contextMenu.Show(m_listView, new Point(e.X, e.Y));						
			}				
		}

		private void linkLabel_LinkClicked(object sender, System.Windows.Forms.LinkLabelLinkClickedEventArgs e)
		{
			if (sender is LinkLabel)
			{
				// Determine which link was clicked within the LinkLabel.
				LinkLabel linkLabel = (LinkLabel) sender;
				linkLabel.Links[linkLabel.Links.IndexOf(e.Link)].Visited = true;
			}

			// Display the appropriate link based on the value of the LinkData property of the Link object.
			Process.Start(e.Link.LinkData.ToString());
		}

        private void m_listView_KeyDown(object sender, System.Windows.Forms.KeyEventArgs e)
        {
            if (e.KeyCode == Keys.C && e.Control == true)
            {
                m_miCopy_Click(sender, e);
            }
        }
	}
}
