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

using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Logging;

namespace EA.Eaconfig
{
	/// <summary>
	/// Prints the all of the options in an optionset.
	/// This task should only be used for debugging build scripts.
	/// </summary>
	[TaskName("EchoOptionSet")]
	public class EchoOptionSetTask : Task
	{
		/// <summary>
		/// The name of the optionset to print
		/// </summary>
		[TaskAttribute("Name", Required = true)]
		public string OptionSetName { get; set; }

		public EchoOptionSetTask() : base() { }

		public EchoOptionSetTask(Project project, string name)
			: base()
		{
			Project = project;
			OptionSetName = name;
		}

		protected override void ExecuteTask()
		{
			OptionSet optionset = Project.NamedOptionSets[OptionSetName];
			if (optionset == null) throw new BuildException("Unknown option set '" + OptionSetName + "'.");
			Log.Status.WriteLine(LogPrefix + "Echoing optionset " + OptionSetName);
			foreach (var option in optionset.Options)
			{
				Log.Status.WriteLine(LogPrefix + option.Key + "=" + option.Value);
			}
		}
	}
}
