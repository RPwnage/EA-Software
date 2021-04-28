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
using System.Linq;

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Attributes;
using NAnt.Core.Logging;
using EA.FrameworkTasks.Model;

using EA.Eaconfig.Core;
using EA.Eaconfig.Build;

namespace EA.Eaconfig
{
	[TaskName("get-module-asset-files")]
	sealed class GetModuleAssetFilesTask : Task
	{
		[TaskAttribute("module-groupname", Required = true)]
		public string ModuleGroupName { get; set; }

		[TaskAttribute("asset-filesets-property", Required = true)]
		public string AssetFilesetsProperty { get; set; }

		/// <summary>The prefix used when sending messages to the log.</summary>
		public override string LogPrefix
		{
			get
			{
				return " [get-module-asset-files] ";
			}
		}

		protected override void ExecuteTask()
		{
			var module = GetModule(ModuleGroupName) as ProcessableModule;

			if (module == null)
			{
				Log.Error.WriteLine(LogPrefix + "Can't extract module object for module groupname '{0}'", ModuleGroupName);
			}
			else
			{
				var assetProcessor = new ProcessModuleAssetFiles(module, LogPrefix);

				if (assetProcessor.IsDeployAssets)
				{
					var assetFilesetNames = assetProcessor.GetModuleAssetFileSetNames();
					Project.Properties[AssetFilesetsProperty] = assetFilesetNames.OrderedDistinct().ToString(Environment.NewLine);
				}
				else
				{
					Log.Status.WriteLine("Asset deployment is not enabled for module {0}", module.ModuleIdentityString());
				}
			}
		}

		private IModule GetModule(string moduleGroupName)
		{
			IModule module = null;
			IPackage p;
			if (Project.TryGetPackage(out p))
			{
				module = p.Modules.FirstOrDefault(m => m.GroupName == moduleGroupName);
			}
			return module;
		}
	}
}