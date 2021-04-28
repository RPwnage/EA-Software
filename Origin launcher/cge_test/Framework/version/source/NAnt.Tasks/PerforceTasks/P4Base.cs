// NAnt - A .NET build tool
// Copyright (C) 2003-2015 Electronic Arts Inc.
// Copyright (C) 2001 Gerry Shaw
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
// 
// 
// Electronic Arts (Frostbite.Team.CM@ea.com)
// Jim Petrick (jpetrick@ea.com)

using System;
using System.IO;
using System.Xml;

using NAnt.Core.Attributes;
using NAnt.Core.Logging;
using NAnt.Core.Tasks;

namespace NAnt.PerforceTasks
{
	/// <summary>
	///	This class serves as a base class for other Perforce classes.  
	///	It provides Global Options services.
	///	</summary>
	public abstract class P4Base : ExternalProgramBase   
	{
		public P4Base(string name) : base(name) { }

		string user	= null;

		/// <summary>Suppresses normal output messages but still prints error messages.</summary>
		[TaskAttribute("quiet")]
		public bool Silent { get { return base.Quiet; } set { base.Quiet = value; } }

		/// <summary>
		/// Overrides P4CLIENT with this client name.
		/// Can also be set via the ${p4.client} property.
		/// </summary>
		[TaskAttribute("client")]
		public string Client { get; set; } = null;

		/// <summary>
		/// Overrides PWD with the specified directory.
		/// Can also be set via the ${p4.pwd} property.
		/// </summary>
		[TaskAttribute("dir")]
		public string Dir { get; set; } = null;

		/// <summary>
		/// Overrides P4HOST with specified hostname.
		/// Can also be set via the ${p4.host} property.
		/// </summary>
		[TaskAttribute("host")]
		public string Host { get; set; } = null;

		/// <summary>Overrides P4PORT with specified hostname.
		/// Can also be set via the ${p4.port} property.
		/// </summary>
		[TaskAttribute("port")]
		public string Port { get; set; } = null;

		/// <summary>
		/// Overrides P4PASSWD with specified hostname.
		/// Can also be set via the ${p4.passwd} property.
		/// </summary>
		[TaskAttribute("password")]
		public string Pass { get; set; } = null;

		/// <summary>
		/// Overrides P4USER|USER|USERNAME with specified hostname.
		/// Can also be set via the ${p4.user} property.
		/// </summary>
		[TaskAttribute("user")]
		public string User				{ get { return user; }		set { user = value; } }

		/// <summary>
		/// Overrides P4CHARSET with specified charset.
		/// Can also be set via the ${p4.charset} property.
		/// </summary>
		[TaskAttribute("charset")]
		public string Charset { get; set; } = null;

		protected override void InitializeTask(XmlNode taskNode) 
		{
			base.InitializeTask(taskNode);

			//	Attempt to get default values for the following properties.
			string prop;

			if ((prop = Project.Properties["p4.client"]) != null)
				Client = prop.Trim();
			if ((prop = Project.Properties["p4.pwd"]) != null)
				Dir = prop.Trim();
			if ((prop = Project.Properties["p4.host"]) != null)
				Host = prop.Trim();
			if ((prop = Project.Properties["p4.port"]) != null)
				Port = prop.Trim();
			if ((prop = Project.Properties["p4.passwd"]) != null)
				Pass = prop.Trim();
			if ((prop = Project.Properties["p4.user"]) != null)
				User = prop.Trim();
			if ((prop = Project.Properties["p4.charset"]) != null)
				Charset = prop.Trim();
		}	//	end P4Base

		protected override void ExecuteTask()
		{
			Log.Info.WriteLine(LogPrefix + "{0} {1}\nWorking Dir: '{2}'", ProgramFileName, GetCommandLine(), BaseDirectory);

			base.ExecuteTask();
		}

		/// <summary>
		/// Returns a formatted command line version for the global options.
		/// </summary>
		public string GlobalOptions		
		{ 
			get 
			{ 
				string opts = String.Empty;
				if (!String.IsNullOrEmpty(Client))
					opts += " -c " + Client;
				if (!String.IsNullOrEmpty(Dir))
					opts += " -d " + Dir;
				if (!String.IsNullOrEmpty(Host))
					opts += " -H " + Host;
				if (!String.IsNullOrEmpty(Port))
					opts += " -p " + Port;
				if (!String.IsNullOrEmpty(Pass))
					opts += " -P " + Pass;
				if (!String.IsNullOrEmpty(User))
					opts += " -u " + User;
				if (!String.IsNullOrEmpty(Charset))
					opts += " -C " + Charset;
				if (Silent)
					opts += " -q";
				opts += " ";
				return opts; 
			}		
		}
		
		/// <summary>
		/// Gets the name of the application to start.
		/// </summary>
		public override string ProgramFileName
		{
			get 
			{
				if (Environment.OSVersion.Platform == PlatformID.Unix || (int)Environment.OSVersion.Platform == 6)
				{
					string[] defaultUnixP4InstallPaths = new string[] {
						"/usr/local/bin/p4",
						"/usr/bin/p4",
						"/sbin/p4"
					};
					foreach (string defaultUnixP4InstallPath in defaultUnixP4InstallPaths)
					{
						if (File.Exists(defaultUnixP4InstallPath))
						{
							return defaultUnixP4InstallPath;
						}
					}

					return "p4";
				}

				string path = null;
				//	Get Perforce path from registry.
				Microsoft.Win32.RegistryKey key = Microsoft.Win32.Registry.LocalMachine.OpenSubKey(@"SOFTWARE\Perforce\Environment");

				if (key == null)
				{
					key = Microsoft.Win32.Registry.LocalMachine.OpenSubKey(@"SOFTWARE\Wow6432Node\Perforce\Environment");
				}
				if (key != null)
				{
					path = (string)key.GetValue("P4INSTROOT");
					if (path != null)
					{
						path = Path.Combine(path, "p4.exe");
					}
				}

				return (path == null) ? "p4.exe" : path;
			}
		}   //	end ProgramFileName          

		/// <summary>
		/// Gets the command line arguments for the application.
		/// </summary>
		public override string ProgramArguments		
		{ 	
			get { 	return GlobalOptions; }
		}
	}	// end class P4Base
}
