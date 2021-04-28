// Originally based on NAnt - A .NET build tool
// Copyright (C) 2003-2015 Electronic Arts Inc.
// Copyright (C) 2001-2003 Gerry Shaw
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
// 
// 
// Gerry Shaw (gerry_shaw@yahoo.com)
// Ian MacLean (ian_maclean@another.com)
// Electronic Arts (Frostbite.Team.CM@ea.com)

using System;
using System.Collections.Specialized;
using System.Diagnostics;
using System.IO;
using System.Text;
using System.Linq;

using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Util;
using NAnt.Core.Logging;

namespace EA.CPlusPlusTasks
{

	/// <summary>A generic C/C++ library manager tool.</summary>    
	/// <remarks>
	/// <para>
	/// The task requires the following properties:
	/// </para>
	/// <list type='table'>
	/// <listheader><term>Property</term><description>Description</description></listheader>
	/// <item><term>${lib}</term><description>The absolute pathname of the librarian executable to be used with this invocation of the task.</description></item>
	/// <item><term>${lib.userresponsefile}</term><description>If <c>true</c> a response file, containing the entire commandline, will be passed as the only argument to the compiler. Default is false.</description></item>
	/// </list>
	/// <para>
	/// The task declares the following properties:
	/// </para>
	/// <list type='table'>
	/// <listheader><term>Property</term>                    <description>Description</description></listheader>
	/// <item>      <term>${lib.options}</term>              <description>The options flags for the librarian.</description></item>
	/// <item>      <term>${lib.useresponsefile}</term>      <description>If <c>true</c> a response file, containing the entire commandline, will be passed as the only argument to the archiver. Default is false.</description></item>
	/// <item>      <term>${lib.userelativepaths}</term>     <description>If <c>true</c> the working directory of the archiver will be set to the <c>outputdir</c>. All object files will then be made relative to this path. Default is <c>false</c>.</description></item>
	/// <item>      <term>${lib.template.objectfile}</term>  <description>The syntax template to transform the <term>objects</term> file set into librarian flags.</description></item>
	/// <item>      <term>${lib.template.responsefile}</term><description>The syntax template to transform the <term>responsefile</term> into a response file flag. Default is @"%responsefile%".</description></item>
	/// <item>      <term>${lib.template.commandline}</term><description>The template to use when creating the command line.  Default is %options% %objectfiles%.</description></item>
	/// <item>      <term>${lib.template.responsefile.commandline}</term><description>The template to use when creating the command line for response file. Default value is ${lib.template.responsefile.commandline}</description></item>
	/// <item>      <term>${lib.responsefile.separator}</term><description>Separator used in response files. Default is " ".</description></item>
	/// <item>      <term>${lib.template.responsefile.objectfile}</term><description>The syntax template to transform the <term>objects</term> file set into librarian flags in response file. Default ${lib.template.objectfile}</description></item>
	/// <item>      <term>%responsefile%</term>              <description>Used by the <term>${cc.template.responsefile}</term> property to represent the filename of the response file.</description></item>
	///	<item>      
	///		<term>${lib.forcelowercasefilepaths}</term>      
	///		<description>If <c>true</c> file paths (folder and file names) will be 
	///		forced to lower case (useful in avoiding redundant NAnt builds 
	///		between VS.net and command line environments in the case of capitalized
	///		folder names when file name case is unimportant).  Default is false.</description>
	///	</item>
	/// </list>
	/// <para>
	/// The task declares the following template items in order to help user defining the above properties:
	/// </para>
	/// <list type='table'>
	/// <listheader><term>Template item</term><description>Description</description></listheader>
	/// <item>      <term>%outputdir%</term>  <description>Used in the <c>${lib.options}</c> property to represent the actual values of the <c>%outputdir%</c> attribute.</description></item>
	/// <item>      <term>%outputname%</term> <description>Used in the <c>${lib.options}</c> property to represent the actual value of the <c>%outputname%</c> attribute.</description></item>
	/// <item>      <term>%objectfile%</term> <description>Used in the <c>${lib.template.objectfile}</c> property to represent the individual value of the <c>objects</c> file set.</description></item>
	/// </list>
	/// <para>This task can use a named option set for specifying options for unique components.  For more information see the Fundamentals->Option Sets topic in help file.</para>
	/// </remarks>
	/// <example>
	///   <para>Archive the given object files into the specified library.</para>
	///   <code>
	///     <![CDATA[
	/// <project>
	///     <dependent name="VisualStudio" version="7.0.0"/>
	///
	///     <lib outputdir="lib" outputname="mylib">
	///         <objects>
	///             <includes name="obj/*.obj"/>
	///         </objects>
	///     </lib>
	/// </project>
	///     ]]>
	///   </code>
	/// </example>
	/// <include file="Examples/Shared/VisualCppSetup.example" path="example"/>
	[TaskName("lib")]
	public class LibTask : BuildTaskBase {
		// properties used by task
		const string ProgramProperty                = "lib";
		const string OptionsProperty                = "lib.options";

