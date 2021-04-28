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
using System.Linq;
using System.IO;

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Writers;
using EA.FrameworkTasks.Model;

using EA.Eaconfig.Core;
using EA.Eaconfig.Modules;

namespace EA.Eaconfig.Backends.VisualStudio
{
	internal class ExternalVSProject : VSProjectBase
	{
		internal override string RootNamespace => Name;

		public ExternalVSProject(VSSolutionBase solution, IEnumerable<IModule> modules)
			:
			base(solution, modules, GetProjectTypeGuid(modules))
		{
			var vsmodule = modules.FirstOrDefault() as Module_ExternalVisualStudio;
			_projectFile = vsmodule.VisualStudioProject;
			_projectPlatforms = new Dictionary<Configuration, string>();

			foreach (Module_ExternalVisualStudio module in Modules)
			{
				if (!String.IsNullOrEmpty(module.VisualStudioProjectPlatform))
				{
					_projectPlatforms[module.Configuration] = module.VisualStudioProjectPlatform;
				}
			}
		}

		protected override Guid GetVSProjGuid()
		{
			var vsmodule = Modules.FirstOrDefault() as Module_ExternalVisualStudio;
			return vsmodule.VisualStudioProjectGuild;
		}

		public override PathString OutputDir
		{
			get
			{
				if (_outputDir == null)
				{
					_outputDir = PathString.MakeNormalized(Path.GetDirectoryName(_projectFile.Path));
				}
				return _outputDir;
			}

		} private PathString _outputDir;

		private static Guid GetProjectTypeGuid(IEnumerable<IModule> modules)
		{
			var vsmodule = modules.FirstOrDefault() as Module_ExternalVisualStudio;
			if (vsmodule != null)
			{
				var ext = Path.GetExtension(vsmodule.VisualStudioProject.Path).ToLowerInvariant();
				switch(ext)
				{
					case ".csproj":
						return CSPROJ_GUID;
					case ".fsproj":
						return FSPROJ_GUID;
                    case ".vdproj":
                        return VDPROJ_GUID;
					case ".wixproj":
                        return WIXPROJ_GUID;
					case ".vcxproj":
					default:
						return VCPROJ_GUID;
				}
			}
			return VCPROJ_GUID;
		}

		public override string ProjectFileName 
		{
			get { return Path.GetFileName(_projectFile.Path); }
		}


		protected override void PopulateConfigurationNameOverrides()
		{
			foreach (Module_ExternalVisualStudio module in Modules)
			{
			   if(!String.IsNullOrEmpty(module.VisualStudioProjectConfig))
			   {
				   ProjectConfigurationNameOverrides[module.Configuration] = module.VisualStudioProjectConfig;
			   }
			}
		}

		public override string GetProjectTargetPlatform(Configuration configuration)
		{
			return _projectPlatforms[configuration];
		}


		#region None of these functions used in the ExternalVSProject

		protected override IXmlWriterFormat ProjectFileWriterFormat { get { return null; } }

		protected override string UserFileName { get { return null; } }

		protected override IEnumerable<KeyValuePair<string, IEnumerable<KeyValuePair<string, string>>>> GetUserData(ProcessableModule module) { return null; }

		protected override void WriteUserFile() { }

		protected override void WriteProject(IXmlWriter writer) { }

		public override void WriteProject() { }

		#endregion

		private readonly PathString _projectFile;

		private readonly IDictionary<Configuration, string> _projectPlatforms;
	}

}
