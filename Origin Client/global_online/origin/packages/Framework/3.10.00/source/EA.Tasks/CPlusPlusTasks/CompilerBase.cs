// (c) 2002-2005 Electronic Arts Inc.

using System;
using System.Collections;
using System.Collections.Specialized;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Reflection;
using System.Text;
using System.Threading;
using System.Xml;
using System.Linq;
using RE = System.Text.RegularExpressions;

using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Util;
using NAnt.Core.Logging;

using DG = EA.DependencyGenerator;

namespace EA.CPlusPlusTasks {

    

    public abstract class CompilerBase : BuildTaskBase {

        /// <summary>Used to map dependent header file paths to their last write time.</summary>
        class DependentDictionary : DictionaryBase {
            public virtual object this[string name] {
                get { return Dictionary[name]; }
                set { Dictionary[name] = value; }
            }
        }

        /// <summary>Used to mManage the thread pool when compiling files.</summary>
        class ThreadCollection : ArrayList {
        }

        /// <summary>Used to store dependencies information for FileItem.</summary>
        class FileToCompile
        {
            FileItem _fileItem;
            StringCollection _dependencies = new StringCollection();

            public FileToCompile(FileItem fileItem, FileSet dependencies)
            {
                _fileItem = fileItem;

                foreach (FileItem tmp in dependencies.FileItems) 
                {
                    _dependencies.Add(tmp.FileName);
                }
            }

            public FileItem SourceFile
            {
                get { return _fileItem; }
            }

            public StringCollection Dependencies
            {
                get { return _dependencies; }
            }
        }

        class FileToCompileList : ArrayList 
        {
            public FileToCompileList() : base() 
            {
            }

            public FileToCompileList(int capacity) : base(capacity) 
            {
            }
        }

        // Used to make sure we don't run these programs at the same time across multiple threads for a single module.
        private Mutex _compilerMutex = new Mutex();
        // Used to throtle number of threads across modules.
        private static ConcurrentDictionary<string, SemaphoreSlim> _compilerSemaphore = new ConcurrentDictionary<string, SemaphoreSlim>();

        // properties used by task
        protected string ProgramProperty                   { get { return Name;								} }
        protected string OptionsProperty                   { get { return Name + ".options";				} }
        protected string DefinesProperty                   { get { return Name + ".defines";				} }
        protected string GccDefinesProperty                { get { return Name + ".gccdefines";				} }
        protected string IncludeDirectoriesProperty        { get { return Name + ".includedirs";			} }
        protected string UsingDirectoriesProperty          { get { return Name + ".usingdirs";			    } }
        protected string NoDependencyProperty              { get { return Name + ".nodep";					} }
        protected string DependencyWarningsProperty		   { get { return Name + ".depwarnings";            } }
        protected string NoCompileProperty                 { get { return Name + ".nocompile";				} }
        protected string ThreadCountProperty               { get { return Name + ".threadcount";			} }
        protected string IncludeDirectoryTemplateProperty  { get { return Name + ".template.includedir";	} }
        protected string UsingDirectoryTemplateProperty    { get { return Name + ".template.usingdir";	    } }
        protected string CommandLineTemplateProperty       { get { return Name + ".template.commandline";	} }
        protected string DefineTemplateProperty            { get { return Name + ".template.define";		} }
        protected string SourceFileTemplateProperty        { get { return Name + ".template.sourcefile";	} }
        protected string ObjectFileExtensionProperty       { get { return Name + ".objfile.extension";	    } }
        protected string ParallelCompilerProperty          { get { return Name + ".parallelcompiler";		} }
        protected string CompilerInternalDefines           { get { return Name + ".compilerinternaldefines";} }

        // used in cc.template properties as placeholders for actual values passed into via task
        const string IncludeDirectoryVariable       = "%includedir%";   // used in IncludeDirectoryTemplateProperty
        const string UsingDirectoryVariable         = "%usingdir%";     // used in UsingDirectoryTemplateProperty
        const string DefineVariable                 = "%define%";       // used in DefineTemplateProperty
        const string SourceFileVariable             = "%sourcefile%";   // used in SourceFileTemplateProperty
        const string ObjectFileVariable             = "%objectfile%";   // used in OptionsProperty
        const string OutputDirectoryVariable        = "%outputdir%";    // used in OptionsProperty
        const string OutputNameVariable             = "%outputname%";   // used in OptionsProperty

        const string DefinesVariable                = "%defines%";
        const string IncludeDirectoriesVariable     = "%includedirs%";
        const string UsingDirectoriesVariable       = "%usingdirs%";
        const string OptionsVariable                = "%options%";

        // task defaults
        const int    DefaultThreadCount         = 1;
        const string DefaultSourceFileTemplate  = "\"%sourcefile%\"";
        const string DefaultCommandLineTemplate = 
            DefinesVariable + " " + 
            IncludeDirectoriesVariable + " " + 
            UsingDirectoriesVariable + " " + 
            OptionsVariable + " ";

        // file extensions
        public const string DefaultObjectFileExtension       = ".obj";

        bool				_forceLowerCaseFilePaths= false;
        string              _outputDir;
        string              _outputName;
        FileSet             _sources                = new FileSet();
        FileSet             _pch_sources            = new FileSet();
        PropertyElement     _optionsProperty        = new PropertyElement();
        PropertyElement     _definesProperty        = new PropertyElement();
        PropertyElement     _includePathProperty    = new PropertyElement();
        PropertyElement     _usingPathProperty      = new PropertyElement();
        DependentDictionary _dependentMap           = new DependentDictionary();
        FileSet             _dependencies           = new FileSet();
        
