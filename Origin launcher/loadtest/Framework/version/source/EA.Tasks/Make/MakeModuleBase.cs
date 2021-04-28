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

using System;
using System.IO;
using System.Collections.Generic;

using NAnt.Core;
using NAnt.Core.Events;
using NAnt.Core.Writers;
using NAnt.Core.Util;
using NAnt.Core.Logging;

using EA.CPlusPlusTasks;


using EA.Make.MakeItems;

namespace EA.Make
{
	public enum QuotedPath
	{
		UseDefault	= 0,
		Quoted		= 1,
		NotQuoted	= 2
	};

	public enum PathType
	{
		UseDefault		= 0,
		FullPath		= 1,
		RelativePath	= 2
	};

	public abstract class MakeModuleBase : IDisposable
	{
		public enum PathModeType { Auto = 0, Relative = 1, Full = 2, SupportsQuote=4};

		public readonly MakeFileSections Sections;

		public readonly MakeEmptyRecipes EmptyRecipes;

		public readonly PathStyle PathStyle;

		public readonly MakeToolWrappers ToolWrapers;

		protected readonly List<string> FilesToClean;

		protected MakeModuleBase(Log log, PathModeType pathmode, MakeToolWrappers toolWrapers)
		{
			Log = log;
			LogPrefix = "[makegenerator] ";
			MakeFile = new MakeFile();
			PathMode = pathmode & (~PathModeType.SupportsQuote);
			SupportsQuotes = (pathmode & PathModeType.SupportsQuote) == PathModeType.SupportsQuote;

			Sections = new MakeFileSections(MakeFile);
			EmptyRecipes = new MakeEmptyRecipes();

			FilesToClean = new List<string>();

			ToolWrapers = toolWrapers;
		}

		protected abstract string MakeFileDir { get; }

		protected abstract string PackageBuildDir { get; }

		protected abstract string MakeFileName { get; }

		private void OnMakeFileUpdate(object sender, CachedWriterEventArgs e)
		{
			if (e != null)
			{
				string message = string.Format("{0}{1} MakeFile file '{2}'", LogPrefix, e.IsUpdatingFile ? "    Updating" : "NOT Updating", Path.GetFileName(e.FileName));
				if (e.IsUpdatingFile)
					Log.Minimal.WriteLine(message);
				else
					Log.Status.WriteLine(message);
			}
		}


		public string ToMakeFilePath(
						PathString path,
						QuotedPath quoteOverride = QuotedPath.UseDefault,
						PathType pathTypeOverride = PathType.UseDefault)
		{
			return ToMakeFilePath(path.Path, quoteOverride, pathTypeOverride);
		}

