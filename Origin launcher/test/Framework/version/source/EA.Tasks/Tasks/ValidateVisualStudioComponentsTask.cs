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
using System;
using System.Collections.Generic;
using System.Linq;

namespace EA.Eaconfig
{
	/// <summary>
	/// Ensures that the user's Visual Studio installation has the specified components.
	/// Determines the Visual Studio installation to check by using the vsversion and package.VisualStudio.AllowPreview properties
	/// </summary>
	[TaskName("validate-visual-studio-components", XmlSchema = false)]
	public class Validate_Visual_Studio_Components_Task : Task
	{
		/// <summary>The components to ensure are part of the user's Visual Studio installation</summary>
		[TaskAttribute("components", Required = true)]
		public string Components { get; set; } = null;

		/// <summary>If false, will not fail if Visual Studio is not installed. Default is true.</summary>
		[TaskAttribute("failonmissing", Required = false)]
		public bool FailOnMissing { get; set; } = true;

		protected override void ExecuteTask()
		{
			if (String.IsNullOrEmpty(Components))
			{
				return;
			}

			string vsversion = Project.Properties.GetPropertyOrDefault("vsversion."+Project.Properties["config-system"], Project.Properties.GetPropertyOrDefault("vsversion", "2019"));
			bool allowPreview = Project.Properties.GetBooleanPropertyOrDefault("package.VisualStudio.AllowPreview", false);

			IEnumerable<string> missingComponents = VisualStudioUtilities.GetMissingVisualStudioComponents(
				Components.Split((char[])null, StringSplitOptions.RemoveEmptyEntries), vsversion, allowPreview, FailOnMissing);

			if (missingComponents.Any())
			{
				string warningMessage = "Your Visual Studio installation appears to be missing some components, which may cause problems for this build. See the Visual Studio Setup guide for more details: https://frostbite.ea.com/x/-04ADQ" + Environment.NewLine + "Missing components:";
				foreach (string missingComponent in missingComponents)
				{
					warningMessage += Environment.NewLine + "  " + missingComponent;
				}

				Log.ThrowWarning(
					NAnt.Core.Logging.Log.WarningId.MissingVisualStudioComponents,
					NAnt.Core.Logging.Log.WarnLevel.Minimal,
					warningMessage);
			}
		}
	}
}
