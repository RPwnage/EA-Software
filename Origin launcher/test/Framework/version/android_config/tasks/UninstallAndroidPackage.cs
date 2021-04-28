// (c) Electronic Arts. All rights reserved.

using System;

using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Logging;

namespace EA.AndroidConfig
{
	[TaskName("UninstallAndroidPackage")]
	public class UninstallAndroidPackage : Task
	{
		[TaskAttribute("packagename")]
		public string PackageName
		{
			get { return mPackageName; }
			set { mPackageName = value; }
		} string mPackageName;

		[TaskAttribute("timeout")]
		public int Timeout
		{
			get { return mTimeout; }
			set { mTimeout = value; }
		} protected int mTimeout = Int32.MaxValue;

		public UninstallAndroidPackage()
			: base()
		{
		}

		protected override void ExecuteTask()
		{
			if (String.IsNullOrEmpty(mPackageName))
			{
				throw new BuildException("Unable to get package name!");
			}

			try
			{
				// check to see if the package is installed first
				AdbTask task = AdbTask.Execute(Project, "shell pm list packages", mTimeout, 1, readstdout : true, checkemulator: true);
				if(task.StdOut.Contains(mPackageName))
				{
					string arguments = string.Concat("uninstall \"", mPackageName, "\"");
					AdbTask.Execute(Project, arguments, mTimeout, 1, checkemulator: true);	
				}
				else
				{
					Log.Status.WriteLine("Package [{0}] is not installed.  Skipping uninstall step.", mPackageName);
				}
			}
			catch (Exception e)
			{
				string message = "Unable to uninstall package: " + e.Message;
				Log.Error.WriteLine(message);
				throw new BuildException(message);
			}
		}

		/// <summary>A factory method to construct an UninstallAndroidPackage task and execute it</summary>
		public static void Execute(Project project, string packagename, int timeout)
		{
			UninstallAndroidPackage uninstall_task = new UninstallAndroidPackage();
			uninstall_task.Project = project;
			uninstall_task.PackageName = packagename;
			uninstall_task.Timeout = timeout;
			uninstall_task.Execute();
		}
	}
}
