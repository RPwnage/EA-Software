// Originally based on NAnt - A .NET build tool
// Copyright (C) 2018 Electronic Arts Inc.
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
using NAnt.Core.Attributes;
using NAnt.Core.Structured;
using NAnt.Core.Tasks;

namespace EA.Eaconfig.Structured
{
	// In config package, instead of creating each individual property, we should use structured XML to create the property info.  These properties
	// ultimately get used by the nant <build>, <cc>, <as>, etc tasks and also get used in setting up build optionsets.
	// May want to update this as we go to include the extra fields available in CompilerBase.cs (for both CcTask and AsTask), LibTask.cs, and 
	// LinkTask.cs

	public abstract class BuildToolTemplates : ConditionalElementContainer
	{
		/// <summary>Create template property {tool}.template.commandline.  Note that each argument should be separated by new line.</summary>
		[Property("commandline", Required = true)]
		public ConditionalPropertyElement CommandLine { get; set; } = new ConditionalPropertyElement();

		/// <summary>Templates that control response files, their contents and how they are passed to the build tool (optional)</summary>
		[Property("responsefile", Required = false)]
		public ResponseFileTemplates Responsefile { get; set; } = new ResponseFileTemplates();
	}

	// NOTE: in non-SXML the naming off the response file properties seemed backward, so they have been change to better reflect what they do.
	// However, for the purposes of backward compatibility we must still keep the original names under the hood.
	public sealed class ResponseFileTemplates : ConditionalElementContainer
	{
		/// <summary>A template of the command line when response files are being used.
		/// Normally this would just contain the path to the response file, using the token %responsefile%,
		/// but it can also contain any other properties that can't be put inside the response file.
		/// This corresponds to the old template property {tool}.template.responsefile (optional)</summary>
		[Property("commandline", Required = false)]
		public ConditionalPropertyElement CommandLine { get; set; } = new ConditionalPropertyElement();

		/// <summary>A template of the contents of the reponse file.
		/// This corresponds to the old template property {tool}.template.responsefile.commandline (optional)</summary>
		[Property("contents", Required = false)]
		public ConditionalPropertyElement Contents { get; set; } = new ConditionalPropertyElement();
	}

	public abstract class BuildToolTask<T> : Task where T : BuildToolTemplates, new()
	{
		/// <summary>Create readonly "{tool}" property (or verify property is not changed) and points to full path of c++ compiler.</summary>
		[TaskAttribute("path", Required = true)]
		public string Path { get; set; } = null;

		/// <summary>Create readonly "{tool}.version" property indicating tool version.</summary>
		[TaskAttribute("version", Required = true)]
		public string Version { get; set; } = null;

		/// <summary>Create readonly "{tool}.common-options" property indicating compiler options that will be applied to all c++ compiles (optional).  Note that each option should be separated by new line.</summary>
		[Property("common-options", Required = false)]
		public ConditionalPropertyElement CommonOptions { get; set; } = new ConditionalPropertyElement();

		[BuildElement("template", Required = true)]
		public T Templates { get; set; } = new T();

		protected abstract string ToolName { get; }

