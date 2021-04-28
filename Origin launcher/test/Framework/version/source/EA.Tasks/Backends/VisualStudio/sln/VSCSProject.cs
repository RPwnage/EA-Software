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

using EA.FrameworkTasks.Model;
using EA.Eaconfig.Modules;

namespace EA.Eaconfig.Backends.VisualStudio
{
	internal abstract class VSCSProject : VSDotNetProject
	{
		internal VSCSProject(VSSolutionBase solution, IEnumerable<IModule> modules)
			: base(solution, modules, CSPROJ_GUID)
		{
		}

		public override string ProjectFileName
		{
			get
			{
				return ProjectFileNameWithoutExtension + ".csproj";
			}
		}


		protected override IDictionary<string, string> GetProjectProperties()
		{
			var data = base.GetProjectProperties();

			data.Add("ValidateXaml", true);
			data.Add("MinimumVisualStudioVersion", Version);
			data.Add("ThrowErrorsInValidation", true);
			
			if (!StartupModule.LanguageVersion.IsNullOrEmpty())
			{
				data.Add("LangVersion", StartupModule.LanguageVersion);
			}

			return data;
		}

		protected override void WriteImportProperties(IXmlWriter writer)
		{
			if (!UseNewCSProjFormat())
			{
				if (StartupModule.IsProjectType(DotNetProjectTypes.WebApp))
				{
					writer.WriteStartElement("PropertyGroup");
					{
						writer.WriteStartElement("VisualStudioVersion");
						writer.WriteAttributeString("Condition", "'$(VisualStudioVersion)' == ''");
						writer.WriteString("10.0");
						writer.WriteEndElement(); //VisualStudioVersion
					}
					{
						writer.WriteStartElement("VSToolsPath");
						writer.WriteAttributeString("Condition", "'$(VSToolsPath)' == ''");
						writer.WriteString(@"$(MSBuildExtensionsPath32)\Microsoft\VisualStudio\v$(VisualStudioVersion)");
						writer.WriteEndElement(); //VSToolsPath
					}
					writer.WriteEndElement(); //PropertyGroup
				}

				writer.WriteStartElement("Import");
				writer.WriteAttributeString("Project", @"$(MSBuildExtensionsPath)\$(MSBuildToolsVersion)\Microsoft.Common.props");
				writer.WriteAttributeString("Condition", @"Exists('$(MSBuildExtensionsPath)\$(MSBuildToolsVersion)\Microsoft.Common.props')");
				writer.WriteEndElement();
			}

			base.WriteImportProperties(writer);
		}

		// Subtypes
		internal static readonly Guid CSPROJ_WINRT_GUID = new Guid("BC8A1FFA-BEE3-4634-8014-F334798102B3");
	}
}