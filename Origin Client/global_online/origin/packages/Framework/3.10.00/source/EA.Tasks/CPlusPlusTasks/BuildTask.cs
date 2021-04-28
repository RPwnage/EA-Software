// Copyright (C) Electronic Arts Canada Inc. 2002.  All rights reserved.

using System;
using System.Collections;
using System.Collections.Specialized;
using System.Diagnostics;
using System.IO;
using System.Reflection;

using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Util;
using NAnt.Core.Logging;

namespace EA.CPlusPlusTasks {

    public enum PathStyle {
        Unix,
        Windows
    }

    /// <summary>Builds C++ source files into a program or library.</summary>
    /// <remarks>
    ///   <para>
    ///   The <c>build</c> task combines the <see cref="CcTask"/>, <see cref="AsTask"/>, 
    ///   <see cref="LibTask"/>, and <see cref="LinkTask"/>
    ///   to provide an easy way to build a program or library.  The 
    ///   <c>build</c> task expects
    ///   the compiler, linker, and librarian properties to be set
    ///   correctly before it is invoked.
    ///   </para>
	///   <para>All the C++ tasks refer to the <c>build.pathstyle</c> property to deterine how 
	///   format filename paths.  For Unix ('/') based SDK's this property should be set to <c>Unix</c>.
	///   For Windows ('\') based SDK's this property should be set to <c>Windows</c>.  If the
	///   property is not set no path conversions will take place.</para>
	///   <para>
	///   To compile individual files use the <c>build.sourcefile.X</c> property where <c>X</c> is a unique identifier. 
	///   This will let all subsequent <c>build</c> tasks know to only build these files from the given <c>build</c> tasks <c>sources</c> file set. 
	///   If any <c>build.sourcefile.X</c> properties are defined no linking or archiving will occur. 
	///   You may pass these values in through the command line using the <c>-D</c> argument for defining properties.
	///   For example the command <c>nant -D:build.sourcefile.0=src/test.c</c> will let all <c>build</c> tasks know to only compile the source file <c>src/test.c</c>.
	///   </para>
    ///   <para>If the <c>outputdir</c> attribute is not specified the task will look for a property called 
    ///   <c>build.outputdir</c>.  The value of the property may contain the term <c>%outputname%</c> which 
    ///   will be replaced with value specified in the <c>name</c> attribute.  If the <c>build.outputdir</c>
    ///   property is not defined and you are building a package the output directory will be either 1)
    ///   <c>${package.builddir}/build/${package.config}/%outputname%</c> for a Framework 1 package, or
    ///   2) <c>${package.builddir}/${package.config}/build/%outputname%</c> for a Framework 2 package.  
    ///   If you are not building a package then
    ///   the output directory will be the current directory.
    ///   </para>
    ///   <para><b>NOTE: </b> Empty outputdir will be treated as not specifying outputdir. If outputdir is specified,
    ///   absolute path is recommended, such as ${build.dir}/${config}. Relative path such as "." or "temp" may have 
    ///   unexpected result.
	///   </para>
	///   <para>This task can use a named option set for specifying options for unique components.  For more information see the Fundamentals->Option Sets topic in help file.</para>
	/// </remarks>
	/// <include file='Examples/Build/HelloWorld.example' path='example'/>
	/// <include file='Examples/Build/BuildFileSet.example' path='example'/>
    [TaskName("build")]
    public class BuildTask : Task 
    {
        public BuildTask() : base("build") { }

        // global property used to determine how filename path's should be converted (see PathStyle enum for details)
        public static readonly string PathStyleProperty = "build.pathstyle";

        // special build types to maintain backwards compatiblity with Framework version < 0.7.0
        const string DefaultProgramOptionSetName = "Program";
        const string DefaultLibraryOptionSetName = "Library";
        const string SourceFilePropertyNamePrefix = "build.sourcefile";

		public static StringCollection GetIndividualSourceFiles(Project project) { 
			// check for single file compiles
			StringCollection _individualSourceFiles = new StringCollection();
            foreach (Property property in project.Properties)
            {				
				if (property.Prefix == SourceFilePropertyNamePrefix) {
					_individualSourceFiles.Add(property.Value.Trim());
				}
			}
            string value = project.Properties[SourceFilePropertyNamePrefix];
            if (value != null) {
                _individualSourceFiles.Add(value.Trim());
            }

			return _individualSourceFiles;
		}

		public static bool CheckForIndividualFileCompile(Project project) {
			return (BuildTask.GetIndividualSourceFiles(project).Count > 0);
		}