		protected override void ExecuteTask()
		{
			AddNewReadOnlyOrVerifyProperty(Project, ToolName, Path);
			AddNewReadOnlyProperty(Project, ToolName + ".version", Version);

			string commonOptionsFullValue = CommonOptions.Value;
			string userOptions = GetUserToolAdditionalOptions(Project, ToolName);
			if (userOptions != null)
			{
				commonOptionsFullValue += Environment.NewLine + userOptions;
			}
			AddNewReadOnlyProperty(Project, ToolName + ".common-options", commonOptionsFullValue, allowEmpty: true);

			AddNewReadOnlyPropertyOrUpdate(Project, ToolName + ".template.commandline", Templates.CommandLine.Value);
			AddNewReadOnlyPropertyOrUpdate(Project, ToolName + ".template.responsefile", Templates.Responsefile.CommandLine.Value, addOnlyIfNotNullOrEmpty: true);
			AddNewReadOnlyPropertyOrUpdate(Project, ToolName + ".template.responsefile.commandline", Templates.Responsefile.Contents.Value, addOnlyIfNotNullOrEmpty: true);

			// this should already exist, as it's defined with the platform agnostic option early in eaconfig init
			if (FindParentTask<NativeToolchain>() != null) // DAVE-TODO: horrific compatiblity hack
			{
				OptionSet configOptionsCommon = Project.NamedOptionSets.GetOrAdd("config-options-common", (key) => new OptionSet());
				SetOptionsetOption(ref configOptionsCommon, ToolName, Path);
				SetOptionsetOption(ref configOptionsCommon, $"buildset.{ToolName}.options", commonOptionsFullValue);
				UpdateConfigOptionsCommon(ref configOptionsCommon);
			}

			// Developer config validation
			if (Project.Log.DeveloperWarningEnabled)
			{
				Project.Properties.UpdateReadOnly("eaconfig.used-build-tool-tasks", "true");
				if (FindParentTask<NativeToolchain>() is null)
				{
					string configSystem = Project.Properties.GetPropertyOrFail("config-system");
					DoOnce.Execute($"eaconfig.{configSystem}.{Name}.buildtool", () => Project.Log.DeveloperWarning.WriteLine($"{Location} Structured tool '{Name}' for platform '{configSystem}' called outside of 'nativetoolchain' task."));
				}
			}
		}

		protected abstract void UpdateConfigOptionsCommon(ref OptionSet configOptionsCommon);

		protected static void AddNewReadOnlyProperty(Project proj, string propertyName, string value, bool allowEmpty = false, bool addOnlyIfNotNullOrEmpty = false)
		{
			if (String.IsNullOrWhiteSpace(value))
			{
				if (addOnlyIfNotNullOrEmpty)
				{
					return;
				}
				else if (!allowEmpty)
				{
					throw new BuildException($"Error setting property '{propertyName}' from structured build tool. Value is empty.");
				}
			}

			// Need to evaluate and expand the property right away or the value may get re-assigned later on.
			string expandedValue = proj.ExpandProperties(value ?? String.Empty);

			// Throw exception if value already exists.  The intent is that we should only set the tool once for each config
			// and once set, it should not be able to modified.
			string currValue = proj.Properties.GetPropertyOrDefault(propertyName, null);
			if (currValue != null)
			{
				throw new BuildException(string.Format("Attempting to create new read only property '{0}' and assign it to '{1}'.  But this property already exists and has value: '{2}'", propertyName, expandedValue, currValue));
			}
			proj.Properties.AddReadOnly(propertyName, expandedValue);
		}

		protected static void AddNewReadOnlyPropertyOrUpdate(Project proj, string propertyName, string value, bool allowEmpty = false, bool addOnlyIfNotNullOrEmpty = false)
		{
			if (String.IsNullOrWhiteSpace(value))
			{
				if (addOnlyIfNotNullOrEmpty)
				{
					return;
				}
				else if (!allowEmpty)
				{
					throw new BuildException($"Error setting property '{propertyName}' from structured build tool. Value is empty.");
				}
			}

			// Need to evaluate and expand the property right away or the value may get re-assigned later on.
			string expandedValue = proj.ExpandProperties(value);

			// Right now, we got same property being redefined all over the places and too many changes all in one go.
			// So for now, allow property to be defined somewhere else, but the execution of these build tool tasks will update value to new one.
			proj.Properties.UpdateReadOnly(propertyName, expandedValue);
		}