		// templates used by task to format command line
		const string ObjectFileTemplateProperty    = "lib.template.objectfile";
		const string ObjectFileResponseTemplateProperty    = "lib.template.responsefile.objectfile";

		// template items user uses in the template properties to represent the actual values
		const string ObjectFileVariable             = "%objectfile%";

		// template items uses users in lib.options property to represent actual values
		const string OutputDirectoryVariable        = "%outputdir%";
		const string OutputNameVariable             = "%outputname%";

		// For some reason, eaconfig force insert an option to use %liboutputname% instead of the above %outputname% and
		// Framework's module processor also look for %liboutputname%.  So we should look for this token here as well.
		const string AlternateOutputNameVariable     = "%liboutputname%";

		// template items used in the link.template.commandline template
		const string OptionsVariable = "%options%";
		const string ObjectFilesVariable = "%objectfiles%";


		bool			_forceLowerCaseFilePaths= false;
		string          _outputDir;
		string _libraryExtension = String.Empty;
		string _libraryPrefix = String.Empty;

		public LibTask() : base("lib") {}

		/// <summary>Name of option set to use for compiling options.  If null
		/// given values from the global <c>lib.*</c> properties.  Default is null.</summary>
		[TaskAttribute("type", Required=false)]
		public override string OptionSetName { 
			get { return _optionSetName; } 
			set { _optionSetName = value; }
		}

		/// <summary>Directory where all output files are placed.  Default is the base directory of the build file.</summary>
		[TaskAttribute("outputdir")]
		public string OutputDir { 
			get { return _outputDir; } 
			set { _outputDir = PathNormalizer.Normalize(value, false); }
		}

		/// <summary>The base name of the library and any associated output files.  Default is an empty string.</summary>
		[TaskAttribute("outputname")]
		public string OutputName { get; set; }

		/// <summary>The list of object files to combine into a library.</summary>
		[FileSet("objects")]
		public FileSet Objects { get; } = new FileSet();

		/// <summary>
		/// Custom program options for this task.  These get appended to the options specified in the <c>lib.options</c> property.
		/// </summary>
		[Property("options")]
		public PropertyElement Options { get; } = new PropertyElement();

		/// <summary>library output extension</summary>
		[TaskAttribute("libextension", Required = false)]
		public string LibraryExtension
		{
			get { return _libraryExtension; }
			set { _libraryExtension = value; }
		}

		/// <summary>library output prefix</summary>
		[TaskAttribute("libprefix", Required = false)]
		public string LibraryPrefix
		{
			get { return _libraryPrefix; }
			set { _libraryPrefix = value; }
		}

		public bool BuildExecuted { get; set; } = false;

		/// <summary>
		/// Initialize the task first with the default global options in the lib.*
		/// properties and then if from the options in the <c>specialOptions</c>.
		/// </summary>
		/// <param name="specialOptions">Special options for this task instance
		/// that will override any global options.</param>
		public override void InitializeFromOptionSet(OptionSet specialOptions) {
			// set initial option set values from global properties
			AddProperty(ProgramProperty);
			AddProperty(OptionsProperty);
			AddProperty(ObjectFileTemplateProperty);
			AddProperty(ObjectFileResponseTemplateProperty);
			AddProperty(BuildTask.PathStyleProperty);
			AddProperty(UseResponseFileProperty);
			AddProperty(UseAltSepInResponseFileProperty);
			AddProperty(ResponseFileTemplateProperty);
			AddProperty(UseRelativePathsProperty);
			AddProperty(ForceLowerCaseFilePathsProperty);
			AddProperty(CommandLineTemplateProperty);
			AddProperty(ResponseCommandLineTemplateProperty);
			AddProperty(ResponseFileSeparator);

			//after adding ForceLowerCaseFilePathsProperty we can set a flag, to avoid redundant calls to ForceLowerCaseFilePaths
			_forceLowerCaseFilePaths= ForceLowerCaseFilePaths;
			
			// merge any options passed via a named option set
			_optionSet.Append(specialOptions);

			CheckForRequiredProperties(_optionSet);
		}

