// NAnt - A .NET build tool
// Copyright (C) 2003-2015 Electronic Arts Inc.
// Copyright (C) 2001-2002 Gerry Shaw
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
// File Maintainers:
// Electronic Arts (Frostbite.Team.CM@ea.com)
// Gerry Shaw (gerry_shaw@yahoo.com)
// Mike Krueger (mike@icsharpcode.net)
// Ian MacLean ( ian_maclean@another.com )

using System;
using System.IO;
using System.Collections.Specialized;
using System.Text.RegularExpressions;

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Tasks;
using NAnt.Core.Logging;
using NAnt.Core.Attributes;
using NAnt.Core.PackageCore;
using System.Linq;

namespace NAnt.DotNetTasks
{

	/// <summary>Provides the abstract base class for a Microsoft compiler task.</summary>
	public abstract class CompilerBase : ExternalProgramBase {

		public CompilerBase(string name) : base(name) { }

		string _responseFileName;

		// Microsoft common compiler options
		string _output = null;
		bool _debug = false;
		protected ResGenTask _resgenTask = null;
		string _compiler = null;

		/// <summary>Output file name.</summary>
		[TaskAttribute("output", Required=true)]
		public string Output {
			get { return _output; } 
			set { _output = PathNormalizer.Normalize(value, false); }
		}

		/// <summary>Output type (<c>library</c> for dll's, <c>winexe</c> for GUI apps, <c>exe</c> for console apps, or <c>module</c> for, well, modules).</summary>
		[TaskAttribute("target", Required = true)]
		public string OutputTarget { get; set; } = null;

		/// <summary>Generate debug output (<c>true</c>/<c>false</c>). Default is false.</summary>
		[TaskAttribute("debug")]
		public bool Debug {
			get { return Convert.ToBoolean(_debug); } 
			set { _debug = value; }
		}

		/// <summary>
		/// Target the build as "Framework" (.Net Framework), "Standard" (.Net Standard), or "Core" (.Net Core).
		/// Default is "Framework"
		/// </summary>
		[TaskAttribute("targetframeworkfamily")]
		public string TargetFrameworkFamily { get; set; } = null;

		/// <summary>Define conditional compilation symbol(s). Corresponds to <c>/d[efine]:</c> flag.</summary>
		[TaskAttribute("define")]
		public string Define { get; set; } = null;

		/// <summary>Icon to associate with the application. Corresponds to <c>/win32icon:</c> flag.</summary>
		[TaskAttribute("win32icon")]
		public string Win32Icon { get; set; } = null;

		/// <summary>Manifest file to associate with the application. Corresponds to <c>/win32manifest:</c> flag.</summary>
		[TaskAttribute("win32manifest")]
		public string Win32Manifest { get; set; } = null;

		/// <summary>Reference metadata from the specified assembly files.</summary>
		[FileSet("references")]
		public FileSet References { get; set; } = new FileSet();

		/// <summary>Set resources to embed.</summary>
		///<remarks>This can be a combination of resx files and file resources. .resx files will be compiled by resgen and then embedded into the 
		///resulting executable. The Prefix attribute is used to make up the resourcename added to the assembly manifest for non resx files. For resx files the namespace from the matching source file is used as the prefix. 
		///This matches the behaviour of Visual Studio </remarks>
		[FileSet("resources")]
		public ResourceFileSet Resources { get; set; } = new ResourceFileSet();

		/// <summary>Link the specified modules into this assembly.</summary>
		[FileSet("modules")]
		public FileSet Modules { get; set; } = new FileSet();

		/// <summary>The set of source files for compilation.</summary>
		[FileSet("sources")]
		public FileSet Sources { get; set; } = new FileSet();


		/// <summary>
		/// The full path to the C# compiler. 
		/// Default is to use the compiler located in the current Microsoft .Net runtime directory.
		/// </summary>
		[TaskAttribute("compiler", Required=false)]
		public string Compiler {
			get { return _compiler; }
			set { _compiler = PathNormalizer.Normalize(value); }
		}

		/// <summary>
		/// The full path to the resgen .Net SDK compiler. This attribute is required 
		/// only when compiling resources.
		/// </summary>
		[TaskAttribute("resgen", Required = false)]
		public string Resgen { get; set; } = null;

		/// <summary>
		/// Path to system reference assemblies.  (Corresponds to the -lib argument in csc.exe)
		/// Although the actual -lib argument list multiple directories using comma separated,
		/// you can list them using ',' or ';' or '\n'.  Note that space is a valid path
		/// character and therefore won't be used as separator characters.
		/// </summary>
		[TaskAttribute("referencedirs")]
		public string ReferenceAssemblyDirs { get; set; } = null;

