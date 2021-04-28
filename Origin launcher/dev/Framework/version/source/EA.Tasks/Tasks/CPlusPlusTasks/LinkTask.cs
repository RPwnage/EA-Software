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
using System.Collections.Concurrent;
using System.Diagnostics;
using System.IO;
using System.Text;
using System.Linq;
using System.Threading;

using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Util;
using NAnt.Core.Logging;

namespace EA.CPlusPlusTasks
{

	/// <summary>A generic C/C++ linker task.</summary>
	/// <remarks>
	/// <para>The task requires the following properties:</para>
	/// <list type='table'>
	/// <listheader><term>Property</term><description>Description</description></listheader>
	/// <item><term>${link}</term><description>The absolute pathname of the linker executable.</description></item>
	/// </list>
	/// <para>
	/// The task will use the following properties:
	/// </para>
	/// <list type='table'>
	/// <listheader><term>Property</term>                    <description>Description</description></listheader>
	/// <item>      <term>${link.options}</term>             <description>The option flags for the linker.</description></item>
	/// <item>      <term>${link.libraries}</term>           <description>The system libraries to link with.</description></item>
	/// <item>      <term>${link.librarydirs}</term>         <description>The library search paths for the linker.</description></item>
	/// <item>      <term>${link.threadcount}</term>         <description>The number of parallel linker processes. Default is 1 per cpu.</description></item>
	/// <item>      <term>${link.template.commandline}</term><description>The template to use when creating the command line.  Default is %options% %librarydirs% %libraryfiles% %objectfiles%.</description></item>
	/// <item>      <term>${link.template.librarydir}</term> <description>The syntax template to transform the <term>${link.librarydirs}</term> property into linker flags.</description></item>
	/// <item>      <term>${link.template.libraryfile}</term><description>The syntax template to transform the <term>libraries</term> file set into linker flags.</description></item>
	/// <item>      <term>${link.template.objectfile}</term> <description>The syntax template to transform the <term>objects</term> file set into linker flags.</description></item>
	/// <item>      <term>${link.postlink.program}</term>    <description>The path to program to run after linking.  If not defined no post link process will be run.</description></item>
	/// <item>      <term>${link.postlink.workingdir}</term> <description>The directory the post link program will run in.  Defaults to the program's directory.</description></item>
	/// <item>      <term>${link.postlink.commandline}</term><description>The command line to use when running the post link program.</description></item>
	/// <item>      <term>${link.postlink.redirect}</term>   <description>Whether to redirect output of postlink program.</description></item>
	/// <item>      <term>${link.useresponsefile}</term>     <description>If <c>true</c> a response file, containing the entire commandline, will be passed as the only argument to the linker. Default is false.</description></item>
	/// <item>      <term>${link.userelativepaths}</term>    <description>If <c>true</c> the working directory of the linker will be set to the <code>outputdir</code>. All object files will then be made relative to this path. Default is <c>false</c>.</description></item>
	/// <item>      <term>${link.template.responsefile}</term><description>The syntax template to transform the <term>responsefile</term> into a response file flag. Default is @"%responsefile%".</description></item>
	/// <item>      <term>${link.template.responsefile.commandline}</term><description>The template to use when creating the command line for response file. Default value is ${link.template.responsefile.commandline}</description></item>
	/// <item>      <term>${link.responsefile.separator}</term><description>Separator used in response files. Default is " ".</description></item>
	/// <item>      <term>${link.template.responsefile.objectfile}</term><description>The syntax template to transform the <term>objects</term> file set into linker flags in response file. Default ${link.template.objectfile}</description></item>
	/// <item>      <term>${link.template.responsefile.libraryfile}</term><description>The syntax template to transform the <term>libraries</term> file set into linker flags in response file. Default ${link.template.libraryfile}</description></item>
	/// <item>      <term>${link.template.responsefile.librarydir}</term><description>The syntax template to transform the <term>${link.librarydirs}</term> property into linker flags in response file. Default ${link.template.librarydir}</description></item>
	///	<item>      
	///		<term>${link.forcelowercasefilepaths}</term>      
	///		<description>If <c>true</c> file paths (folder and file names) will be 
	///		forced to lower case (useful in avoiding redundant NAnt builds 
	///		between VS.net and command line environments in the case of capitalized
	///		folder names when file name case is unimportant).  Default is false.</description>
	///	</item>
	/// 
	/// </list>
	/// <para>
	/// The task declares the following template items in order to help user defining the above properties:
	/// </para>
	/// <list type='table'>
	/// <listheader><term>Template item</term><description>Description</description></listheader>
	/// <item>      <term>%outputdir%</term>  <description>Used by the <term>${link.options}</term> property to represent the actual values of the <term>outputdir</term> attribute.</description></item>
	/// <item>      <term>%outputname%</term> <description>Used by the <term>${link.options}</term> property to represent the actual value of the <term>outputname</term> attribute.</description></item>
	/// <item>      <term>%librarydir%</term> <description>Used by the <term>${link.template.librarydir}</term> property to represent the individual value of the <term>${link.librarydirs}</term> property.</description></item>
	/// <item>      <term>%libraryfile%</term><description>Used by the <term>${link.template.libraryfile}</term> property to represent the individual value of the <term>libraries</term> file set.</description></item>
	/// <item>      <term>%objectfile%</term> <description>Used by the <term>${link.template.objectfile}</term> property to represent the individual value of the <term>objects</term> file set.</description></item>
	/// <item>      <term>%responsefile%</term> <description>Used by the <term>${cc..template.responsefile}</term> property to represent the filename of the response file.</description></item>
	/// </list>
	/// <para>The post link program allows you to run another program after linking.  For example if you are
	/// building programs for the XBox you will know that you need to run the imagebld program in order to
	/// use the program on the xbox.  Instead of having to run a second task after each build task you can
	/// setup the properties to run imagebld each time you link.  Use the <c>%outputdir%</c> and <c>%outputname</c>
	/// template items in the <c>link.postlink.commandline</c> to generalize the task for all the components
	/// in your package.</para>
	/// <para>This task can use a named option set for specifying options for unique components.  For more information see the Fundamentals->Option Sets topic in help file.</para>
	/// </remarks>
	/// <include file="Examples/Shared/HelloWorld.example" path="example"/>
	/// <include file="Examples/Shared/VisualCppSetup.example" path="example"/>
	/// <include file='Examples/Link/CommandLineTemplate.example' path='example'/>
	[TaskName("link")]
	public class LinkTask : BuildTaskBase {
		// properties used by task
		const string ProgramProperty                    = "link";
		const string OptionsProperty                    = "link.options";
		const string LibrariesProperty                  = "link.libraries";
		const string LibraryDirectoriesProperty         = "link.librarydirs";
		const string ThreadCountProperty                = "link.threadcount"; 

