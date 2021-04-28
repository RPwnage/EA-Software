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
using System.Collections.Generic;
using System.Collections.Specialized;
using System.IO;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;

namespace EA.CPlusPlusTasks
{

	/// <summary>Helper class for Link and Lib tasks to generate dependency information.</summary>
	public class Dependency {
		public static readonly string CompilerDependencyFileExtension = ".d";	// compiler generated dependency file
		public static readonly string DependencyFileExtension = ".dep";		// nant generated compile AND link/lib dependency file
		public static readonly string DependencyFileVariable = "%dependencyfile%";

		public static string UpdateCompilerCommandLineToGenerateDependency(
				string compilerType,	// cc, cc-clanguage, or as
				string commandLine,
				string configComplier,
				string configPlatform,
				bool   generateDependencyOnly,
				string customCompilerGenDependencySwitch,
				string customCompilerGenDependencyOnlySwitch,
				string dependencyFile,
				NAnt.Core.Logging.Log logger
				)
		{
			string newCommandLine = commandLine;

			// Insert compiler options for generating dependency file.
			if (generateDependencyOnly)
			{
				if (!String.IsNullOrEmpty(customCompilerGenDependencyOnlySwitch))
				{
					string additionalOption = customCompilerGenDependencyOnlySwitch.Replace(DependencyFileVariable, dependencyFile);
					newCommandLine += " " + additionalOption;
				}
				else if (configComplier == "clang" || configComplier == "gcc")
				{
					newCommandLine = newCommandLine.Replace("-MMD", "");
					newCommandLine = newCommandLine.Replace("-MD", "");
					newCommandLine = newCommandLine.Replace("--write-user-dependencies", "");
					newCommandLine = newCommandLine.Replace("--write-dependencies", "");
					newCommandLine += " -MM -MF \"" + dependencyFile + "\"";
				}
				else if (configComplier == "vc")
				{
					if (compilerType.StartsWith("cc"))
					{
						string showIncludeSwitches = "";
						if (!newCommandLine.Contains("/Pnul") &&
							!newCommandLine.Contains("-Pnul"))
						{
							// appending nul after /P to prevent creation of the [filename].i file.  We just want to stop the compile.
							showIncludeSwitches += "/Pnul ";
						}
						if (!newCommandLine.Contains("/showIncludes") &&
							!newCommandLine.Contains("-showIncludes"))
						{
							showIncludeSwitches = "/showIncludes ";
							newCommandLine = showIncludeSwitches + newCommandLine;
						}
					}
				}
				else
				{
					if (logger != null)
					{
						logger.Warning.WriteLine("Unable to determine compiler switch to generate dependency \".d\" file without compiling the file.  If this is a custom config package and not using clang or vc compiler, please provide property {0}.{1}.compiler-write-dependency-only-option to specify the compiler switch.", configPlatform, compilerType);
					}
				}
			}
			else
			{
				if (!String.IsNullOrEmpty(customCompilerGenDependencySwitch))
				{
					string additionalOption = customCompilerGenDependencySwitch.Replace(DependencyFileVariable, dependencyFile);
					newCommandLine += " " + additionalOption;
				}
				else if (configComplier == "clang" || configComplier == "gcc")
				{
					if (!newCommandLine.Contains("-MMD") &&
						!newCommandLine.Contains("--write-user-dependencies") &&
						!newCommandLine.Contains("-MD") &&
						!newCommandLine.Contains("--write-dependencies"))
					{
						newCommandLine = "-MD " + newCommandLine;
					}
				}
				else if (configComplier == "vc")
				{
					if (compilerType.StartsWith("cc"))
					{
						if (!newCommandLine.Contains("/showIncludes") &&
							!newCommandLine.Contains("-showIncludes"))
						{
							newCommandLine = "/showIncludes " + newCommandLine;
						}
					}
				}
				else
				{
					if (logger != null)
					{
						logger.Warning.WriteLine("Unable to determine compiler switch to generate dependency \".d\" file while compiling the file.  If this is a custom config package and not using clang or vc compiler, please provide property {0}.{1}.compiler-write-dependency-option to specify the compiler switch.", configPlatform, compilerType);
					}
				}
			}

			return newCommandLine;
		}

