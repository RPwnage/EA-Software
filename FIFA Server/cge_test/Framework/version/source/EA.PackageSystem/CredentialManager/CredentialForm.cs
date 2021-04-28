// Originally based on NAnt - A .NET build tool
// Copyright (C) 2003-2018 Electronic Arts Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// As a special exception, the copyright holders of this software give you
// permission to link the assemblies with independent modules to produce
// new assemblies, regardless of the license terms of these independent
// modules, and to copy and distribute the resulting assemblies under terms
// of your choice, provided that you also meet, for each linked independent
// module, the terms and conditions of the license of that module. An
// independent module is a module which is not derived from or based
// on these assemblies. If you modify this software, you may extend
// this exception to your version of the software, but you are not
// obligated to do so. If you do not wish to do so, delete this exception
// statement from your version.
//
// Electronic Arts (Frostbite.Team.CM@ea.com)

using System;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using System.Reflection;
using System.Windows.Forms;

using NAnt.Perforce.ConnectionManagment;

namespace CredentialManager
{
	public partial class CredentialForm : Form
	{
		public const int NO_TIMEOUT = -5;

		private const string TimerMessageTemplate = "({0} seconds remaining)";
		private int timerValue;

		public String Username = null;
		public String Password = null;

		public CredentialForm(string server, int timeout, bool usePackageServer = false)
		{
			InitializeComponent();

			this.timerValue = timeout;

			this.serverBox.Text = server;
			if (usePackageServer)
			{
				this.label5.Text = "Email";
			}
			if (timeout != NO_TIMEOUT)
			{
				this.TimerLabel.Text = string.Format(TimerMessageTemplate, timerValue);
			}
			else
			{
				this.timer.Enabled = false;
				this.TimerLabel.Text = "";
			}
			this.AcceptButton = this.CancelBtn;

			this.UsernameBox.Select();

			if (string.IsNullOrEmpty(server))
			{
				this.label1.Text = "Please enter your credentials.";
				this.label3.Text = "A list of possible servers can be selected from the drop box.";

				// In the case where a user runs the application outside of framework we should try to populate a list of likely servers to make it more conveinent
				// If we fail to find the proxy map it is not a big deal since this is just a minor usability improvement.
				string expectedProxyMapLocation = Path.Combine(Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location),
					"..", "data", "P4ProtocolOptimalProxyMap.xml");
				if (File.Exists(expectedProxyMapLocation))
				{
					List<ProxyMap.Server> optimalServers = new List<ProxyMap.Server>();
					try
					{
						ProxyMap proxyMap = ProxyMap.Load(expectedProxyMapLocation);
						optimalServers = proxyMap.ListOptimalServers();
					}
					catch (Exception e)
					{
						Console.WriteLine("Warning: Failed to load proxy map file to populate server suggestions: " + e.Message);
					}
					this.serverBox.Items.Add(String.Empty);
					foreach (ProxyMap.Server optimalServer in optimalServers)
					{
						this.serverBox.Items.Add(new ServerListItem(optimalServer));
					}
				}

				this.serverBox.Enabled = true;
				this.serverBox.Select();
			}
			else
			{
				this.serverBox.Enabled = false;
			}

			this.serverBox.TextChanged += new System.EventHandler(serverBoxTextChanged);
			this.UsernameBox.TextChanged += new System.EventHandler(usernameBoxTextChanged);
			this.PasswordBox.TextChanged += new System.EventHandler(passwordBoxTextChanged);
		}

		public string GetSelectedServer()
		{
			object selectedServer = this.serverBox.SelectedItem;
			if (selectedServer is string)
			{
				return selectedServer as string;
			}
			else if (selectedServer is ServerListItem)
			{
				return (selectedServer as ServerListItem).Server.Address;
			}
			if (!string.IsNullOrEmpty(this.serverBox.Text))
			{
				return this.serverBox.Text;
			}
			return null;
		}

		public class ServerListItem
		{
			public ProxyMap.Server Server;

			public ServerListItem(ProxyMap.Server server)
			{
				Server = server;
			}

			public override string ToString()
			{
				if (string.IsNullOrEmpty(Server.Name))
				{
					return Server.Address;
				}
				else
				{
					return "(" + Server.Name + ") " + Server.Address;
				}
			}
		}

		private void stopTimer()
		{
			if (this.timer.Enabled)
			{
				this.timer.Enabled = false;
				this.TimerLabel.ForeColor = Color.ForestGreen;
				this.TimerLabel.Text = "(Timer Paused)";
			}
		}

		private void usernameBoxTextChanged(object sender, EventArgs e)
		{
			stopTimer();

			if (!String.IsNullOrEmpty(this.UsernameBox.Text))
			{
				this.OkBtn.Enabled = true;
				this.AcceptButton = this.OkBtn;
			}
			else
			{
				this.OkBtn.Enabled = false;
				this.AcceptButton = this.CancelBtn;
			}
		}

		private void serverBoxTextChanged(object sender, EventArgs e)
		{
			if (serverBox.SelectedItem is ServerListItem)
			{
				serverBox.Text = (serverBox.SelectedItem as ServerListItem).Server.Address;
				// Ensure cusor stays at the end of the box after the text was changed
				serverBox.Select(serverBox.Text.Length, serverBox.Text.Length);
			}
		}

		private void passwordBoxTextChanged(object sender, EventArgs e)
		{
			stopTimer();
		}

		private void okBtnClick(object sender, EventArgs e)
		{
			Username = this.UsernameBox.Text;
			Password = this.PasswordBox.Text;
			this.DialogResult = DialogResult.OK;
			this.Close();
		}

		private void cancelBtnClick(object sender, EventArgs e)
		{
			this.DialogResult = DialogResult.Cancel;
			this.Close();
		}

		private void timerTick(object sender, EventArgs e)
		{
			if (timerValue != NO_TIMEOUT)
			{
				timerValue--;
				this.TimerLabel.Text = string.Format(TimerMessageTemplate, timerValue);

				if (timerValue <= 0)
				{
					this.Close();
				}
			}
		}
	}
}