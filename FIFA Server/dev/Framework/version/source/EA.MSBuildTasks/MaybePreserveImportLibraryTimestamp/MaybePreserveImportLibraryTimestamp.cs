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
using Microsoft.Build.Framework;
using Microsoft.Build.Utilities;
using PowerCode.BuildTools;

namespace EA.Framework.MsBuild.CppTasks
{
	public class MaybePreserveImportLibraryTimestamp : Task
	{
		[Required]
		public string ImportLibrary { get; set; }

		[Required]
		public string TrackerIntermediateDirectory { get; set; }

		public bool AllLoggingAsLow { get; set; }

		public override bool Execute()
		{
			Log.LogMessage(MessageImportance.Low, "EA.Framework.MsBuild: MaybePreserveImportLibraryTimestamp started \"" + ImportLibrary + "\" \"" + TrackerIntermediateDirectory + "\".");

			try
			{
				PreserveImportLibraryTimestamp();
			}
			catch (Exception e)
			{
				string importLibrary = ImportLibrary ?? "(null)";
				Log.LogMessage(AllLoggingAsLow ? MessageImportance.Low : MessageImportance.High,
						"EA.Framework.MsBuild: " + e.Message,
						importLibrary);
			}

			return true;
		}

		private void PreserveImportLibraryTimestamp()
		{
			// This is non-standard build functionality added to reduce unnecessary linking
			// The reasoning is as follows:
			// Import libraries consists of a long array of data for functions exported from a dynamic library
			// One of the data items is a timestamp, that gets updated at every build. However, the only thing that 
			// depending code really care about is if any method signatures have changed or if functions have been added or removed.
			//
			// So here we keep a copy of a previous version of the import library and uses it to compare to the newly created one.
			// If they are identical except for mismatching timestamps, we use the timestamp from the previous version on the newly 
			// created one. 
			// Since this makes the new library seems older than depending libraries, they don't relink when they don't have to.
			// We never modify the built import libraries, we only update the LastWriteTime of the file

			var impLib = ImportLibrary;

			if (String.IsNullOrEmpty(impLib))
			{
				return;
			}
			var original = Path.Combine(TrackerIntermediateDirectory, Path.GetFileName(ImportLibrary) + ".tracked");
			if (!File.Exists(impLib))
			{
				return;
			}

			if (!Directory.Exists(TrackerIntermediateDirectory))
				Directory.CreateDirectory(TrackerIntermediateDirectory);

			// Copy of the import library to use for future comparisons
			if (!File.Exists(original))
			{
				Log.LogMessage(MessageImportance.Low, "EA.Framework.MsBuild: Preserving import library.");
				File.Copy(impLib, original);
				return;
			}

			MessageImportance importance = AllLoggingAsLow ? MessageImportance.Low : MessageImportance.High;
			bool archiveAreEquivalentExceptTimestamps = false;

			using (var orig = new Archive(original))
			using (var candidate = new Archive(impLib))
			{
				archiveAreEquivalentExceptTimestamps = Archive.ArchiveTimestampInsensitiveComparer.Equals(orig, candidate);
			}
			if (archiveAreEquivalentExceptTimestamps)
			{
				Log.LogMessage(importance, "EA.Framework.MsBuild: Only timestamp differs for import lib \"{0}\", resetting timestamp to original.", impLib);
				// if only timestamp were different, we use the previous versions timestamp
				var origFileInfo = new FileInfo(original);
				File.SetLastWriteTime(impLib, origFileInfo.LastWriteTime);
			}
			else
			{
				Log.LogMessage(importance, "EA.Framework.MsBuild: Actual change detected in import lib \"{0}\"", impLib);
				// If something differs that is not timestamp related, we update the copy
				// used for timestamp comparison
				File.Copy(impLib, original, true);
			}
		}
	}

	public class FrazelPreserveImportLibraryTimestamp : Task
	{
		[Required]
		public string ImportLibraries { get; set; }

		public override bool Execute()
		{
			Log.LogMessage(MessageImportance.Low, "EA.Framework.MsBuild: FrazelPreserveImportLibraryTimestamp started \"" + ImportLibraries);

			try
			{
				PreserveImportLibraryTimestamp();
			}
			catch (Exception e)
			{
				Log.LogMessage(MessageImportance.Normal, "Error in FrazelPreserveImportLibraryTimestamp task: " + e.Message);
			}

			return true;
		}

		private void PreserveImportLibraryTimestamp()
		{
			// This is non-standard build functionality added to reduce unnecessary linking
			// The reasoning is as follows:
			// Import libraries consists of a long array of data for functions exported from a dynamic library
			// One of the data items is a timestamp, that gets updated at every build. However, the only thing that 
			// depending code really care about is if any method signatures have changed or if functions have been added or removed.
			//
			// So here we keep a copy of a previous version of the import library and uses it to compare to the newly created one.
			// If they are identical except for mismatching timestamps, we use the timestamp from the previous version on the newly 
			// created one. 
			// Since this makes the new library seems older than depending libraries, they don't relink when they don't have to.
			// We never modify the built import libraries, we only update the LastWriteTime of the file

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
				string original = importLib + ".backup";
				// Copy of the import library to use for future comparisons
				if (!File.Exists(original))
				{
					continue;
				}

				bool archiveAreEquivalentExceptTimestamps = false;
				using (var orig = new Archive(original))
				using (var candidate = new Archive(importLib))
				{
					archiveAreEquivalentExceptTimestamps = Archive.ArchiveTimestampInsensitiveComparer.Equals(orig, candidate);
				}
				if (archiveAreEquivalentExceptTimestamps)
				{
					Log.LogMessage(MessageImportance.Low, "FrazelPreserveImportLibraryTimestamp: Only timestamp differs for import lib \"{0}\", resetting timestamp to original.", importLib);
					// if only timestamp were different, we use the previous versions timestamp
					var origFileInfo = new FileInfo(original);
					File.SetLastWriteTime(importLib, origFileInfo.LastWriteTime);
				}
			}
		}
	}
}