		void CheckForRequiredProperties(OptionSet optionSet) {
			if (optionSet.Options[ProgramProperty] == null) {
				string message = String.Format("Program property '{0}' is not defined.", ProgramProperty);
				throw new BuildException(message, Location);
			}
		}

		protected override void ExecuteTask() {
			if (BuildTask.CheckForIndividualFileCompile(Project)) {
				return;
			}

			if (Objects.BaseDirectory == null) {
				Objects.BaseDirectory = Project.BaseDirectory;
			}
			OutputDir = Project.GetFullPath(OutputDir);

			// create directory if required
			if (!Directory.Exists(OutputDir)) {
				Directory.CreateDirectory(OutputDir);
			}

			LibObjects();
		}

		void AddTemplatedArgument(string template, StringBuilder arguments, string templateVariableName, string variableValue, string separator = " ") {
			string result = template.Replace(templateVariableName, variableValue);
			arguments.Append(result);
			arguments.Append(separator);
		}

		public override string WorkingDirectory  {
			get {
				return OutputDir;
			}
		}

		protected override string DefaultCommandLineTemplate
		{
			get 
			{
				return String.Format("{0} {1}",
					OptionsVariable,
					ObjectFilesVariable);
			}
		}

		/// <summary></summary>
		/// <param name="responsefilecommand">The command line arguments to the library, if a response is being used, else null</param>
		/// <returns>The contents of the response file</returns>
		internal string GetCommandLine(out string responsefilecommand) {
			// NOTE: We do not expand the properties at this point because we still need to add
			// our task properties to the Project.Properties dictionary.  Expanding properties
			// at this point would prevent the user from specifying where the source files and 
			// object file names should appear on the command lone.

			// add user options last to override any system options
			StringCollection options = new StringCollection();
			AddDelimitedProperties(options, GetOption(OptionsProperty));
			AddDelimitedProperties(options, Options.Value);

			// Build the commandline by replacing the items in the template with their actual values
			var commandLineBuilder = GetCommandLineTemplate();

			responsefilecommand = null;
			string separator = " ";
			string objectfiletemplate = GetOption(ObjectFileTemplateProperty);
			if (UseResponseFile)
			{
				responsefilecommand = GetOption(ResponseFileTemplateProperty);
				separator = GetOption(ResponseFileSeparator) ?? separator;
				objectfiletemplate = GetOption(ObjectFileResponseTemplateProperty) ?? objectfiletemplate;
			}
			StringBuilder responsefiletemplate = !String.IsNullOrEmpty(responsefilecommand) ? new StringBuilder(responsefilecommand): null;

			string optionsStr = options.ToString(" ");
			if (UseAltSepInResponseFile)
				optionsStr = BuildTask.LocalizePath(optionsStr, PathStyle.Unix.ToString()); // Some compilers (like unixclang on windows) don't like its parameters passed with \ character instead of treating it as a path separator it treats it as a the start of a special character like \n

			commandLineBuilder.Replace(OptionsVariable, optionsStr); 
			if (responsefiletemplate != null)
			{
				responsefiletemplate.Replace(OptionsVariable, optionsStr);
			}

			// get path style property, for LocalizePath calls below, needed for standardizing on / or \ for the path
			string pathStyle = GetOption(BuildTask.PathStyleProperty);

			var objects = new StringBuilder();

			// add objects
			foreach (FileItem fileItem in Objects.FileItems) 
			{
				
				string objPath = fileItem.FileName;
				if (UseRelativePaths) {
					objPath = PathNormalizer.MakeRelative(objPath, WorkingDirectory);
				}
			
				Debug.Assert(objPath!=null, "\n null objPath in LibTask::GetCommandLine!");
				
				//standardize on / or \ for the path
				objPath= BuildTask.LocalizePath(objPath, pathStyle);

				if (_forceLowerCaseFilePaths)
				{
					// By forcing the path to lower case, we get around any case sensitivity issues with file paths
					AddTemplatedArgument(objectfiletemplate, objects, ObjectFileVariable, objPath.ToLower(), separator);
				}
				else
				{
					AddTemplatedArgument(objectfiletemplate, objects, ObjectFileVariable, objPath, separator);
				}
			}

			commandLineBuilder.Replace(ObjectFilesVariable, objects.ToString());
			if (responsefiletemplate != null)
			{
				responsefiletemplate.Replace(ObjectFilesVariable, objects.ToString());
			}


			// replace variables
			if (OutputDir != null)
			{
				string outputDir = OutputDir;

				//standardize on / or \ for the path
				outputDir= BuildTask.LocalizePath(outputDir, pathStyle);

				if (_forceLowerCaseFilePaths)
				{
					// By forcing the path to lower case, we get around any case sensitivity issues with file paths
					commandLineBuilder.Replace(OutputDirectoryVariable, outputDir.ToLower());
					if (responsefiletemplate != null)
					{
						responsefiletemplate.Replace(OutputDirectoryVariable, outputDir.ToLower());
					}

				}
				else
				{
					commandLineBuilder.Replace(OutputDirectoryVariable, outputDir);
					if (responsefiletemplate != null)
					{
						responsefiletemplate.Replace(OutputDirectoryVariable, outputDir);
					}

				}
			}
			else
			{
				commandLineBuilder.Replace(OutputDirectoryVariable, OutputDir);
				if (responsefiletemplate != null)
				{
					responsefiletemplate.Replace(OutputDirectoryVariable, OutputDir);
				}

			}

			string fullOutputName = OutputName;
			if (fullOutputName != null)
			{
				fullOutputName = LibraryPrefix + OutputName + LibraryExtension;
				if (_forceLowerCaseFilePaths)
				{
					fullOutputName = fullOutputName.ToLower();
				}
			}

			commandLineBuilder.Replace(LibraryPrefix + OutputNameVariable + LibraryExtension, fullOutputName);
			commandLineBuilder.Replace(OutputNameVariable, fullOutputName);
			if (responsefiletemplate != null)
			{
				responsefiletemplate.Replace(OutputNameVariable, fullOutputName);
			}
			commandLineBuilder.Replace(LibraryPrefix + AlternateOutputNameVariable + LibraryExtension, fullOutputName);
			commandLineBuilder.Replace(AlternateOutputNameVariable, fullOutputName);
			if (responsefiletemplate != null)
			{
				responsefiletemplate.Replace(AlternateOutputNameVariable, fullOutputName);
			}

			// expand properties and return string as commandline
			string commandLine = Project.ExpandProperties(commandLineBuilder.ToString());
			if (responsefiletemplate != null)
			{
				responsefilecommand = Project.ExpandProperties(responsefiletemplate.ToString());
			}
			return commandLine;
		}