		public static void GenerateNantCompileDependencyFileFromCompilerDependency(
			NAnt.Core.FileItem sourceFile,
			string compilerDependencyFile,
			string compilerProgram,
			string compilerCommandLine,
			string outputNantDependencyFile,
			StringCollection fileItemDependencies)
		{
			string normalizedSrcPath = NAnt.Core.Util.PathString.MakeNormalized(sourceFile.Path).Path.ToLower();
			StringCollection dependents;
			GetCompilerGenerateMakefileDependencyInfo(compilerDependencyFile, out dependents);
			using (StreamWriter writer = File.CreateText(outputNantDependencyFile))
			{
				writer.WriteLine(compilerProgram);
				writer.WriteLine(compilerCommandLine);

				foreach (string dependent in dependents)
				{
					string normalizedDependent = NAnt.Core.Util.PathString.MakeNormalized(dependent).Path.ToLower();
					if (normalizedDependent != normalizedSrcPath)
					{
						writer.WriteLine(dependent);
					}
				}

				foreach (string dependent in fileItemDependencies)
				{
					writer.WriteLine(dependent);
				}
			}
		}

		public static string ProcessCompilerStdoutForDependencies(
			string inputStdout, 
			string stdoutProcessingApp, 
			string configCompiler, 
			string srcFilePath,
			string objFilePath,
			string outDependencyFile)
		{
			string outputStdout = inputStdout;

			if (!string.IsNullOrEmpty(stdoutProcessingApp))
			{
				string rawStdinFile = Path.ChangeExtension(objFilePath, CompilerDependencyFileExtension + ".rawStdin.txt");
				File.WriteAllText(rawStdinFile, inputStdout);
				System.Diagnostics.Process process = new System.Diagnostics.Process();
				process.StartInfo.FileName = stdoutProcessingApp;
				process.StartInfo.Arguments = "\"" + rawStdinFile + "\" \"" + srcFilePath + "\" \"" + objFilePath + "\" \"" + outDependencyFile + "\"";
				using (NAnt.Core.Util.ProcessRunner processRunner = new NAnt.Core.Util.ProcessRunner(process))
				{
					processRunner.Start();
					outputStdout = processRunner.GetOutput();
				}
			}
			else if (configCompiler == "vc")
			{
				string outputDependencyData = null;
				outputStdout = VCExtractIncludesFromStdoutAndBuildMakefileDependencyData(inputStdout, srcFilePath, objFilePath, out outputDependencyData);
				File.WriteAllText(outDependencyFile, outputDependencyData);
			}
			else
			{
				// If an app is not provided or it is not vc compiler, assume we don't need to process stdout
			}

			return outputStdout;
		}

