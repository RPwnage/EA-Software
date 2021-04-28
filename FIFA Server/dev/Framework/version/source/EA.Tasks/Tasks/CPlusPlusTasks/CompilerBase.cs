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
using System.Collections;
using System.Collections.Specialized;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Text;
using System.Threading;
using System.Linq;

using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Util;
using NAnt.Core.Logging;

namespace EA.CPlusPlusTasks
{



	public abstract class CompilerBase : BuildTaskBase {

		/// <summary>Used to map dependent header file paths to their last write time.</summary>
		class DependentDictionary : DictionaryBase {
			public virtual object this[string name] {
				get { return Dictionary[name]; }
				set { Dictionary[name] = value; }
			}
		}

		/// <summary>Used to store dependencies information for FileItem.</summary>
		class FileToCompile
		{
			public FileToCompile(FileItem fileItem, FileSet dependencies)
			{
				SourceFile = fileItem;

				foreach (FileItem tmp in dependencies.FileItems) 
				{
					Dependencies.Add(tmp.FileName);
				}
			}

			public FileItem SourceFile { get; }

			public StringCollection Dependencies { get; } = new StringCollection();
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

		// Used to throttle number of threads across modules.
		private static ConcurrentDictionary<string, Semaphore> _compilerSemaphore = new ConcurrentDictionary<string, Semaphore>();

		// properties used by task
		protected string ProgramProperty                    { get { return Name;                                    } }
		protected string OptionsProperty                    { get { return Name + ".options";                       } }
		protected string DefinesProperty                    { get { return Name + ".defines";                       } }
		protected string IncludeDirectoriesProperty         { get { return Name + ".includedirs";                   } }
		protected string SystemIncludeDirectoriesProperty   { get { return Name + ".system-includedirs"; } }
		protected string UsingDirectoriesProperty           { get { return Name + ".usingdirs";                     } }
		protected string NoDependencyProperty               { get { return Name + ".nodep";                         } }
		protected string DependencyWarningsProperty         { get { return Name + ".depwarnings";                   } }
		protected string NoCompileProperty                  { get { return Name + ".nocompile";                     } }
		protected string ThreadCountProperty                { get { return Name + ".threadcount";                   } }
		protected string ForceUsingAssembliesProperty       { get { return Name + ".forceusing-assemblies";         } }
		protected string IncludeDirectoryTemplateProperty   { get { return Name + ".template.includedir";           } }
		protected string SystemIncludeDirectoryTemplateProperty { get { return Name + ".template.system-includedir"; } }
		protected string UsingDirectoryTemplateProperty     { get { return Name + ".template.usingdir";             } }
		protected string DefineTemplateProperty             { get { return Name + ".template.define";               } }
		protected string ForceUsingAssemblyTemplateProperty { get { return Name + ".template.forceusing-assembly";  } }
		protected string SourceFileTemplateProperty         { get { return Name + ".template.sourcefile";           } }
		protected string ObjectFileExtensionProperty        { get { return Name + ".objfile.extension";             } }
		protected string ParallelCompilerProperty           { get { return Name + ".parallelcompiler";              } }
		protected string CompilerInternalDefines            { get { return Name + ".compilerinternaldefines";       } }

		// used in cc.template properties as placeholders for actual values passed into via task
		const string IncludeDirectoryVariable       = "%includedir%";           // used in IncludeDirectoryTemplateProperty
		const string SystemIncludeDirectoryVariable = "%system-includedir%";    // used in SystemIncludeDirectoryTemplateProperty
		const string UsingDirectoryVariable         = "%usingdir%";             // used in UsingDirectoryTemplateProperty
		const string DefineVariable                 = "%define%";               // used in DefineTemplateProperty
		const string ForceUsingAssemblyVariable     = "%forceusing-assembly%";  // used in ForceUsingAssemblyProperty
		const string SourceFileVariable             = "%sourcefile%";           // used in SourceFileTemplateProperty
		const string ObjectFileVariable             = "%objectfile%";           // used in OptionsProperty
		const string OutputDirectoryVariable        = "%outputdir%";            // used in OptionsProperty
		const string OutputNameVariable             = "%outputname%";           // used in OptionsProperty

