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
using EA.FrameworkTasks.Model;

using EA.Eaconfig.Backends.VisualStudio;
using EA.Eaconfig.Backends;
using EA.Eaconfig.Modules.Tools;

namespace EA.Eaconfig.Core
{
	/*
	Location of visual Studio Extension invocation with respect to project structure:
	 
	<Project>
		<!-- Set default startup configuration -->
		<PropertyGroup>
			<Configuration/>
			<Platform/>
		</PropertyGroup>
		*************************************************************
		***   WriteExtensionProjectProperties(IXmlWriter writer)  ***
		*************************************************************
		<ItemGroup Label="ProjectConfigurations">
			<ProjectConfiguration>
				<Configuration/>
				<Platform/>
			</ProjectConfiguration>
		</ItemGroup>
		<PropertyGroup Label="Globals">
			********************************************************************
			***   ProjectGlobals(IDictionary<string, string> projectGlobals) ***
			********************************************************************
			<ProjectGuid/>
			<ProjectName/>
		</PropertyGroup>
		<Import Project="$(VCTargetsPath)\Microsoft.Cpp.Default.props" />
		<PropertyGroup Label="Configuration">
			********************************************************************************
			***  ProjectConfiguration(IDictionary<string, string> ConfigurationElements) ***
			********************************************************************************
			<ConfigurationType/>
			<PlatformToolset/>
		</PropertyGroup>
		<Import Project="$(VCTargetsPath)\Microsoft.Cpp.props" />
		<ImportGroup Label="PropertySheets">
			**************************************************************************************
			***  AddPropertySheetsImportList(List<Tuple<string,string,string>> propSheetList)  ***
			***     Tuple<string,string,string> => (file, condition, label)                    ***
			**************************************************************************************
			<Import Project="$(UserRootDir)\Microsoft.Cpp.$(Platform).user.props" Condition="exists('$(UserRootDir)\Microsoft.Cpp.$(Platform).user.props')" />
		</ImportGroup>
		<ItemDefinitionGroup>
			<Midl />
			<ClCompile>
			******************************************************************************************
			***  WriteExtensionCompilerToolProperties(IDictionary<string, string> toolProperties)  ***
			******************************************************************************************
			</ClCompile>
			<Link>
			******************************************************************************************
			***   WriteExtensionLinkerToolProperties(IDictionary<string, string> toolProperties)   ***
			******************************************************************************************
			*******************************************************
			***   WriteExtensionLinkerTool(IXmlWriter writer)   ***
			*******************************************************			
			</Link>
			<PreBuildEvent/>
			<PostBuildEvent/>
			*************************************************
			***   WriteExtensionTool(IXmlWriter writer)   ***
			*************************************************
		</ItemDefinitionGroup>
		 <PropertyGroup>
			<OutDir/>
			<IntDir/>
			<TargetName/>
			<TargetExt/>
			*********************************************************
			***   WriteExtensionToolProperties(IXmlWriter writer) ***
			*********************************************************
			*********************************************************
			***   SetVCPPDirectories(VCPPDirectories directories) ***
			*********************************************************
			<ExecutablePath/>
			<IncludePath/>
			<ReferencePath>$(ReferencePath)</ReferencePath>
			<LibraryPath>$(LibraryPath)</LibraryPath>
			<SourcePath>$(SourcePath)</SourcePath>
		 </PropertyGroup>
		 <ItemGroup "ProjectReferences">
		 </ItemGroup>
		 <ItemGroup>
			 Files
		 </ItemGroup>
		************************************************
		***   WriteExtensionItems(IXmlWriter writer) ***
		************************************************
		<Import Project="$(VCTargetsPath)\Microsoft.Cpp.targets" />
		**************************************************
		***   WriteMsBuildOverrides(IXmlWriter writer) ***
		**************************************************
		<ProjectExtensions/>
	</Project>
	
	*/
	public abstract class IMCPPVisualStudioExtension : IVisualStudioExtensionBase
	{
		#region Interface

		public virtual void WriteExtensionItems(IXmlWriter writer)
		{
		}

		public virtual void WriteExtensionProjectProperties(IXmlWriter writer)
		{
		}

		public virtual void WriteExtensionTool(IXmlWriter writer)
		{
		}

		public virtual void AddPropertySheetsImportList(List<Tuple<string,string,string>> propSheetList)
		{
		}

		public virtual void WriteExtensionCompilerToolProperties(CcCompiler cc, FileItem file, IDictionary<string, string> toolProperties)
		{
		}

		public virtual void WriteExtensionLinkerTool(IXmlWriter writer)
		{
		}

		public virtual void WriteExtensionLinkerToolProperties(IDictionary<string, string> toolProperties)
		{
		}

		public virtual void WriteExtensionToolProperties(IXmlWriter writer)
		{
		}

		public virtual void SetVCPPDirectories(VCPPDirectories directories)
		{
		}

		public virtual void ProjectGlobals(IDictionary<string, string> projectGlobals)
		{
		}

		public virtual void UserData(IDictionary<string, string> userData)
		{
		}

		public virtual void ProjectConfiguration(IDictionary<string, string> ConfigurationElements)
		{
		}

		public virtual void WriteMsBuildOverrides(IXmlWriter writer)
		{
		}

		#endregion
	}

	public abstract class IDotNetVisualStudioExtension : IVisualStudioExtensionBase
	{
		#region Interface

