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
    /// <item>      <term>%responsefile%</term>              <description>Used by the <term>${cc..template.responsefile}</term> property to represent the filename of the response file.</description></item>
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
    ///   <para>Archieve the given object files into the specified library.</para>
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

        // template items user uses in the template properties to represent the actual values
        const string ObjectFileVariable             = "%objectfile%";

        // template items uses users in lib.options property to represent actual values
        const string OutputDirectoryVariable        = "%outputdir%";
        const string OutputNameVariable             = "%outputname%";

        const string CommandLineTemplateProperty = "lib.template.commandline";

        // template items used in the link.template.commandline template
        const string OptionsVariable = "%options%";
        const string ObjectFilesVariable = "%objectfiles%";


		bool			_forceLowerCaseFilePaths= false;
		string          _outputDir;
        string          _outputName;
        FileSet         _objects    = new FileSet();
        PropertyElement _libOptions = new PropertyElement();
        string _libraryExtension = String.Empty;

        public LibTask() : base("lib") {}

        /// <summary>Name of option set to use for compiling options.  If null
            /// given values from the global <c>lib.*</c> propertes.  Default is null.</summary>
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
        public string OutputName { 
            get { return _outputName; } 
            set { _outputName = value; }
        }

        /// <summary>The list of object files to combine into a library.</summary>
        [FileSet("objects")]
        public FileSet Objects { 
            get { return _objects; }
        }

        /// <summary>
        /// Custom program options for this task.  These get appended to the options specified in the <c>lib.options</c> property.
        /// </summary>
        [Property("options")]
        public PropertyElement Options {
            get { return _libOptions; }
        }

        /// <summary>library output extension</summary>
        [TaskAttribute("libextension", Required = false)]
        public string LibraryExtension
        {
            get { return _libraryExtension; }
            set { _libraryExtension = value; }
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
            AddProperty(BuildTask.PathStyleProperty);
            AddProperty(UseResponseFileProperty);
            AddProperty(UseAltSepInResponseFileProperty);
            AddProperty(ResponseFileTemplateProperty);
            AddProperty(UseRelativePathsProperty);
			AddProperty(ForceLowerCaseFilePathsProperty);
            AddProperty(CommandLineTemplateProperty);

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

        void AddTemplatedArgument(StringBuilder arguments, string templatePropertyName, string templateVariableName, string variableValue) {
            string template = GetOption(templatePropertyName);
            string result = template.Replace(templateVariableName, variableValue);
            arguments.Append(result);
            arguments.Append(" ");
        }

        public override string WorkingDirectory  {
            get {
                return OutputDir;
            }
        }

        string GetCommandLineTemplate()
        {
            string template = GetOption(CommandLineTemplateProperty);
            if (template != null)
            {
                template = Project.ExpandProperties(template);

                // strip any newlines and extra white space
                StringBuilder trimmedTemplate = new StringBuilder();
                foreach (string token in template.Split(new char[] { ' ', '\n' }))
                {
                    string trimmed = token.Trim();
                    if (trimmed.Length > 0)
                    {
                        trimmedTemplate.Append(trimmed);
                        trimmedTemplate.Append(" ");
                    }
                }
                template = trimmedTemplate.ToString();

            }
            else
            {
                // use default command line template
                template = String.Format("{0} {1}",
                    OptionsVariable,
                    ObjectFilesVariable);
            }
            return template;
        }


        string GetCommandLine() {
            // NOTE: We do not expand the properties at this point because we still need to add
            // our task properties to the Project.Properties dictionary.  Expanding properties
            // at this point would prevent the user from specifying where the source files and 
            // object file names should appear on the command lone.

            // add user options last to override any system options
            StringCollection options = new StringCollection();
            AddDelimitedProperties(options, GetOption(OptionsProperty));
            AddDelimitedProperties(options, Options.Value);

            // Build the commandline by replacing the items in the template with their actual values
            var commandLineBuilder = new StringBuilder(GetCommandLineTemplate());
            

            commandLineBuilder.Replace(OptionsVariable, options.ToString(" "));

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
                    AddTemplatedArgument(objects, ObjectFileTemplateProperty, ObjectFileVariable, objPath.ToLower());
				}
				else
				{
                    AddTemplatedArgument(objects, ObjectFileTemplateProperty, ObjectFileVariable, objPath);
				}
            }

            commandLineBuilder.Replace(ObjectFilesVariable, objects.ToString());

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
				}
				else
				{
                    commandLineBuilder.Replace(OutputDirectoryVariable, outputDir);
				}
			}
			else
			{
                commandLineBuilder.Replace(OutputDirectoryVariable, OutputDir);
			}

			if (OutputName != null && _forceLowerCaseFilePaths)
			{
				// By forcing the path to lower case, we get around any case sensitivity issues with file paths
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

        // Some archivers (mostly GCC) always add new obj files, without removing files that aren't present anymorehave to be dele
        void CleanOldLib(string outputDir, string outputName, string extension, string cmdline)
        {
            if (!PlatformUtil.IsWindows || cmdline.Contains("-rs ") || Properties["config-compiler"]=="ghs")
            {
                var path = "unknown";
                try
                {
                    path = Path.Combine(outputDir, outputName + extension);
                    
                    if (File.Exists(path))
                    {
                        Log.Info.WriteLine(LogPrefix + "Deleting old library: {0}", path);
                        File.Delete(path);
                    }
                }
                catch (Exception ex)
                {
                    Log.Warning.WriteLine(LogPrefix + "Failed to releate library file '{0}' before recreating library. Reason: {1}", path, ex.Message);
                }

            }
        }

        void RunArchiver(string program, string commandLine, string reason) {
            Log.Status.WriteLine(LogPrefix + OutputName);
            Log.Info.WriteLine(LogPrefix + "Archiving {0} files because {1}", Objects.FileItems.Count, reason);
            Log.Info.WriteLine(LogPrefix + "Creating library in {0}", UriFactory.CreateUri(OutputDir).ToString());
			
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

            using (ProcessRunner processRunner = new ProcessRunner(process)) {
                try {
                    processRunner.Start();
                } 
                catch (Exception e) {
                    string msg = String.Format("Error starting library tool '{0}'.", program);
                    throw new BuildException(msg, Location, e, BuildException.StackTraceType.XmlOnly);
                }

                string output = processRunner.GetOutput();

                if (processRunner.ExitCode != 0) {
                    string msg = String.Format("Could not create library (exit code={0}).{1}{2}", processRunner.ExitCode, Environment.NewLine, output);
                    throw new BuildException(msg, Location, BuildException.StackTraceType.None);
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
            string commandLine = GetCommandLine();

            string[] libraryExtensions = {"lib", "a", "so"};
            string dependencyFilePath = Path.Combine(OutputDir, OutputName + Dependency.DependencyFileExtension);

            string reason;

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

            if (!Dependency.IsUpdateToDate(dependencyFilePath, outputDirOverride, OutputName, program, commandLine, libraryExtensions, null, null, out reason))
            {
                CleanOldLib(outputDirOverride, OutputName, LibraryExtension, commandLine);
                RunArchiver(program, commandLine, reason);
                Dependency.GenerateDependencyFile(dependencyFilePath, program, commandLine, Objects.FileItems.ToStringCollection());
            }
        }
    }
}
