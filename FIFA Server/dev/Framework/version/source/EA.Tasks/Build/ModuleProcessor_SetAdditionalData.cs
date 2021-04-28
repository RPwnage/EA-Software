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

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Tasks;
using EA.FrameworkTasks.Model;
using EA.Eaconfig.Core;
using EA.Eaconfig.Modules;


namespace EA.Eaconfig.Build
{
	class ModuleAdditionalDataProcessor : ModuleProcessorBase, IModuleProcessor, IDisposable
	{
		internal ModuleAdditionalDataProcessor(ProcessableModule module)
			: base(module)
		{
		}

		public void Process(ProcessableModule module)
		{
			module.Apply(this);
		}

		public void Process(Module_Native module)
		{
			AddAdditionalDataToModule(module);
		}

		public void Process(Module_DotNet module)
		{
			AddAdditionalDataToModule(module);
		}

		public void Process(Module_Utility module)
		{
			AddAdditionalDataToModule(module);
		}
		
		public void Process(Module_MakeStyle module)
		{
			AddAdditionalDataToModule(module);
		}

		public void Process(Module_Python module)
		{
			AddAdditionalDataToModule(module);
		}

		public void Process(Module_ExternalVisualStudio module)
		{
		}

		public void Process(Module_UseDependency module)
		{
		}

		public void Process(Module_Java module)
		{
		}

		private void AddAdditionalDataToModule(ProcessableModule module)
		{
			AddDebugData(module);
			AddSharedPchInformation(module);
			module.AdditionalData.GeneratorTemplatesData = GeneratorTemplatesData.CreateGeneratorTemplateData(Project, module.BuildGroup);
		}

		private void AddDebugData(ProcessableModule module)
		{

			module.AdditionalData.DebugData = DebugData.CreateDebugData
				(
					executable      : PropGroupValue("commandprogram", String.Empty),
					options         : PropGroupValue("commandargs", PropGroupValue("run.args", String.Empty)),
					workingDir      : PropGroupValue("workingdir", PropGroupValue("run.workingdir", String.Empty)),
					env             : GetEnv(),
					remoteMachine   : PropGroupValue("remotemachine", String.Empty),
					enableUnmanagedDebugging: module.IsKindOf(Module.DotNet) ? 
							   ConvertUtil.ToNullableBoolean(PropGroupValue("enableunmanageddebugging", PropGroupValue("csproj.enableunmanageddebugging"))) 
							   : null,
					createAlways: BuildOptionSet.GetBooleanOptionOrDefault("visualstudio.create-default-userfile", false)
				);
		}

		private void AddSharedPchInformation(ProcessableModule module)
		{
			string sharedPchPackage = Project.Properties.GetPropertyOrDefault(module.GroupName + ".sharedpchpackage", string.Empty);
			if (String.IsNullOrEmpty(sharedPchPackage) == false)
			{
				string sharedPchModule = Project.Properties.GetPropertyOrDefault(module.GroupName + ".sharedpchmodule", string.Empty);
				module.AdditionalData.SharedPchData = new SharedPchData()
				{
					SharedPchModule = sharedPchModule,
					SharedPchPackage = sharedPchPackage
				};
			}
		}

		private IDictionary<string, string> GetEnv()
		{
			var env = new Dictionary<string,string>();

			// Only check global properties since environment variables are stored there
			foreach (Property property in Project.Properties.GetGlobalProperties()) 
			{
					if (property.Prefix == ExecTask.EnvironmentVariablePropertyNamePrefix) 
					{
						env[property.Suffix] = property.Value;
					}
			}
			return env.Count > 0 ? env : null;
		}



		#region Dispose interface implementation

		public void Dispose()
		{
			Dispose(true);
			GC.SuppressFinalize(this);
		}

		void Dispose(bool disposing)
		{
			if (!this._disposed)
			{
				if (disposing)
				{
					// Dispose managed resources here
				}
			}
			_disposed = true;
		}
		private bool _disposed = false;

		#endregion

	}
}
