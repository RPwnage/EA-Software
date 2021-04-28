using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

using NAnt.Core;

namespace NAnt.Core.Util
{
    public static class FileSetExtensions
    {

        public static int AppendWithBaseDir(this FileSet fileSet, FileSet fromfileSet, string baseDir=null, bool copyNoFailOnMissing = false)
        {
            int ret = 0;
            if (fileSet != null && fromfileSet != null)
            {
                fileSet.BaseDirectory = baseDir ?? fromfileSet.BaseDirectory;
                fileSet.Append(fromfileSet);
                if (copyNoFailOnMissing && fromfileSet.FailOnMissingFile == false)
                {
                    fileSet.FailOnMissingFile = false; 
                }
                ret = 1;
            }
            return ret;
        }

        public static void Exclude(this FileSet fileSet, string excludePattern)
        {
            if (fileSet != null)
            {
                if (String.IsNullOrEmpty(excludePattern))
                {
                    throw new BuildException("Exclude pattern pattern can not be empty");
                }

                Pattern pattern = PatternFactory.Instance.CreatePattern(excludePattern, fileSet.BaseDirectory);
                fileSet.Excludes.Add(pattern);
                // Reset internal _initialized flag to false
                fileSet.Includes.AddRange(EMPTY_COLLECTION);
            }
        }

        public static void Exclude(this FileSet fileSet, FileSet fromfileSet)
        {
            if (fromfileSet != null)
            {
                foreach (FileItem fileItem in fromfileSet.FileItems)
                {
                    Pattern pattern = PatternFactory.Instance.CreatePattern(fileItem.FileName, fileItem.BaseDirectory, fileItem.AsIs);
                    FileSetItem newFileSetItem = new FileSetItem(pattern, fileItem.OptionSetName, fileItem.AsIs, fileItem.Force);
                    fileSet.Excludes.Add(newFileSetItem);
                }
                if (fromfileSet.FileItems.Count > 0)
                {
                    // Reset internal _initialized flag to false
                    fileSet.Includes.AddRange(EMPTY_COLLECTION);
                }
            }

        }

        public static void IncludePatternWithMacro(this FileSet fileSet, string includePattern, string baseDir = null, bool asIs = false, bool failonmissing = true, object data = null, string optionSetName = null)
        {
            if (!asIs)
            {
                //Check if path starts with VS Macro, if it does - normalize and use asIs=true:
                if (includePattern != null && includePattern.TrimWhiteSpace().StartsWith("$(", StringComparison.Ordinal))
                {
                    asIs = true;
                    includePattern = PathNormalizer.Normalize(includePattern, false);
                }
            }
            Include(fileSet, includePattern, baseDir, asIs, failonmissing, data, optionSetName);
        }

        public static void Include(this FileSet fileSet, string includePattern, string baseDir = null, bool asIs = false, bool failonmissing = true, object data = null, string optionSetName=null)
        {
            if (fileSet != null)
            {
                if (String.IsNullOrEmpty(includePattern))
                {
                    throw new BuildException("Include pattern can not be empty");
                }

                if (asIs)
                {
                    Pattern pattern = PatternFactory.Instance.CreatePattern(includePattern, baseDir ?? fileSet.BaseDirectory, asIs);
                    pattern.FailOnMissingFile = failonmissing;
                    pattern.PreserveBaseDirValue = baseDir != null;
                    FileSetItem newFileSetItem = new FileSetItem(pattern, optionSetName, asIs, false, data);
                    fileSet.Includes.Add(newFileSetItem);

                }
                else
                {
                    Pattern pattern = PatternFactory.Instance.CreatePattern(includePattern, baseDir ?? fileSet.BaseDirectory, asIs);
                    pattern.FailOnMissingFile = failonmissing;
                    pattern.PreserveBaseDirValue = baseDir != null; 
                    FileSetItem newFileSetItem = new FileSetItem(pattern, optionSet:optionSetName, data:data);
                    fileSet.Includes.Add(newFileSetItem);
                }
                fileSet.BaseDirectory = baseDir ?? fileSet.BaseDirectory;
            }
        }