		public string ToMakeFilePath(
						string path,
						QuotedPath quoteOverride = QuotedPath.UseDefault,
						PathType pathTypeOverride = PathType.UseDefault)
		{
			string projectPath = path;

			bool quote = PlatformUtil.IsWindows;	// Default
			if (quoteOverride == QuotedPath.Quoted)
			{
				quote = true;
			}
			else if (quoteOverride == QuotedPath.NotQuoted)
			{
				quote = false;
			}

			var pathMode = PathMode & (~PathModeType.SupportsQuote);

			if (pathTypeOverride == PathType.FullPath)
			{
				pathMode = PathModeType.Full;
			}
			else if (pathTypeOverride == PathType.RelativePath)
			{
				pathMode = PathModeType.Relative;
			}

			switch (pathMode)
			{
				case PathModeType.Auto:
					{
						if (PathUtil.IsPathInDirectory(path, PackageBuildDir))
						{
							projectPath = PathUtil.RelativePath(path, MakeFileDir, addDot: false);
						}
						else
						{
							projectPath = path;
						}

						if (projectPath.Length >= 260)
						{
							Log.ThrowWarning
							(
								Log.WarningId.LongPathWarning, Log.WarnLevel.Normal,
								"Project path is longer than 260 characters which can lead to unbuildable project: {0}", projectPath
							);
						}
					}
					break;
				case PathModeType.Relative:
					{
						bool isSdk = false;
						//IMTODO: Possibly we want to support portable mode in future
						//if (BuildGenerator.IsPortable && !PathUtil.IsPathInDirectory(path, OutputDir.Path))
						//{
						//    // Check if path is an SDK path
						//    projectPath = BuildGenerator.PortableData.FixPath(path, out isSdk);
						//}
						if (path.StartsWith("%") || path.StartsWith("\"%"))
						{
							isSdk = true;
						}

						if (!isSdk)
						{
							projectPath = PathUtil.RelativePath(path, MakeFileDir, addDot: false);

							// Need to check the combined length which the relative path is going to have
							// before choosing whether to relativize or not.  Before we had this check in place
							// it was possible for <nanttovcproj> to generate an unbuildable VCPROJ, because
							// the relativized path overflowed the maximum path length limit.
							string combinedFilePath = Path.Combine(MakeFileDir, projectPath);
							if (combinedFilePath.Length >= 260)
							{
								Log.ThrowWarning
								(
									Log.WarningId.LongPathWarning, Log.WarnLevel.Normal,
									"Relative path combined with project path is longer than 260 characters which can lead to unbuildable project: {0}", combinedFilePath
								);
							}
						}
					}
					break;
				case PathModeType.Full:
					{
						projectPath = path;

						if (projectPath.Length >= 260)
						{
							Log.ThrowWarning
							(
								Log.WarningId.LongPathWarning, Log.WarnLevel.Normal,
								"project path is longer than 260 characters which can lead to unbuildable project: {0}", projectPath
							);
						}
					}
					break;
			}
			//Vsimake does not work properly when backslash is in the path of target names.
			projectPath = projectPath.Replace('\\', '/');
			if ((bool)quote && SupportsQuotes)
			{
				return projectPath.Quote();
			}
			return projectPath.TrimQuotes();
		}

		public void AddFileToClean(PathString path)
		{
			if (!path.IsNullOrEmpty())
			{
				FilesToClean.Add(ToMakeFilePath(path));
			}
		}

		public void AddFileToClean(string path)
		{
			if (!String.IsNullOrEmpty(path))
			{
				FilesToClean.Add(ToMakeFilePath(path));
			}
		}

		public void AddFilesToClean(IEnumerable<string> paths)
		{
			if(paths != null)
			{
				foreach(var p in paths)
				{
					AddFileToClean(p);
				}
			}
		}

		public void AddFilesToClean(IEnumerable<PathString> paths)
		{
			if (paths != null)
			{
				foreach (var p in paths)
				{
					AddFileToClean(p);
				}
			}
		}

		public void AddPrerequisites(MakeTarget target, MakeTarget dependency)
		{
			if (target != null && dependency!= null)
			{
				AddPrerequisites(target, dependency.Target);
			}
		}

		public void AddPrerequisites(MakeTarget target, string dependency)
		{
			if (target != null && !String.IsNullOrEmpty(dependency))
			{
				if(MakeFile.PHONY.Contains(dependency))
				{
					target.OrderOnlyPrerequisites.Add(dependency);
				}
				else
				{
					target.Prerequisites.Add(dependency);
				}
			}
		}



		public void Dispose()
		{
			Dispose(true);
			GC.SuppressFinalize(this);
		}

		private void Dispose(bool disposing)
		{
			if (!this.disposed)
			{
				if (disposing)
				{
					using (var writer = new MakeWriter())
					{
						writer.FileName = Path.Combine(MakeFileDir, MakeFileName);
						writer.CacheFlushed += new NAnt.Core.Events.CachedWriterEventHandler(OnMakeFileUpdate);

						MakeFile.WriteMakeFile(writer);
					}
				}
				disposed = true;
			}
		}
		

		protected readonly string LogPrefix;
		protected readonly Log Log;
		public readonly MakeFile MakeFile;
		protected readonly PathModeType PathMode;
		protected readonly bool SupportsQuotes;

		private bool disposed = false;
	}
}