		private static string VCExtractIncludesFromStdoutAndBuildMakefileDependencyData(
			string originalStdout, 
			string srcFilePath, 
			string objfilepath, 
			out string outDependencyData)
		{
			string srcFileName = Path.GetFileName(srcFilePath);
			string srcFileNameLower = srcFileName.ToLower();
			string inputStdout = (originalStdout.StartsWith(srcFileName) || originalStdout.StartsWith(srcFileNameLower)) ? originalStdout.Substring(srcFileName.Length) : originalStdout;
			StringBuilder includeDependencyContent = new StringBuilder(objfilepath + ":");
			includeDependencyContent.Append(" \\" + Environment.NewLine + "  " + srcFilePath);
			HashSet<string> includes = new HashSet<string>();
			StringBuilder newstdout = new StringBuilder();
			foreach (string line in inputStdout.Split(new char[] { '\n' }))
			{
				if (line.StartsWith("Note: including file:"))
				{
					string includeLine = line.Replace("Note: including file:", "").Trim();
					// We call MakeNormalized to make sure that lines like:  C:\abc\def\ghi and C:\abc\def\aaa\..\ghi evaluates to same path.
					// AND for crying out loud, DON'T change everything to lower case.  Cannot guarantee all external compiler/programs can 
					// handle case insensitive compare even on case insensitive file system and sometimes, we need to preserve the case.  
					// In the future, if we need to do file name case insensitive compare, create a filesystem specific FilepathComparer 
					// and pass that to includes.Contains(x,comparer)!
					string formattedIncludeLine = NAnt.Core.Util.PathString.MakeNormalized(includeLine).Path;
					// Need to quote file path that has space.  It is valid for those .d files to have multiple files
					// in one line separated by space.
					if (formattedIncludeLine.Contains(" "))
						formattedIncludeLine = "\"" + formattedIncludeLine + "\"";
					if (!includes.Contains(formattedIncludeLine))
					{
						includes.Add(formattedIncludeLine);
						includeDependencyContent.Append(" \\" + Environment.NewLine + "  " + formattedIncludeLine);
					}
					continue;
				}
				newstdout.AppendLine(line);
			}

			outDependencyData = includeDependencyContent.ToString();

			return newstdout.ToString();
		}

		public static void GetNantGeneratedCompileDependentInfo(
			string dependencyPath, 
			out StringCollection dependents,
			out string commandLine, 
			out string compiler)
		{
			dependents = new StringCollection();

			using (StreamReader r = File.OpenText(dependencyPath))
			{
				// compiler exe is stored on the first line
				compiler = r.ReadLine();

				// command line is stored on the second line
				commandLine = r.ReadLine();

				// each dependent file is stored on it's own line
				string dependent = r.ReadLine();
				while (dependent != null)
				{
					dependents.Add(dependent);
					dependent = r.ReadLine();
				}
			}
		}

		// This function parse the compiler generated .d file which is in a makefile compatible format like below:
		//		<object file path>: [first header file] \
		//		    <second header file> \
		//		    <file 1> <file 2> \
		//		    ...
		//		    <last header file>
		//		Note that it is possible for a single line to contain multiple files separated by space.
		public static void GetCompilerGenerateMakefileDependencyInfo(string dependencyPath, out StringCollection dependents)
		{
			dependents = new StringCollection();

			using (StreamReader r = File.OpenText(dependencyPath))
			{
				string firstLine = r.ReadLine();
				int objColon = firstLine.IndexOf(": ");
				if (objColon < 0)
				{
					// corrupted file?
					return;
				}
				string firstDependent = firstLine.Substring(objColon + 1).Trim(new char[] { '\t', ' ', '\\' });
				if (!String.IsNullOrEmpty(firstDependent))
				{
					dependents.AddRange(ExtractOptionalQuotedFilesInLine(firstDependent));
				}

				// each dependent file is stored on it's own line (start with white space and may end with a continuation '\' character.)
				string dependent = r.ReadLine();
				while (dependent != null)
				{
					dependent = dependent.Trim(new char[] { '\t', ' ', '\\' });
					dependents.AddRange(ExtractOptionalQuotedFilesInLine(dependent));
					dependent = r.ReadLine();
				}
			}
		}

		private static string[] ExtractOptionalQuotedFilesInLine(string line)
		{
			// (['\""])(?<value>.+?)\1     Match: "quoted file path with space"
			// |                           or
			// (?<value>[^ ]+)             Match: filepath_with_no_space
			string[] retval = Regex.Matches(line, @"(['\""])(?<value>.+?)\1|(?<value>[^ ]+)")
					.Cast<Match>()
					.Select(m => m.Groups["value"].Value)
					.ToArray();

			//StringBuilder debugList = new StringBuilder();
			//debugList.AppendLine("+++++++");
			//debugList.AppendFormat("   Input line: {0}{1}", line, Environment.NewLine);
			//debugList.AppendLine("-------");
			//foreach (string fpath in retval)
			//	debugList.AppendFormat("   {0}{1}", fpath, Environment.NewLine);
			//debugList.AppendLine("+++++++");
			//Console.WriteLine(debugList.ToString());

