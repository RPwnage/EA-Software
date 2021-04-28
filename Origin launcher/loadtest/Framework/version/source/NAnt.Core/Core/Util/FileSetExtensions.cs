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

namespace NAnt.Core.Util
{
	public static class FileSetExtensions
	{
		/// <summary>
		/// Appends a fileset and overwrites the base directory of the original fileset. 
		/// The base directory of the fromfileset will be used unless an explicit basedir is provided.
		/// If you want to preserve the basedirectory from the original fileset you should probably just use append.
		/// </summary>
		/// <param name="fileSet">The original fileset that will be modified by this function</param>
		/// <param name="fromfileSet">The fileset to append, this filesets base directory will overwrite the one from the original fileset</param>
		/// <param name="baseDir">The base directory to use instead of the base directory of the fromfileset</param>
		/// <param name="copyNoFailOnMissing">Wheather to copy the value of failonmissing from the fromfileset</param>
		/// <returns>The integer 1 for success and 0 for failure</returns>
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

		/// <summary>
		/// Appends only Includes from a fileset and overwrites the base directory of the original fileset. 
		/// Includes from the original fileset are given the base directory of the original if it was set.
		/// </summary>
		/// <param name="fileSet">The original fileset that will be modified by this function</param>
		/// <param name="fromfileSet">The fileset to append, this fileset's base directory will overwrite the one from the original fileset</param>
		/// <param name="optionSetName">A custom optionset to attach to each of the file items from the fromfileset, will replace any optionset they original had</param>
		/// <param name="force">Overrides the force status of the fileitems appended from the fromfileset</param>
		/// <returns>true if the function succeeded to append files, false if one of the filesets was null or if no files were appended</returns>
		public static bool IncludeWithBaseDir(this FileSet fileSet, FileSet fromfileSet, string optionSetName = null, bool? force = false)
		{
			if (fileSet != null && fromfileSet != null)
			{
				bool success = false;
				fileSet.BaseDirectory = fromfileSet.BaseDirectory;

				foreach (FileItem fileItem in fromfileSet.FileItems)
				{
					Pattern pattern = PatternFactory.Instance.CreatePattern(fileItem.FileName, fileItem.BaseDirectory ?? fromfileSet.BaseDirectory, fileItem.AsIs);
					pattern.FailOnMissingFile = false; // This item was already verified once. No need to do it second time
					pattern.PreserveBaseDirValue = fileItem.BaseDirectory != null;
					// When doing include need to reset force.
					FileSetItem newFileSetItem = new FileSetItem(pattern, optionSetName ?? fileItem.OptionSetName, fileItem.AsIs, force: force ?? fileItem.Force, data: fileItem.Data);
					fileSet.Includes.Add(newFileSetItem);

					success = true;
				}
				return success;
			}
			return false;
		}