		// Some archivers (mostly GCC) always add new obj files, without removing files that aren't present anymore
		void CleanOldLib(string outputDir, string outputName, string libprefix, string extension, string cmdline)
		{
			if (!PlatformUtil.IsWindows || cmdline.Contains("-rs "))
			{
				var path = "unknown";
				try
				{
					path = Path.Combine(outputDir, libprefix + outputName + extension);
					
					if (File.Exists(path))
					{
						Log.Info.WriteLine(LogPrefix + "Deleting old library: {0}", path);
						File.Delete(path);
					}
				}
				catch (Exception ex)
				{
					Log.Warning.WriteLine(LogPrefix + "Failed to delete library file '{0}' before recreating library. Reason: {1}", path, ex.Message);
				}

			}
		}

		void RunArchiver(string program, string commandLine, string responsefilecommand, string reason) {
			Log.Status.WriteLine(LogPrefix + OutputName);
			Log.Info.WriteLine(LogPrefix + "Archiving {0} files because {1}", Objects.FileItems.Count, reason);
			Log.Info.WriteLine(LogPrefix + "Creating library in {0}", UriFactory.CreateUri(OutputDir).ToString());
			
			if (UseResponseFile) {
				commandLine = GetResponseFileCommandLine(OutputName, OutputDir, commandLine, null, responsefilecommand);
			}

			Process process = CreateProcess(program, commandLine);

			if (Log.Level >= Log.LogLevel.Detailed)
			{
				ProcessUtil.SafeInitEnvVars(process.StartInfo);
				Log.Info.WriteLine("{0}commandline:{1}", LogPrefix, OutputName);
				Log.Info.WriteLine("{0}{1} {2}", LogPrefix, program, commandLine.Trim());
				Log.Info.WriteLine("{0}WorkingDirectory=\"{1}\"", LogPrefix, process.StartInfo.WorkingDirectory);
				Log.Info.WriteLine("{0}Environment=\"{1}\"", LogPrefix, process.StartInfo.EnvironmentVariables.Cast<System.Collections.DictionaryEntry>().ToString("; ", de => de.Key + "=" + de.Value));
			}

			using (ProcessRunner processRunner = new ProcessRunner(process)) {
				try {
					processRunner.Start();
				} 
				catch (Exception e) {
					string msg = String.Format("Error starting library tool '{0}'.", program);
					if (Log.Level < Log.LogLevel.Detailed)
					{
						msg += String.Format("\n\tWorking Directory: {0}\n\tCommand Line: {1}",process.StartInfo.WorkingDirectory, process.StartInfo.Arguments);
					}
					throw new BuildException(msg, Location, e, stackTrace: BuildException.StackTraceType.XmlOnly);
				}

				string output = processRunner.GetOutput();

				if (processRunner.ExitCode != 0) {
					string msg = String.Format("Could not create library {0}.{1}{2}", ProcessUtil.DecodeProcessExitCode(processRunner.ExitCode), Environment.NewLine, output);
					throw new BuildException(msg, Location, stackTrace: BuildException.StackTraceType.None);
				}

				// If the word "warning" is in the output the program has issued a 
				// warning message but the return code is still zero because 
				// warnings are not errors.  We still need to display the message 
				// but doing this feels like such a hack.  See bug 415 for details.
				if ((Log.Level >= Log.LogLevel.Detailed && output.Length > 0) || (output.IndexOf("warning") != -1))
				{
					int indentLevel = Log.IndentLevel;
					Log.IndentLevel = 0;
					Log.Status.WriteLine(output);
					Log.IndentLevel = indentLevel;
				}
			}
		}