        public CompilerBase(string name) : base(name) {
            _optionsProperty.Value      = "";
            _definesProperty.Value      = "";
            _includePathProperty.Value  = "";
            _usingPathProperty.Value    = "";
        }

        DependentDictionary DependentMap {
            get { return _dependentMap; }
        }

        /// <summary>Name of option set to use for compiling options.  If null
        /// given values from the global <c>cc.*</c> propertes.  Default is null.</summary>
        [TaskAttribute("type", Required=false)]
        public override string OptionSetName { 
            get { return _optionSetName; } 
            set { _optionSetName = value; }
        }

        /// <summary>Directory where all output files are placed.  Default is the base directory 
        /// of the build file.</summary>
        [TaskAttribute("outputdir")]
        public string OutputDir { 
            get { return _outputDir; } 
            set { _outputDir = PathNormalizer.Normalize(value, false); }
        }

        /// <summary>The base name of any associated output files (e.g. the symbol table name).  
        /// Default is an empty string.</summary>
        [TaskAttribute("outputname")]
        public string OutputName { 
            get { return _outputName; } 
            set { _outputName = value; }
        }

        public FileSet Sources { 
            get { return _sources; }
        }

        public FileSet PchSources
        {
            get { return _pch_sources; }
        }


        public FileSet Dependencies 
         {
            get { return _dependencies; } 
            set { _dependencies = value; }
        }

        protected PropertyElement IncludeDirectories {
            get { return _includePathProperty; }
        }

        protected PropertyElement UsingDirectories 
        {
            get { return _usingPathProperty; }
        }

        protected PropertyElement Options 
        {
            get { return _optionsProperty; }
        }

        protected PropertyElement Defines {
            get { return _definesProperty; }
        }

        /// <summary>Determines if compiler output should always be shown (true) or only shown if the
        /// compiler returns an error code (false).  Default is false.  (Depreciated)</summary>
        protected virtual bool ShowAllOutput {
            get { return false; }
        }

        ///<summary>Initializes task and ensures the supplied attributes are valid.  Hack so that 
        ///the build task can initialize this task.</summary>
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
        /// Initialize the task first with the default global options in the cc.*
        /// properties and then if from the options in the <c>specialOptions</c>.
        /// </summary>
        /// <param name="specialOptions">Special options for this task instance
        /// that will override any global options.</param>
        public override void InitializeFromOptionSet(OptionSet specialOptions) {
            // set initial option set values from global properties
            AddProperty(ProgramProperty);
            AddProperty(OptionsProperty);
            AddProperty(CommandLineTemplateProperty);
            AddProperty(DefinesProperty);
            AddProperty(GccDefinesProperty);
            AddProperty(IncludeDirectoriesProperty);
            AddProperty(IncludeDirectoryTemplateProperty);
            AddProperty(UsingDirectoriesProperty);
            AddProperty(UsingDirectoryTemplateProperty);
            AddProperty(DefineTemplateProperty);
            AddProperty(SourceFileTemplateProperty);
            AddProperty(BuildTask.PathStyleProperty);
            AddProperty(NoDependencyProperty);
            AddProperty(NoCompileProperty);
            AddProperty(ThreadCountProperty);
            AddProperty(DependencyWarningsProperty);
            AddProperty(ObjectFileExtensionProperty);
            AddProperty(UseResponseFileProperty);
            AddProperty(UseAltSepInResponseFileProperty);
            AddProperty(ResponseFileTemplateProperty);
            AddProperty(UseRelativePathsProperty);
            AddProperty(WorkingdirTemplateProperty);
            AddProperty(ForceLowerCaseFilePathsProperty);
            AddProperty(ParallelCompilerProperty);
            AddProperty(CompilerInternalDefines);

            //after adding ForceLowerCaseFilePathsProperty we can set a flag, to avoid redundant calls to ForceLowerCaseFilePaths
            _forceLowerCaseFilePaths= ForceLowerCaseFilePaths;

            // merge any options passed via a named option set
            _optionSet.Append(specialOptions);

            CheckForRequiredProperties(_optionSet);
        }

        void CheckForRequiredProperties(OptionSet optionSet) {
            string[] requiredProperties = {
                ProgramProperty, 
                IncludeDirectoryTemplateProperty,
            };

            foreach(string property in requiredProperties) {
                if (optionSet.Options[property] == null) {
                    string message = String.Format("Program property '{0}' is not defined.", property);
                    throw new BuildException(message, Location);
                }
            }
        }

        /// <summary>True if we should generate a .dep file for each source file, otherwise false.</summary>
        bool IsGenerateDependencyOn(FileItem fileItem) {
            string v = GetOption(fileItem, NoDependencyProperty);
            if (v != null && String.Compare(v, "true", true) == 0) {
                return false;
            } else {
                return true;
            }
        }

        /// <summary>True if we should generate an object file for each source file, otherwise false.</summary>
        bool IsGenerateObjectOn(FileItem fileItem) {
            string v = GetOption(fileItem, NoCompileProperty);
            if (v != null && String.Compare(v, "true", true) == 0) {
                return false;
            } else {
                return true;
            }
        }

