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
	/// <summary>Sets various attributes for a config</summary>
	[ElementName("config", StrictAttributeCheck = true, NestedElementsCheck = true)]
	public class ConfigElement : Element
	{
		private ModuleBaseTask _module;

		internal readonly OptionSet buildOptionSet = new OptionSet();

		public ConfigElement(ModuleBaseTask module)
		{
			_module = module;

			BuildOptions = new BuildTypeElement(_module, buildOptionSet);
		}

		/// <summary>Gets the build options for this config.</summary>
		[BuildElement("buildoptions", Required = false)]
		public BuildTypeElement BuildOptions { get; }

		/// <summary>Gets the macros defined for this config</summary>
		[Property("defines", Required = false)]
		public ConditionalPropertyElement Defines { get; } = new ConditionalPropertyElement();

		/// <summary>Set target framework version for managed modules (4.0, 4.5, ...)</summary>
		[Property("targetframeworkversion", Required = false)]
		public DeprecatedPropertyElement TargetFrameworkVersion { get; set; } = new DeprecatedPropertyElement
			(
				NAnt.Core.Logging.Log.DeprecationId.ModuleTargetFrameworkVersion, NAnt.Core.Logging.Log.DeprecateLevel.Minimal,
				"Target .NET version is now determined by .NET family and the relevant DotNet or DotNetCoreSdk package version in masterconfig."
			);

		/// <summary>Set target framework family for managed modules (framework, standard, core)</summary>
		[Property("targetframeworkfamily", Required = false)]
		public ConditionalPropertyElement TargetFrameworkFamily { get; } = new ConditionalPropertyElement();

		/// <summary>Gets the warning suppression property for this config</summary>
		[Property("warningsuppression", Required = false)]
		public ConditionalPropertyElement Warningsuppression { get; } = new ConditionalPropertyElement();

		/// <summary>Preprocess step can be C# task derived from AbstractModuleProcessorTask class or a target. Multiple preprocess steps are supported</summary>
		[Property("preprocess", Required = false)]
		public ConditionalPropertyElement Preprocess { get; } = new ConditionalPropertyElement();

		/// <summary>Preprocess step can be C# task derived from AbstractModuleProcessorTask class or a target. Multiple preprocess steps are supported</summary>
		[Property("postprocess", Required = false)]
		public ConditionalPropertyElement Postprocess { get; } = new ConditionalPropertyElement();

		/// <summary>Set up precompiled headers.  NOTE: To use this option, you still need to list a source file and specify that file with 'create-precompiled-header' optionset.</summary>
		[Property("pch", Required = false)]
		public PrecompiledHeadersElement PrecompiledHeader { get; } = new PrecompiledHeadersElement();

		/// <summary>Define options to remove from the final optionset</summary>
		[Property("remove", Required = false)]
		public RemoveBuildOptions RemoveOptions { get; } = new RemoveBuildOptions();
	}
}