		public virtual void ProjectGlobals(IDictionary<string, string> projectGlobals)
		{
		}

		public virtual void WriteExtensionItems(IXmlWriter writer)
		{
		}

		#endregion
	}

	public abstract class IVisualStudioExtensionBase : Task
	{
		public ProcessableModule Module { get; set; }

		public IProjectGenerator Generator { get; set; }

		private new void Execute()
		{
		}
		protected override void ExecuteTask()
		{
		}

		// This method was moved to ModuleExtensions.cs but remains here since many config packages extend 
		// this class and we don't want to break them. We want to move away from using this though.
		protected string PropGroupValue(string name, string defVal = "")
		{
			return Module.PropGroupValue(name, defVal);
		}

		protected string PropGroupBoolValue(string name, string defVal = "")
		{
			string val = String.Empty;
			if (!String.IsNullOrEmpty(name))
			{
				var tval = Project.Properties[Module.PropGroupName(name)].TrimWhiteSpace();
				if (tval.Equals("on", StringComparison.OrdinalIgnoreCase) || tval.Equals("true", StringComparison.OrdinalIgnoreCase) || tval.Equals("yes", StringComparison.OrdinalIgnoreCase))
				{
					val = "true";
				}
				else if (tval.Equals("off", StringComparison.OrdinalIgnoreCase) || tval.Equals("false", StringComparison.OrdinalIgnoreCase) || tval.Equals("no", StringComparison.OrdinalIgnoreCase))
				{
					val = "false";
				}
			}
			return val;
		}

		public interface IProjectGenerator
		{
			ModuleGenerator Interface {get;}

			string GetVSProjConfigurationName(Configuration configuration);

			string GetVSProjTargetConfiguration(Configuration configuration);

			string GetVSProjTargetPlatform(Configuration configuration);

			string GetConfigCondition(Configuration config);

			bool ExplicitlyIncludeDependentLibraries { set; }
		}

		public class MCPPProjectGenerator : IProjectGenerator
		{
			private VSCppMsbuildProject _generator;

			internal MCPPProjectGenerator(VSCppMsbuildProject project)
			{
				_generator = project;
			}

			public ModuleGenerator Interface
			{
				get
				{
					return _generator;
				}
			}

			public string GetVSProjConfigurationName(Configuration configuration)
			{
				return _generator.GetVSProjConfigurationName(configuration);
			}

			public string GetVSProjTargetConfiguration(Configuration configuration)
			{
				return _generator.GetVSProjTargetConfiguration(configuration);
			}

			public string GetVSProjTargetPlatform(Configuration configuration)
			{
				return _generator.GetProjectTargetPlatform(configuration);
			}
			
			public string GetConfigCondition(Configuration config)
			{
				return  _generator.GetConfigCondition(config);
			}

			public bool ExplicitlyIncludeDependentLibraries
			{
				set
				{
					_generator.ExplicitlyIncludeDependentLibraries = value;
				}
			}

		}

		public class DotNetProjectGenerator : IProjectGenerator
		{
			private VSDotNetProject _generator;

			internal DotNetProjectGenerator(VSDotNetProject project)
			{
				_generator = project;
			}

			public ModuleGenerator Interface
			{
				get
				{
					return _generator;
				}
			}

			public string GetVSProjConfigurationName(Configuration configuration)
			{
				return _generator.GetVSProjConfigurationName(configuration);
			}

			public string GetVSProjTargetConfiguration(Configuration configuration)
			{
				return _generator.GetVSProjTargetConfiguration(configuration);
			}

			public string GetVSProjTargetPlatform(Configuration configuration)
			{
				return _generator.GetProjectTargetPlatform(configuration);
			}

			public string GetConfigCondition(Configuration config)
			{
				return  _generator.GetConfigCondition(config);
			}

			public bool ExplicitlyIncludeDependentLibraries
			{
				set
				{
				}
			}
		}
	}


	public abstract class IVisualStudioSolutionExtension : Task
	{
		public IGenerator SolutionGenerator { get; set; }

		public interface IGenerator
		{
			Generator Interface { get; }
		}

		public class SlnGenerator : IGenerator
		{
			private VSSolutionBase m_vsSlnGenerator;

			internal SlnGenerator(VSSolutionBase sln)
			{
				m_vsSlnGenerator = sln;
			}

			public Generator Interface
			{
				get { return m_vsSlnGenerator; }
			}
		}

		protected override void ExecuteTask()
		{
		}

		public virtual void PostGenerate()
		{
		}

		public virtual void WriteGlobalSection(IMakeWriter writer)
		{
		}

		// This function is expected to be called by solution generator's PostGenerate() function.
		// At present, this is used by VisualStudio's solution generator's PostGenerate() function to
		// loop through each extension to generate content for the before.[solution_name].sln.targets file.
		// which will be executed by MSBuild before starting solution build.
		public virtual void BeforeSolutionBuildTargets(System.Text.StringBuilder builder)
		{
		}

		// This function is expected to be called by solution generator's PostGenerate() function.
		// At present, this is used by VisualStudio's solution generator's PostGenerate() function to
		// loop through each extension to generate content for the after.[solution_name].sln.targets file.
		// which will be executed by MSBuild after solution build finished.
		public virtual void AfterSolutionBuildTargets(System.Text.StringBuilder builder)
		{
		}
	}
}