		/// <summary>Converts a path to the current style specified in the PathStyleProperty property.</summary>
        /// <remarks>If pathStyleValue is null or empty then the path will not be converted.</remarks>
        /// <param name="path">The path to localize.</param>
        /// <param name="pathStyleValue">The value of the PathStyleProperty property (build.pathstyle).</param>
        /// <returns>The localized path.</returns>
        public static string LocalizePath(string path, string pathStyleValue)
        {
            if (!String.IsNullOrEmpty(pathStyleValue))
            {
                PathStyle style = PathStyle.Unix;
                try
                {
                    style = ConvertUtil.ToEnum<PathStyle>(pathStyleValue);
                }
                catch (Exception ex)
                {
                    string msg = String.Format("Property '{0}' : {1}.", PathStyleProperty, ex.Message);
                    throw new BuildException(msg);
                }

                if (style == PathStyle.Unix)
                {
                    path = path.Replace(@"\", @"/");

                }
                else if (style == PathStyle.Windows)
                {
                    path = path.Replace(@"/", @"\");
                }
            }

            return path;
        }

        /// <summary>Converts a path inside command line to the current style specified in the PathStyleProperty property. Leaves options starting with "/" unchanged</summary>
        /// <remarks>If pathStyleValue is null or empty then the path will not be converted.</remarks>
        /// <param name="path">The path to localize.</param>
        /// <param name="pathStyleValue">The value of the PathStyleProperty property (build.pathstyle).</param>
        /// <returns>The localized path.</returns>
        public static string LocalizePathInCommandLine(string path, string pathStyleValue)
        {
            if (pathStyleValue != null && pathStyleValue.Length > 0)
            {
                PathStyle style = PathStyle.Unix;
                try
                {
                    style = (PathStyle)Enum.Parse(style.GetType(), pathStyleValue);
                }
                catch
                {
                    string msg = String.Format("Invalid value '{0}' for property '{1}'.", pathStyleValue, PathStyleProperty);
                    throw new BuildException(msg);
                }

                if (style == PathStyle.Unix)
                {
                    path = path.Replace(@"\", @"/");

                }
                else if (style == PathStyle.Windows)
                {                    
                    path = System.Text.RegularExpressions.Regex.Replace(path, "(?<=[^ ])(/)", @"\");
                }
            }

            return path;
        }


        string      _buildDirectory = null;
        string      _buildName;
        string      _buildType;

        CcTask      _complierTask = new CcTask();
        AsTask      _assemblerTask = new AsTask();
        LinkTask    _linkTask = new LinkTask();
        LibTask     _libTask = new LibTask();

        FileSet     _objects = new FileSet();
		FileSet     _dependencies = new FileSet();

        string _primaryOutputExtension = String.Empty;

        /// <summary>Directory where all output files are placed.  See task details for more information if this attribute is not specified.</summary>
        [TaskAttribute("outputdir")]
        public string BuildDirectory { 
            get { return _buildDirectory; } 
            set 
			{ 
				_buildDirectory = value; 
				// Treat "" as null, so that InitializeTask() can get proper value
				if (_buildDirectory == "")
					_buildDirectory = null;
			}
        }

        /// <summary>The base name of any associated output files.  The appropiate extension will be added.</summary>
        [TaskAttribute("name", Required=true)]
        public string BuildName { 
            get { return _buildName; } 
            set { _buildName = value; }
        }

        /// <summary>Type of the <c>build</c> task output.  Valid values are <c>Program</c>, <c>Library</c> or the name of an named option set.</summary>
        [TaskAttribute("type", Required=true)]
        public string OptionSetName { 
            get { return _buildType; } 
            set { _buildType = value; }
        }

        /// <summary>The set of source files to compile.</summary>
        [FileSet("sources")]
        public FileSet Sources { 
            get { return _complierTask.CcSources; }
        }

        /// <summary>Additional object files to link or archive with.</summary>
        [FileSet("objects")]
        public FileSet Objects { 
            get { return _objects; }
        }

        /// <summary>Custom include directories.  New line <c>'\n'</c> or semicolon <c>';'</c> delimited.</summary>
        [Property("includedirs")]
        public PropertyElement IncludeDirectories {
            get { return _complierTask.CcIncludeDirectories; }
        }

		/// <summary>Custom using directories.  New line <c>'\n'</c> or semicolon <c>';'</c> delimited.</summary>
		[Property("usingdirs")]
		public PropertyElement UsingDirectories 
		{
			get { return _complierTask.CcUsingDirectories; }
		}

        /// <summary>Custom compiler options for these files.</summary>
        [Property("ccoptions")]
        public PropertyElement CompilerOptions {
            get { return _complierTask.CcOptions; }
        }

        /// <summary>Custom compiler defines.</summary>
        [Property("defines")]
        public PropertyElement Defines {
            get { return _complierTask.CcDefines; }
        }

		/// <summary>The set of source files to assemble.</summary>
        [FileSet("asmsources")]
        public FileSet AsmSources {
			get { return _assemblerTask.AsSources; }
		}

		/// <summary>Custom include directories for these files.  New line <c> '\n'</c> or semicolon <c>';'</c> delimited.</summary>
		[Property("asmincludedirs")]
		public PropertyElement AsmIncludePath {
			get { return _assemblerTask.AsIncludeDirectories; }
		}

		/// <summary>Custom assembler options for these files.</summary>
		[Property("asmoptions")]
		public PropertyElement AsmOptions {
			get { return _assemblerTask.AsOptions; }
		}

		/// <summary>Custom assembler defines for these files.</summary>
		[Property("asmdefines")]
		public PropertyElement AsmDefines {
			get { return _assemblerTask.AsDefines; }
		}

        /// <summary>The set of libraries to link with.</summary>
        [FileSet("libraries")]
        public FileSet Libraries { 
            get { return _linkTask.Libraries; }
        }

		/// <summary>Additional file dependencies of a build</summary>
		[FileSet("dependencies")]
		public FileSet Dependencies
		{
			get { return _dependencies; }
		}

        /// <summary>primary output extension</summary>
        [TaskAttribute("outputextension", Required = false)]
        public string PrimaryOutputExtension
        {
            get { return _primaryOutputExtension; }
            set { _primaryOutputExtension = value; }
        }


        ///<summary>Initializes task and ensures the supplied attributes are valid.</summary>
        ///<param name="taskNode">Xml node used to define this task instance.</param>
        protected override void InitializeTask(System.Xml.XmlNode taskNode) {
			if (BuildDirectory == null && Project.Properties["build.outputdir"] != null) {
                BuildDirectory = Project.Properties["build.outputdir"].Replace("%outputname%", BuildName);
            }

            if (BuildDirectory == null && Project.Properties["package.dir"] != null) 
			{
				if (Project.Properties["package.frameworkversion"] == "2")
				{
					BuildDirectory = Project.ExpandProperties("${package.builddir}/${config}/build/" + BuildName);
				}
				else
				{
					BuildDirectory = Project.ExpandProperties("${package.builddir}/build/${config}/" + BuildName);
				}
            }

            // if not outputdir specified (and we are not building a package) use the project directory 
            if (BuildDirectory == null) {
                BuildDirectory = Project.BaseDirectory;
            }

            // If build task set to verbose mark our component tasks as verbose too
            if (Verbose) {
                _complierTask.Verbose = Verbose;
                _assemblerTask.Verbose = Verbose;
                _linkTask.Verbose = Verbose;
                _libTask.Verbose = Verbose;
            }
        }

        protected override void ExecuteTask() {
            //Log.Info.WriteLine(LogPrefix + "{0} ({1})", BuildName, OptionSetName);

            Project.Properties["build.name"] = BuildName;

            OptionSet optionSet = Project.NamedOptionSets.GetOrAdd(OptionSetName, (key) =>
            {
                optionSet = new OptionSet();

                // special case option set names Library and Program to maintain backwards compatibility
                if (key == DefaultProgramOptionSetName)
                {
                    optionSet.Options["build.tasks"] = "asm cc link";
                }
                else if (OptionSetName == DefaultLibraryOptionSetName)
                {
                    optionSet.Options["build.tasks"] = "asm cc lib";
                }
                else
                {
                    string msg = String.Format("Cannot find named option set '{0}'.", key);
                    throw new BuildException(msg, Location);
                }
                return optionSet;
            });

			CheckForRequiredProperties(optionSet);

			string[] buildTasks = optionSet.Options["build.tasks"].Split();
            foreach (string task in buildTasks) {
                switch (task) {
                    case "asm":
                        ExecuteAsTask(optionSet);
                        break;

                    case "cc":
                        ExecuteCcTask(optionSet);
                        break;

					case "link":
						ExecuteLinkTask(optionSet);
                        break;

                    case "lib":
						ExecuteLibTask(optionSet);
                        break;

                    case "": // String.Split() doesn't handle whitespace well.
                        break;

                    default:
                        ExecuteCustomTask(task, optionSet);
                        break;
                }
            }
        }

		void CheckForRequiredProperties(OptionSet optionSet) {
			if (optionSet.Options["build.tasks"] == null) {
				string message = String.Format("Option '{0}' is not defined in optionset", "build.tasks");
				throw new BuildException(message, Location);
			}
		}


		void ExecuteAsTask(OptionSet optionSet) {
            if (AsmSources.FileItems.Count > 0) {
                _assemblerTask.Project = Project;
                _assemblerTask.OutputDir = BuildDirectory;
                _assemblerTask.OutputName = BuildName;
                _assemblerTask.InitializeFromOptionSet(optionSet);
				_assemblerTask.Dependencies = Dependencies;
                _assemblerTask.Execute();
            }
        }

        void ExecuteCcTask(OptionSet optionSet) {
            if (Sources.FileItems.Count > 0) {
                _complierTask.Project = Project;
                _complierTask.OutputDir  = BuildDirectory;
                _complierTask.OutputName = BuildName;
                _complierTask.InitializeFromOptionSet(optionSet);
				_complierTask.Dependencies = Dependencies;

                _complierTask.Execute();
            }
        }

        void ExecuteLinkTask(OptionSet optionSet) {
            // use the <link> task to link the sources
            _linkTask.Project    = Project;
            _linkTask.OutputDir  = BuildDirectory;
            _linkTask.OutputName = BuildName;
            _linkTask.LinkOutputExtension = PrimaryOutputExtension;
            _linkTask.InitializeFromOptionSet(optionSet);

            // add the object files
            _linkTask.Objects.Append(Objects);
            
            foreach (FileItem fileItem in _complierTask.Sources.FileItems) {
                string objectPath = _complierTask.GetObjectPath(fileItem);
                _linkTask.Objects.Includes.Add(PatternFactory.Instance.CreatePattern(objectPath));
            }

            foreach (FileItem fileItem in _complierTask.PchSources.FileItems) {
                string objectPath = _complierTask.GetObjectPath(fileItem);
                if(File.Exists(objectPath)) // Some compilers do not spit obj when compiling pch (RevolutionCodewarrior).
                {
                _linkTask.Objects.Includes.Add(PatternFactory.Instance.CreatePattern(objectPath));
                }
            }

            foreach (FileItem fileItem in _assemblerTask.Sources.FileItems) {
                string objectPath = _assemblerTask.GetObjectPath(fileItem);
                _linkTask.Objects.Includes.Add(PatternFactory.Instance.CreatePattern(objectPath));
           }

            _linkTask.Execute();
        }

        void ExecuteLibTask(OptionSet optionSet) {
            // use the <lib> task to create a library
            _libTask.Project    = Project;
            _libTask.OutputDir  = BuildDirectory;
            _libTask.OutputName = BuildName;
            _libTask.LibraryExtension = PrimaryOutputExtension;
            _libTask.InitializeFromOptionSet(optionSet);

            // add the object files
            _libTask.Objects.Append(Objects);

            foreach(FileItem fileItem in _complierTask.Sources.FileItems) {
                string objectPath = _complierTask.GetObjectPath(fileItem);
                _libTask.Objects.Includes.Add(PatternFactory.Instance.CreatePattern(objectPath));
            }

            foreach (FileItem fileItem in _assemblerTask.Sources.FileItems) {
                string objectPath = _assemblerTask.GetObjectPath(fileItem);
                _libTask.Objects.Includes.Add(PatternFactory.Instance.CreatePattern(objectPath));
            }

            _libTask.Execute();
        }

        void ExecuteCustomTask(string taskName, OptionSet optionSet) 
        {
            // find a task with the given name
            Task task = Project.TaskFactory.CreateTask(taskName, Project);

            if (task == null)
            {
                string msg = String.Format("Failed to load custom build task: {0}", taskName);
                throw new BuildException(msg, Location);
            }

			// An external build task is either one of the following:
			//  1) A subclass of ExternalBuildTask and not abstract or
			//  2) A subclass of Task, implements interface IExternalBuildTask and not abstract

			Type type = task.GetType();
            if (type.IsSubclassOf(typeof(ExternalBuildTask)) && !type.IsAbstract) 
            {
                ExternalBuildTask externalBuildTask = (ExternalBuildTask)task;
                externalBuildTask.ExecuteBuild(this, optionSet);
            }
			else if (type.IsSubclassOf(typeof(Task)) && 
				type.GetInterface(typeof(IExternalBuildTask).ToString()) != null &&
				!type.IsAbstract) 
			{
				IExternalBuildTask externalBuildTask = (IExternalBuildTask)task;
				externalBuildTask.ExecuteBuild(this, optionSet);
			}
            else
            {
                string msg = String.Format("Custom build tasks must inherit from: {0}", typeof(ExternalBuildTask).ToString());
                throw new BuildException(msg, Location);
            }
        }
    }
}
