// Copyright (C) Electronic Arts Canada Inc. 2002.  All rights reserved.

using System;
using System.Collections.Specialized;
using System.IO;
using System.Diagnostics;

namespace EA.CPlusPlusTasks {

	/// <summary>Helper class for Link and Lib tasks to generate dependency information.</summary>
	public class Dependency {
        public static readonly string DependencyFileExtension = ".dep";

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

                // close dependeny file
                writer.Close();
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
                        reason = String.Format("dependent file '{0}' is newer than output file '{1}'.", dependentFileInfo.FullName, outputFileInfo.FullName);
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
                DirectoryInfo [] dirInfos = dirInfo.GetDirectories(wildcardDir);    // all the directories represented but the wildcard subdir

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
                // no subdir in pattern, or no wildcard character in subdir
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
                    // JL: For now, I'm ignoring the wildcard stuff from the next block.
                    //     I believe those are intended for libs (when this function is
                    //     being called to check lib files).
                    fileInfo = GetMatchingFile(outputLocation, outputName);
                    if (fileInfo != null)
                        return fileInfo;
                }
                else
                {
                    //IM TODO: Globbing output files this way is very bad. There may be files from different modules 
                    // that could accidently match, and result dependency check will be invalid.
                    //
                    // This globbing is done because Framework may not know exact outputname (when tool options are defined
                    // without using framework templates).
                    //
                    fileInfo = GetMatchingFile(outputLocation, outputName + "." + ext);
                    if (fileInfo != null)
                        return fileInfo;

                    fileInfo = GetMatchingFile(outputLocation, outputName + "*." + ext);
                    // IM: Globbing may lead to mistakes in dependency check. To compensate for this verify that file name is on the command line
                    if (fileInfo != null && !String.IsNullOrEmpty(commandline) && -1 == commandline.IndexOf(fileInfo.Name, StringComparison.OrdinalIgnoreCase))
                    {
                        fileInfo = null;
                    }
                    if (fileInfo != null)
                        return fileInfo;

                    fileInfo = GetMatchingFile(outputLocation, "*" + outputName + "." + ext);
                    // IM: Globbing may lead to mistakes in dependency check. To compensate for this verify that file name is on the command line
                    if (fileInfo != null && !String.IsNullOrEmpty(commandline) && -1 == commandline.IndexOf(fileInfo.Name, StringComparison.OrdinalIgnoreCase))
                    {
                        fileInfo = null;
                    }
                    if (fileInfo != null)
                        return fileInfo;

                    fileInfo = GetMatchingFile(outputLocation, "*" + outputName + "*." + ext);
                    // IM: Globbing may lead to mistakes in dependency check. To compensate for this verify that file name is on the command line
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
