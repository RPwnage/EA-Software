// Originally based on NAnt - A .NET build tool
// Copyright (C) Electronic Arts Inc.
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
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Reflection;

using Newtonsoft.Json;

using NAnt.Core.Util;
using NAnt.Shared;

namespace NAnt.Core.Metrics
{
	public struct Event
	{
		public string Name;
		public DateTime TimeStamp;
		public Dictionary<string, string> Properties;

		public Event(string name, Dictionary<string, string> properties, DateTime time)
		{
			Name = name;
			TimeStamp = time;
			Properties = properties;
		}
	}

	public static class Telemetry
    {
		public static bool Enabled = true;

		private static ConcurrentBag<Event> s_events = new ConcurrentBag<Event>();

        public static void TrackInvocation(string [] commandArgs)
        {
			if (Enabled)
			{
				s_events.Add(new Event
				(
					"Invocation",
					new Dictionary<string, string>
					{
						{ "cmdargs", string.Join(" ", commandArgs) }
					},
					DateTime.Now
				));
			}
        }

		public static void TrackLoadPackage(string packageName, string config, string version, string driftVersion, Dictionary<string, string> extraData = null)
		{
			if (Enabled)
			{
				Event newEvent = new Event
				(
					"LoadPackage",
					new Dictionary<string, string>
					{
						{ "name", packageName },
						{ "config", config },
						{ "version", version },
						{ "driftversion", driftVersion ?? string.Empty }
					},
					DateTime.Now
				);
				if (extraData != null)
					extraData.ToList().ForEach(x => newEvent.Properties.Add(x.Key, x.Value));
				s_events.Add(newEvent);
			}
		}

        public static void Shutdown()
        {
			if (Enabled)
			{
				try
				{
					if (s_events.Any())
					{

						string targetDir = CopyTelemetryClientToTempDirectory();


						// write telemetry to json file
						string serializedEvents = JsonConvert.SerializeObject(s_events.ToArray());
						string serializedEventsFile = Path.Combine(targetDir, Process.GetCurrentProcess().Id.ToString());
						File.WriteAllText(serializedEventsFile, serializedEvents);

						UploadTelemetry(targetDir, serializedEventsFile);
					}
				}
				catch
				{
				}
			}
        }

		public static void UploadTelemetry(string targetDir, string serializedEventsFilePath)
		{
			// launch detached process to post telemetry
			string telemetryClient = Path.Combine(targetDir, "NAnt.TelemetryClient.exe");
			ProcessUtil.CreateDetachedProcess
			(
				program: telemetryClient,
				args: serializedEventsFilePath,
				workingdir: targetDir,
				environment: null,
				clearEnv: false
			);
		}

		public static string CopyTelemetryClientToTempDirectory()
		{
			// copy telemetry client to temp folder
			// Since these directories are used to launch detached processes and not automatically cleaned up, only keep a max number of them
			int maxNumberOfTempDirs = 30;
			string targetDir = string.Empty;

			string rootTempDir = Path.Combine(Path.GetTempPath(), "Nant.TelemetryClient");
			DirectoryInfo rootInfo = new DirectoryInfo(rootTempDir);
			DirectoryInfo[] subDirs;
			if (rootInfo.Exists)
				subDirs = rootInfo.GetDirectories();
			else
				subDirs = new DirectoryInfo[0];
			
			if (subDirs.Length < maxNumberOfTempDirs)
			{// haven't reached max number of directories yet
				var dirNames = subDirs.Select(subDir => subDir.Name).OrderBy(x => x);
				for (int i = 1; i <= maxNumberOfTempDirs; i++)
				{
					if (dirNames.Contains(i.ToString()) == false)
					{
						targetDir = Path.Combine(rootTempDir, i.ToString());
						break;
					}
				}
			}
			else
			{
				// keep the maxNumberOfTempDirs around while deleting anything on top of that
				var subDirsByLeastRecent = subDirs.OrderBy(subDir => subDir.LastWriteTime);
				foreach(DirectoryInfo subdir in subDirsByLeastRecent.Take(subDirsByLeastRecent.Count() - maxNumberOfTempDirs))
				{
					PathUtil.DeleteDirectory(subdir.FullName);
				}
				// get subdirs again after deleting the extras
				subDirs = rootInfo.GetDirectories();
				targetDir = subDirs.OrderBy(subDir => subDir.LastWriteTime).First().FullName;
				PathUtil.DeleteDirectory(targetDir);
			}
			
			string telemetryClientLocation = Path.Combine(Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location), "TelemetryClient");

			try
			{
				Directory.CreateDirectory(targetDir);
				foreach (string file in Directory.EnumerateFiles(telemetryClientLocation))
				{
					LinkOrCopyCommon.LinkOrCopy
					(
						file,
						Path.Combine(targetDir, Path.GetFileName(file)),
						OverwriteMode.OverwriteReadOnly,
						retries: 3,
						retryDelayMilliseconds: 500,
						methods: new LinkOrCopyMethod[] { LinkOrCopyMethod.Hardlink, LinkOrCopyMethod.Copy }
					);
				}
			}
			catch
			{
				// allow copy to fail and still attempt to post - most likely reason for copy to fail is
				// a parallel nant process attempting the same thing so we probably have a valid telemetry
				// client
			}
			return targetDir;
		}
    }
}