        void ProcessFile(FileToCompile fileToCompile, DG.IDependencyGenerator dependencyGenerator)
        {
            int exitcode = 0;
            BufferedLog bufferedLogger = new BufferedLog(Project.Log);
            
            try {
                string objectPath = GetObjectPath(fileToCompile.SourceFile);
                if ( fileToCompile.SourceFile.FullPath.Length > 260 || objectPath.Length > 260 )
                {
                    string message = String.Format("The object path of {0} is too long. ", Path.GetFileName(fileToCompile.SourceFile.FileName));
                    throw new BuildException(message, Location);
                }
                if (Log.Level >= Log.LogLevel.Detailed)
                {

                    string reason = String.Empty;
                    EA.Eaconfig.Core.FileData filedata = fileToCompile.SourceFile.Data as EA.Eaconfig.Core.FileData;
                    if (filedata != null)
                    {
                        reason = filedata.Data as string;
                    }
                    else
                    {
                        reason = (string)fileToCompile.SourceFile.Data;
                    }


                    bufferedLogger.Info.WriteLine(LogPrefix + "Compiling because {0}.", reason);
                    bufferedLogger.Info.WriteLine(LogPrefix + "Creating object file at {0}.", objectPath);
                }

                if (IsGenerateObjectOn(fileToCompile.SourceFile)) 
                {
                    exitcode = RunCompiler(fileToCompile.SourceFile, bufferedLogger);
                }

                if ((exitcode == 0) && IsGenerateDependencyOn(fileToCompile.SourceFile)) {
                    exitcode = RunDependencyGenerator(fileToCompile, bufferedLogger, dependencyGenerator);
                }

            } catch (Exception e) {
                bufferedLogger.Error.WriteLine();
                bufferedLogger.Error.WriteLine(e.Message);
                bufferedLogger.Error.WriteLine(e.StackTrace.ToString());
                bufferedLogger.Error.WriteLine(e.ToString());
                exitcode = 1;
            } finally {
                Report(fileToCompile.SourceFile, bufferedLogger);
            }

            if (exitcode != 0) {
                Interlocked.Increment(ref _errorCount);
            }
        }


        // Shared resources between worker threads
        Queue _jobQueue = null; // contains the list of files that still needs to be procseed
        int _errorCount = 0;    // number of files that returned errors during compile

        public int ErrorCount {
            get { return _errorCount; }
            set { _errorCount = value; }
        }

        void Report(FileItem fileItem, BufferedLog bufferedLogger) 
        {            
            Log.Status.WriteLine("{0}{1}{2}", LogPrefix, GetRelativeSourcePath(fileItem), bufferedLogger.ToString());            
        }

        FileToCompile GetNextSourceFile() {
            FileToCompile item = null;
            lock (_jobQueue) {
                if (_jobQueue.Count > 0) {
                    item = (FileToCompile) _jobQueue.Dequeue();
                }
            }
            return item;
        }

        void WorkerThread() 
        {
            try
            {
                // Create one instance of a dependency generator per thread, so that we can run it in parallel.
                using (DG.IDependencyGenerator dependencyGenerator = DG.DependencyGeneratorFactory.Create())
                {
                    FileToCompile fileItem = GetNextSourceFile();
                    while (fileItem != null)
                    {
                        ProcessFile(fileItem, dependencyGenerator);
                        fileItem = GetNextSourceFile();
                    }
                }
            } catch (Exception e) {
                Log.Error.WriteLine("{0}{1}{2}", e.Message, Environment.NewLine, e.StackTrace.ToString());
                Interlocked.Increment(ref _errorCount);
            } 
        }

        int GetThreadCount() {
            // Create default number of threads for every CPU in order to keep the CPU's working 100%.
            // get cpu count
            int cpuCount;
            try {
                cpuCount = Environment.ProcessorCount;
            } catch {
                cpuCount = 1;
            }

            int baseThreadCount = DefaultThreadCount;
            try {
                string threadCount = GetOption(ThreadCountProperty);
                if (threadCount != null) {
                    baseThreadCount = Convert.ToInt32(threadCount);
                    if (baseThreadCount <= 0) {
                        string msg = String.Format("Invalid thread count value '{0}'.", baseThreadCount);
                        throw new Exception(msg);
                    }
                }
                else {
                    baseThreadCount *= cpuCount;
                    baseThreadCount++;
                }
            }
            catch (Exception e) {
                throw new BuildException(e.Message, Location, e);
            }


            return baseThreadCount;
        }

        protected override void ExecuteTask() {
            _errorCount = 0;

            // Process precompiled header files first;
            _jobQueue = new Queue(GetChangedPrecompiledFiles());

            if (_jobQueue.Count > 0)
            {
                WorkerThread();
            }

            // add changed souces to the jobQueue
            _jobQueue = new Queue(GetChangedSourceFiles());

            // don't bother creating threads if nothing has changed
            if (_jobQueue.Count > 0) {
                int threadCount = GetThreadCount();
                if (threadCount > 1) {
                    Log.Info.WriteLine(LogPrefix + "Using {0} threads", threadCount);
                }
                _compilerSemaphore.AddOrUpdate(Properties["config-platform"] ?? "default", key => new SemaphoreSlim(Math.Max(threadCount, 1)), (key, sem) => sem);

                ThreadCollection workers = new ThreadCollection();
                ThreadPriority priority = threadCount > 1 ? ThreadPriority.BelowNormal : ThreadPriority.Normal;
                for (int i = 0; i < threadCount; i++) {
                    Thread worker = new Thread(new ThreadStart(WorkerThread));
                    worker.Priority = priority;
                    workers.Add(worker);
                }

                // now start all the threads
                foreach (Thread worker in workers) {
                    ThreadRunner.Instance.ThreadStart(worker);
                }

                // now wait for all the worker threads to finish
                foreach (Thread worker in workers) {
                    worker.Join();
                    ThreadRunner.Instance.ThreadDispose(worker);
                }

                if (_errorCount > 0) {
                    string message = String.Format("Could not process {0} files.", _errorCount);
                    throw new BuildException(message, Location, BuildException.StackTraceType.None);
                }
            }

            // detroy the job queue
            _jobQueue = null;
        }