		public override string ProgramFileName 
		{
			get { 
				if (Compiler == null) 
				{
					if (TargetFrameworkFamily != null && (TargetFrameworkFamily.ToLower() == "core" || TargetFrameworkFamily.ToLower() == "standard"))
					{
						return PathString.MakeNormalized(Project.Properties["package.DotNetCoreSdk.dotnet.exe"]).Path;
					}
					else
					{
						// Instead of relying on the .NET compilers to be in the user's path point
						// to the compiler directly since it lives in the .NET Framework's runtime directory
						return Path.Combine(Path.GetDirectoryName(System.Runtime.InteropServices.RuntimeEnvironment.GetRuntimeDirectory()), Name);
					}
				}
				return Compiler;
			}
		}

		public override string ProgramArguments {
			get {
				if (TargetFrameworkFamily != null && (TargetFrameworkFamily.ToLower() == "core" || TargetFrameworkFamily.ToLower() == "standard"))
				{
					if (this is CscTask)
					{
						return PathString.MakeNormalized(Project.Properties["package.DotNetCoreSdk.csc.dll"]) + " @\"" + _responseFileName + "\"";
					}
					else if (this is FscTask)
					{
						return PathString.MakeNormalized(Project.Properties["package.DotNetCoreSdk.fsc.exe"]) + " @\"" + _responseFileName + "\"";
					}
					else
					{
						// Shouldn't get here.  Maybe throw an exception as unsupported tool?  Or we want to allow future workflow
						// to just do "dotnet.exe ..."
						return "@\"" + _responseFileName + "\"";
					}
				}
				else
				{
					return "@\"" + _responseFileName + "\"";
				}
			}
		}

		/// <summary>Allows derived classes to provide compiler-specific options.</summary>
		protected virtual void WriteOptions(TextWriter writer) {
		}

		/// <summary>Write an option using the default output format.</summary>
		protected virtual void WriteOption(TextWriter writer, string name) {
			writer.WriteLine("/{0}", name);
		}

		/// <summary>Write an option and its value using the default output format.</summary>
		protected virtual void WriteOption(TextWriter writer, string name, string arg) {           
			// Always quote arguments ( )
			writer.WriteLine("\"/{0}:{1}\"", name, arg);          
		}
	
		/// <summary>Gets the complete output path.</summary>
		protected string GetOutputPath() {
			return Path.GetFullPath(Path.Combine(BaseDirectory, Output));
		}

		/// <summary>Determines whether compilation is needed.</summary>
		protected virtual bool NeedsCompiling() {
			// return true as soon as we know we need to compile

			FileInfo outputFileInfo = new FileInfo(GetOutputPath());
			if (!outputFileInfo.Exists) {
				return true;
			}

			//Sources Updated?
			string fileName = FileSet.FindMoreRecentLastWriteTime(Sources.FileItems.ToStringCollection(), outputFileInfo.LastWriteTime);
			if (fileName != null) {
				Log.Info.WriteLine(LogPrefix + "{0} is out of date, recompiling.", fileName);
				return true;
			}

			//References Updated?
			fileName = FileSet.FindMoreRecentLastWriteTime(References.FileItems.ToStringCollection(), outputFileInfo.LastWriteTime);
			if (fileName != null) {
				Log.Info.WriteLine(LogPrefix + "{0} is out of date, recompiling.", fileName);
				return true;
			}

			//Modules Updated?
			fileName = FileSet.FindMoreRecentLastWriteTime(Modules.FileItems.ToStringCollection(), outputFileInfo.LastWriteTime);
			if (fileName != null) {
				Log.Info.WriteLine(LogPrefix + "{0} is out of date, recompiling.", fileName);
				return true;
			}

			//Resources Updated?
			fileName = FileSet.FindMoreRecentLastWriteTime(Resources.FileItems.ToStringCollection(), outputFileInfo.LastWriteTime);
			if (fileName != null) {
				Log.Info.WriteLine(LogPrefix + "{0} is out of date, recompiling.", fileName);
				return true;
			}
 
			// check the args for /res or /resource options.
			StringCollection resourceFileNames = new StringCollection();
			foreach (string arg in Args) {
				if (arg.StartsWith("/res:") || arg.StartsWith("/resource:")) {
					string path = arg.Substring(arg.IndexOf(':') + 1);

					int indexOfComma = path.IndexOf(',');
					if (indexOfComma != -1) {
						path = path.Substring(0, indexOfComma);
					}
					resourceFileNames.Add(path);
				}
			}
			fileName = FileSet.FindMoreRecentLastWriteTime(resourceFileNames, outputFileInfo.LastWriteTime);
			if (fileName != null) {
				Log.Info.WriteLine(LogPrefix + "{0} is out of date, recompiling.", fileName);
				return true;
			}

			// if we made it here then we don't have to recompile
			return false;
		}

