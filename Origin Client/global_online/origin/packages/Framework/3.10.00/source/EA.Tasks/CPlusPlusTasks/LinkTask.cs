// (c) 2002-2003 Electronic Arts Inc.

using System;
using System.Collections;
using System.Collections.Specialized;
using System.Diagnostics;
using System.IO;
using System.Text;
using System.Linq;

using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Util;
using NAnt.Core.Logging;

namespace EA.CPlusPlusTasks {
    
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

        const string NoDependencyProperty               = "link.nodep";
        const string NoLinkProperty                     = "link.nolink";

        const string PostLinkProgramProperty            = "link.postlink.program";
        const string PostLinkWorkingDirectoryProperty   = "link.postlink.workingdir";
        const string PostLinkCommandLineProperty        = "link.postlink.commandline";
        const string PostLinkRedirectProperty           = "link.postlink.redirect";

        // templates used by task to format command line
        const string CommandLineTemplateProperty        = "link.template.commandline";
        const string LibraryDirectoryTemplateProperty   = "link.template.librarydir";
        const string LibraryFileTemplateProperty        = "link.template.libraryfile";
        const string ObjectFileTemplateProperty         = "link.template.objectfile";

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


		bool			_forceLowerCaseFilePaths= false;
		string          _outputDir;
        string          _outputName;
        FileSet         _objects     = new FileSet();
        FileSet         _libraries   = new FileSet();
        PropertyElement _linkOptions = new PropertyElement();
        PropertyElement _libraryPath = new PropertyElement();
        StringCollection _libraryDirectoryCollection = null;
        string _linkOutputExtension = String.Empty;

        public LinkTask() : base("link") {}

        /// <summary>Name of option set to use for compiling options.  If null
        /// given values from the global <c>link.*</c> propertes.  Default is null.</summary>
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
        [TaskAttribute("outputname", Required=true)]
        public string OutputName { 
            get { return _outputName; } 
            set { _outputName = value; }
        }

        /// <summary>The set of object files to link.</summary>
        [FileSet("objects")]
        public FileSet Objects { 
            get { return _objects; }
        }

        /// <summary>The list of libraries to link with.</summary>
        [FileSet("libraries")]
        public FileSet Libraries { 
            get { return _libraries; }
        }

        /// <summary>Directories to search for additional libraries.  New line <c>"\n"</c> or semicolon <c>";"</c> delimited.</summary>
        [Property("librarydirs")]
        public PropertyElement LibraryPath {
            get { return _libraryPath; }
        }

        /// <summary>Custom program options for these files.</summary>
        [Property("options")]
        public PropertyElement Options {
            get { return _linkOptions; }
        }


        /// <summary>library output extension</summary>
        [TaskAttribute("linkoutputextension", Required = false)]
        public string LinkOutputExtension
        {
            get { return _linkOutputExtension; }
            set { _linkOutputExtension = value; }
        }

        ///<summary>Initializes task and ensures the supplied attributes are valid.</summary>
        ///<param name="taskNode">Xml node used to define this task instance.</param>
        protected override void InitializeTask(System.Xml.XmlNode taskNode) {
            OptionSet specialOptions = null;
            if (OptionSetName != null) {
                Project.NamedOptionSets.TryGetValue(OptionSetName, out specialOptions);
            }
            if (specialOptions == null) {
                specialOptions = new OptionSet();
            }
            InitializeFromOptionSet(specialOptions);
        }

        /// <summary>
        /// Initialize the task first with the default global options in the link.*
        /// properties and then if from the options in the <c>specialOptions</c>.
        /// </summary>
        /// <param name="specialOptions">Special options for this task instance
        /// that will override any global options.</param>
        public override void InitializeFromOptionSet(OptionSet specialOptions) {
            // set initial option set values from global properties
            AddProperty(ProgramProperty);
            AddProperty(OptionsProperty);
            AddProperty(LibrariesProperty);
            AddProperty(LibraryDirectoriesProperty);
            AddProperty(NoDependencyProperty);
            AddProperty(NoLinkProperty);
            AddProperty(PostLinkProgramProperty);
            AddProperty(PostLinkWorkingDirectoryProperty);
            AddProperty(PostLinkCommandLineProperty);
            AddProperty(PostLinkRedirectProperty);
            AddProperty(CommandLineTemplateProperty);
            AddProperty(LibraryDirectoryTemplateProperty);
            AddProperty(LibraryFileTemplateProperty);
            AddProperty(ObjectFileTemplateProperty);
            AddProperty(BuildTask.PathStyleProperty);
            AddProperty(UseResponseFileProperty);
            AddProperty(UseAltSepInResponseFileProperty);
            AddProperty(ResponseFileTemplateProperty);
            AddProperty(UseRelativePathsProperty);
			AddProperty(ForceLowerCaseFilePathsProperty);

			//after adding ForceLowerCaseFilePathsProperty we can set a flag, to avoid redundant calls to ForceLowerCaseFilePaths
			_forceLowerCaseFilePaths= ForceLowerCaseFilePaths;
			
			// merge any options passed via a named option set
            _optionSet.Append(specialOptions);

            CheckForRequiredProperties(_optionSet);
        }