        FileToCompileList GetChangedPrecompiledFiles()
        {
            FileToCompileList changedSources = new FileToCompileList();
            
            // reset the depedent map
            DependentMap.Clear();

            // get the list of individual source files to compile if any
            StringCollection individualFiles = BuildTask.GetIndividualSourceFiles(Project);

            FileSet newSources = new FileSet(Sources);
            newSources.Clear();
            bool hasPch = false;

            foreach (FileItem fileItem in Sources.FileItems)
            {
                string createPch = GetOption(fileItem, "create-pch");
                
                if (!String.IsNullOrEmpty(createPch) && 0 == String.Compare(createPch, "on", true))
                {
                    hasPch = true;
                    // First check for individual file compiles
                    if (individualFiles.Count > 0)
                    {
                        bool b = false;
                        foreach (string singleFile in individualFiles)
                        {
                            string normalizedFile = PathNormalizer.Normalize(singleFile);
                            if (fileItem.FileName == singleFile || fileItem.FileName == normalizedFile)
                            {
                                b = true;
                                break;
                            }
                        }
                        if (b == false)
                            continue;
                    }

                    string reason = "";
                    if (individualFiles.Count > 0 || NeedsCompiling(fileItem, out reason))
                    {
                        if (Log.Level >= Log.LogLevel.Detailed)
                        {
                            if (individualFiles.Count > 0 && reason.Length == 0)
                                reason = "file was individualy selected.";

                            EA.Eaconfig.Core.FileData filedata = fileItem.Data as EA.Eaconfig.Core.FileData;
                            if (filedata != null)
                            {
                                filedata.Data = reason;
                            }
                            else
                            {
                                fileItem.Data = reason;
                            }
                        }

                        changedSources.Add(new FileToCompile(fileItem, Dependencies));
                    }
                    _pch_sources.Includes.Add(PatternFactory.Instance.CreatePattern(fileItem.FileName));
                }
                else
                {
                    newSources.Include(fileItem, failonmissing : false);
                }
            }
            if (hasPch)
            {
                _sources = newSources;
            }
            return changedSources;
        }


        FileToCompileList GetChangedSourceFiles() {
            FileToCompileList changedSources = new FileToCompileList();
            
            // reset the depedent map
            DependentMap.Clear();

            // get the list of individual source files to compile if any
            StringCollection individualFiles = BuildTask.GetIndividualSourceFiles(Project);

            foreach (FileItem fileItem in Sources.FileItems) {
                
                // First check for individual file compiles
                if (individualFiles.Count > 0) {
                    bool b = false;
                    foreach(string singleFile in individualFiles) {
                        string normalizedFile = PathNormalizer.Normalize(singleFile);
                        if (fileItem.FileName == singleFile || fileItem.FileName == normalizedFile) {
                            b = true;
                            break;
                        }
                    }
                    if (b == false)
                        continue;
                }
                
                string reason = "";
                if (individualFiles.Count > 0 || NeedsCompiling(fileItem, out reason)) 
                {
                    if (Log.Level >= Log.LogLevel.Detailed)
                    {
                        if (individualFiles.Count > 0 && reason.Length == 0)
                            reason = "file was individualy selected.";

                        EA.Eaconfig.Core.FileData filedata = fileItem.Data as EA.Eaconfig.Core.FileData;
                        if (filedata != null)
                        {
                            filedata.Data = reason;
                        }
                        else
                        {
                            fileItem.Data = reason;
                        }
                    }

                    changedSources.Add(new FileToCompile(fileItem, Dependencies));
                }
            }
            return changedSources;
        }

        public string GetObjectFileExtension(FileItem fileItem) {
            string ext = GetOption(fileItem, ObjectFileExtensionProperty);
            if (ext == null) {
                ext = DefaultObjectFileExtension;
            }
            return ext;
        }

        public virtual string GetObjectPath(FileItem fileItem)
        {
            string objPath;

            EA.Eaconfig.Core.FileData filedata = fileItem.Data as EA.Eaconfig.Core.FileData;

            if (filedata != null && filedata.ObjectFile != null && !String.IsNullOrEmpty(filedata.ObjectFile.Path))
            {
                objPath = filedata.ObjectFile.Path;
            }
            else
            {
                objPath = GetOutputBasePath(fileItem) + GetObjectFileExtension(fileItem);
            }
            return objPath;
        }

        string GetDependencyPath(FileItem fileItem) {
            return GetOutputBasePath(fileItem) + Dependency.DependencyFileExtension;
        }
        string GetPchPath(FileItem fileItem) {
            return GetOption(fileItem, "pch-file"); 
        }
        
        string GetOutputBasePath(FileItem  fileItem) {
            return GetOutputBasePath(fileItem, Project.GetFullPath(OutputDir), fileItem.BaseDirectory??Sources.BaseDirectory);
        }

        public static string GetOutputBasePath(FileItem  fileItem, string outputDir, string baseDir) {
            string basePath = null;

            string outputDirectoryPath = PathNormalizer.Normalize(outputDir);

            string sourceDirectoryPath = PathNormalizer.Normalize(baseDir);
            string sourcePath = fileItem.FullPath;
            if (!String.IsNullOrEmpty(sourceDirectoryPath) && sourcePath.StartsWith(sourceDirectoryPath)) {
                basePath = sourcePath.Replace(sourceDirectoryPath, outputDirectoryPath);
            } 
            else {
                // Because the source path doesn't come from the source 
                // directory it is possible that there could be multiple 
                // sources with the same base file name.  A simple replace 
                // .cpp with .obj would result in duplicate object names.  
                // Instead we create a suffix that is unqiue to the file's
                // source path to create a unique object path.
                basePath = Path.Combine(outputDirectoryPath, Path.GetFileName(fileItem.FullPath));
                basePath += "." + GetUniqueFileNameSuffix(fileItem.FullPath);
            }
            return basePath;
        }
 
