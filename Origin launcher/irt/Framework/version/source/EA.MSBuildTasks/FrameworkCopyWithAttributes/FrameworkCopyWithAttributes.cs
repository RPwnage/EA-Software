// Originally based on NAnt - A .NET build tool
// Copyright (C) 2003-2016 Electronic Arts Inc.
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
// Electronic Arts (Frostbite.Team.CM@ea.com)

using System;
using System.Collections.Generic;
using System.Threading;

using Microsoft.Build.Utilities;

using NAnt.Shared;

namespace EA.Framework.MsBuild
{
	// We write out own Touch task instead of using MSBuild's Touch task because we have situation where
	// multiple processes are trying to "touch" the same file. So we need to write out own add retries 
	// mechanism to the task.
	public class FrameworkTouchFiles : Task
	{
		[Microsoft.Build.Framework.Required]
		public Microsoft.Build.Framework.ITaskItem[] Files { get; set; }

		public int MaxRetries { get; set; } = 5;

		public int RetryDelayMilliseconds { get; set; } = 2000;

		public override bool Execute()
		{
			if (Files == null)
				return true;

			for (int i = 0; i < Files.Length; i++)
			{
				string fname = Files[i].ItemSpec;

				int retryCount = 0;
				bool updateSuccess = false;
				while (!updateSuccess && retryCount <= MaxRetries)
				{
					using (Mutex fileAccessMutex = new Mutex(true, FileWriteAccessMutexName.GetMutexName(fname), out bool createdNewMutex))
					{
						try
						{
							if (!createdNewMutex)
							{
								fileAccessMutex.WaitOne();  // Wait forever
							}
							System.IO.FileInfo updateFileInfo = new System.IO.FileInfo(fname);
							if (updateFileInfo.Exists)
							{
								updateFileInfo.LastWriteTime = System.DateTime.Now;
							}
							else
							{
								// We can be setting up copylocal of pdb files that actually doesn't exist.  So if file doesn't
								// exist, just log a message (only on detail output) but let it continue.
								Log.LogMessage(Microsoft.Build.Framework.MessageImportance.Low, "FrameworkTouchFiles warning: Input file does not exist: {0}", fname);
							}
							updateSuccess = true;
						}
						catch
						{
						}
						fileAccessMutex.ReleaseMutex();
						fileAccessMutex.Close();
					}
					if (!updateSuccess)
					{
						++retryCount;
						if (RetryDelayMilliseconds > 0 && retryCount <= MaxRetries)
						{
							System.Threading.Thread.Sleep(RetryDelayMilliseconds);
						}
					}
				}
				if (!updateSuccess)
				{
					Log.LogError("FrameworkTouchFiles unable to update timestamp for file: {0}", fname);
					return false;
				}
			}

			return true;
		}
	}

	public class FrameworkCopyWithAttributes : Task
	{
		[Microsoft.Build.Framework.Required]
		public Microsoft.Build.Framework.ITaskItem[] SourceFiles { get; set; }

		[Microsoft.Build.Framework.Required]
		public Microsoft.Build.Framework.ITaskItem[] DestinationFiles { get; set; }

		public bool OverwriteReadOnlyFiles { get; set; } = true;

		public bool SkipUnchangedFiles { get; set; } = true;

		public bool UseHardlinksIfPossible { get; set; } = false;

		public bool UseSymlinksIfPossible { get; set; } = false;

		public bool PreserveTimestamp { get; set; } = true;

		public string ProjectPath { get; set; }

		public int Retries
		{
			get { return _retries; }
			set 
			{ 
				_retries = value; 
				if(_retries < 1)
				{
					_retries = 1;
				}
			}
		} int _retries = 1;

		public int RetryDelayMilliseconds { get; set; } = 0;

		public bool MaintainAttributes { get; set; } = true;

		public bool ContinueOnError { get; set; } = false;

		public string LogMessageLevel { get; set; }

		public override bool Execute()
		{
			if (SourceFiles.Length != DestinationFiles.Length)
			{
				Log.LogError("Lengths of SourceFiles [{0}] and DestinationFiles [{1}] arrays do not match", SourceFiles.Length, DestinationFiles.Length);

				return false;
			}

			bool ret = true;

			for (int i = 0; i < SourceFiles.Length; i++)
			{
				string src = SourceFiles[i].ItemSpec;
				string dst = DestinationFiles[i].ItemSpec;
				if (!String.IsNullOrEmpty(ProjectPath))
				{
					// Evaluate full path if input is relative to project path.
					src = System.IO.Path.Combine(ProjectPath, src);
					dst = System.IO.Path.Combine(ProjectPath, dst);
				}

				CopyFlags copyFlags = (MaintainAttributes ? CopyFlags.MaintainAttributes : CopyFlags.None) |
					(SkipUnchangedFiles ? CopyFlags.SkipUnchanged : CopyFlags.None) |
					(PreserveTimestamp ? CopyFlags.PreserveTimestamp : CopyFlags.None);

				List<LinkOrCopyMethod> methods = new List<LinkOrCopyMethod>();
				if (UseSymlinksIfPossible)
				{
					methods.Add(LinkOrCopyMethod.Symlink);
				}
				if (UseHardlinksIfPossible)
				{
					methods.Add(LinkOrCopyMethod.Hardlink);
				}
				methods.Add(LinkOrCopyMethod.Copy);

				LinkOrCopyStatus result = LinkOrCopyStatus.Failed;
				using (Mutex fileAccessMutex = new Mutex(true, FileWriteAccessMutexName.GetMutexName(dst), out bool createdNewMutex))
				{
					if (!createdNewMutex)
					{
						fileAccessMutex.WaitOne();  // Wait forever!
					}
					result = LinkOrCopyCommon.TryLinkOrCopy
					(
						src,
						dst,
						OverwriteReadOnlyFiles ? OverwriteMode.OverwriteReadOnly : OverwriteMode.Overwrite,
						copyFlags,
						(uint)Retries,
						(uint)RetryDelayMilliseconds,
						methods.ToArray()
					);
					fileAccessMutex.ReleaseMutex();
					fileAccessMutex.Close();
				}

				bool succeeded = result != LinkOrCopyStatus.Failed;

				if (!succeeded)
				{
					if (ContinueOnError)
					{
						Log.LogWarning("Cannot copy {0} to {1}.", SourceFiles[i].ItemSpec, DestinationFiles[i].ItemSpec);
						succeeded = true;
					}
					else
					{
						// If we are exiting this task with "false" return value, it will fail the build.  So we should output as ERROR message rather than WARNING.
						Log.LogError("Cannot copy {0} to {1}.", SourceFiles[i].ItemSpec, DestinationFiles[i].ItemSpec);
					}
				}

				ret = ret && succeeded;
			}

			return ret;
		}

	}
}