		protected static void AddNewReadOnlyOrVerifyProperty(Project proj, string propertyName, string value, bool allowEmpty = false, bool addOnlyIfNotNullOrEmtpy = false)
		{
			if (String.IsNullOrWhiteSpace(value))
			{
				if (addOnlyIfNotNullOrEmtpy)
				{
					return;
				}
				else if (!allowEmpty)
				{
					throw new BuildException($"Error setting property '{propertyName}' from structured build tool. Value is empty.");
				}
			}

			// Need to evaluate and expand the property right away or the value may get re-assigned later on.
			string expandedValue = proj.ExpandProperties(value);

			// Should only use this for the "cc", "as", "link", "lib" properties as they are setup in SDK packages.
			// The rest of the "template" properties should be setup in the config packages and should use the above functions.
			string currValue = proj.Properties.GetPropertyOrDefault(propertyName, null);
			if (currValue != null && currValue != expandedValue)
			{
				throw new BuildException(string.Format("Attempting to create new read only property '{0}' and assign it to '{1}'.  But this property already exists and has different value as: '{2}'", propertyName, expandedValue, currValue));
			}
			proj.Properties.AddReadOnly(propertyName, expandedValue);
		}

		// Provide people ability to quickly add additional options and experiment on new compiler/linker, etc features.
		protected static string GetUserToolAdditionalOptions(Project proj, string toolname)
		{
			string currConfigSystem = proj.Properties.GetPropertyOrFail("config-system");
			string currConfigCompiler = proj.Properties.GetPropertyOrFail("config-compiler");
			string currConfigPlatform = proj.Properties.GetPropertyOrFail("config-platform");

			string[] addtionalPropertyNames = new string[]
			{
				toolname + ".additional-options",
				toolname + "." + currConfigSystem + ".additional-options",
				toolname + "." + currConfigCompiler + ".additional-options",
				toolname + "." + currConfigPlatform + ".additional-options"
			};
			string[] additionalPropertyValues = addtionalPropertyNames
				.Select(propName => proj.Properties[propName])
				.Where(propValue => propValue != null)
				.ToArray();

			if (additionalPropertyValues.Any())
			{
				return String.Join(Environment.NewLine, additionalPropertyValues);
			}
			else
			{
				return null;
			}
		}

		protected void SetOptionsetOption(ref OptionSet optionset, string option, string value, bool append = false)
		{
			value = value ?? String.Empty;
			if (Project.Log.DeveloperWarningEnabled && !append)
			{
				string configSystem = Project.Properties.GetPropertyOrFail("config-system");
				if (optionset.Options.Contains(option))
				{
					DoOnce.Execute($"eaconfig.{configSystem}.config-options-common.defined-option-{option}", () => Project.Log.DeveloperWarning.WriteLine($"{Location} Optionset 'config-option-common' had existing value for option '{option}' before task '{Name}'."));
				}
			}
			if (append && optionset.Options.TryGetValue(option, out string existingOption))
			{
				optionset.Options[option] = existingOption + Environment.NewLine + value;
			}
			else
			{
				optionset.Options[option] = value;
			}
		}
	}


	//<compiler
	//	path="${cc}"				// This property usually expected to get set in SDK packages or compiler packages like VisualStudio, 
	//								// kettlesdk, UnixClang, PCClang, etc.  It uses a standardized name "cc" for c++ compiler.
	//	path-clanguage="${cc-clanguage}"	// cc-clanguage for c compiler.
	//	version="${cc.version}"
	//	>
	//	<template>					// Template is expected to be set in config package.
	//		<commandline>			// setup property cc.template.commandline
	//			%defines%
	//			%system-includedir%
	//			%includedirs%
	//			%usingdirs%
	//			%options%
	//			-Fo"%objectfile%"
	//			"%sourcefile%"
	//			"forceusing-assemblies%"
	//		</commandline>
	//		<commandline-clanguage>	// setup property cc-clanguage.template.commandline
	//			%defines%
	//			%system-includedir%
	//			%includedirs%
	//			%usingdirs%
	//			%options%
	//			-Fo"%objectfile%"
	//			"%sourcefile%"
	//			"forceusing-assemblies%"
	//		</commandline-clangulage>
	//		<responsefile.commandline>@"%responsefile%"</responsefile.commandline>
	//		<define>-D "%define%"</define>
	//		<includedir>-I "%includedir%"</includedir>
	//		<system-includedir>-I "%system-includedir%"</system-includedir>  // Non-pc system has other command line.
	//		<usingdir>-AI "%usingdir%"</usingdir>
	//		<forceusing-assembly>-FU "%forceusing-assembly%"</forceusing-assembly>
	//	</template>
	//</compiler>