		const string NoDependencyProperty               = "link.nodep";
		const string NoLinkProperty                     = "link.nolink";

		const string PostLinkProgramProperty            = "link.postlink.program";
		const string PostLinkWorkingDirectoryProperty   = "link.postlink.workingdir";
		const string PostLinkCommandLineProperty        = "link.postlink.commandline";
		const string PostLinkRedirectProperty           = "link.postlink.redirect";

		// templates used by task to format command line
		const string LibraryDirectoryTemplateProperty   = "link.template.librarydir";
		const string LibraryDirectoryResponseTemplateProperty = "link.template.responsefile.librarydir";
		const string LibraryFileTemplateProperty        = "link.template.libraryfile";
		const string LibraryFileResponseTemplateProperty = "link.template.responsefile.libraryfile";
		const string ObjectFileTemplateProperty         = "link.template.objectfile";
		const string ObjectFileResponseTemplateProperty = "link.template.responsefile.objectfile";

		// template items user uses in the template properties to represent the actual values
		const string LibraryDirectoryVariable       = "%librarydir%";
		const string LibraryFileVariable            = "%libraryfile%";
		const string ObjectFileVariable             = "%objectfile%";

		// template items used in the link.template.commandline template
		const string OptionsVariable                = "%options%";
		const string LibraryDirectoriesVariable     = "%librarydirs%";
		const string LibraryFilesVariable           = "%libraryfiles%";
		const string ObjectFilesVariable            = "%objectfiles%";

		// template items uses users in cc.options property to represent actual values
		const string OutputDirectoryVariable        = "%outputdir%";
		const string OutputNameVariable             = "%outputname%";

		// We have other part of Framework adding the following tokens in some optionset.  So if 
		// people are executing this task explicitly and not using <Module> build script to set
		// things up, we will see the following token left over.  So we'll need to monitor
		// these tokens.
		const string AlternateOutputNameVariable    = "%linkoutputname%";
		const string LinkOutputPdbNameVariable      = "%linkoutputpdbname%";
		const string LinkOutputMapNameVariable      = "%linkoutputmapname%";

		const int DefaultThreadCount = 1;

		bool			_forceLowerCaseFilePaths= false;
		string          _outputDir;
		StringCollection _libraryDirectoryCollection = null;

