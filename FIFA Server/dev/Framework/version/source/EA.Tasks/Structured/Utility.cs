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
using System.Xml;

using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Structured;
using NAnt.Core.Util;

namespace EA.Eaconfig.Structured
{
	/// <summary></summary>
	[TaskName("Utility", NestedElementsCheck = true)]
	public class UtilityTask : ModuleBaseTask
	{
		public UtilityTask() : base("Utility")
		{
			BuildSteps = new BuildStepsElement(this);
		}

		/// <summary>Specified that any dependencies listed in this module will be propagated for transitive linking. Only use this if module is a wrapper for a static library.</summary>
		[TaskAttribute("transitivelink", Required = false)]
		public bool TransitiveLink { get; set; } = false;

		/// <summary>Setup the location where &lt;copylocal&gt; of dependencies should copy the files to.</summary>
		[TaskAttribute("outputdir", Required = false)]
		public String OutputDir { get; set; } = String.Empty;

		/// <summary>Files with associated custombuildtools</summary>
		[FileSet("custombuildfiles", Required = false)]
		public StructuredFileSetCollection CustomBuildFiles { get; } = new StructuredFileSetCollection();

		/// <summary>Files that are part of the project but are excluded from the build.</summary>
		[FileSet("excludedbuildfiles", Required = false)]
		public StructuredFileSet ExcludedFiles { get; set; } = new StructuredFileSet();

		/// <summary>Sets the build steps for a project</summary>
		[BuildElement("buildsteps", Required = false)]
		public BuildStepsElement BuildSteps { get; }

		/// <summary>Set up special step to do file copies.</summary>
		[BuildElement("filecopystep", Required = false)]
		public FileCopyStepCollection FileCopySteps { get; } = new FileCopyStepCollection();

		/// <summary>Sets the configuration for a project</summary>
		[BuildElement("config", Required = false)]
		public UtilityConfigElement Config { get; } = new UtilityConfigElement();

		protected override void SetupModule()
		{
			if (OutputDir != String.Empty)
			{
				SetModuleProperty("outputdir", OutputDir);
			}

			SetupDependencies();
			SetupFiles();
		}

		protected void SetupFiles()
		{
			string projType = BuildTypeInfo.IsCSharp ? "csproj" : "vcproj";

			SetModuleFileset(projType + ".excludedbuildfiles", ExcludedFiles);

			StructuredFileSet custombuildfiles = new StructuredFileSet();
			foreach (StructuredFileSet fs in CustomBuildFiles.FileSets)
			{
				custombuildfiles.AppendWithBaseDir(fs);
				custombuildfiles.AppendBase = fs.AppendBase;
			}
			if (custombuildfiles.BaseDirectory == null) custombuildfiles.BaseDirectory = Project.BaseDirectory;

			SetModuleProperty("transitivelink", TransitiveLink ? "true" : "false");

			SetModuleFileset("custombuildfiles", custombuildfiles);

			SetModuleTarget(BuildSteps.PrebuildTarget, "prebuildtarget");
			SetModuleTarget(BuildSteps.PostbuildTarget, "postbuildtarget");

			SetModuleProperty(projType + ".pre-build-step", BuildSteps.PrebuildTarget.Command.Value);
			SetModuleProperty(projType + ".post-build-step", BuildSteps.PostbuildTarget.Command.Value);

			SetModuleProperty("vcproj.custom-build-tool", BuildSteps.CustomBuildStep.Script.Value);
			SetModuleProperty("vcproj.custom-build-outputs", BuildSteps.CustomBuildStep.OutputDependencies.Value);
			SetModuleFileset("vcproj.custom-build-dependencies", BuildSteps.CustomBuildStep.InputDependencies);
			SetModuleProperty("custom-build-before-targets", BuildSteps.CustomBuildStep.Before);
			SetModuleProperty("custom-build-after-targets", BuildSteps.CustomBuildStep.After);

			SetModuleProperty("preprocess", Config.Preprocess);
			SetModuleProperty("postprocess", Config.Postprocess);

			// --- Run ---
			SetModuleProperty("workingdir", TrimWhiteSpace(BuildSteps.RunData.WorkingDir), false);
			SetModuleProperty("commandargs", TrimWhiteSpace(BuildSteps.RunData.Args), false);
			SetModuleProperty("commandprogram", TrimWhiteSpace(BuildSteps.RunData.StartupProgram), false);

			SetModuleProperty("run.workingdir", TrimWhiteSpace(BuildSteps.RunData.WorkingDir), false);
			SetModuleProperty("run.args", TrimWhiteSpace(BuildSteps.RunData.Args), false);

			// filecopystep feature 
			int copystepCount=0;
			foreach (var copystep in FileCopySteps.fileCopyStepCollection)
			{
				string propName = string.Format("filecopystep{0}",copystepCount);
				if (copystep.ToDir != null && copystep.FileSet != null)
				{
					SetModuleProperty(string.Format("{0}.todir", propName), copystep.ToDir, false);
					SetModuleFileset(string.Format("{0}.fileset", propName), copystep.FileSet);
				}
				else if (copystep.ToDir != null && copystep.File != null)
				{
					SetModuleProperty(string.Format("{0}.todir", propName), copystep.ToDir, false);
					SetModuleProperty(string.Format("{0}.file", propName), copystep.File, false);
				}
				else if (copystep.ToFile != null && copystep.File != null)
				{
					SetModuleProperty(string.Format("{0}.tofile", propName), copystep.ToFile, false);
					SetModuleProperty(string.Format("{0}.file", propName), copystep.File, false);
				}
				++copystepCount;
			}
		}