		// If this compile task is being used together with a link task, eaconfig will change compiler's pdb output
		// to %intermediatedir% to avoid conflict with linker generated pdb with the same name.
		// The standard <build> or <cc> task won't let user set this token.  For now, we'll just hard code something.
		const string IntermediateDirectoryVariable  = "%intermediatedir%";

		const string PchFileVariable                = "%pchfile%";

		const string DefinesVariable                = "%defines%";
		const string IncludeDirectoriesVariable     = "%includedirs%";
		const string SystemIncludeDirectoriesVariable = "%system-includedirs%";
		const string UsingDirectoriesVariable       = "%usingdirs%";
		const string OptionsVariable                = "%options%";
		const string ForceUsingAssembliesVariable   = "%forceusing-assemblies%";

		// task defaults
		const int    DefaultThreadCount         = 1;
		const string DefaultSourceFileTemplate  = "\"%sourcefile%\"";
		const string _defaultCommandLineTemplate = 
			DefinesVariable + " " + 
			SystemIncludeDirectoriesVariable + " " + 
			IncludeDirectoriesVariable + " " + 
			UsingDirectoriesVariable + " " + 
			OptionsVariable + " ";

		// file extensions
		public const string DefaultObjectFileExtension       = ".obj";

		bool                _forceLowerCaseFilePaths= false;
		string              _outputDir;
		PropertyElement     _includePathProperty            = new PropertyElement();

		public CompilerBase(string name) : base(name) {
			Options.Value              = "";
			Defines.Value              = "";
			_includePathProperty.Value          = "";
			UsingDirectories.Value            = "";
		}

		DependentDictionary DependentMap { get; } = new DependentDictionary();

		/// <summary>Name of option set to use for compiling options.  If null
		/// given values from the global <c>cc.*</c> properties.  Default is null.</summary>
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
		public string OutputName { get; set; }

		/// <summary>Collect compilation time. 
		/// Default is an empty string.</summary>
		[TaskAttribute("collectcompilationtime")]
		public bool CollectCompilationTime { get; set; }

		/// <summary>
		/// Doesn't actually do build.  Just create the dependency .dep file only for each source file.
		/// </summary>
		[TaskAttribute("gendependencyonly")]
		public bool GenerateDependencyOnly { get; set; }

		public FileSet Sources { get; private set; } = new FileSet();

		public FileSet PchSources { get; } = new FileSet();
		public FileSet Dependencies { get; set; } = new FileSet();

		public FileSet ForceUsingAssemblies { get; } = new FileSet();

		protected PropertyElement IncludeDirectories {
			get { return _includePathProperty; }
		}

		protected PropertyElement UsingDirectories { get; } = new PropertyElement();

		protected PropertyElement Options { get; } = new PropertyElement();
		protected PropertyElement Defines { get; } = new PropertyElement();

		public override string LogPrefix
		{
			get
			{
				if (GenerateDependencyOnly)
				{
					// If we're not doing a real build and we're generating dependency files only, prepand the
					// tool name with dep-gen- to reflect that.
					return "[dep-gen-" + Name + "] ";
				}
				else
				{
					return base.LogPrefix;
				}
			}
		}


		/// <summary>Determines if compiler output should always be shown (true) or only shown if the
		/// compiler returns an error code (false).  Default is false.  (Depreciated)</summary>
		protected virtual bool ShowAllOutput {
			get { return false; }
		}

		internal bool ConfigHasSystemIncludeDirsSetup = false;

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
			AddProperty(PchCommandLineTemplateProperty);
			AddProperty(ResponseCommandLineTemplateProperty);
			AddProperty(DefinesProperty);
			AddProperty(IncludeDirectoriesProperty);
			AddProperty(SystemIncludeDirectoriesProperty);
			AddProperty(IncludeDirectoryTemplateProperty);
			AddProperty(SystemIncludeDirectoryTemplateProperty);
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
			AddProperty(ResponseFileSeparator);
			AddProperty(ForceUsingAssembliesProperty);
			AddProperty(ForceUsingAssemblyTemplateProperty);