		// Used to throtle number of threads across modules.
		private static ConcurrentDictionary<string, Semaphore> _linkerSemaphore = new ConcurrentDictionary<string, Semaphore>();


		public LinkTask() : base("link") {}

		/// <summary>Name of option set to use for compiling options.  If null
		/// given values from the global <c>link.*</c> properties.  Default is null.</summary>
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

		/// <summary>The base name of the library or program and any associated output files.  Default is an empty string.</summary>
		[TaskAttribute("outputname", Required = true)]
		public string OutputName { get; set; }

		/// <summary>The set of object files to link.</summary>
		[FileSet("objects")]
		public FileSet Objects { get; } = new FileSet();

		/// <summary>The list of libraries to link with.</summary>
		[FileSet("libraries")]
		public FileSet Libraries { get; } = new FileSet();

		/// <summary>Directories to search for additional libraries.  New line <c>"\n"</c> or semicolon <c>";"</c> delimited.</summary>
		[Property("librarydirs")]
		public PropertyElement LibraryPath { get; } = new PropertyElement();

		/// <summary>Custom program options for these files.</summary>
		[Property("options")]
		public PropertyElement Options { get; } = new PropertyElement();


		/// <summary>library output extension</summary>
		[TaskAttribute("linkoutputextension", Required = false)]
		public string LinkOutputExtension { get; set; } = String.Empty;

		public bool BuildExecuted { get; set; } = false;

		/// <summary>
		/// Initialize the task first with the default global options in the link.*
		/// properties and then if from the options in the <c>specialOptions</c>.
		/// </summary>
		/// <param name="specialOptions">Special options for this task instance
		/// that will override any global options.</param>
		public override void InitializeFromOptionSet(OptionSet specialOptions) 
		{
			// set initial option set values from global properties
			AddProperty(ProgramProperty);
			AddProperty(OptionsProperty);
			AddProperty(LibrariesProperty);
			AddProperty(LibraryDirectoriesProperty);
			AddProperty(ThreadCountProperty);
			AddProperty(NoDependencyProperty);
			AddProperty(NoLinkProperty);
			AddProperty(PostLinkProgramProperty);
			AddProperty(PostLinkWorkingDirectoryProperty);
			AddProperty(PostLinkCommandLineProperty);
			AddProperty(PostLinkRedirectProperty);
			AddProperty(CommandLineTemplateProperty);
			AddProperty(ResponseCommandLineTemplateProperty);
			AddProperty(LibraryDirectoryTemplateProperty);
			AddProperty(LibraryFileTemplateProperty);
			AddProperty(LibraryFileResponseTemplateProperty);
			AddProperty(ObjectFileTemplateProperty);
			AddProperty(ObjectFileResponseTemplateProperty);
			AddProperty(BuildTask.PathStyleProperty);
			AddProperty(UseResponseFileProperty);
			AddProperty(UseAltSepInResponseFileProperty);
			AddProperty(ResponseFileTemplateProperty);
			AddProperty(UseRelativePathsProperty);
			AddProperty(ForceLowerCaseFilePathsProperty);
			AddProperty(ResponseFileSeparator);
			AddProperty(LibraryFileResponseTemplateProperty);

			//after adding ForceLowerCaseFilePathsProperty we can set a flag, to avoid redundant calls to ForceLowerCaseFilePaths
			_forceLowerCaseFilePaths= ForceLowerCaseFilePaths;
			
			// merge any options passed via a named option set
			_optionSet.Append(specialOptions);

			CheckForRequiredProperties(_optionSet);
		}

		void CheckForRequiredProperties(OptionSet optionSet) 
		{
			string[] requiredProperties = 
			{
				ProgramProperty, 
				LibraryDirectoryTemplateProperty,
				LibraryFileTemplateProperty,
				ObjectFileTemplateProperty,
			};

			foreach(string property in requiredProperties) 
			{
				if (optionSet.Options[property] == null) 
				{
					string message = String.Format("Program property '{0}' is not defined.", property);
					throw new BuildException(message, Location);
				}
			}
		}

		/// <summary>True if we should run the linker.</summary>
		bool GenerateOutputFile {
			get {
				string v = GetOption(NoLinkProperty);
				if (v != null && String.Compare(v, "true", true) == 0) {
					return false;
				} else {
					return true;
				}
			}
		}

		/// <summary>True if we should generate a .dep file for each source file, otherwise false.</summary>
		bool GenerateDependencyFile {
			get {
				string v = GetOption(NoDependencyProperty);
				if (v != null && String.Compare(v, "true", true) == 0) {
					return false;
				} else {
					return true;
				}
			}
		}