	/// <summary>Setup c/c++ compiler template information.  This block meant to be used under &lt;compiler&gt; block.</summary>
	[ElementName("compilertemplate", StrictAttributeCheck = true, NestedElementsCheck = true)]
	public sealed class CompilerTemplates : BuildToolTemplates
	{
		/// <summary>Create template property cc-clanguage.template.commandline (optional).  Note that each argument should be separated by new line.</summary>
		[Property("commandline-clanguage", Required = false)]
		public ConditionalPropertyElement CommandLineCLanguage { get; set; } = new ConditionalPropertyElement();

		/// <summary>Create template property cc.template.pch.commandline (optional)</summary>
		[Property("pch.commandline", Required = false)]
		public ConditionalPropertyElement PchCommandLine { get; set; } = new ConditionalPropertyElement();

		/// <summary>Create template property cc.template.define</summary>
		[Property("define", Required = true)]
		public ConditionalPropertyElement Define { get; set; } = new ConditionalPropertyElement();

		/// <summary>Create template property cc.template.includedir</summary>
		[Property("includedir", Required = true)]
		public ConditionalPropertyElement IncludeDir { get; set; } = new ConditionalPropertyElement();

		/// <summary>Create template property cc.template.system-includedir</summary>
		[Property("system-includedir", Required = true)]
		public ConditionalPropertyElement SystemIncludeDir { get; set; } = new ConditionalPropertyElement();

		/// <summary>Create template property cc.template.usingdir (optional - should only get used for managed c++ build)</summary>
		[Property("usingdir", Required = false)]
		public ConditionalPropertyElement UsingDir { get; set; } = new ConditionalPropertyElement();

		/// <summary>Create template property cc.template.forceusing-assembly (optional - should only get used for managed c++ build)</summary>
		[Property("forceusing-assembly", Required = false)]
		public ConditionalPropertyElement ForceUsingAssembly { get; set; } = new ConditionalPropertyElement();
	}

	/// <summary>Setup c/c++ compiler properties for used by the build optionsets.</summary>
	[TaskName("compiler", StrictAttributeCheck = true, NestedElementsCheck = true)]
	public sealed class CompilerTask : BuildToolTask<CompilerTemplates>
	{
		/// <summary>Create readonly "cc.common-defines" property indicating defines that will be applied to all c++ compiles (optional).  Note that each define should be separated by new line.(</summary>
		[Property("common-defines", Required = false)]
		public ConditionalPropertyElement CommonDefines { get; set; } = new ConditionalPropertyElement();

		/// <summary>Create readonly "cc.system-includedirs" property indicating system (SDK) include directories that will be applied to all c/c++ compiles (optional).  Note that each path should be separated by new line.</summary>
		[Property("system-includedirs", Required = false)]
		public ConditionalPropertyElement SystemIncludeDirs { get; set; } = new ConditionalPropertyElement();

		/// <summary>Create readonly "cc-clanguage" property (or verify property is not changed) and points to full path of c compiler (optional).</summary>
		[TaskAttribute("path-clanguage", Required = false)]
		public string PathCLanguage { get; set; } = null;

		/// <summary>Create readonly "cc-clanguage.common-defines" property indicating defines that will be applied to all c compiles (optional).  Note that each define should be separated by new line.</summary>
		[Property("common-defines-clanguage", Required = false)]
		public ConditionalPropertyElement CommonDefinesCLanguage { get; set; } = new ConditionalPropertyElement();

		/// <summary>Create readonly "cc-clanguage.common-options" property indicating compiler options that will be applied to all c compiles (optional).  Note that each option should be separated by new line.</summary>
		[Property("common-options-clanguage", Required = false)]
		public ConditionalPropertyElement CommonOptionsCLanguage { get; set; } = new ConditionalPropertyElement();

		protected sealed override string ToolName { get { return "cc"; } }

		private const string CLanguageSuffix = "-clanguage";