			//after adding ForceLowerCaseFilePathsProperty we can set a flag, to avoid redundant calls to ForceLowerCaseFilePaths
			_forceLowerCaseFilePaths= ForceLowerCaseFilePaths;

			// merge any options passed via a named option set
			_optionSet.Append(specialOptions);

			ConfigHasSystemIncludeDirsSetup =
				_optionSet.Options.Contains(SystemIncludeDirectoriesProperty) &&
				_optionSet.Options.Contains(SystemIncludeDirectoryTemplateProperty) &&
				_optionSet.Options.Contains(CommandLineTemplateProperty) &&
				_optionSet.Options[CommandLineTemplateProperty].Contains(SystemIncludeDirectoriesVariable);

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

		void ProcessFile(FileToCompile fileToCompile)
		{
			int exitcode = 0;
			using (BufferedLog bufferedLogger = new BufferedLog(Project.Log))
			{
				try
				{
					string objectPath = GetObjectPath(fileToCompile.SourceFile);
					if (fileToCompile.SourceFile.FullPath.Length > 260 || objectPath.Length > 260)
					{
						string message = String.Format("The object path of {0} is too long. ", Path.GetFileName(fileToCompile.SourceFile.FileName));
						throw new BuildException(message, Location);
					}
					if (Log.Level >= Log.LogLevel.Detailed)
					{

						string reason = String.Empty;
						Eaconfig.Core.FileData filedata = fileToCompile.SourceFile.Data as EA.Eaconfig.Core.FileData;
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

					if (IsGenerateObjectOn(fileToCompile.SourceFile) || GenerateDependencyOnly)
					{
						exitcode = RunCompiler(fileToCompile.SourceFile, bufferedLogger);
					}

					if (GenerateDependencyOnly || (IsGenerateObjectOn(fileToCompile.SourceFile) && IsGenerateDependencyOn(fileToCompile.SourceFile)))
					{
						Dependency.GenerateNantCompileDependencyFileFromCompilerDependency(
							fileToCompile.SourceFile,
							GetDependencyFilePath(fileToCompile.SourceFile),
							GetCompilerProgram(fileToCompile.SourceFile),
							GetCompilerCommandLine(fileToCompile.SourceFile),
							GetNantDependencyPath(fileToCompile.SourceFile),
							fileToCompile.Dependencies);
					}
				}
				catch (Exception e)
				{
					bufferedLogger.Error.WriteLine();
					bufferedLogger.Error.WriteLine(e.Message);
					bufferedLogger.Error.WriteLine(e.StackTrace.ToString());
					bufferedLogger.Error.WriteLine(e.ToString());
					exitcode = 1;
				}
			}

			if (exitcode != 0) {
				Interlocked.Increment(ref _errorCount);
			}
		}

		// Shared resources between worker threads
		Queue _jobQueue = null; // contains the list of files that still needs to be processed
		int _errorCount = 0;    // number of files that returned errors during compile

		public int ErrorCount {
			get { return _errorCount; }
			set { _errorCount = value; }
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
				FileToCompile fileItem = GetNextSourceFile();
				while (fileItem != null)
				{
					ProcessFile(fileItem);
					fileItem = GetNextSourceFile();
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

			// add changed sources to the jobQueue
			_jobQueue = new Queue(GetChangedSourceFiles());

			// don't bother creating threads if nothing has changed
			if (_jobQueue.Count > 0)
			{
				int threadCount = GetThreadCount();
				if (threadCount > 1) {
					Log.Info.WriteLine(LogPrefix + "Using {0} threads", threadCount);
				}
				_compilerSemaphore.AddOrUpdate(Properties["config-platform"] ?? "default", key => new Semaphore(Math.Max(threadCount, 1), Math.Max(threadCount, 1)), (key, sem) => sem);

				int instancethreads = ParallelCompile ? threadCount : 1;
				var workers = new System.Threading.Tasks.Task[instancethreads];

				//ThreadPriority priority = threadCount > 1 ? ThreadPriority.BelowNormal : ThreadPriority.Normal;
				for (int i = 0; i < workers.Length; i++) 
				{
					var worker = new System.Threading.Tasks.Task(WorkerThread, System.Threading.Tasks.TaskCreationOptions.LongRunning);
					worker.Start();
					//worker.Priority = priority;
					workers[i]=worker;
				}

				System.Threading.Tasks.Task.WaitAll(workers);
				
				foreach (var worker in workers) 
				{
					worker.Dispose();
				}

				if (_errorCount > 0)
				{
					string message = String.Format("Could not process {0} files.", _errorCount);
					if (CollectCompilationTime || GenerateDependencyOnly)
					{
						Log.Error.WriteLine(LogPrefix + message);
					}
					else
					{
						throw new BuildException(message, Location, stackTrace: BuildException.StackTraceType.None);
					}
				}
			}

			// destroy the job queue
			_jobQueue = null;
		}

		FileToCompileList GetChangedPrecompiledFiles()
		{
			FileToCompileList changedSources = new FileToCompileList();
			
			// reset the dependent map
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
								reason = "file was individually selected.";

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

					FileSetItem pchSourceItem = new FileSetItem(PatternFactory.Instance.CreatePattern(fileItem.FileName));
					// We need to keep the fileData info.  Otherwise, we could end up with incorrect object path computation and
					// we won't be able to do proper linking during nant build.
					pchSourceItem.Data = fileItem.Data;
					PchSources.Includes.Add(pchSourceItem);
				}
				else
				{
					newSources.Include(fileItem, failonmissing : false);
				}
			}
			if (hasPch)
			{
				Sources = newSources;
			}
			return changedSources;
		}


