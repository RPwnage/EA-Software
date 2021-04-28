// Originally based on NAnt - A .NET build tool
// Copyright (C) 2003-2018 Electronic Arts Inc.
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
// Electronic Arts (Frostbite.Team.CM@ea.com)

using NAnt.Core;
using NAnt.Core.Util;

using System;
using System.Collections.Generic;


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
			return project.NamedFileSets.ContainsKey(filesetName);
		}

		public static FileSet GetFileSet(Project project, string filesetName)
		{
			if(String.IsNullOrEmpty(filesetName))
			{
				return null;
			}
			if (!project.NamedFileSets.TryGetValue(filesetName, out FileSet fileSet))
			{
				fileSet = null;
			}

			return fileSet;
		}

		public static FileSet CreateFileSetInProject(Project project, string fileSetName, string baseDirectory)
		{
			if (String.IsNullOrEmpty(fileSetName))
			{
				throw new BuildException("Can't create file set with empty name.");
			}

			FileSet fileSet = new FileSet
			{
				Project = project
			};

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
				throw new BuildException("Can't create file set with empty name.");
			}

			project.NamedFileSets.GetOrAdd(fileSetName, new FileSet());
		}

		public static void FileSetInclude(FileSet fileSet, string includePattern)
		{
			fileSet.Includes.Add(PatternFactory.Instance.CreatePattern(includePattern, fileSet.BaseDirectory, false));
		}

		public static void FileSetInclude(FileSet fileSet, string includePattern, string baseDir)
		{
			var pattern = PatternFactory.Instance.CreatePattern(includePattern, baseDir, false);
			pattern.PreserveBaseDirValue = baseDir != null;
			fileSet.Includes.Add(pattern);
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
			pattern.PreserveBaseDirValue = baseDir != null;
			FileSetItem newFileSetItem = new FileSetItem(pattern, String.Empty, asIs, false);

			fileSet.Includes.Add(newFileSetItem);
		}

		public static void FileSetInclude(FileSet fileSet, string includePattern, string baseDir, string optionSet, bool asIs)
		{
			Pattern pattern = PatternFactory.Instance.CreatePattern(includePattern, baseDir, asIs);
			pattern.PreserveBaseDirValue = baseDir != null;
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
					Pattern pattern = PatternFactory.Instance.CreatePattern(fileItem.FileName, fileItem.BaseDirectory, fileItem.AsIs);
					pattern.FailOnMissingFile = false; // This item was already verified once. No need to do it second time
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

		public static void FileSetExclude(FileSet fileSet, string excludePattern, string baseDir, string optionSet, bool asIs)
		{
			Pattern pattern = PatternFactory.Instance.CreatePattern(excludePattern, baseDir, asIs);
			pattern.PreserveBaseDirValue = baseDir != null;
			FileSetItem newFileSetItem = new FileSetItem(pattern, optionSet, asIs, false);

			fileSet.Excludes.Add(newFileSetItem);
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

		private static readonly ICollection<FileSetItem> EMPTY_COLLECTION = new List<FileSetItem>();
	}
}