		void LibObjects() {
			string program = GetOption(ProgramProperty);
			string responsefilecommand;
			string commandLine = GetCommandLine(out responsefilecommand);

			string[] libraryExtensions = {"lib", "a", "so"};
			string dependencyFilePath = Path.Combine(OutputDir, OutputName + Dependency.DependencyFileExtension);

			string reason;

			// Check for the overridden outputdir and if exists, use it to do dependency checking instead of the default one.
			string outputDirOverride = "";
			if (Project.Properties.Contains("build.outputdir.override"))
			{
				outputDirOverride = Project.Properties["build.outputdir.override"];
			}
			else
			{
				outputDirOverride = OutputDir;
			}

			if (!Dependency.IsUpdateToDate(dependencyFilePath, outputDirOverride, OutputName, program, commandLine, libraryExtensions, null, null, out reason))
			{
				CleanOldLib(outputDirOverride, OutputName, LibraryPrefix, LibraryExtension, commandLine);
				RunArchiver(program, commandLine, responsefilecommand, reason);
				if (PlatformUtil.IsOSX)
				{
					string outputLibFile = Path.Combine(outputDirOverride, LibraryPrefix + OutputName + LibraryExtension);
					FileInfo outfile = new FileInfo(outputLibFile);
					if (outfile.Exists)
					{
						// The "libtool" on osx seems to not write out timestamp info down to milliseconds and we can sometimes get into
						// dependency check failed due to input *.obj file has newer timestamp (from clang++ compiler output) due to those
						// timestamp containing milliseconds info.  For now, we do this hack to get around this issue by touching the timestamp
						// after running libtool.
						// This issue shows up in our unit test in test_nant\tests\CrossPlatformTests\PrePostBuildStepsExecution
						// when run on osx 10.14.3 using Xcode 10.
						outfile.LastWriteTime = DateTime.Now;
					}
				}
				Dependency.GenerateDependencyFile(dependencyFilePath, program, commandLine, Objects.FileItems.ToStringCollection());
				BuildExecuted = true;
			}
			else
			{
				BuildExecuted = false;
			}
		}
	}
}