        void CheckForRequiredProperties(OptionSet optionSet) {
            string[] requiredProperties = {
                ProgramProperty, 
                LibraryDirectoryTemplateProperty,
                LibraryFileTemplateProperty,
                ObjectFileTemplateProperty,
            };

            foreach(string property in requiredProperties) {
                if (optionSet.Options[property] == null) {
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

            LinkObjects();
        }

        void AddTemplatedCommandLineArgument(StringBuilder commandLine, string templatePropertyName, string templateVariableName, string variableValue) {
            string template = GetOption(templatePropertyName);
            string result = template.Replace(templateVariableName, variableValue);
            commandLine.Append(result);
            commandLine.Append(" ");
        }

        string GetCommandLine() {
            // Get the template that will be used to assembly the parts of the command line
            string commandLineTemplate = GetCommandLineTemplate();

			// get path style property, for LocalizePath calls below, needed for standardizing on / or \ for the path
			string pathStyle = GetOption(BuildTask.PathStyleProperty);

			// Get all the parts that the template specifies
            string options             = GetOptionsCommandLine();
            string libraryDirectories  = BuildTask.LocalizePath(GetLibraryDirectoriesCommandLine(), pathStyle);
            string libraryFiles        = BuildTask.LocalizePath(GetLibraryFilesCommandLine(), pathStyle);
            string objectFiles         = BuildTask.LocalizePath(GetObjectFilesCommandLine(), pathStyle);

			//Console.WriteLine(" options: " +options);
			
			// Build the commandline by replacing the items in the template with their actual values
            StringBuilder commandLineBuilder = new StringBuilder(commandLineTemplate);
            commandLineBuilder.Replace(OptionsVariable, options);
			
			//BEWARE libraryDirectories contains a platform specific command line switch as well!
			//Also note there's no need to force these library directories to lowercase, since VS.net doesn't have
			//problems with capitalized folder names in this specific case when executing nant on a makefile project.
			commandLineBuilder.Replace(LibraryDirectoriesVariable, libraryDirectories);
//			Console.WriteLine(" libdir: " +libraryDirectories);
//			Console.WriteLine(" libfiles: " +libraryFiles);

			if (_forceLowerCaseFilePaths)
			{
				Debug.Assert(libraryFiles!=null, "\n null libraryFiles in LinkTask::GetCommandLine!");
				// force the path to lower case to get around any case sensitivity issues with file paths
				commandLineBuilder.Replace(LibraryFilesVariable, libraryFiles.ToLower());
			}
			else
			{
				commandLineBuilder.Replace(LibraryFilesVariable, libraryFiles);
			}
            
			commandLineBuilder.Replace(ObjectFilesVariable, objectFiles);
            

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
				}
				else
				{
					if (outputDir == string.Empty)
					{
						// If path is empty, we don't want the path seperator anymore or else
						// the file will be created at the root of the drive
						commandLineBuilder.Replace(OutputDirectoryVariable + @"\", outputDir);
						commandLineBuilder.Replace(OutputDirectoryVariable + "/", outputDir);

						// This is provided as a safty measure where if the first two replace
						// fails, %outputname% will get replaced
						commandLineBuilder.Replace(OutputDirectoryVariable, outputDir);
					} 
					else 
					{
						commandLineBuilder.Replace(OutputDirectoryVariable, outputDir);
					}
				}
			}
			else
			{
				commandLineBuilder.Replace(OutputDirectoryVariable, OutputDir);
			}

			if (OutputName != null && _forceLowerCaseFilePaths)
			{
				// force the path to lower case to get around any case sensitivity issues with file paths
				commandLineBuilder.Replace(OutputNameVariable, OutputName.ToLower());
			}
			else
			{
				commandLineBuilder.Replace(OutputNameVariable, OutputName);
			}

			// expand properties and return string as commandline
            string commandLine = Project.ExpandProperties(commandLineBuilder.ToString());
			return commandLine;
		}

        string GetCommandLineTemplate() {
            string template = GetOption(CommandLineTemplateProperty);
            if (template != null) {
                template = Project.ExpandProperties(template);

                // strip any newlines and extra white space
                StringBuilder trimmedTemplate = new StringBuilder();
                foreach (string token in template.Split(new char[] {' ', '\n'})) {
                    string trimmed = token.Trim();
                    if (trimmed.Length > 0) {
                        trimmedTemplate.Append(trimmed);
                        trimmedTemplate.Append(" ");
                    }
                }
                template = trimmedTemplate.ToString();

            } else {
                // use default command line template
                template = String.Format("{0} {1} {2} {3}",
                    OptionsVariable,
                    LibraryDirectoriesVariable,
                    ObjectFilesVariable,
                    LibraryFilesVariable);
            }
            return template;
        }

        string GetOptionsCommandLine() {
            // add task instance specific options last to override any global options
            StringCollection options = new StringCollection();
            AddDelimitedProperties(options, GetOption(OptionsProperty)); // global options
			AddDelimitedProperties(options, Options.Value);                       // instance options

            StringBuilder commandLine = new StringBuilder();
            foreach (string option in options) {
                commandLine.Append(option);
                commandLine.Append(" ");
            }
			return commandLine.ToString();
        }

        string GetLibraryDirectoriesCommandLine() {
            // add user lib directories first to override any system directories (this is different than options)
            _libraryDirectoryCollection = new StringCollection();
            AddDelimitedProperties(_libraryDirectoryCollection, LibraryPath.Value);
            AddDelimitedProperties(_libraryDirectoryCollection, GetOption(LibraryDirectoriesProperty));

            StringBuilder commandLine = new StringBuilder();
            foreach (string library in _libraryDirectoryCollection)
            {
                AddTemplatedCommandLineArgument(
                    commandLine, 
                    LibraryDirectoryTemplateProperty, 
                    LibraryDirectoryVariable, 
                    library);
				if(UseResponseFile)
				{
					commandLine.Append("\n");
				}
            }
            return commandLine.ToString();
        }

        string GetLibraryFilesCommandLine() {
            StringBuilder commandLine = new StringBuilder();
            
            StringCollection libraryFiles = new StringCollection();
            AddDelimitedProperties(libraryFiles, GetOption(LibrariesProperty));
            foreach (string libraryFileName in libraryFiles) {
                AddTemplatedCommandLineArgument(
                    commandLine, 
                    LibraryFileTemplateProperty, 
                    LibraryFileVariable, 
                    libraryFileName);
				if(UseResponseFile)
				{
					commandLine.Append("\n");
				}
            }

            foreach (FileItem fileItem in Libraries.FileItems.OrderedDistinct(FileItem.PathEqualityComparer)) {
                AddTemplatedCommandLineArgument(
                    commandLine, 
                    LibraryFileTemplateProperty, 
                    LibraryFileVariable, 
                    fileItem.FileName);
				if(UseResponseFile)
				{
					commandLine.Append("\n");
				}
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
            foreach (FileItem fileItem in Objects.FileItems) {

                string objPath = fileItem.FileName;
                if (UseRelativePaths) {
                    objPath = PathNormalizer.MakeRelative(objPath, WorkingDirectory);
                }

				Debug.Assert(objPath!=null, "\n null objPath in GetObjectFilesCommandLine!");

				if (_forceLowerCaseFilePaths)
				{
					// force the path to lower case to get around any case sensitivity issues with file paths
					AddTemplatedCommandLineArgument(
						commandLine, 
						ObjectFileTemplateProperty, 
						ObjectFileVariable, 
						objPath.ToLower());
				}
				else
				{
					AddTemplatedCommandLineArgument(
						commandLine, 
						ObjectFileTemplateProperty, 
						ObjectFileVariable, 
						objPath);
				}
				if(UseResponseFile)
				{
					commandLine.Append("\n");
				}
            }
            return commandLine.ToString();
        }

        void RunLinker(string program, string commandLine, string reason) {
			Log.Status.WriteLine(LogPrefix + OutputName);
			Log.Info.WriteLine(LogPrefix + "Linking {0} files because {1}", Objects.FileItems.Count, reason);
            Log.Info.WriteLine(LogPrefix + "Creating program in {0}", UriFactory.CreateUri(OutputDir).ToString());

            if (UseResponseFile) {
                commandLine = GetResponseFileCommandLine(OutputName, OutputDir, commandLine, null);
            }

            Process process = CreateProcess(program, commandLine);

            if (Log.Level >= Log.LogLevel.Detailed)
            {
                Log.Info.WriteLine(LogPrefix + "{0} {1}", program, commandLine.Trim());
                Log.Info.WriteLine(LogPrefix + "WorkingDirectory=\"{0}\"", process.StartInfo.WorkingDirectory);
                Log.Info.WriteLine(LogPrefix + "Environment=\"{0}\"", process.StartInfo.EnvironmentVariables.Cast<System.Collections.DictionaryEntry>().ToString("; ", de => de.Key + "=" + de.Value));
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
                    throw new BuildException(msg, Location, e);
                } 

                string output = processRunner.GetOutput();

                if (processRunner.ExitCode != 0) 
                {
                    string msg = String.Format("Could not link files(exit code={0}).{1}{2}", processRunner.ExitCode, Environment.NewLine, output);
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

        void LinkObjects() {
            string program = GetOption(ProgramProperty);
            string commandLine = GetCommandLine();

            string [] programExtensions = { "exe", "self", "elf", "dll", "xex", "xbe", "prx", "so", "" };
            string dependencyFilePath = Path.Combine(OutputDir, OutputName + Dependency.DependencyFileExtension);

            string reason;

			string commandLineWithoutNewline = commandLine;
			if (UseResponseFile) 
			{
				commandLineWithoutNewline = commandLine.Replace("\n", "");
			}

            // Check for the overriden outputdir and if exists, use it to do dependency checking instead of the default one.
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

            if (!Dependency.IsUpdateToDate(dependencyFilePath, outputDirOverride, OutputName, program, commandLineWithoutNewline, programExtensions, _libraryDirectoryCollection, toSearchLibraryFiles, out reason)) 
			{
                if (GenerateOutputFile) {
                    RunLinker(program, commandLine, reason);
                    RunPostLinkProgram();
                }

                if (GenerateDependencyFile) {
                    Dependency.GenerateDependencyFile(dependencyFilePath, program, commandLineWithoutNewline, Objects.FileItems.ToStringCollection(), Libraries.FileItems.ToStringCollection());
                }
            }
            else if (IsRunPostlinkAlwaysEnabled())
            {
                RunPostLinkProgram();
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
            if (commandLineTemplate == null) {
                string msg = String.Format("If you define '{0}' you must also define '{1}'.", 
                    PostLinkProgramProperty, PostLinkCommandLineProperty);
                throw new BuildException(msg, Location);
            }

            StringBuilder commandLineBuilder = new StringBuilder();

            // strip any newlines and extra white space
            foreach (string token in commandLineTemplate.Split(new char[] {' ', '\n'})) {
                string trimmed = token.Trim();
                if (trimmed.Length > 0) {
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
			if (OutputName != null && _forceLowerCaseFilePaths)
			{
				// force the path to lower case to get around any case sensitivity issues with file paths
				commandLineBuilder.Replace(OutputNameVariable, BuildTask.LocalizePath(OutputName.ToLower(), GetOption(BuildTask.PathStyleProperty)));
			}
			else
			{
				commandLineBuilder.Replace(OutputNameVariable, BuildTask.LocalizePath(OutputName, GetOption(BuildTask.PathStyleProperty)));
			}

            // expand properties
            string commandLine = Project.ExpandProperties(commandLineBuilder.ToString());

            // localize path based on path style property (smelly)
            commandLine = BuildTask.LocalizePathInCommandLine(commandLine, GetOption(BuildTask.PathStyleProperty));

            return commandLine;
        }

        void RunPostLinkProgram() {
            string program = GetOption(PostLinkProgramProperty);
            if (program != null) {
                string commandLine = GetPostLinkCommandLine();
                string workingDir = GetOption(PostLinkWorkingDirectoryProperty);
                string redirect = GetOption(PostLinkRedirectProperty);
                bool bRedirect = true;
                if (!String.IsNullOrEmpty(redirect))
                    bRedirect = Boolean.Parse(redirect);

                Process process = CreateProcess(program, commandLine);
                if (workingDir != null) {
                    process.StartInfo.WorkingDirectory = workingDir;
                }

                if (Log.Level >= Log.LogLevel.Detailed)
                {
                    Log.Info.WriteLine(LogPrefix + "postlink:");
                    Log.Info.WriteLine(LogPrefix + "{0} {1}", process.StartInfo.FileName,process.StartInfo.Arguments);
                    Log.Info.WriteLine(LogPrefix + "WorkingDirectory=\"{0}\"", process.StartInfo.WorkingDirectory);
                    Log.Info.WriteLine(LogPrefix + "Environment=\"{0}\"", process.StartInfo.EnvironmentVariables.Cast<System.Collections.DictionaryEntry>().ToString("; ", de => de.Key + "=" + de.Value));
                }

                using (ProcessRunner processRunner = new ProcessRunner(process, bRedirect, bRedirect, true))
                {
                    try {
                        Log.Info.WriteLine(LogPrefix + "{0} {1}", program, commandLine);
                        processRunner.Start();
                    } 
                    catch (Exception e) {
                        string msg = String.Format("Error starting post link program '{0}'.", program);
                        throw new BuildException(msg, Location, e);
                    } 

                    if (processRunner.ExitCode != 0) {
                        int indentLevel = Log.IndentLevel;
                        Log.IndentLevel = 0;                    
                        Log.Error.WriteLine(processRunner.StandardError);
                        Log.IndentLevel = indentLevel;

                        string msg = String.Format("Could not run post link program.\n{0}", processRunner.StandardOutput);
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
    }
}