		protected override void ExecuteTask()
		{
			base.ExecuteTask();

			AddNewReadOnlyOrVerifyProperty(Project, ToolName + CLanguageSuffix, PathCLanguage);

			AddNewReadOnlyProperty(Project, ToolName + ".common-defines", CommonDefines.Value, allowEmpty: true);

			AddNewReadOnlyProperty(Project, ToolName + ".system-includedirs", SystemIncludeDirs.Value, allowEmpty: true);

			AddNewReadOnlyProperty(Project, ToolName + ".template.pch.commandline", Templates.PchCommandLine.Value, addOnlyIfNotNullOrEmpty: true); // pch command line is optional, defaults to regular command line otherwise
			AddNewReadOnlyPropertyOrUpdate(Project, ToolName + ".template.define", Templates.Define.Value);
			AddNewReadOnlyPropertyOrUpdate(Project, ToolName + ".template.includedir", Templates.IncludeDir.Value);
			AddNewReadOnlyPropertyOrUpdate(Project, ToolName + ".template.system-includedir", Templates.SystemIncludeDir.Value);

			// special clanguage variants - only set if not null
			AddNewReadOnlyPropertyOrUpdate(Project, ToolName + CLanguageSuffix + ".template.commandline", Templates.CommandLineCLanguage.Value, addOnlyIfNotNullOrEmpty: true);
			AddNewReadOnlyProperty(Project, ToolName + CLanguageSuffix + ".common-defines", CommonDefinesCLanguage.Value, allowEmpty: true);

			string cLanguageCommonOptionsFull = (CommonOptionsCLanguage.Value ?? CommonOptions.Value) + // if user has set c options but there are no default c opts append to regular opts
					Environment.NewLine +
					GetUserToolAdditionalOptions(Project, ToolName + CLanguageSuffix);
			AddNewReadOnlyProperty(Project, ToolName + CLanguageSuffix + ".common-options", cLanguageCommonOptionsFull, allowEmpty: true);
			//TODO!!!

			// The following two are really PC specific!
			AddNewReadOnlyPropertyOrUpdate(Project, ToolName + ".template.usingdir", Templates.UsingDir.Value, addOnlyIfNotNullOrEmpty: true);
			AddNewReadOnlyPropertyOrUpdate(Project, ToolName + ".template.forceusing-assembly", Templates.ForceUsingAssembly.Value, addOnlyIfNotNullOrEmpty: true);
		}

		protected override void UpdateConfigOptionsCommon(ref OptionSet configOptionsCommon)
		{
			SetOptionsetOption(ref configOptionsCommon, "buildset.cc.defines", CommonDefines.Value);
			SetOptionsetOption(ref configOptionsCommon, "buildset.cc.system-includedirs", SystemIncludeDirs.Value);
		}
	}

	//<assembler
	//	path="${as}"
	//	version="${as.version}"
	//	>
	//	<template>
	//		<commandline>
	//			%defines%
	//			%system-includedir%
	//			%includedirs%
	//			%options%
	//			-Fo"%objectfile%"
	//		</commandline>
	//		<responsefile.commandline>@"%responsefile%"</responsefile.commandline>
	//		<define>-D "%define%"</define>
	//		<includedir>-I "%includedir%"</includedir>
	//		<system-includedir>-I "%system-includedir%"</system-includedir>
	//	</template>
	//</assembler>

	/// <summary>Setup c/c++ assembler template information.  This block meant to be used under &lt;assembler&gt; block.</summary>
	[ElementName("assemblertemplate", StrictAttributeCheck = true, NestedElementsCheck = true)]
	public sealed class AssemblerTemplates : BuildToolTemplates
	{
		/// <summary>Create template property as.template.define</summary>
		[Property("define", Required = true)]
		public ConditionalPropertyElement Define { get; set; } = new ConditionalPropertyElement();

		/// <summary>Create template property as.template.includedir</summary>
		[Property("includedir", Required = true)]
		public ConditionalPropertyElement IncludeDir { get; set; } = new ConditionalPropertyElement();