		public bool CompileExecuted { get; set; } = false;

		protected override void ExecuteTask() {
			this.References.FailOnMissingFile = false;
			string output = GetOutputPath();
			using (PackageAutoBuildCleanMap.PackageAutoBuildCleanState buildState = PackageAutoBuildCleanMap.AssemblyAutoBuildCleanMap.StartBuild(output, "dotnet"))
			{
				if (NeedsCompiling())
				{
					// create temp response file to hold compiler options
					_responseFileName = Path.Combine(Project.TempPath, Path.GetRandomFileName());
					StreamWriter writer = new StreamWriter(_responseFileName);

					try
					{
						if (References.BaseDirectory == null)
						{
							References.BaseDirectory = BaseDirectory;
						}
						if (Modules.BaseDirectory == null)
						{
							Modules.BaseDirectory = BaseDirectory;
						}
						if (Sources.BaseDirectory == null)
						{
							Sources.BaseDirectory = BaseDirectory;
						}

						Log.Status.WriteLine(LogPrefix + Path.GetFileName(output));

						// specific compiler options
						WriteOptions(writer);

						if (ReferenceAssemblyDirs != null)
						{
							WriteOption(writer, "lib", string.Join(",", ReferenceAssemblyDirs.ToArray(new char[] { ',', ';', '\n', '\r'}).Select(p=>p.Trim()).Where(p=>!String.IsNullOrEmpty(p))));
						}

						// Microsoft common compiler options
						WriteOption(writer, "nologo");

						WriteOption(writer, "target", OutputTarget);

						foreach(var def in Define.ToArray())
						{
							WriteOption(writer, "define", def);
						}

						WriteOption(writer, "out", output);

						if (Win32Icon != null && Win32Icon != string.Empty)
						{
							WriteOption(writer, "win32icon", Win32Icon);
						}

						if (Win32Manifest != null && Win32Manifest != string.Empty)
						{
							WriteOption(writer, "win32manifest", Win32Manifest);
						}

						foreach (var fileItem in References.FileItems)
						{
							WriteOption(writer, "reference", fileItem.FileName);
						}
						foreach (FileItem fileItem in Modules.FileItems)
						{
							WriteOption(writer, "addmodule", fileItem.FileName);
						}

						// compile resources
						if (Resources.ResxFiles.FileItems.Count > 0)
						{
							CompileResxResources(Resources.ResxFiles);
						}

						// Resx args
						foreach (FileItem fileItem in Resources.ResxFiles.FileItems)
						{
							string prefix = GetFormNamespace(fileItem.FileName); // try and get it from matching form
							string className = GetFormClassName(fileItem.FileName);
							if (prefix == null)
							{
								prefix = Resources.Prefix;
							}
							string tmpResourcePath = Path.ChangeExtension(fileItem.FileName, "resources");
							string manifestResourceName = Path.GetFileName(tmpResourcePath);
							if (prefix != "")
							{
								string actualFileName = Path.GetFileNameWithoutExtension(fileItem.FileName);
								if (className != null)
									manifestResourceName = manifestResourceName.Replace(actualFileName, prefix + "." + className);
								else
									manifestResourceName = manifestResourceName.Replace(actualFileName, prefix + "." + actualFileName);
							}
							string resourceoption = tmpResourcePath + "," + manifestResourceName;
							WriteOption(writer, "resource", resourceoption);
						}

						// other resources
						foreach (FileItem fileItem in Resources.NonResxFiles.FileItems)
						{
							string baseDir = Path.GetFullPath(Resources.BaseDirectory);
							string manifestResourceName = fileItem.FileName.Replace(baseDir, null);
							manifestResourceName = manifestResourceName.TrimStart(Path.DirectorySeparatorChar);
							manifestResourceName = manifestResourceName.Replace(Path.DirectorySeparatorChar, '.');
							if (Resources.Prefix != "")
							{
								manifestResourceName = Resources.Prefix + "." + manifestResourceName;
							}
							string resourceoption = fileItem.FileName + "," + manifestResourceName;
							WriteOption(writer, "resource", resourceoption);
						}

						foreach (FileItem fileItem in Sources.FileItems)
						{
							writer.WriteLine("\"" + fileItem.FileName + "\"");
						}

						// Make sure to close the response file otherwise contents
						// will not be written to disc and EXecuteTask() will fail.
						writer.Close();

						// display command line and response file contents
						Log.Info.WriteLine(LogPrefix + ProgramFileName + " " + GetCommandLine());
						Log.Info.WriteLine(LogPrefix + "Contents of " + _responseFileName);
						if (Log.InfoEnabled)
						{
							Log.Info.WriteLine(File.ReadAllText(_responseFileName));
						}

						// call base class to do the work
						base.ExecuteTask();

						// clean up generated resources.
						if (_resgenTask != null)
						{
							_resgenTask.RemoveOutputs();
						}

						CompileExecuted = true;
					}
					finally
					{
						// make sure we delete response file even if an exception is thrown
						writer.Close(); // make sure stream is closed or file cannot be deleted
						File.Delete(_responseFileName);
						_responseFileName = null;
					}
				}
			}
		}