        static string GetUniqueFileNameSuffix(string sourcPath) {
            // Create a MD5 hash of the path and convert the result into a string.  
            System.Security.Cryptography.HashAlgorithm algorithm = 
                new System.Security.Cryptography.MD5CryptoServiceProvider();
            byte[] buffer = System.Text.Encoding.UTF8.GetBytes(sourcPath);

            // Munge the hash into a base64 string then remove the slash (/) 
            // character which is invalid for paths.
            // http://www.freesoft.org/CIE/RFC/1521/7.htm
            string suffix = Convert.ToBase64String(algorithm.ComputeHash(buffer)).Replace("/", String.Empty);
            // we need to remove the '=' as well because CodeWarrior cannot cope with them.
            suffix = suffix.Replace("=", String.Empty);
            // we need to remove the '+' as well because CodeWarrior and Radix both cannot cope with them.
            suffix = suffix.Replace("+", String.Empty);
            return suffix;
        }

        /// <summary>Determines if the source file needs to be compiled.</summary>
        /// <param name="fileItem">The reference to the C/C++ source file.</param>
        /// <param name="reason">The reason why the source file needs compiling.  Empty if no reason given or file doesn't need compiling.</param>
        /// <returns><c>true</c> if the source file needs compiling, otherwise <c>false</c>.</returns>
        bool NeedsCompiling(FileItem fileItem, out string reason) {
            string objectPath = GetObjectPath(fileItem);
            string dependencyPath = GetDependencyPath(fileItem);
            string pchFile = GetPchPath(fileItem);
            if (!String.IsNullOrEmpty(pchFile)) {
                objectPath = pchFile;
            }

            reason = ""; // by default don't give a reason

            if (!File.Exists(objectPath)) {
                reason = String.Format("missing object file");
                return true;

            } else if (!File.Exists(dependencyPath)) {
                reason = String.Format("missing dependency file");
                return true;

            } else {
                // date stamp for object file
                DateTime objectDateStamp = File.GetLastWriteTime(objectPath);
                DateTime sourceDateStamp = File.GetLastWriteTime(fileItem.FullPath);

                if (sourceDateStamp > objectDateStamp) {
                    reason = String.Format("source file is newer than object file. '{0}'", fileItem.FullPath);
                    return true;
                }

                string compiler = GetCompilerProgram(fileItem);
                // create a collection of file names of the files we depend on
                try {
                    StringCollection dependentPaths;
                    string originalCommandLine;
                    string originalCompiler = null;
                    GetDependentInfo(dependencyPath, out dependentPaths, out originalCommandLine, out originalCompiler);
                    if (compiler != originalCompiler)
                    {
                        reason = "compiler changed.";
                        return true;
                    }

                    if (GetCompilerCommandLine(fileItem) != originalCommandLine) {
//						Console.WriteLine(" orig: " +originalCommandLine);
//						Console.WriteLine("  get: " +GetCompilerCommandLine(fileItem));
                        reason = "command line changed.";
                        return true;
                    }

                    foreach (string dependentPath in dependentPaths) {
                        DateTime dependentDateStamp;

                        // DateTime can't be null so do initial compare with object references
                        object dependentMapEntry = DependentMap[dependentPath];

                        if (dependentMapEntry == null) {
                            if (!File.Exists(dependentPath)) {
                                reason = String.Format("missing dependent file '{0}'", dependentPath);
                                return true;
                            }

                            dependentDateStamp = File.GetLastWriteTime(dependentPath);

                            // Add dependent file time stamp to map so we don't have to
                            // call the slow File.GetLastWriteTime method again for this file.
                            DependentMap[dependentPath] = dependentDateStamp;
                        } else {
                            dependentDateStamp = (DateTime) dependentMapEntry;
                        }

                        if (dependentDateStamp > objectDateStamp) {
                            reason = String.Format("dependent file is more recent than object file");
                            return true;
                        }
                    }
                } catch {
                    // something went wrong, when in doubt recompile
                    return true;
                }
            }
            // all checks have passed so the source file does not need compiling
            return false;
        }