		/// <summary>Create template property as.template.system-includedir</summary>
		[Property("system-includedir", Required = true)]
		public ConditionalPropertyElement SystemIncludeDir { get; set; } = new ConditionalPropertyElement();
	}

	/// <summary>Setup c/c++ assembler properties for used by the build optionsets.</summary>
	[TaskName("assembler", StrictAttributeCheck = true, NestedElementsCheck = true)]
	public sealed class AssemblerTask : BuildToolTask<AssemblerTemplates>
	{
		/// <summary>Create readonly "as.common-defines" property indicating defines that will be applied to all c++ assembly compiles (optional).  Note that each define should be separated by new line.</summary>
		[Property("common-defines", Required = false)]
		public ConditionalPropertyElement CommonDefines { get; set; } = new ConditionalPropertyElement();

		/// <summary>Create readonly "as.system-includedirs" property indicating system (SDK) include directories that will be applied to all assembly compiles (optional).  Note that each path should be separated by new line.</summary>
		[Property("system-includedirs", Required = false)]
		public ConditionalPropertyElement SystemIncludeDirs { get; set; } = new ConditionalPropertyElement();

		protected sealed override string ToolName { get { return "as"; } }

		protected override void ExecuteTask()
		{
			base.ExecuteTask();

			AddNewReadOnlyProperty(Project, ToolName + ".common-defines", CommonDefines.Value, allowEmpty: true);
			AddNewReadOnlyProperty(Project, ToolName + ".system-includedirs", SystemIncludeDirs.Value, allowEmpty: true);

			AddNewReadOnlyPropertyOrUpdate(Project, ToolName + ".template.define", Templates.Define.Value);
			AddNewReadOnlyPropertyOrUpdate(Project, ToolName + ".template.includedir", Templates.IncludeDir.Value);
			AddNewReadOnlyPropertyOrUpdate(Project, ToolName + ".template.system-includedir", Templates.SystemIncludeDir.Value);
		}

		protected override void UpdateConfigOptionsCommon(ref OptionSet configOptionsCommon)
		{
			SetOptionsetOption(ref configOptionsCommon, "buildset.as.system-includedirs", SystemIncludeDirs.Value);
		}
	}

	//<librarian
	//	path="${lib}"
	//	path-clanguage="${lib-clanguage}"
	//	version="${lib.version}"
	//	>
	//	<template>
	//		<commandline>
	//			%options%
	//			%objectfiles%
	//		</commandline>
	//		<responsefile.commandline>@"%responsefile%"</responsefile.commandline>
	//		<objectfile>"%objectfile%"</objectfile>
	//	</template>
	//</librarian>

	/// <summary>Setup c/c++ librarian (archiver) template information.  This block meant to be used under &lt;librarian&gt; block.</summary>
	[ElementName("librariantemplate", StrictAttributeCheck = true, NestedElementsCheck = true)]
	public sealed class LibrarianTemplates : BuildToolTemplates
	{
		/// <summary>Create template property lib.template.objectfile</summary>
		[Property("objectfile", Required = true)]
		public ConditionalPropertyElement ObjectFile { get; set; } = new ConditionalPropertyElement();
	}

	/// <summary>Setup c/c++ librarian (archiver) properties for used by the build optionsets.</summary>
	[TaskName("librarian", StrictAttributeCheck = true, NestedElementsCheck = true)]
	public sealed class LibrarianTask : BuildToolTask<LibrarianTemplates>
	{
		protected sealed override string ToolName { get { return "lib"; } }

		protected override void ExecuteTask()
		{
			base.ExecuteTask();

			AddNewReadOnlyPropertyOrUpdate(Project, ToolName + ".template.objectfile", Templates.ObjectFile.Value);
		}

		protected override void UpdateConfigOptionsCommon(ref OptionSet configOptionsCommon)
		{
		}
	}