			return retval;
		}

		public static void GenerateDependencyFile(string dependencyFilePath, string program, string commandLineOptions, StringCollection objectFileNames) {
			StringCollection libraryFileNames = new StringCollection();
			GenerateDependencyFile(dependencyFilePath, program, commandLineOptions, objectFileNames, libraryFileNames);
		}

		public static void GenerateDependencyFile(string dependencyFilePath, string program, string commandLineOptions, StringCollection objectFileNames, StringCollection libraryFileNames)
		{
			using (StreamWriter writer = new StreamWriter(dependencyFilePath)) {
				writer.WriteLine(program + " " + commandLineOptions);
		
				// write each object file
				foreach (string fileName in objectFileNames) {
					writer.WriteLine(fileName);
				}

				// write each library file
				foreach (string fileName in libraryFileNames) {
					writer.WriteLine(fileName);
				}
			}
		}

		//since we don't know what the extension is, for now we will do a hacked solution where we look for the output name 
		//in the output dir with a set of potential extensions
		public static bool IsUpdateToDate(string dependencyFilePath, string outputDir, string outputName, string program, string commandLineOptions, string [] extensions, StringCollection libraryDirs, StringCollection libraryFiles, out string reason) {
			
			reason = ""; // by default don't give a reason
			
			if (!File.Exists(dependencyFilePath)) 
			{
				reason = "dependency file does not exist.";
				return false;
			}

			if(!Directory.Exists(outputDir))
			{
				reason = "could not find output directory: " + outputDir;
				return false;
			}

			// get the file info of the output file 
			FileInfo outputFileInfo = GetOutputFileInfo(outputDir, outputName, extensions, commandLineOptions);

			if (outputFileInfo == null) {
				reason = "could not find output file: " + outputDir + "/" + outputName + ".*";
				return false;
			}

			// read the dependency file (first line are the command line options, other lines are dependent files)
			StreamReader reader = new StreamReader(dependencyFilePath);
			try {
				string line = reader.ReadLine();
				if (line == null || line != program + " " + commandLineOptions) {
					reason = "command line options changed.";
					return false;
				}

				line = reader.ReadLine();
				while (line != null) {
					FileInfo dependentFileInfo = new FileInfo(line);
					if (!dependentFileInfo.Exists)
					{
						// If it is a library check all library folders for it.
						dependentFileInfo = GetMatchingFile(dependentFileInfo, libraryDirs, libraryFiles);
					}

					if (!dependentFileInfo.Exists) {

							reason = String.Format("dependent file '{0}' does not exist.", dependentFileInfo.FullName);
							return false;
					}

					if (dependentFileInfo.LastWriteTime > outputFileInfo.LastWriteTime) {
						reason = String.Format("dependent file '{0}' ({2}) is newer than output file '{1}' ({3}).", dependentFileInfo.FullName, outputFileInfo.FullName,
							dependentFileInfo.LastWriteTime.ToString(), outputFileInfo.LastWriteTime.ToString());
						return false;
					}

					line = reader.ReadLine();
				}
				
			} catch {
				reason = "something unexpected happened, assuming out of date.";
				return false;

			} finally {
				reader.Close();
			}

			// if we made it here, the output file is newer than all the dependents and is up to date
			return true;
		}

		//searches file name in the collection of files, and then checks if file exists in the directory set.
		private static FileInfo GetMatchingFile(FileInfo fileInfo, StringCollection dirs, StringCollection files)
		{
			if (files != null && dirs != null)
			{
				if (-1 != files.IndexOf(fileInfo.Name))
				{
					foreach (string dir in dirs)
					{
						if (File.Exists(Path.Combine(dir, fileInfo.Name)))
						{
							fileInfo = new FileInfo(Path.Combine(dir, fileInfo.Name));
							break;
						}
					}
				}
			}
			return fileInfo;
		}