        int RunCompiler(FileItem fileItem, BufferedLog bufferedLogger)
        {
            string compiler = GetCompilerProgram(fileItem);
            string commandLine = GetCompilerCommandLine(fileItem);
            string output;
            int exitCode = 1;
            //lock (this) 
            {
                // make sure the directory exists where we are going to create the object file
                string outputDirectory = Path.GetDirectoryName(GetObjectPath(fileItem));
                if (!String.IsNullOrEmpty(outputDirectory) && !Directory.Exists(outputDirectory))
                {
                    Directory.CreateDirectory(outputDirectory);
                }

                if (UseResponseFile)
                {
                    string basePath = GetObjectPath(fileItem);
                    string outputName = Path.GetFileNameWithoutExtension(basePath);
                    string outputDir = Path.GetDirectoryName(basePath);
                    commandLine = GetResponseFileCommandLine(outputName, outputDir, commandLine, bufferedLogger);
                }

                // run the compiler and capture the output and return code
                Process process = CreateProcess(compiler, commandLine);

                if (bufferedLogger.Level >= Log.LogLevel.Detailed)
                {
                    bufferedLogger.Info.WriteLine(LogPrefix + "{0} {1}", compiler, commandLine.Trim());
                    bufferedLogger.Info.WriteLine(LogPrefix + "WorkingDirectory=\"{0}\"", process.StartInfo.WorkingDirectory);
                    bufferedLogger.Info.WriteLine(LogPrefix + "Environment=\"{0}\"", process.StartInfo.EnvironmentVariables.Cast<System.Collections.DictionaryEntry>().ToString("; ", de => de.Key + "=" + de.Value));
                }

                // Run the compiler in parallel if the "parallel compiler" property is set, otherwise
                // force it to run in exclusion.
                string parallelCompiler = GetOption(ParallelCompilerProperty);

                bool parallelCompile = ((parallelCompiler != null) && (Boolean.Parse(parallelCompiler) == true));

                if (parallelCompile == false)
                {
                    _compilerMutex.WaitOne();
                }

                var sem = _compilerSemaphore.GetOrAdd(Properties["config-platform"] ?? "default", key => new SemaphoreSlim(Math.Max(GetThreadCount(), 1)));
                try
                {
                    sem.Wait();

                    RunProgram(process, out exitCode, out output);
                }
                finally
                {
                    sem.Release();
                }

                if (parallelCompile == false)
                {
                    _compilerMutex.ReleaseMutex();
                }

                // HACK: the visual studio compiler always outputs the filename being compiled 
                // on the first line so we need to remove it.
                string visualCppSourceName = null;
                if (_forceLowerCaseFilePaths)
                {
                    visualCppSourceName = Path.GetFileName(fileItem.FileName).ToLower();
                }
                else
                {
                    visualCppSourceName = Path.GetFileName(fileItem.FileName);
                }

                if (output.StartsWith(visualCppSourceName))
                {
                    output = output.Remove(0, visualCppSourceName.Length);
                }

                foreach (var line in output.LinesToArray())
                {
                    bufferedLogger.Status.WriteLine(line);
                }
            }

            return exitCode;
        }

        int RunDependencyGenerator(FileToCompile fileToCompile, BufferedLog bufferedLogger, DG.IDependencyGenerator dependencyGenerator) {
            if (fileToCompile.SourceFile == null) {
                throw new ArgumentNullException("fileItem");
            }
            if (bufferedLogger == null) {
                throw new ArgumentNullException("bufferedLogger");
            }

            int exitCode = 0;

            if(dependencyGenerator != null)
            {
                string depWarn = GetOption(DependencyWarningsProperty);
                
                if (depWarn != null) {
                dependencyGenerator.SuppressWarnings = !(Boolean.Parse(depWarn) == true);
                }
                
                dependencyGenerator.Reset();
                foreach (string define in GetGccDefines(fileToCompile.SourceFile)) {
                    dependencyGenerator.AddDefine(define);
                }
                foreach (string include in GetIncludeDirectories(fileToCompile.SourceFile)) {
                    dependencyGenerator.AddIncludePath(Project.GetFullPath(include));
                }
                foreach (string usingname in GetUsingDirectories(fileToCompile.SourceFile)) 
                {
                    dependencyGenerator.AddUsingPath(Project.GetFullPath(usingname));
                }

                //TODO: cc options may redefine default path. Parse options
                dependencyGenerator.DependencyFilePath = GetDependencyFilePath(fileToCompile.SourceFile); 

                try 
                {
                    StringCollection dependents = dependencyGenerator.ParseFile(fileToCompile.SourceFile.FullPath);

                    // write command line and dependent info to .dep file
                    string dependencyPath = GetDependencyPath(fileToCompile.SourceFile);
                    
                    // make sure the directory exists where we are going to create the dep file
                    string outputDirectory = Path.GetDirectoryName(dependencyPath);
                    if (!Directory.Exists(outputDirectory)) {
                        Directory.CreateDirectory(outputDirectory);
                    }

                    using (StreamWriter writer = File.CreateText(dependencyPath)) {
                        writer.WriteLine(GetCompilerProgram(fileToCompile.SourceFile));
                        writer.WriteLine(GetCompilerCommandLine(fileToCompile.SourceFile));

                        foreach (string dependent in dependents) {
                            writer.WriteLine(dependent);
                        }

                        foreach (string dependent in fileToCompile.Dependencies) 
                        {
                            writer.WriteLine(dependent);
                        }

                        writer.Close();
                    }
                } catch (Exception e) {
                    bufferedLogger.Warning.WriteLine("Could not generate dependency file.  " + e.Message);
                    // Not being able to generate an dependdency file shouldn't cause the
                    // build to stop.  The object file will have been created just not the .dep
                    // file.  This error is occurs due to bugs in the dependency generator.
                }
            }

            return exitCode;
        }

        void GetDependentInfo(string dependencyPath, out StringCollection dependents, 
            out string commandLine, out string compiler) {
            dependents = new StringCollection();

            using (StreamReader r = File.OpenText(dependencyPath)) {
                // compiler exe is stored on the first line
                compiler = r.ReadLine();

                // command line is stored on the second line
                commandLine = r.ReadLine();

                // each dependent file is stored on it's own line
                string dependent = r.ReadLine();
                while (dependent != null) {
                    dependents.Add(dependent);
                    dependent = r.ReadLine();
                }

                r.Close();
            }
        }

        void RunProgram(Process process, out int exitCode, out string output) {
            using (ProcessRunner processRunner = new ProcessRunner(process)) {
                processRunner.Start();

                exitCode = processRunner.ExitCode;
                output   = processRunner.GetOutput().Trim();
            }
        }

        string GetRelativeSourcePath(FileItem fileItem) {
            // normalize the paths so we can compare them
            string sourceDir = PathNormalizer.Normalize(fileItem.BaseDirectory??Sources.BaseDirectory);
            string sourceFileName = fileItem.FullPath;

            // display build message
            if (!String.IsNullOrEmpty(sourceDir) && sourceFileName.StartsWith(sourceDir)) {
                sourceFileName = sourceFileName.Replace(sourceDir, String.Empty);
                
                // trim the leading directory seperator
                sourceFileName = sourceFileName.Substring(1);
            }
            return sourceFileName;
        }
        