        public static void Include(this FileSet fileSet, string includePattern, bool asIs)
        {
            if (fileSet != null)
            {
                if (String.IsNullOrEmpty(includePattern))
                {
                    throw new BuildException("Include pattern can not be empty");
                }
                Pattern pattern = PatternFactory.Instance.CreatePattern(includePattern, fileSet.BaseDirectory, asIs);
                FileSetItem newFileSetItem = new FileSetItem(pattern, null, asIs, false);
                fileSet.Includes.Add(newFileSetItem);
            }
        }

        public static bool Include(this FileSet fileSet, FileSet fromfileSet, string optionSetName = null, bool? force=null, string baseDir=null)
        {
            bool ret = false;
            if (fileSet != null && fromfileSet != null)
            {
                foreach (FileItem fileItem in fromfileSet.FileItems)
                {
                    Pattern pattern = PatternFactory.Instance.CreatePattern(fileItem.FileName, baseDir ?? (fileItem.BaseDirectory ?? fileSet.BaseDirectory), fileItem.AsIs);
                    pattern.FailOnMissingFile = false; // This item was already verified once. No need to do it second time
                    pattern.PreserveBaseDirValue = (fileItem.BaseDirectory != null) || (baseDir!= null);
                    FileSetItem newFileSetItem = new FileSetItem(pattern, optionSetName??fileItem.OptionSetName, fileItem.AsIs, force ?? fileItem.Force, fileItem.Data);
                    fileSet.Includes.Add(newFileSetItem);

                    ret = true;
                }
            }
            return ret;
        }

        // Default: failonmissing = false. fileItem should be already verified, no need to do it again.
        public static bool Include(this FileSet fileSet, FileItem fileItem, bool failonmissing = false, string baseDir = null)
        {
            bool ret = false;
            if (fileSet != null && fileItem != null)
            {
                Pattern pattern = PatternFactory.Instance.CreatePattern(fileItem.FileName, baseDir ??(fileItem.BaseDirectory ?? fileSet.BaseDirectory), fileItem.AsIs);
                pattern.FailOnMissingFile = failonmissing;
                pattern.PreserveBaseDirValue = (fileItem.BaseDirectory != null) || (baseDir != null);
                FileSetItem newFileSetItem = new FileSetItem(pattern, fileItem.OptionSetName, fileItem.AsIs, fileItem.Force, fileItem.Data);
                fileSet.Includes.Add(newFileSetItem);
                fileSet.BaseDirectory = baseDir ?? fileSet.BaseDirectory;

                ret = true;
            }
            return ret;
        }


        public static bool IncludeWithBaseDir(this FileSet fileSet, FileSet fromfileSet, string optionSetName = null, bool? force = false)
        {
            bool ret = false;
            if (fileSet != null && fromfileSet != null)
            {
                fileSet.BaseDirectory = fromfileSet.BaseDirectory;

                foreach (FileItem fileItem in fromfileSet.FileItems)
                {
                    Pattern pattern = PatternFactory.Instance.CreatePattern(fileItem.FileName, fileItem.BaseDirectory ?? fromfileSet.BaseDirectory, fileItem.AsIs);
                    pattern.FailOnMissingFile = false; // This item was already verified once. No need to do it second time
                    pattern.PreserveBaseDirValue = fileItem.BaseDirectory != null;
                    // When doing include need to reset force.
                    FileSetItem newFileSetItem = new FileSetItem(pattern, optionSetName ?? fileItem.OptionSetName, fileItem.AsIs, force: force??fileItem.Force, data: fileItem.Data);
                    fileSet.Includes.Add(newFileSetItem);

                    ret = true;
                }
            }
            return ret;
        }

        private static ICollection<FileSetItem> EMPTY_COLLECTION = new List<FileSetItem>();

    }
}