		//searches and returns first match or null if not found
		private static FileInfo GetMatchingFile(string path, string pattern)
		{
			// Fix for ps3 multi-graphics system rebuild problem
			int slashIdx = pattern.IndexOf('\\');   // Does pattern contain a slash (a subdir)?
			int wildcardIdx = pattern.IndexOf('*'); // Look for the first *

			DirectoryInfo dirInfo = new DirectoryInfo(path);
			FileInfo [] fileInfos = null;

			if ( slashIdx!= -1 && wildcardIdx != -1 && wildcardIdx < slashIdx )
			{
				// pattern contains a subdir and there is a * in the subdir
				string wildcardDir = pattern.Substring(0, slashIdx);    // the subdir with *
				pattern = pattern.Substring(slashIdx+1);                // update pattern to remove the subdir portion 
				DirectoryInfo [] dirInfos = dirInfo.GetDirectories(wildcardDir);    // all the directories represented but the wild card subdir

				foreach (DirectoryInfo di in dirInfos)
				{
					fileInfos = di.GetFiles(pattern);
					if ( fileInfos != null && fileInfos.Length > 0 )
					{
						return fileInfos[0];
					}
				}
				return null;
			}
			else
			{
				// no subdir in pattern, or no wild card character in subdir
				fileInfos = dirInfo.GetFiles(pattern);
				if(fileInfos != null && fileInfos.Length > 0)
				{
					return fileInfos[0];
				}
				else
				{
					return null;
				}
			}
		}

		private static FileInfo GetOutputFileInfo(string outputLocation, string outputName, string [] extensions, string commandline)
		{
			if (extensions == null)
			{
				extensions = new string[1];
				extensions[0] = string.Empty;
			}

			//loop thru each extension trying to find the file using different patterns
			foreach(string ext in extensions)
			{
				FileInfo fileInfo;

				if (ext == "" || ext == string.Empty)
				{
					// This part of the code is really intended for UNIX executables.
					// UNIX executables don't have extension.
					// JL: For now, I'm ignoring the wild card stuff from the next block.
					//     I believe those are intended for libs (when this function is
					//     being called to check lib files).
					fileInfo = GetMatchingFile(outputLocation, outputName);
					if (fileInfo != null)
						return fileInfo;
				}
				else
				{
					//IM TODO: Globing output files this way is very bad. There may be files from different modules 
					// that could accidentally match, and result dependency check will be invalid.
					//
					// This globing is done because Framework may not know exact outputname (when tool options are defined
					// without using framework templates).
					//
					fileInfo = GetMatchingFile(outputLocation, outputName + "." + ext);
					if (fileInfo != null)
						return fileInfo;

					fileInfo = GetMatchingFile(outputLocation, outputName + "*." + ext);
					// IM: Globing may lead to mistakes in dependency check. To compensate for this verify that file name is on the command line
					if (fileInfo != null && !String.IsNullOrEmpty(commandline) && -1 == commandline.IndexOf(fileInfo.Name, StringComparison.OrdinalIgnoreCase))
					{
						fileInfo = null;
					}
					if (fileInfo != null)
						return fileInfo;

					fileInfo = GetMatchingFile(outputLocation, "*" + outputName + "." + ext);
					// IM: Globing may lead to mistakes in dependency check. To compensate for this verify that file name is on the command line
					if (fileInfo != null && !String.IsNullOrEmpty(commandline) && -1 == commandline.IndexOf(fileInfo.Name, StringComparison.OrdinalIgnoreCase))
					{
						fileInfo = null;
					}
					if (fileInfo != null)
						return fileInfo;

					fileInfo = GetMatchingFile(outputLocation, "*" + outputName + "*." + ext);
					// IM: Globing may lead to mistakes in dependency check. To compensate for this verify that file name is on the command line
					if (fileInfo != null && !String.IsNullOrEmpty(commandline) && -1 == commandline.IndexOf(fileInfo.Name, StringComparison.OrdinalIgnoreCase))
					{
						fileInfo = null;
					}
					if (fileInfo != null)
						return fileInfo;
				}
			}

			return null;
		}
	}
}
