using NAnt.Core;
using NAnt.Core.Util;

using System;
using System.Collections;
using System.Collections.Generic;
using System.Text;


namespace EA.Eaconfig
{
    public class FileSetUtil
    {
        public static bool FileSetExists(Project project, string filesetName)
        {
            if (String.IsNullOrEmpty(filesetName))
            {
                return false;
            }
            return (project.NamedFileSets[filesetName] != null);
        }

        public static FileSet GetFileSet(Project project, string filesetName)
        {
            if(String.IsNullOrEmpty(filesetName))
            {
                return null;
            }
            return project.NamedFileSets[filesetName];
        }

        public static FileSet CreateFileSetInProject(Project project, string fileSetName, string baseDirectory)
        {
            if (String.IsNullOrEmpty(fileSetName))
            {
                Error.Throw(project, "", "Can't create  file set with empty name");
            }

            FileSet fileSet = new FileSet();

            fileSet.Project = project;

            if (baseDirectory != null)
            {
                fileSet.BaseDirectory = baseDirectory;
            }

            project.NamedFileSets[fileSetName] = fileSet;
            return fileSet;
        }

        public static void CreateFileSetInProjectIfMissing(Project project, string fileSetName)
        {
            if (String.IsNullOrEmpty(fileSetName))
            {
                Error.Throw(project, "", "Can't create  file set with empty name");
            }
            if (project.NamedFileSets[fileSetName] == null)
            {
                project.NamedFileSets[fileSetName] = new FileSet(); ;
            }
        }


        public static void FileSetInclude(FileSet fileSet, string includePattern)
        {
            fileSet.Includes.Add(PatternFactory.Instance.CreatePattern(includePattern, fileSet.BaseDirectory, false));
        }

        public static void FileSetInclude(FileSet fileSet, string includePattern, string baseDir)
        {
            fileSet.Includes.Add(PatternFactory.Instance.CreatePattern(includePattern, baseDir, false));
        }

        public static void FileSetInclude(FileSet fileSet, string includePattern, bool asIs)
        {
            Pattern pattern = PatternFactory.Instance.CreatePattern(includePattern, fileSet.BaseDirectory, asIs);
            FileSetItem newFileSetItem = new FileSetItem(pattern, String.Empty, asIs, false);

            fileSet.Includes.Add(newFileSetItem);
        }

        public static void FileSetInclude(FileSet fileSet, string includePattern, string baseDir, bool asIs)
        {
            Pattern pattern = PatternFactory.Instance.CreatePattern(includePattern, baseDir, asIs);
            FileSetItem newFileSetItem = new FileSetItem(pattern, String.Empty, asIs, false);

            fileSet.Includes.Add(newFileSetItem);
        }

        public static void FileSetInclude(FileSet fileSet, string includePattern, string baseDir, string optionSet, bool asIs)
        {
            Pattern pattern = PatternFactory.Instance.CreatePattern(includePattern, baseDir, asIs);
            FileSetItem newFileSetItem = new FileSetItem(pattern, optionSet, asIs, false);

            fileSet.Includes.Add(newFileSetItem);
        }

        public static bool FileSetInclude(FileSet fileSet, FileSet fromfileSet)
        {
            bool ret = false;
            if (fromfileSet != null)
            {
                foreach (FileItem fileItem in fromfileSet.FileItems)
                {
                    Pattern pattern = PatternFactory.Instance.CreatePattern(fileItem.FileName, fileItem.AsIs);
                    FileSetItem newFileSetItem = new FileSetItem(pattern, fileItem.OptionSetName, fileItem.AsIs, fileItem.Force);
                    fileSet.Includes.Add(newFileSetItem);

                    ret = true;
                }
            }
            return ret;
        }

        public static void FileSetExclude(FileSet fileSet, FileSet fromfileSet)
        {
            if (fromfileSet != null)
            {
                foreach (FileItem fileItem in fromfileSet.FileItems)
                {
                    Pattern pattern = PatternFactory.Instance.CreatePattern(fileItem.FileName, fileItem.AsIs);
                    FileSetItem newFileSetItem = new FileSetItem(pattern, fileItem.OptionSetName, fileItem.AsIs, fileItem.Force);
                    fileSet.Excludes.Add(newFileSetItem);
                    // Reset internal _initialized flag to false
                    fileSet.Includes.AddRange(EMPTY_COLLECTION);
                }
            }
        }

        public static void FileSetExclude(FileSet fileSet, string excludePattern)
        {
            fileSet.Excludes.Add(PatternFactory.Instance.CreatePattern(excludePattern, fileSet.BaseDirectory, false));
            // Reset internal _initialized flag to false
            fileSet.Includes.AddRange(EMPTY_COLLECTION);
        }

        public static int FileSetAppend(FileSet fileSet, FileSet fromfileSet)
        {
            int ret = 0;
            if (fromfileSet != null)
            {
                fileSet.Append(fromfileSet);
                ret = 1;
            }
            return ret;
        }


        public static int FileSetAppendNoFailOnMissing(FileSet fileSet, FileSet fromfileSet)
        {
            int ret = 0;
            if (fromfileSet != null)
            {
                fileSet.FailOnMissingFile = false;
                fileSet.Append(fromfileSet);
                ret = 1;
            }
            return ret;
        }


        public static int FileSetAppendWithBaseDir(FileSet fileSet, FileSet fromfileSet)
        {
            int ret = 0;
            if (fromfileSet != null)
            {
                fileSet.BaseDirectory = fromfileSet.BaseDirectory;
                fileSet.Append(fromfileSet);
                ret = 1;
            }
            return ret;
        }

        public static int FileSetAppendWithBaseDir(FileSet fileSet, FileSet fromfileSet, string baseDir)
        {
            int ret = 0;
            if (fromfileSet != null)
            {
                fileSet.BaseDirectory = baseDir;
                fileSet.Append(fromfileSet);
                ret = 1;
            }
            return ret;
        }

        private static ICollection EMPTY_COLLECTION = new ArrayList();
    }
}