		FileToCompileList GetChangedSourceFiles() {
			FileToCompileList changedSources = new FileToCompileList();
			
			// reset the dependent map
			DependentMap.Clear();

			// get the list of individual source files to compile if any
			StringCollection individualFiles = BuildTask.GetIndividualSourceFiles(Project);

			foreach (FileItem fileItem in Sources.FileItems)
			{
				// First check for individual file compiles
				if (individualFiles.Count > 0) {
					bool b = false;
					foreach(string singleFile in individualFiles)
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
							reason = "file was individually selected.";

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

		string GetNantDependencyPath(FileItem fileItem) {
			return Path.ChangeExtension(GetObjectPath(fileItem), Dependency.DependencyFileExtension);
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
				// Instead we create a suffix that is unique to the file's
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
			string dependencyPath = GetNantDependencyPath(fileItem);
			string pchFile = GetPchPath(fileItem);

			reason = ""; // by default don't give a reason

			if (!String.IsNullOrEmpty(pchFile) && !File.Exists(pchFile))
			{
				reason = "missing precompiled header file";
				return true;
			}
			else if (!File.Exists(objectPath))
			{
				// Only Visual Studio compiler create both .obj and .pch at the same time.  Under clang,
				// if we are building .pch, it won't generate a .obj
				if (Properties["config-compiler"] == "vc" || String.IsNullOrEmpty(pchFile))
				{
					reason = "missing object file";
					return true;
				}
			}
			else if (!File.Exists(dependencyPath))
			{
				reason = "missing dependency file";
				return true;
			}
			else
			{
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
					Dependency.GetNantGeneratedCompileDependentInfo(dependencyPath, out dependentPaths, out originalCommandLine, out originalCompiler);
					if (compiler != originalCompiler)
					{
						reason = "compiler changed.";
						return true;
					}

					if (GetCompilerCommandLine(fileItem) != originalCommandLine)
					{
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
							reason = "dependent file is more recent than object file";
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

		internal string GenerateResponseCommandLine(string objectFile, string compilerCommandLine)
		{
			string outputName = Path.GetFileNameWithoutExtension(objectFile);
			string outputDir = Path.GetDirectoryName(objectFile);
			return GetResponseFileCommandLine(outputName, outputDir, compilerCommandLine, null, GetOption(ResponseFileTemplateProperty));
		}

		int RunCompiler(FileItem fileItem, BufferedLog bufferedLogger)
		{
			string compiler = GetCompilerProgram(fileItem);
			string commandLine = GetCompilerCommandLine(fileItem);
			string output;
			int exitCode = 1;

			// make sure the directory exists where we are going to create the object file
			string outputDirectory = Path.GetDirectoryName(GetObjectPath(fileItem));
			if (!String.IsNullOrEmpty(outputDirectory) && !Directory.Exists(outputDirectory))
			{
				Directory.CreateDirectory(outputDirectory);
			}

			string pchPath = GetPchPath(fileItem);
			if (!String.IsNullOrEmpty(pchPath) && GetOption(fileItem,"create-pch")=="on")
			{
				string pchOutputDirectory = Path.GetDirectoryName(pchPath);
				if (!String.IsNullOrEmpty(pchOutputDirectory) && !Directory.Exists(pchOutputDirectory))
				{
					Directory.CreateDirectory(pchOutputDirectory);
				}
			}

			if (UseResponseFile)
			{
				commandLine = GenerateResponseCommandLine(GetObjectPath(fileItem), commandLine);
			}

			string configCompiler = Properties["config-compiler"];
			string configPlatform = Properties["config-platform"];
			string customCompilerGenDependencySwitch = Properties.GetPropertyOrDefault(configPlatform + "." + ProgramProperty + ".compiler-write-dependency-option", null);
			string customCompilerGenDependencyOnlySwitch = Properties.GetPropertyOrDefault(configPlatform + "." + ProgramProperty + ".compiler-write-dependency-only-option", null);
			string stdoutProcessingApp = Properties.GetPropertyOrDefault(configPlatform + "." + ProgramProperty + ".compiler-stdout-include-processing-exe", null);
			string dependencyFile = GetDependencyFilePath(fileItem);

			if (GenerateDependencyOnly || (IsGenerateObjectOn(fileItem) && IsGenerateDependencyOn(fileItem)))
			{
				commandLine = Dependency.UpdateCompilerCommandLineToGenerateDependency(
					ProgramProperty, // cc or as
					commandLine,
					configCompiler,
					configPlatform,
					GenerateDependencyOnly,
					customCompilerGenDependencySwitch,
					customCompilerGenDependencyOnlySwitch,
					dependencyFile,
					Project.Log
					);
			}

			// run the compiler and capture the output and return code
			Process process = CreateProcess(compiler, commandLine);

			if (bufferedLogger.Level >= Log.LogLevel.Detailed)
			{
				ProcessUtil.SafeInitEnvVars(process.StartInfo);
				bufferedLogger.Info.WriteLine("{0}commandline:{1}", LogPrefix, OutputName);
				bufferedLogger.Info.WriteLine("{0}{1} {2}", LogPrefix, compiler, commandLine.Trim());
				bufferedLogger.Info.WriteLine("{0}WorkingDirectory=\"{1}\"", LogPrefix, process.StartInfo.WorkingDirectory);
				bufferedLogger.Info.WriteLine("{0}Environment=\"{1}\"", LogPrefix, process.StartInfo.EnvironmentVariables.Cast<System.Collections.DictionaryEntry>().ToString("; ", de => de.Key + "=" + de.Value));
			}

			var sem = _compilerSemaphore.GetOrAdd(Properties["config-platform"] ?? "default", key => new Semaphore(Math.Max(GetThreadCount(), 1), Math.Max(GetThreadCount(), 1)));
			try
			{
				sem.WaitOne();

				if (CollectCompilationTime)
				{
					var timer = new Chrono();

					RunProgram(process, out exitCode, out output);

					CompileStatistics.AddEntry(exitCode == 0 ? CompileStatistics.StatusType.Success : CompileStatistics.StatusType.Success, fileItem.Path.Path, timer.GetElapsed());
				}
				else
				{
					RunProgram(process, out exitCode, out output);
				}

				// We could loose application output error message unless we throw an exception
				if (exitCode != 0 && !String.IsNullOrEmpty(output))
				{
					throw new ApplicationException(output);
				}
			}
			catch (ApplicationException ex)
			{
				var msg = String.Format("Failed to execute compiler process '{0}' {1}", compiler, commandLine);
				throw new BuildException(msg, Location, ex);
			}
			catch (Exception ex)
			{
				var msg = String.Format("Failed to start compiler process '{0}' {1}", compiler, commandLine);
				throw new BuildException(msg, Location, ex);
			}
			finally
			{
				sem.Release();
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
			output = Dependency.ProcessCompilerStdoutForDependencies(output, stdoutProcessingApp, configCompiler, fileItem.FileName, GetObjectPath(fileItem), dependencyFile);

			if (exitCode != 0 && String.IsNullOrWhiteSpace(output))
			{
				output = ProcessUtil.DecodeProcessExitCode(Path.GetFileNameWithoutExtension(compiler), exitCode);
			}

			foreach (var line in output.LinesToArray())
			{
				bufferedLogger.Status.WriteLine(line);
			}

			return exitCode;
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
				
				// trim the leading directory separator
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
			string objfile = GetObjectPath(sourcefile);
			EA.Eaconfig.Core.FileData filedata = sourcefile.Data as EA.Eaconfig.Core.FileData;
			if (filedata != null)
			{
				// For standard clang comopiler, if we are doing pch build, the output is .pch file and the default
				// compiler generated .d file is beside the .pch file.
				EA.Eaconfig.Modules.Tools.CcCompiler cc = filedata.Tool as EA.Eaconfig.Modules.Tools.CcCompiler;
				if (cc != null && cc.PrecompiledHeaders == Eaconfig.Modules.Tools.CcCompiler.PrecompiledHeadersAction.CreatePch)
				{
					string pchPath = GetPchPath(sourcefile);
					if (!String.IsNullOrEmpty(pchPath))
					{
						objfile = pchPath;
					}
				}
			}
			return Path.ChangeExtension(objfile, Dependency.CompilerDependencyFileExtension);
		}

		internal string GetCompilerCommandLine(FileItem fileItem) {
			// get path style property, for LocalizePath calls below, needed for standardizing on / or \ for the path
			string pathStyle = GetOption(BuildTask.PathStyleProperty);
			
			string defines              = GetDefineOptions(fileItem);
			string includeDirs          = BuildTask.LocalizePath(GetIncludeDirectoryOptions(fileItem), pathStyle);
			string systemincludeDirs    = BuildTask.LocalizePath(GetSystemIncludeDirectoryOptions(fileItem), pathStyle);
			string usingDirs            = BuildTask.LocalizePath(GetUsingDirectoryOptions(fileItem), pathStyle);
			string forceUsingAssemblies = BuildTask.LocalizePath(GetForceUsingAssemblyOptions(fileItem), pathStyle);

			string options = string.Empty;
			if (UseAltSepInResponseFile)
				options = BuildTask.LocalizePath(GetCompilerOptions(fileItem), PathStyle.Unix.ToString()); // Some compilers (like unixclang on windows) don't like its parameters passed with \ character instead of treating it as a path separator it treats it as a the start of a special character like \n
			else
				options = GetCompilerOptions(fileItem);


			var commandLine = GetCommandLineTemplate(fileItem);

			commandLine.Replace(DefinesVariable,                defines);
			commandLine.Replace(IncludeDirectoriesVariable,     includeDirs);
			commandLine.Replace(SystemIncludeDirectoriesVariable, systemincludeDirs);
			commandLine.Replace(UsingDirectoriesVariable,       usingDirs);
			commandLine.Replace(OptionsVariable,                options);
			commandLine.Replace(ForceUsingAssembliesVariable,   forceUsingAssemblies);
			commandLine.Replace(Dependency.DependencyFileVariable, GetDependencyFilePath(fileItem));

			string finalOutputDir = OutputDir;
			string intermediateDir = "intermediate";
			if (finalOutputDir != null)
			{
				finalOutputDir = BuildTask.LocalizePath(finalOutputDir, pathStyle);
				if (_forceLowerCaseFilePaths)
				{
					finalOutputDir = finalOutputDir.ToLower();
				}
				intermediateDir = BuildTask.LocalizePath(finalOutputDir + "/intermediate", pathStyle);
			}
			commandLine.Replace(OutputDirectoryVariable, finalOutputDir);
			if (!Directory.Exists(intermediateDir) && commandLine.ToString().Contains(IntermediateDirectoryVariable))
			{
				Directory.CreateDirectory(intermediateDir);
				commandLine.Replace(IntermediateDirectoryVariable, intermediateDir);
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

			string pchPath = GetPchPath(fileItem);
			if (!String.IsNullOrEmpty(pchPath))
			{
				if (UseRelativePaths)
				{
					pchPath = PathNormalizer.MakeRelative(pchPath, WorkingDirectory);
				}
				pchPath = BuildTask.LocalizePath(pchPath, pathStyle);
				if (_forceLowerCaseFilePaths)
				{
					commandLine.Replace(PchFileVariable, pchPath.ToLower());
				}
				else
				{
					commandLine.Replace(PchFileVariable, pchPath);
				}
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

		private string GetForceUsingAssemblyOptions(FileItem fileItem)
		{
			return GetTemplatedOptionsFromCollection(
				GetOption(fileItem, ForceUsingAssemblyTemplateProperty) ?? "", // not all platform needs to set force using template
				ForceUsingAssemblyVariable,
				GetForceUsings(fileItem));
		}

		string GetDefineOptions(FileItem fileItem) {
			return GetTemplatedOptionsFromCollection(
				GetOption(fileItem, DefineTemplateProperty),
				DefineVariable,
				GetDefines(fileItem));
		}

		string GetIncludeDirectoryOptions(FileItem fileItem) {
			HashSet<string> incDirHash = new HashSet<string>(GetIncludeDirectories(fileItem).Cast<string>().ToList());
			HashSet<string> sysIncDirHash = new HashSet<string>(GetSystemIncludeDirectories(fileItem).Cast<string>().ToList());
			if (sysIncDirHash.Any() && ConfigHasSystemIncludeDirsSetup)
			{
				// We need to make sure the current config has set up the new %system-includedirs% token in command line
				// and have provided cc.template.system-includedir template before we can trim the current include directory
				// list with the system includedirs.  The %system-includedirs% token is a new setup and want to make sure 
				// this change is backward compatible.  
				incDirHash.ExceptWith(sysIncDirHash);
			}
			StringCollection includeDirectories = new StringCollection();
			includeDirectories.AddRange(incDirHash.ToArray());

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
				IncludeDirectoryTemplateProperty,
				GetOption(fileItem, IncludeDirectoryTemplateProperty),
				IncludeDirectoryVariable,
				includeDirectories);
		}

		string GetSystemIncludeDirectoryOptions(FileItem fileItem)
		{
			StringCollection systemIncludeDirectories = GetSystemIncludeDirectories(fileItem);

			if (_forceLowerCaseFilePaths)
			{
				// By forcing the path to lower case, we get around any case sensitivity issues with file paths
				for (int i = 0; i < systemIncludeDirectories.Count; i++)
				{
					Debug.Assert(systemIncludeDirectories[i] != null, "\n null systemincludeDirectories[i] in GetSystemIncludeDirectoryOptions!");
					// By forcing the path to lower case, we get around any case sensitivity issues with file paths
					systemIncludeDirectories[i] = systemIncludeDirectories[i].ToLower();
				}
			}

			return GetTemplatedDirectoriesFromCollection(
				SystemIncludeDirectoryTemplateProperty,
				GetOption(fileItem, SystemIncludeDirectoryTemplateProperty),
				SystemIncludeDirectoryVariable,
				systemIncludeDirectories);
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

		StringCollection GetSystemIncludeDirectories(FileItem fileItem)
		{
			// Compiler system include directories shold only come from the property named in the SystemIncludeDirectoriesProperty. 
			// We do not provide task specific override.
			StringCollection systemIncludeDirectories = new StringCollection();
			AddDelimitedProperties(systemIncludeDirectories, GetOption(fileItem, SystemIncludeDirectoriesProperty));

			return systemIncludeDirectories;
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
				UsingDirectoryTemplateProperty,
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

		private StringCollection GetForceUsings(FileItem fileItem)
		{
			var forceUsings = new StringCollection();
			AddDelimitedProperties(forceUsings, GetOption(fileItem, ForceUsingAssembliesProperty));
			foreach (FileItem item in ForceUsingAssemblies.FileItems)
			{
				if (item.AsIs)
				{
					if (Path.IsPathRooted(item.FileName))
					{
						forceUsings.Add(item.FileName);
					}
					else
					{
						string assemblyPath = PathNormalizer.Normalize(Path.Combine(Project.GetPropertyOrFail("package.DotNet.referencedir"), item.FileName));
						if (File.Exists(assemblyPath))
						{
							forceUsings.Add(assemblyPath);
						}
						else
						{
							string assemblyFacadePath = PathNormalizer.Normalize(Path.Combine(Project.GetPropertyOrFail("package.DotNet.referencedir"), "Facades", item.FileName));
							if (File.Exists(assemblyFacadePath))
							{
								forceUsings.Add(assemblyFacadePath);
							}
							else
							{
								throw new BuildException(String.Format("FASTBuild error - cannot determine correct reference path for assembly '{0}'.", item.FileName));
							}
						}
					}
				}
				else
				{
					forceUsings.Add(item.FullPath);
				}
			}
			return forceUsings;
		}

		string GetCompilerOptions(FileItem fileItem)
		{
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

		string GetTemplatedDirectoriesFromCollection(string templateName, string template, string variable, StringCollection items)
		{
			if (string.IsNullOrEmpty(template) && items.Count > 0)
			{
				// The 'template' parameter could potentially be null (return of GetOption()).  Usually, when template is null,
				// The 'items' list is also empty.  However, we run into user build script error with the situation where
				// the template parameter is null and items list is not empty.  So we print the following error message instead 
				// of letting Framework crash could help people track down problem a little easier.
				throw new BuildException(String.Format("Directory item '{0}' needed to access template '{1}' property. But that template is not defined for the current configuration!", items[0], templateName));
			}

			StringBuilder options = new StringBuilder();
			foreach (string item in items)
			{
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

		protected override string GetWorkingDirFromTemplate(string program, string defaultVal)
		{
			string option = GetOption(WorkingdirTemplateProperty);
			if (String.IsNullOrWhiteSpace(option))
			{
				return defaultVal;
			}
			return option.Replace("%tooldir%", program).Replace("%outputdir%", OutputDir).Replace("%srcbase%", this.Sources.BaseDirectory);

		}

		protected override string DefaultCommandLineTemplate
		{
			get
			{
				return _defaultCommandLineTemplate;
			}
		}


		private bool ParallelCompile
		{
			get
			{
				if (_parallelCompile == null)
				{
					// Run the compiler in parallel if the "parallel compiler" property is set, otherwise
					// force it to run in exclusion.
					string parallelCompiler = GetOption(ParallelCompilerProperty);

					_parallelCompile = ((parallelCompiler != null) && (Boolean.Parse(parallelCompiler) == true));

				}
				return (bool)_parallelCompile;
			}
		}
		private bool? _parallelCompile = null;
	}
}
