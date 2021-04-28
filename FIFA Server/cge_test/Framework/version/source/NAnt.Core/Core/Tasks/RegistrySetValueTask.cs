// NAnt - A .NET build tool
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

using Microsoft.Win32;

using NAnt.Core.Attributes;

namespace NAnt.Core.Tasks
{
	/// <summary>Add or update a value in an existing registry key.</summary>
	/// <remarks>
	///   <para>This task will add or update a value in an existing registry key.
	///   A build exception will be thrown if the key does not exist or the process
	///   does not have sufficient privileges to modify the registry.  Use this
	///   task with caution as changing registry values can wreak havoc on your
	///   system.
	///   </para>
	///   </remarks>
	[TaskName("RegistrySetValue", NestedElementsCheck = true)]
	public class RegistrySetValueTask : Task
	{
		public RegistrySetValueTask() : base("registrysetvalue") { }

		/// <summary>The top-level node in the windows registry. Possible values are: LocalMachine, Users, CurrentUser, and ClassesRoot.</summary>
		[TaskAttribute("hive", Required = true)]
		public string Hive { get; set; } = null;

		/// <summary>Key in which the value is to be updated.</summary>
		[TaskAttribute("key", Required = true)]
		public string Key { get; set; } = null;

		/// <summary>Name of the value to add or change.</summary>
		[TaskAttribute("name", Required = true)]
		public string ValueName { get; set; } = null;

		/// <summary>New value of the entry.</summary>
		[TaskAttribute("value", Required = true)]
		public string Value { get; set; } = null;

		public static void Execute(Project project, string hive, string key, string name, string value)
		{
			RegistrySetValueTask task = new RegistrySetValueTask();
			task.Project = project;
			task.Hive = hive;
			task.Key = key;
			task.ValueName = name;
			task.Value = value;
			task.Execute();
		}

		protected override void ExecuteTask()
		{
			try
			{
				RegistryHive hive = RegistryHive.LocalMachine;

				switch(Hive)
				{
					case "LocalMachine":
						hive = RegistryHive.LocalMachine;
						break;

					case "Users":
						hive = RegistryHive.Users;
						break;

					case "CurrentUser":
						hive = RegistryHive.CurrentUser;
						break;

					case "ClassesRoot":
						hive = RegistryHive.ClassesRoot;
						break;
				}

				// Use the Wow6432Node
				RegistryKey rk = RegistryKey.OpenBaseKey(hive, RegistryView.Registry32);
				RegistryKey rk2 = rk.OpenSubKey(Key, true);

				if (rk2 == null)
				{
					throw new BuildException(string.Format(
						"Error: RegistrySetValue could not open subkey {0} in hive '{1}'.  The key does not exist and will need to be created.", Key, Hive), Location);
				}

				rk2.SetValue(ValueName, Value, RegistryValueKind.String);

				rk2.Close();
				rk.Close();
			}
			catch (Exception e)
			{
				throw new ContextCarryingException(e, Name, Location);
			}
		}
	}
}