	//<linker
	//	path="${link}"
	//	version="${link.version}"
	//	>
	//	<template>
	//		<commandline>
	//			%options%
	//			%librarydirs%
	//			%objectfiles%
	//			%libraryfiles%
	//		</commandline>
	//		<responsefile.commandline>@"%responsefile%"</responsefile.commandline>
	//		<librarydir>-libpath:"%librarydir%"</librarydir>
	//		<libraryfile>"%libraryfile%"</libraryfile>
	//		<objectfile>"%objectfile%"</objectfile>
	//	</template>
	//</linker>

	/// <summary>Setup c/c++ linker template information.  This block meant to be used under &lt;linker&gt; block.</summary>
	[ElementName("linkertemplate", StrictAttributeCheck = true, NestedElementsCheck = true)]
	public sealed class LinkerTemplates : BuildToolTemplates
	{
		/// <summary>Create template property link.template.librarydir</summary>
		[Property("librarydir", Required = true)]
		public ConditionalPropertyElement LibraryDir { get; set; } = new ConditionalPropertyElement();

		/// <summary>Create template property link.template.libraryfile</summary>
		[Property("libraryfile", Required = true)]
		public ConditionalPropertyElement LibraryFile { get; set; } = new ConditionalPropertyElement();

		/// <summary>Create template property link.template.objectfile</summary>
		[Property("objectfile", Required = true)]
		public ConditionalPropertyElement ObjectFile { get; set; } = new ConditionalPropertyElement();
	}

	/// <summary>Setup c/c++ linker properties for used by the build optionsets.</summary>
	[TaskName("linker", StrictAttributeCheck = true, NestedElementsCheck = true)]
	public sealed class LinkerTask : BuildToolTask<LinkerTemplates>
	{
		/// <summary>Create readonly "link.system-librarydirs" property indicating system (SDK) library directories that will be applied to all build (optional).  Note that each path should be separated by new line.</summary>
		[Property("system-librarydirs", Required = false)]
		public ConditionalPropertyElement SystemLibraryDirs { get; set; } = new ConditionalPropertyElement();

		/// <summary>
		/// Create readonly "linker.system-libs" property indicating system (SDK) libraries that will be applied to all builds (optional).  Note: Try not to use full path.  Just use library filename.  Each library should be separated by new line.
		/// Library path information should be setup in &lt;system-librarydirs&gt; block.
		/// </summary>
		/// <example>
		///  <para>Example PC style specification:</para>
		///  <code><![CDATA[
		///  <link ...>
		///    <system-libs>
		///       kernel32.lib
		///       User32.lib
		///    </system-libs>
		///  </link>
		///  ]]></code>
		///  <para>Example gcc style specification where -lXYZ stands for libXYZ.a, just use -lXYZ:</para>
		///  <code><![CDATA[
		///  <link ...>
		///    <system-libs>
		///       -lpthread
		///    </system-libs>
		///  </link>
		///  ]]></code>
		/// </example>
		[Property("system-libs", Required = false)]
		public ConditionalPropertyElement SystemLibs { get; set; } = new ConditionalPropertyElement();

		protected sealed override string ToolName { get { return "link"; } }

		protected override void ExecuteTask()
		{
			base.ExecuteTask();

			AddNewReadOnlyProperty(Project, ToolName + ".system-librarydirs", SystemLibraryDirs.Value, allowEmpty: true);
			AddNewReadOnlyProperty(Project, ToolName + ".system-libs", SystemLibs.Value, allowEmpty: true);

			AddNewReadOnlyPropertyOrUpdate(Project, ToolName + ".template.librarydir", Templates.LibraryDir.Value);
			AddNewReadOnlyPropertyOrUpdate(Project, ToolName + ".template.libraryfile", Templates.LibraryFile.Value);
			AddNewReadOnlyPropertyOrUpdate(Project, ToolName + ".template.objectfile", Templates.ObjectFile.Value);
		}

		protected override void UpdateConfigOptionsCommon(ref OptionSet configOptionsCommon)
		{
			SetOptionsetOption(ref configOptionsCommon, "buildset.link.librarydirs", SystemLibraryDirs.Value);
			SetOptionsetOption(ref configOptionsCommon, "buildset.link.libraries", SystemLibs.Value);
		}
	}
}