		// creates a new fileset from a base fileset and merges another fileset in preserving any base dirs or falling back to a default base dir
		public static FileSet MergeWithBaseDir(this FileSet fileSet, FileSet fromFileSet, string baseDir = null)
		{
			// create a new file set if at least one fs arg is not null
			FileSet merged = null;
			if (fileSet != null || fromFileSet != null)
			{
				merged = new FileSet();
			}

			// merge in excludes and excludes from first argument preserving base dir on per item basis
			if (fileSet != null)
			{
				foreach (FileSetItem include in fileSet.Includes)
				{
					Pattern pattern = PatternFactory.Instance.CreatePattern(include.Pattern.Value, include.Pattern.BaseDirectory ?? fileSet.BaseDirectory, include.AsIs);
					pattern.PreserveBaseDirValue = true;
					merged.Includes.Add(new FileSetItem(pattern, include.OptionSet, include.AsIs, include.Force, include.Data));
				}
				foreach (FileSetItem exclude in fileSet.Excludes)
				{
					Pattern pattern = PatternFactory.Instance.CreatePattern(exclude.Pattern.Value, exclude.Pattern.BaseDirectory ?? fileSet.BaseDirectory, exclude.AsIs);
					pattern.PreserveBaseDirValue = true;
					merged.Excludes.Add(new FileSetItem(pattern, exclude.OptionSet, exclude.AsIs, exclude.Force, exclude.Data));
				}
			}

			// merge in excludes and excludes from second argument preserving base dir on per item basis, if neither item or fileset has base dir use default
			if (fromFileSet != null)
			{
				foreach (FileSetItem include in fromFileSet.Includes)
				{
					Pattern pattern = PatternFactory.Instance.CreatePattern(include.Pattern.Value, include.Pattern.BaseDirectory ?? fromFileSet.BaseDirectory ?? baseDir, include.AsIs);
					pattern.PreserveBaseDirValue = true;
					merged.Includes.Add(new FileSetItem(pattern, include.OptionSet, include.AsIs, include.Force, include.Data));
				}
				foreach (FileSetItem exclude in fromFileSet.Excludes)
				{
					Pattern pattern = PatternFactory.Instance.CreatePattern(exclude.Pattern.Value, exclude.Pattern.BaseDirectory ?? fromFileSet.BaseDirectory ?? baseDir, exclude.AsIs);
					pattern.PreserveBaseDirValue = true;
					merged.Excludes.Add(new FileSetItem(pattern, exclude.OptionSet, exclude.AsIs, exclude.Force, exclude.Data));
				}
			}

			// Merging a file set that supports missing files with one that doesn't will
			// create problems down the road when evaluating the merged fileset. This
			// will force the merged fileset to only fail on missing files if both
			// filesets expect this behavior.
			if (fileSet != null && fromFileSet != null)
			{
				merged.FailOnMissingFile = fileSet.FailOnMissingFile && fromFileSet.FailOnMissingFile;
			}
			else if (fileSet != null && fromFileSet == null)
			{
				merged.FailOnMissingFile = fileSet.FailOnMissingFile;
			}
			else if (fileSet == null && fromFileSet != null)
			{
				merged.FailOnMissingFile = fromFileSet.FailOnMissingFile;
			}


			// return new merged fileset
			return merged;
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

		public static void Include(this FileSet fileSet, string includePattern, string baseDir = null, bool asIs = false, bool failonmissing = true, object data = null, string optionSetName = null, bool force = false)
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
					FileSetItem newFileSetItem = new FileSetItem(pattern, optionSetName, asIs, force, data);
					fileSet.Includes.Add(newFileSetItem);
				}
				else
				{
					Pattern pattern = PatternFactory.Instance.CreatePattern(includePattern, baseDir ?? fileSet.BaseDirectory, asIs);
					pattern.FailOnMissingFile = failonmissing;
					pattern.PreserveBaseDirValue = baseDir != null; 
					FileSetItem newFileSetItem = new FileSetItem(pattern, optionSet:optionSetName, force: force, data: data);
					fileSet.Includes.Add(newFileSetItem);
				}
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
			if (fileSet != null && fromfileSet != null)
			{
				bool success = false;
				foreach (FileItem fileItem in fromfileSet.FileItems)
				{
					Pattern pattern = PatternFactory.Instance.CreatePattern(fileItem.FileName, baseDir ?? (fileItem.BaseDirectory ?? fileSet.BaseDirectory), fileItem.AsIs);
					pattern.FailOnMissingFile = false; // This item was already verified once. No need to do it second time
					pattern.PreserveBaseDirValue = (fileItem.BaseDirectory != null) || (baseDir!= null);
					FileSetItem newFileSetItem = new FileSetItem(pattern, optionSetName??fileItem.OptionSetName, fileItem.AsIs, force ?? fileItem.Force, fileItem.Data);
					fileSet.Includes.Add(newFileSetItem);

					success = true;
				}
				return success;
			}
			return false;
		}

		// Default: failonmissing = false. fileItem should be already verified, no need to do it again.
		public static bool Include(this FileSet fileSet, FileItem fileItem, bool failonmissing = false, string baseDir = null)
		{
			if (fileSet != null && fileItem != null)
			{
				Pattern pattern = PatternFactory.Instance.CreatePattern(fileItem.FileName, baseDir ??(fileItem.BaseDirectory ?? fileSet.BaseDirectory), fileItem.AsIs);
				pattern.FailOnMissingFile = failonmissing;
				pattern.PreserveBaseDirValue = (fileItem.BaseDirectory != null) || (baseDir != null);
				FileSetItem newFileSetItem = new FileSetItem(pattern, fileItem.OptionSetName, fileItem.AsIs, fileItem.Force, fileItem.Data);
				fileSet.Includes.Add(newFileSetItem);

				return true;
			}
			return false;
		}

		private static ICollection<FileSetItem> EMPTY_COLLECTION = new List<FileSetItem>();

	}
}