		protected override void ExecuteTask() 
		{
			if (BuildTask.CheckForIndividualFileCompile(Project)) 
			{
				return;
			}

			if (Objects.BaseDirectory == null) 
			{
				Objects.BaseDirectory = Project.BaseDirectory;
			}
			OutputDir = Project.GetFullPath(OutputDir);

			// create directory if required
			if (!Directory.Exists(OutputDir)) 
			{
				Directory.CreateDirectory(OutputDir);
			}

			LinkObjects();
		}

		void AddTemplatedCommandLineArgument(string template, StringBuilder commandLine, string templateVariableName, string variableValue, string separator) 
		{
			string result = template.Replace(templateVariableName, variableValue);
			commandLine.Append(result);
			commandLine.Append(separator);
		}

		/// <summary></summary>
		/// <param name="responsefilecommand">The command line arguments to the library, if a response is being used, else null</param>
		/// <returns>The contents of the response file</returns>
		internal string GetCommandLine(out string responsefilecommand)
		{
			// Get the template that will be used to assembly the parts of the command line
			var commandLineBuilder = GetCommandLineTemplate();

			responsefilecommand = null;
			if (UseResponseFile)
			{
				responsefilecommand = GetOption(ResponseFileTemplateProperty);
			}
			StringBuilder responsefiletemplate = !String.IsNullOrEmpty(responsefilecommand) ? new StringBuilder(responsefilecommand) : null; 


			// get path style property, for LocalizePath calls below, needed for standardizing on / or \ for the path
			string pathStyle = GetOption(BuildTask.PathStyleProperty);

			// Get all the parts that the template specifies
			string options = string.Empty;
			if (UseAltSepInResponseFile)
				options = BuildTask.LocalizePath(GetOptionsCommandLine(), PathStyle.Unix.ToString()); // Some compilers (like unixclang on windows) don't like its parameters passed with \ character instead of treating it as a path separator it treats it as a the start of a special character like \n
			else
				options = GetOptionsCommandLine();

			string libraryDirectories  = BuildTask.LocalizePath(GetLibraryDirectoriesCommandLine(), pathStyle);
			string libraryFiles        = BuildTask.LocalizePath(GetLibraryFilesCommandLine(), pathStyle);
			string objectFiles         = BuildTask.LocalizePath(GetObjectFilesCommandLine(), pathStyle);

			//Console.WriteLine(" options: " +options);
			
			// Build the commandline by replacing the items in the template with their actual values
			commandLineBuilder.Replace(OptionsVariable, options);

			if (responsefiletemplate != null)
			{
				responsefiletemplate.Replace(OptionsVariable, options);
			}

			
			//BEWARE libraryDirectories contains a platform specific command line switch as well!
			//Also note there's no need to force these library directories to lowercase, since VS.net doesn't have
			//problems with capitalized folder names in this specific case when executing NAnt on a makefile project.
			commandLineBuilder.Replace(LibraryDirectoriesVariable, libraryDirectories);
			if (responsefiletemplate != null)
			{
				responsefiletemplate.Replace(LibraryDirectoriesVariable, libraryDirectories);
			}

//			Console.WriteLine(" libdir: " +libraryDirectories);
//			Console.WriteLine(" libfiles: " +libraryFiles);

			if (_forceLowerCaseFilePaths)
			{
				Debug.Assert(libraryFiles!=null, "\n null libraryFiles in LinkTask::GetCommandLine!");
				// force the path to lower case to get around any case sensitivity issues with file paths
				commandLineBuilder.Replace(LibraryFilesVariable, libraryFiles.ToLower());

				if (responsefiletemplate != null)
				{
					responsefiletemplate.Replace(LibraryFilesVariable, libraryFiles.ToLower());
				}

			}
			else
			{
				commandLineBuilder.Replace(LibraryFilesVariable, libraryFiles);
				if (responsefiletemplate != null)
				{
					responsefiletemplate.Replace(LibraryFilesVariable, libraryFiles);
				}

			}
			
			commandLineBuilder.Replace(ObjectFilesVariable, objectFiles);
			if (responsefiletemplate != null)
			{
				responsefiletemplate.Replace(ObjectFilesVariable, objectFiles);
			}
			

			if (OutputDir != null)
			{
				string outputDir = OutputDir;
				if (UseRelativePaths) 
				{
					outputDir = PathNormalizer.MakeRelative(outputDir, WorkingDirectory);
				}
				//standardize on / or \ for the path
				outputDir= BuildTask.LocalizePath(outputDir, pathStyle);

				if (_forceLowerCaseFilePaths)
				{
					// force the path to lower case to get around any case sensitivity issues with file paths
					commandLineBuilder.Replace(OutputDirectoryVariable, outputDir.ToLower());
					if (responsefiletemplate != null)
					{
						responsefiletemplate.Replace(OutputDirectoryVariable, outputDir.ToLower());
					}

				}
				else
				{
					if (outputDir == string.Empty)
					{
						// If path is empty, we don't want the path separator anymore or else
						// the file will be created at the root of the drive
						commandLineBuilder.Replace(OutputDirectoryVariable + @"\", outputDir);
						commandLineBuilder.Replace(OutputDirectoryVariable + "/", outputDir);

						// This is provided as a safety measure where if the first two replace
						// fails, %outputname% will get replaced
						commandLineBuilder.Replace(OutputDirectoryVariable, outputDir);

						if (responsefiletemplate != null)
						{
							responsefiletemplate.Replace(OutputDirectoryVariable + @"\", outputDir);
							responsefiletemplate.Replace(OutputDirectoryVariable + "/", outputDir);
							responsefiletemplate.Replace(OutputDirectoryVariable, outputDir);
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
			string outputPdbName = ".pdb";
			string outputMapName = ".map";
			if (OutputName != null)
			{
				fullOutputName = OutputName + LinkOutputExtension;
				outputPdbName = OutputName + ".pdb";
				outputMapName = OutputName + ".map";
				if (_forceLowerCaseFilePaths)
				{
					fullOutputName = fullOutputName.ToLower();
					outputPdbName = outputPdbName.ToLower();
					outputMapName = outputMapName.ToLower();
				}
			}
			commandLineBuilder.Replace(OutputNameVariable + LinkOutputExtension, fullOutputName);
			commandLineBuilder.Replace(OutputNameVariable, fullOutputName);
			commandLineBuilder.Replace(AlternateOutputNameVariable + LinkOutputExtension, fullOutputName);
			commandLineBuilder.Replace(AlternateOutputNameVariable, fullOutputName);
			commandLineBuilder.Replace(LinkOutputPdbNameVariable, outputPdbName);
			commandLineBuilder.Replace(LinkOutputMapNameVariable, outputMapName);
			if (responsefiletemplate != null)
			{
				responsefiletemplate.Replace(OutputNameVariable + LinkOutputExtension, fullOutputName);
				responsefiletemplate.Replace(OutputNameVariable, fullOutputName);
				responsefiletemplate.Replace(AlternateOutputNameVariable, fullOutputName);
				responsefiletemplate.Replace(AlternateOutputNameVariable + LinkOutputExtension, fullOutputName);
				responsefiletemplate.Replace(LinkOutputPdbNameVariable, outputPdbName);
				responsefiletemplate.Replace(LinkOutputMapNameVariable, outputMapName);
			}

			// expand properties and return string as commandline
			string commandLine = Project.ExpandProperties(commandLineBuilder.ToString());
			if (responsefiletemplate != null)
			{
				responsefilecommand = Project.ExpandProperties(responsefiletemplate.ToString());
			}
			return commandLine;
		}

		protected override string DefaultCommandLineTemplate
		{
			get
			{
				// use default command line template
				return  String.Format("{0} {1} {2} {3}",
					OptionsVariable,
					LibraryDirectoriesVariable,
					ObjectFilesVariable,
					LibraryFilesVariable);
			}
		}

		string GetOptionsCommandLine() 
		{
			// add task instance specific options last to override any global options
			StringCollection options = new StringCollection();
			AddDelimitedProperties(options, GetOption(OptionsProperty)); // global options
			AddDelimitedProperties(options, Options.Value);                       // instance options

			StringBuilder commandLine = new StringBuilder();
			foreach (string option in options) 
			{
				commandLine.Append(option);
				commandLine.Append(" ");
			}
			return commandLine.ToString();
		}

		string GetLibraryDirectoriesCommandLine() 
		{
			// add user lib directories first to override any system directories (this is different than options)
			_libraryDirectoryCollection = new StringCollection();
			AddDelimitedProperties(_libraryDirectoryCollection, LibraryPath.Value);
			AddDelimitedProperties(_libraryDirectoryCollection, GetOption(LibraryDirectoriesProperty));

			string template = GetOption(LibraryDirectoryTemplateProperty);

			string separator = " ";
			if (UseResponseFile)
			{
				separator = GetOption(ResponseFileSeparator) ?? "\n";
				template = GetOption(LibraryDirectoryResponseTemplateProperty) ?? template;
			}

			StringBuilder commandLine = new StringBuilder();
			foreach (string library in _libraryDirectoryCollection)
			{
				AddTemplatedCommandLineArgument(
					template,
					commandLine, 
					LibraryDirectoryVariable, 
					library,
					separator);
			}
			return commandLine.ToString();
		}

		string GetLibraryFilesCommandLine() 
		{
			StringBuilder commandLine = new StringBuilder();
			
			StringCollection libraryFiles = new StringCollection();
			AddDelimitedProperties(libraryFiles, GetOption(LibrariesProperty));

			string template = GetOption(LibraryFileTemplateProperty);
			string separator = " ";
			if (UseResponseFile)
			{
				separator = GetOption(ResponseFileSeparator) ?? "\n";
				template = GetOption(LibraryFileResponseTemplateProperty) ?? template;
			}

			foreach (string libraryFileName in libraryFiles) 
			{
				AddTemplatedCommandLineArgument(
					template,
					commandLine, 
					LibraryFileVariable, 
					libraryFileName,
					separator);
			}

			template = GetOption(LibraryFileTemplateProperty);
			foreach (FileItem fileItem in Libraries.FileItems.OrderedDistinct(FileItem.PathEqualityComparer)) 
			{
				AddTemplatedCommandLineArgument(
					template,
					commandLine, 
					LibraryFileVariable, 
					fileItem.FileName,
					separator);
			}
			return commandLine.ToString();
		}

		public override string WorkingDirectory {
			get {
				return OutputDir;
			}
		}

		string GetObjectFilesCommandLine() {
			StringBuilder commandLine = new StringBuilder();
			
			string template = GetOption(ObjectFileTemplateProperty);
			string separator = " ";
			if (UseResponseFile)
			{
				separator = GetOption(ResponseFileSeparator) ?? "\n";
				template = GetOption(ObjectFileResponseTemplateProperty) ?? template;
			}

			foreach (FileItem fileItem in Objects.FileItems) 
			{
				string objPath = fileItem.FileName;
				if (UseRelativePaths) 
				{
					objPath = PathNormalizer.MakeRelative(objPath, WorkingDirectory);
				}

				Debug.Assert(objPath!=null, "\n null objPath in GetObjectFilesCommandLine!");

				if (_forceLowerCaseFilePaths)
				{
					// force the path to lower case to get around any case sensitivity issues with file paths
					AddTemplatedCommandLineArgument(
						template,
						commandLine, 						 
						ObjectFileVariable, 
						objPath.ToLower(),
						separator);
				}
				else
				{
					AddTemplatedCommandLineArgument(
						template,
						commandLine, 						
						ObjectFileVariable, 
						objPath,
						separator);
				}
			}
			return commandLine.ToString();
		}

		void RunLinker(string program, string commandLine, string responsefilecommand, string reason) 
		{
			Log.Status.WriteLine(LogPrefix + OutputName);
			Log.Info.WriteLine(LogPrefix + "Linking {0} files because {1}", Objects.FileItems.Count, reason);
			Log.Info.WriteLine(LogPrefix + "Creating program in {0}", UriFactory.CreateUri(OutputDir).ToString());

			if (UseResponseFile) 
			{
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

			
			using (ProcessRunner processRunner = new ProcessRunner(process))
			{
				try 
				{
					processRunner.Start();
				} 
				catch (Exception e) 
				{
					string msg = String.Format("Error starting linker '{0}'.", program);
					if (Log.Level < Log.LogLevel.Detailed)
					{
						msg += String.Format("\n\tWorking Directory: {0}\n\tCommand Line: {1}", process.StartInfo.WorkingDirectory, process.StartInfo.Arguments);
					}
					throw new BuildException(msg, Location, e);
				} 

				string output = processRunner.GetOutput();

				if (processRunner.ExitCode != 0) 
				{
					string msg = String.Format("Could not link files {0}.{1}{2}", ProcessUtil.DecodeProcessExitCode(processRunner.ExitCode), Environment.NewLine, output);
					throw new BuildException(msg, Location);
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

		void LinkObjects() 
		{
			string program = GetOption(ProgramProperty);
			string responsefilecommand;
			string commandLine = GetCommandLine(out responsefilecommand);

			string [] programExtensions = { "exe", "self", "elf", "dll", "xex", "xbe", "prx", "so", "" };
			string dependencyFilePath = Path.Combine(OutputDir, OutputName + Dependency.DependencyFileExtension);

			string reason;

			string commandLineWithoutNewline = commandLine;
			if (UseResponseFile) 
			{
				commandLineWithoutNewline = commandLine.Replace("\n", "");
			}

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

			// Need to pass some extra data to the Dependency checker to properly search for the library directories:
			StringCollection toSearchLibraryFiles = new StringCollection();

			foreach (FileItem fileItem in Libraries.FileItems) 
			{
				if (fileItem.AsIs && !Path.IsPathRooted(fileItem.FullPath))
				{
					toSearchLibraryFiles.Add(fileItem.FullPath);
				}
			}

			BuildExecuted = false;
			if (!Dependency.IsUpdateToDate(dependencyFilePath, outputDirOverride, OutputName, program, commandLineWithoutNewline, programExtensions, _libraryDirectoryCollection, toSearchLibraryFiles, out reason)) 
			{
				if (GenerateOutputFile) 
				{
				   var sem = _linkerSemaphore.GetOrAdd(Properties["config-platform"] ?? "default", key => new Semaphore(Math.Max(GetThreadCount(), 1), Math.Max(GetThreadCount(), 1)));
				   try
				   {
					   sem.WaitOne();

					   RunLinker(program, commandLine, responsefilecommand, reason);
					   RunPostLinkProgram();
				   }
				   finally
				   {
					   sem.Release();
				   }
				}

				if (GenerateDependencyFile) 
				{
					Dependency.GenerateDependencyFile(dependencyFilePath, program, commandLineWithoutNewline, Objects.FileItems.ToStringCollection(), Libraries.FileItems.ToStringCollection());
				}

				BuildExecuted = true;
			}
			else if (IsRunPostlinkAlwaysEnabled())
			{
				   var sem = _linkerSemaphore.GetOrAdd(Properties["config-platform"] ?? "default", key => new Semaphore(Math.Max(GetThreadCount(), 1), Math.Max(GetThreadCount(), 1)));
				   try
				   {
					   sem.WaitOne();
					   RunPostLinkProgram();
				   }
				   finally
				   {
					   sem.Release();
				   }

			}
		}

		// Currently this option is enabled for Android builds so that we can run the packaging
		// process as a post link step even though the linker may not need to run.
		private bool IsRunPostlinkAlwaysEnabled()
		{
			string runAlwaysOption = _optionSet.Options["__private_link_postlink_always"];
			return runAlwaysOption == "on";
		}


		string GetPostLinkCommandLine() {
			string commandLineTemplate = GetOption(PostLinkCommandLineProperty);
			if (commandLineTemplate == null) 
			{
				string msg = String.Format("If you define '{0}' you must also define '{1}'.", 
					PostLinkProgramProperty, PostLinkCommandLineProperty);
				throw new BuildException(msg, Location);
			}

			StringBuilder commandLineBuilder = new StringBuilder();

			// strip any newlines and extra white space
			foreach (string token in commandLineTemplate.Split(new char[] {' ', '\n'})) 
			{
				string trimmed = token.Trim();
				if (trimmed.Length > 0) 
				{
					if(!trimmed.StartsWith("/")) // Options may start with / and we don't want them to be replace to /
						trimmed = BuildTask.LocalizePath(trimmed, GetOption(BuildTask.PathStyleProperty));
						
					commandLineBuilder.Append(trimmed);
					commandLineBuilder.Append(" ");
				}
			}

			// replace commandline variables;
			if (OutputDir != null && _forceLowerCaseFilePaths)
			{
				// force the path to lower case to get around any case sensitivity issues with file paths
				commandLineBuilder.Replace(OutputDirectoryVariable, BuildTask.LocalizePath(OutputDir.ToLower(), GetOption(BuildTask.PathStyleProperty)));
			}
			else
			{
				commandLineBuilder.Replace(OutputDirectoryVariable, BuildTask.LocalizePath(OutputDir, GetOption(BuildTask.PathStyleProperty)));
			}

			string fullOutputName = OutputName;
			string outputPdbName = ".pdb";
			string outputMapName = ".map";
			if (OutputName != null)
			{
				fullOutputName = OutputName + LinkOutputExtension;
				outputPdbName = OutputName + ".pdb";
				outputMapName = OutputName + ".map";
				if (_forceLowerCaseFilePaths)
				{
					fullOutputName = fullOutputName.ToLower();
					outputPdbName = outputPdbName.ToLower();
					outputMapName = outputMapName.ToLower();
				}
			}
			commandLineBuilder.Replace(OutputNameVariable, BuildTask.LocalizePath(fullOutputName, GetOption(BuildTask.PathStyleProperty)));
			commandLineBuilder.Replace(AlternateOutputNameVariable, BuildTask.LocalizePath(fullOutputName, GetOption(BuildTask.PathStyleProperty)));
			commandLineBuilder.Replace(LinkOutputPdbNameVariable, BuildTask.LocalizePath(outputPdbName, GetOption(BuildTask.PathStyleProperty)));
			commandLineBuilder.Replace(LinkOutputMapNameVariable, BuildTask.LocalizePath(outputMapName, GetOption(BuildTask.PathStyleProperty)));


			// expand properties
			string commandLine = Project.ExpandProperties(commandLineBuilder.ToString());

			// localize path based on path style property (smelly)
			commandLine = BuildTask.LocalizePathInCommandLine(commandLine, GetOption(BuildTask.PathStyleProperty));

			return commandLine;
		}

		void RunPostLinkProgram() 
		{
			string program = GetOption(PostLinkProgramProperty);
			if (program != null) 
			{
				string commandLine = GetPostLinkCommandLine();
				string workingDir = GetOption(PostLinkWorkingDirectoryProperty);
				string redirect = GetOption(PostLinkRedirectProperty);
				bool bRedirect = true;
				if (!String.IsNullOrEmpty(redirect))
				{
					bRedirect = Boolean.Parse(redirect);
				}

				Process process = CreateProcess(program, commandLine);
				if (workingDir != null) 
				{
					process.StartInfo.WorkingDirectory = workingDir;
				}

				if (Log.Level >= Log.LogLevel.Detailed)
				{
					ProcessUtil.SafeInitEnvVars(process.StartInfo);
					Log.Info.WriteLine(LogPrefix + "postlink:");
					Log.Info.WriteLine(LogPrefix + "{0} {1}", process.StartInfo.FileName,process.StartInfo.Arguments);
					Log.Info.WriteLine(LogPrefix + "WorkingDirectory=\"{0}\"", process.StartInfo.WorkingDirectory);
					Log.Info.WriteLine(LogPrefix + "Environment=\"{0}\"", process.StartInfo.EnvironmentVariables.Cast<System.Collections.DictionaryEntry>().ToString("; ", de => de.Key + "=" + de.Value));
				}

				using (ProcessRunner processRunner = new ProcessRunner(process, bRedirect, bRedirect, true))
				{
					try 
					{
						Log.Info.WriteLine(LogPrefix + "{0} {1}", program, commandLine);
						processRunner.Start();
					} 
					catch (Exception e) 
					{
						string msg = String.Format("Error starting post link program '{0}'.", program);
						throw new BuildException(msg, Location, e);
					} 

					if (processRunner.ExitCode != 0) 
					{
						int indentLevel = Log.IndentLevel;
						Log.IndentLevel = 0;                    
						Log.Error.WriteLine(processRunner.StandardError);
						Log.IndentLevel = indentLevel;

						string msg = String.Format("Could not run post link program {0}.\n{1}", ProcessUtil.DecodeProcessExitCode(processRunner.ExitCode), processRunner.StandardOutput);
						throw new BuildException(msg, Location);
					}
				}
			}
		}

		protected override string GetWorkingDirFromTemplate(string program, string defaultVal)
		{
			string option = GetOption(WorkingdirTemplateProperty);
			if (String.IsNullOrWhiteSpace(option))
			{
				return defaultVal;
			}
			return option.Replace("%tooldir%", program).Replace("%outputdir%", OutputDir);
		}

		int GetThreadCount()
		{
			// Create default number of threads for every CPU in order to keep the CPU's working 100%.
			// get cpu count
			int cpuCount;
			try
			{
				cpuCount = Environment.ProcessorCount;
			}
			catch
			{
				cpuCount = 1;
			}

			int baseThreadCount = DefaultThreadCount;
			try
			{
				string threadCount = GetOption(ThreadCountProperty);
				if (threadCount != null)
				{
					baseThreadCount = Convert.ToInt32(threadCount);
					if (baseThreadCount <= 0)
					{
						string msg = String.Format("Invalid thread count value '{0}'.", baseThreadCount);
						throw new Exception(msg);
					}
				}
				else
				{
					baseThreadCount *= cpuCount;
					baseThreadCount++;
				}
			}
			catch (Exception e)
			{
				throw new BuildException(e.Message, Location, e);
			}


			return baseThreadCount;
		}

	}
}