		protected override void InitModule() { }

		protected override void FinalizeModule() { }

		/// <summary>Sets various attributes for a config</summary>
		[ElementName("config", StrictAttributeCheck = true, NestedElementsCheck = true)]
		public class UtilityConfigElement : ConditionalElementContainer
		{
			/// <summary>Preprocess step can be C# task derived from AbstractModuleProcessorTask class or a target. Multiple preprocess steps are supported</summary>
			[Property("preprocess", Required = false)]
			public ConditionalPropertyElement Preprocess { get; } = new ConditionalPropertyElement();

			/// <summary>Preprocess step can be C# task derived from AbstractModuleProcessorTask class or a target. Multiple preprocess steps are supported</summary>
			[Property("postprocess", Required = false)]
			public ConditionalPropertyElement Postprocess { get; } = new ConditionalPropertyElement();
		}

		internal class FileCopyStep : ConditionalElementContainer
		{
			[TaskAttribute("file", Required = false)]
			public string File { get; set; } = null;

			[TaskAttribute("tofile", Required = false)]
			public string ToFile { get; set; } = null;

			[TaskAttribute("todir", Required = false)]
			public string ToDir { get; set; } = null;

			[FileSet("fileset", Required = false)]
			public StructuredFileSet FileSet { get; } = new StructuredFileSet();
		}

		/// <summary>Setup a file copy step for a utility module.</summary>
		[ElementName("filecopystep", StrictAttributeCheck = true)]
		public class FileCopyStepCollection : ConditionalElement
		{
			internal readonly List<FileCopyStep> fileCopyStepCollection = new List<FileCopyStep>();

			public override void Initialize(XmlNode elementNode)
			{
				IfDefined = true;
				UnlessDefined = false;
				base.Initialize(elementNode);
			}

			public override bool WarnOnMultiple
			{
				get { return false; }
			}

			// Note that the following 3 properties are technically not needed by this class.  It is added just 
			// for the benefit of the code doc generation.

			/// <summary>File to copy.</summary>
			[TaskAttribute("file", Required = false)]
			public string File { get; set; } = null;

			/// <summary>Destination file. If this is specified, expects 'file' attribute to be specified.</summary>
			[TaskAttribute("tofile", Required = false)]
			public string ToFile { get; set; } = null;

			/// <summary>Destination directory. If this is specified, expects either 'file' attribute or 'fileset' inner element to be specified.</summary>
			[TaskAttribute("todir", Required = false)]
			public string ToDir { get; set; } = null;

			[XmlElement("file", "string", AllowMultiple = false, Description = "File to copy")]
			[XmlElement("tofile", "string", AllowMultiple = false, Description = "Destination file.")]
			[XmlElement("todir", "string", AllowMultiple = false, Description = "Destination directory.")]
			[XmlElement("fileset", "EA.Eaconfig.Structured.StructuredFileSet", AllowMultiple = false, Description = "Fileset specifying a list of files to copy to directory.")]
			protected override void InitializeElement(XmlNode elementNode)
			{
				if (IfDefined && !UnlessDefined)
				{
					FileCopyStep copystep = new FileCopyStep();
					copystep.Project = Project;
					copystep.Initialize(elementNode);
					fileCopyStepCollection.Add(copystep);
				}
			}
		}
	}
}