        string GetCompilerProgram(FileItem fileItem) {
            return GetOption(fileItem, ProgramProperty);
        }

        public override string WorkingDirectory  {
            get {
                return Sources.BaseDirectory;
            }
        }

        protected virtual string GetDependencyFilePath(FileItem sourcefile)
        {
            return Path.ChangeExtension(GetObjectPath(sourcefile), ".d");
        }

        string GetCompilerCommandLine(FileItem fileItem) {
            // get path style property, for LocalizePath calls below, needed for standardizing on / or \ for the path
            string pathStyle = GetOption(BuildTask.PathStyleProperty);
            
            string defines     = GetDefineOptions(fileItem);
            string includeDirs = BuildTask.LocalizePath(GetIncludeDirectoryOptions(fileItem), pathStyle);
            string usingDirs   = BuildTask.LocalizePath(GetUsingDirectoryOptions(fileItem), pathStyle);
            string options     = GetCompilerOptions(fileItem);

//			Console.WriteLine(" defines: " +defines);
//			Console.WriteLine(" cc options: " +options);

            string template = GetOption(fileItem, CommandLineTemplateProperty);
            if (template == null) {
                template = DefaultCommandLineTemplate;
            }

            StringBuilder commandLine = new StringBuilder();

            // strip whitespace in command line template
            foreach (string token in template.Split(' ', '\n')) {
                string trimmed = token.Trim();
                if (trimmed.Length > 0) {
                    commandLine.Append(trimmed);
                    commandLine.Append(" ");
                }
            }

            commandLine.Replace(DefinesVariable,            defines);
            commandLine.Replace(IncludeDirectoriesVariable, includeDirs);
            commandLine.Replace(UsingDirectoriesVariable,   usingDirs);
            commandLine.Replace(OptionsVariable,            options);

            if (OutputDir != null)
            {
                string outputDir = OutputDir;

                //standardize on / or \ for the path
                outputDir= BuildTask.LocalizePath(outputDir, pathStyle);

                if (_forceLowerCaseFilePaths)
                {
                    // By forcing the path to lower case, we get around any case sensitivity issues with file paths
                    commandLine.Replace(OutputDirectoryVariable,    outputDir.ToLower());
                }
                else
                {
                    commandLine.Replace(OutputDirectoryVariable, outputDir);
                }
            }
            else
            {
                commandLine.Replace(OutputDirectoryVariable,    OutputDir);
            }

            if (OutputName != null && _forceLowerCaseFilePaths)
            {
                // By forcing the path to lower case, we get around any case sensitivity issues with file paths
                commandLine.Replace(OutputNameVariable,    OutputName.ToLower());
            }
            else
            {
                commandLine.Replace(OutputNameVariable,    OutputName);
            }

            // replace template variables found anywhere
            string objPath = GetObjectPath(fileItem);
            if (UseRelativePaths) {
                objPath = PathNormalizer.MakeRelative(objPath, WorkingDirectory);
            }

            Debug.Assert(objPath!=null, "\n null objPath in GetCompilerCommandLine!");
            
            //standardize on / or \ for the path
            objPath= BuildTask.LocalizePath(objPath, pathStyle);

            if (_forceLowerCaseFilePaths)
            {
                // By forcing the path to lower case, we get around any case sensitivity issues with file paths
                commandLine.Replace(ObjectFileVariable, objPath.ToLower());
            }
            else
            {
                commandLine.Replace(ObjectFileVariable, objPath);
            }

            if (commandLine.ToString().IndexOf("%sourcefile%") == -1) 
            {
                string fileItemFullPath= fileItem.FullPath;

                //standardize on / or \ for the path
                fileItemFullPath= BuildTask.LocalizePath(fileItemFullPath, pathStyle);
                
                // maintain compatibility with old batch compile mode style
                commandLine.AppendFormat("\"{0}\"", fileItemFullPath);
            } 
            else 
            {
                string path = fileItem.FullPath;
                if (UseRelativePaths && !fileItem.AsIs) {
                    path = PathNormalizer.MakeRelative(path, WorkingDirectory);
                }
                
                Debug.Assert(path!=null, "\n null path in GetCompilerCommandLine!");
                
                //standardize on / or \ for the path
                path= BuildTask.LocalizePath(path, pathStyle);

                if (_forceLowerCaseFilePaths)
                {
                    // By forcing the path to lower case, we get around any case sensitivity issues with file paths
                    commandLine.Replace("%sourcefile%", path.ToLower());
                }
                else
                {
                    commandLine.Replace("%sourcefile%", path);
                }
            }

            string localizedCommand = commandLine.ToString();
            return localizedCommand;
        }

        string GetDefineOptions(FileItem fileItem) {
            return GetTemplatedOptionsFromCollection(
                GetOption(fileItem, DefineTemplateProperty),
                DefineVariable,
                GetDefines(fileItem));
        }

        string GetIncludeDirectoryOptions(FileItem fileItem) {
            StringCollection includeDirectories = GetIncludeDirectories(fileItem);

            if (_forceLowerCaseFilePaths)
            {
                // By forcing the path to lower case, we get around any case sensitivity issues with file paths
                for(int i = 0; i <  includeDirectories.Count; i++)
                {
                    Debug.Assert(includeDirectories[i]!=null, "\n null includeDirectories[i] in GetIncludeDirectoryOptions!");
                    // By forcing the path to lower case, we get around any case sensitivity issues with file paths
                    includeDirectories[i] = includeDirectories[i].ToLower();
                }
            }

            return GetTemplatedDirectoriesFromCollection(
                GetOption(fileItem, IncludeDirectoryTemplateProperty),
                IncludeDirectoryVariable,
                includeDirectories);
        }

