using System;
using System.IO;
using System.Security.Principal;
using Microsoft.Build.Framework;
using Microsoft.Build.Utilities;
using PowerCode.BuildTools;

namespace EA.Framework.MsBuild.CppTasks
{
	public class BackupImportLibrary : Task
	{
		[Required]
		public string ImportLibraries { get; set; }

		public override bool Execute()
		{
			Log.LogMessage(MessageImportance.Low, "EA.Framework.MsBuild: BackupImportLibrary started \"" + ImportLibraries);

			WindowsIdentity identity = WindowsIdentity.GetCurrent();
			WindowsPrincipal principal = new WindowsPrincipal(identity);
			bool isAdmin = principal.IsInRole(WindowsBuiltInRole.Administrator);

			try
			{
				PreserveImportLibraryTimestamp();
			}
			catch (Exception e)
			{
				Log.LogMessage(MessageImportance.Normal, "Error in BackupImportLibrary task: " + e.Message);
			}

			return true;
		}

		private void PreserveImportLibraryTimestamp()
		{

			if (String.IsNullOrEmpty(ImportLibraries))
			{
				return;
			}
			foreach (string importLib in ImportLibraries.Split(';'))
			{
				if (!File.Exists(importLib))
				{
					continue;
				}
				string backupImportLib = importLib + ".backup";
				File.Copy(importLib, backupImportLib, true);
			}
		}
	}
}
