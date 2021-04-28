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
using NAnt.Core.Structured;

namespace EA.Eaconfig.Structured
{
	/// <summary></summary>
	[ElementName("VisualStudio", StrictAttributeCheck = true, NestedElementsCheck = true)]
	public class VisualStudioDataElement : ConditionalElementContainer
	{
		/// <summary></summary>
		[Property("pregenerate-target", Required = false)]
		public BuildTargetElement PregenerateTarget { get; } = new BuildTargetElement("vcproj.prebuildtarget");

		/// <summary>A list of files that are excluded from the build but are added to the visual studio
		/// project as non-buildable files</summary>
		[FileSet("excludedbuildfiles", Required = false)]
		public StructuredFileSet ExcludedBuildFiles { get; set; } = new StructuredFileSet();

		/// <summary></summary>
		[Property("pre-build-step", Required = false)]
		public ConditionalPropertyElement PreBuildStep { get; set; } = new ConditionalPropertyElement();

		/// <summary></summary>
		[Property("post-build-step", Required = false)]
		public ConditionalPropertyElement PostBuildStep { get; set; } = new ConditionalPropertyElement();

		/// <summary></summary>
		[Property("vcproj", Required = false)]
		public VcprojDataElement VcProj { get; } = new VcprojDataElement();

		/// <summary></summary>
		[Property("csproj", Required = false)]
		public CsprojDataElement CsProj { get; } = new CsprojDataElement();

		/// <summary>A list of elements to insert directly into the visualstudio project file in the config build options section.</summary>
		[Property("msbuildoptions", Required = false)]
		public StructuredOptionSet MsbuildOptions { get; set; } = new StructuredOptionSet();

		/// <summary>The name(s) of a Visual Studio Extension task used for altering the solution generation process 
		/// to allow adding custom elements to a project file.</summary>
		[Property("extension", Required = false)]
		public ConditionalPropertyElement Extension { get; set; } = new ConditionalPropertyElement();
	}

	/// <summary></summary>
	[ElementName("Vcproj", StrictAttributeCheck = true, NestedElementsCheck = true)]
	public class VcprojDataElement : ConditionalElementContainer
	{
		/// <summary></summary>
		[Property("pre-link-step", Required = false)]
		public ConditionalPropertyElement PreLinkStep { get; set; } = new ConditionalPropertyElement();

		/// <summary></summary>
		[FileSet("input-resource-manifests", Required = false)]
		public StructuredFileSet InputResourceManifests { get; set; } = new StructuredFileSet();

		/// <summary></summary>
		[FileSet("additional-manifest-files", Required = false)]
		public StructuredFileSet AdditionalManifestFiles { get; set; } = new StructuredFileSet();
	}

	/// <summary></summary>
	[ElementName("Csproj", StrictAttributeCheck = true, NestedElementsCheck = true)]
	public class CsprojDataElement : ConditionalElementContainer
	{
		/// <summary></summary>
		[Property("link.resourcefiles.nonembed", Required = false)]
		public ConditionalPropertyElement LinkNonembeddedResources { get; set; } = new ConditionalPropertyElement();

		//additional-manifest-files
	}
}
/*
E:\packages\eaconfig\dev\release_notes.txt(381):                 ${groupname}.vcproj.additional-manifest-files
  E:\packages\eaconfig\dev\release_notes.txt(382):                 ${groupname}.vcproj.additional-manifest-files.${config-system}
  E:\packages\eaconfig\dev\release_notes.txt(384):                 ${groupname}.vcproj.input-resource-manifests
  E:\packages\eaconfig\dev\release_notes.txt(385):                 ${groupname}.vcproj.input-resource-manifests.${config-system}
  E:\packages\eaconfig\dev\release_notes.txt(738):                 group.<module name>.vcproj.excludedbuildfiles
  E:\packages\eaconfig\dev\release_notes.txt(739):                 group.<module>.vcproj.excludedbuildfiles.${config-system}
 * 
 * vcproj.input-resource-manifests
 * 
 * E:\packages\eaconfig\dev\source\targets\EaconfigVcproj.cs(397):            FileSetUtil.CreateFileSetInProjectIfMissing(Project, PropGroupName("vcproj.custom-build-dependencies"));
  E:\packages\eaconfig\dev\source\targets\EaconfigVcproj.cs(398):            FileSetUtil.CreateFileSetInProjectIfMissing(Project, PropGroupName("vcproj.custom-build-rules"));
 * 
		  FileSetUtil.CreateFileSetInProjectIfMissing(Project, PropGroupName("vcproj.custom-build-dependencies"));
  E:\packages\eaconfig\dev\source\targets\EaconfigVcproj.cs(398):            FileSetUtil.CreateFileSetInProjectIfMissing(Project, PropGroupName("vcproj.custom-build-rules"));
  E:\packages\eaconfig\dev\source\targets\EaconfigVcproj.cs(447):            BuildData.BuildProperties["custom-build-tool"] = PropertyUtil.GetPropertyOrDefault(Project, PropGroupName("vcproj.custom-build-tool"), String.Empty);
  E:\packages\eaconfig\dev\source\targets\EaconfigVcproj.cs(448):            BuildData.BuildProperties["custom-build-outputs"] = StringUtil.EnsureSeparator(PropertyUtil.GetPropertyOrDefault(Project, PropGroupName("vcproj.custom-build-outputs"), String.Empty), ';');
  E:\packages\eaconfig\dev\source\targets\EaconfigVcproj.cs(455):            BuildData.BuildProperties["pre-build-step"] = Properties[PropGroupName("vcproj.pre-build-step")];
  E:\packages\eaconfig\dev\source\targets\EaconfigVcproj.cs(456):            BuildData.BuildProperties["pre-link-step"] = Properties[PropGroupName("vcproj.pre-link-step")];
 * 
 *  To control whether to link or not non embedded resources use property:
		  
					csproj.link.resourcefiles.nonembed 
					
		  Default value is true.
*/