        StringCollection GetIncludeDirectories(FileItem fileItem) {
            // Compiler include directories can come from the property named in the IncludeDirectoriesProperty 
            // and from the task's IncludeDirectories property element.

            // We want to add local include directories first to override global include directories.
            // This is behaviour is different then with command line options and defines
            StringCollection includeDirectories = new StringCollection();
            AddDelimitedProperties(includeDirectories, Project.ExpandProperties(IncludeDirectories.Value));
            AddDelimitedProperties(includeDirectories, GetOption(fileItem, IncludeDirectoriesProperty));

            return includeDirectories;
        }

        string GetUsingDirectoryOptions(FileItem fileItem) 
        {
            StringCollection usingDirectories = GetUsingDirectories(fileItem);

            if (_forceLowerCaseFilePaths)
            {
                // By forcing the path to lower case, we get around any case sensitivity issues with file paths
                for(int i = 0; i <  usingDirectories.Count; i++)
                {
                    Debug.Assert(usingDirectories[i]!=null, "\n null usingDirectories[i] in GetUsingDirectoryOptions!");
                    // By forcing the path to lower case, we get around any case sensitivity issues with file paths
                    usingDirectories[i] = usingDirectories[i].ToLower();
                }
            }

            return GetTemplatedDirectoriesFromCollection(
                GetOption(fileItem, UsingDirectoryTemplateProperty),
                UsingDirectoryVariable,
                usingDirectories);
        }

        StringCollection GetUsingDirectories(FileItem fileItem) 
        {
            // Compiler using directories can come from the property named in the UsingDirectoriesProperty 
            // and from the task's UsingDirectories property element.

            // We want to add local using directories first to override global using directories.
            // This is behaviour is different then with command line options and defines
            StringCollection usingDirectories = new StringCollection();
            AddDelimitedProperties(usingDirectories, Project.ExpandProperties(UsingDirectories.Value));
            AddDelimitedProperties(usingDirectories, GetOption(fileItem, UsingDirectoriesProperty));

            return usingDirectories;
        }

        StringCollection GetDefines(FileItem fileItem) 
        {
            // Compiler defines can come from the property named in the DefinesProperty 
            // and from the task's Defines property element.
            var defines = new StringCollection();
            AddDelimitedProperties(defines, GetOption(fileItem, DefinesProperty));
            AddDelimitedProperties(defines, Project.ExpandProperties(Defines.Value));

            //Make sure there are no duplicates
            var processed = new HashSet<string>();
            var filteredDefines = new StringCollection();
            foreach(var def in defines)
            {
                if (processed.Add(def))
                {
                    filteredDefines.Add(def);
                }
            }

            return filteredDefines;
        }

        string GetCompilerOptions(FileItem fileItem) {
            // Normal program options do not get processed with a user supplied template but
            // they still might have extra newlines and whitespace so we send it through the
            // same path with a very simple template.
            StringCollection items = new StringCollection();
            AddDelimitedProperties(items, GetOption(fileItem, OptionsProperty));
            AddDelimitedProperties(items, Project.ExpandProperties(Options.Value));

            StringBuilder options = new StringBuilder(
                GetTemplatedOptionsFromCollection("%option% ", "%option%", items));

            // replace single variables with values
            options.Replace(OutputDirectoryVariable, Project.GetFullPath(OutputDir));
            options.Replace(OutputNameVariable, OutputName);

            return options.ToString();
        }

        string GetTemplatedDirectoriesFromCollection(string template, string variable, StringCollection items) {
            StringBuilder options = new StringBuilder();
            foreach (string item in items) {
                string path = PathNormalizer.Normalize(Project.GetFullPath(item));

                if (_forceLowerCaseFilePaths)
                {
                    Debug.Assert(path!=null, "\n null path in GetTemplatedDirectoriesFromCollection!");
                    options.Append(template.Replace(variable, path.ToLower()));
                }
                else
                {
                    options.Append(template.Replace(variable, path));
                }
                options.Append(" ");
            }
            //Console.WriteLine("options: " +options.ToString());
            return options.ToString();
        }

        string GetTemplatedOptionsFromCollection(string template, string variable, StringCollection items) {
            StringBuilder options = new StringBuilder();
            foreach (string item in items) {
                options.Append(template.Replace(variable, item));
                options.Append(" ");
            }
            return options.ToString();
        }

        StringCollection GetGccDefines(FileItem fileItem) {
            StringCollection defines = new StringCollection();

            // GCC compiler defines are the same as the normal defines with the additiona of
            // prefixing the values found in the cc.gccdefines property.  These defines are use
            // to mimic the compiler creating the object file so that when gcc parses windows.h
            // it doesn't choke because stuff like _MSC_VER isn't defined causing a .h file
            // to throw a #pragma error.
            AddDelimitedProperties(defines, GetOption(fileItem, GccDefinesProperty));
            AddDelimitedProperties(defines, GetOption(fileItem, DefinesProperty));
            AddDelimitedProperties(defines, Project.ExpandProperties(Defines.Value));

            // Compiler may have it's own defines whic are not part of configuration;
            AddDelimitedProperties(defines, GetOption(fileItem, CompilerInternalDefines));

            return defines;
        }

        protected override string GetWorkingDirFromTemplate(string program, string defaultVal)
        {
            string option = GetOption(WorkingdirTemplateProperty);
            if (String.IsNullOrWhiteSpace(option))
            {
                return defaultVal;
            }
            return option.Replace("%tooldir%", program).Replace("%outputdir%", OutputDir).Replace("%srcbase%", this.Sources.BaseDirectory);

        }

    }
}