		/// <summary>
		/// To be overridden by derived classes. Used to determine the file Extension required by the current compiler
		/// </summary>
		/// <returns></returns>
		protected abstract string GetExtension();

		/// <summary>
		/// Open matching source file to find the name of the correct form or user control class.
		/// </summary>
		/// <param name="resxPath"></param>
		/// <returns></returns>
		protected virtual string GetFormClassName(string resxPath)
		{
			string retclassname = null;

			// Determine Extension for compiler type
			string extension = GetExtension();

			// open matching source file if it exists
			string [] sourceFiles = new string[2];
			sourceFiles[0] = resxPath.Replace("resx", extension);
			sourceFiles[1] = resxPath.Replace("resx", "Designer." + extension);

			// since we are searching Visual Studio 2005 auto-generated code, we can hard-code the search pattern.
			string matchclassname = @"^\s*System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager\(typeof\((.+)\)\)\;\s*$";
			if (! File.Exists(sourceFiles[0]))
			{
				if (File.Exists(sourceFiles[1]))
				{
					// a global resx file
					matchclassname = @"typeof\((\w+.)\).Assembly";
				}
			}

			bool classNameFound = false;
			foreach (string sourceFile in sourceFiles)
			{
				if (File.Exists(sourceFile))
				{
					using (StreamReader sr = File.OpenText(sourceFile))
					{
						while (sr.Peek() > -1)
						{
							string str = sr.ReadLine();
							Regex matchClassnameRE = new Regex(matchclassname);

							if (matchClassnameRE.Match(str).Success)
							{
								Match namematch = matchClassnameRE.Match(str);
								retclassname = namematch.Groups[1].Value;
								retclassname = retclassname.Trim();
								classNameFound = true;
								break;
							}
						}
					}
					if (classNameFound)
						break;
				}
			}
			return retclassname;
		}

		/// <summary>
		/// Open matching source file to find the correct namespace. This may need to be overridden by 
		/// the particular compiler if the namespace syntax is different for that language 
		/// </summary>
		/// <param name="resxPath"></param>
		/// <returns></returns>
		protected virtual string GetFormNamespace(string resxPath){
			string retnamespace = null;

			// Determine Extension for compiler type
			string extension = GetExtension();
			
			// open matching source file if it exists (this is a per user control resx file)
			string sourceFile = resxPath.Replace("resx", extension);
			if (! File.Exists(sourceFile))
			{
				// this is a global resx file.
				sourceFile = resxPath.Replace("resx", "Designer." + extension);
			}

			if (File.Exists(sourceFile))
			{
				using (StreamReader sr = File.OpenText(sourceFile))
				{
					while (sr.Peek() > -1)
					{
						string str = sr.ReadLine();
						string matchnamespace = @"namespace ((\w+.)*)";
						string matchnamespaceCaps = @"Namespace ((\w+.)*)";
						Regex matchNamespaceRE = new Regex(matchnamespace);
						Regex matchNamespaceCapsRE = new Regex(matchnamespaceCaps);

						if (matchNamespaceRE.Match(str).Success)
						{
							Match namematch = matchNamespaceRE.Match(str);
							retnamespace = namematch.Groups[1].Value;
							retnamespace = retnamespace.Replace("{", "");
							retnamespace = retnamespace.Trim();
							break;
						}
						else if (matchNamespaceCapsRE.Match(str).Success)
						{
							Match namematch = matchNamespaceCapsRE.Match(str);
							retnamespace = namematch.Groups[1].Value;
							retnamespace = retnamespace.Trim();
							break;
						}
					}
				}
			}
			return retnamespace;
		}

		/// <summary>
		/// Compile the resx files to temp .resources files
		/// </summary>
		/// <param name="resourceFileSet"></param>
		protected void CompileResxResources(FileSet resourceFileSet) {
			if (this.Resgen == null) {
				throw new BuildException("'resgen' is a required attribute.");
			}
			
			ResGenTask resgen = new ResGenTask();
			resgen.Resources = resourceFileSet; // set the fileset
			resgen.Quiet = true;
			resgen.Parent = this.Parent;
			resgen.Project = this.Project;     
			resgen.ToDirectory = this.BaseDirectory;
			resgen.Compiler = this.Resgen;
					  
			_resgenTask = resgen;
			
			// Fix up the indent level --
			Log.IndentLevel++;
			resgen.Execute();
			Log.IndentLevel--;
		}
	}
}
