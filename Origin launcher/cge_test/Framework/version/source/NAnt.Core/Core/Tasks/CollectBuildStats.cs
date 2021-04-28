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
using System.IO;
using System.Collections.Concurrent;
using System.Diagnostics;
using System.Linq;
using System.Xml;
using NAnt.Core.Attributes;
using NAnt.Core.Logging;
using NAnt.Core.Util;
using NAnt.Core.Writers;

namespace NAnt.Core.Tasks
{

	/// <summary>Collects statistics for a group of tasks wrapped by this element</summary>
	[TaskName("collectbuildstats")]
	public class CollectBuildStats : TaskContainer
	{
		internal static ConcurrentDictionary<string, BuildStats> _BuildStats = new ConcurrentDictionary<string, BuildStats>();

		/// <summary>Unique label name for this set of build stats.
		/// </summary>
		[TaskAttribute("label", Required = true)]
		public string Label { get; set; } = String.Empty;

		protected override void ExecuteTask()
		{
			var statCollector = new StatColector();

			if (FailOnError)
			{
				try
				{
					base.ExecuteTask();

					if (TaskSuccess)
					{
						statCollector.SetSuccess();
					}
				}
				finally
				{
					if (!_BuildStats.TryAdd(Label, statCollector.GetBuildStats()))
					{
						Log.Warning.WriteLine("Build Statistics with lablel '{0}' already exist", Label);
					}

				}
			}
			else
			{
				try
				{
					base.ExecuteTask();

					if (TaskSuccess)
					{
						statCollector.SetSuccess();
					}
				}
				catch (Exception)
				{
				}

				if (!_BuildStats.TryAdd(Label, statCollector.GetBuildStats()))
				{
					Log.Warning.WriteLine("Build Statistics with lablel '{0}' already exist", Label);
				}
			}

		}

		public static void Execute(Project project, string label, Action action, bool failonerror = true)
		{
			var statCollector = new StatColector();
			if (failonerror)
			{
				try
				{
					action();

					statCollector.SetSuccess();
				}
				finally
				{
					if (!_BuildStats.TryAdd(label, statCollector.GetBuildStats()))
					{
						project.Log.Warning.WriteLine("Build Statistics with lablel '{0}' already exist", label);
					}

				}
			}
			else
			{
				try
				{
					action();

					statCollector.SetSuccess();
				}
				catch (Exception)
				{
				}
				if (!_BuildStats.TryAdd(label, statCollector.GetBuildStats()))
				{
					project.Log.Warning.WriteLine("Build Statistics with lablel '{0}' already exist", label);
				}
			}
		}

		class StatColector
		{
			internal StatColector()
			{
				_status = BuildStats.BuildStatus.FAILED;
				_sw.Start();
			}

			internal void SetSuccess()
			{
				_status = BuildStats.BuildStatus.SUCCESS;
			}

			internal BuildStats GetBuildStats()
			{
				_sw.Stop();
				return new BuildStats(_sw.Elapsed, _status);
			}

			private readonly Stopwatch _sw = new Stopwatch();

			private BuildStats.BuildStatus _status;
		}

		public class BuildStats
		{
			public enum BuildStatus { SUCCESS, FAILED }

			public BuildStats(TimeSpan elapsed, BuildStatus status)
			{
				Elapsed = elapsed;
				Status = status;
			}
			public readonly TimeSpan Elapsed;
			public readonly BuildStatus Status;
		}
	}

		/// <summary>Prints colected build statistics</summary>
	[TaskName("printbuildstats")]
	public class PrintBuildStats : TaskContainer
	{
		private string _extraHeader;

		/// <summary>Package list that was in this build.
		/// </summary>
		[TaskAttribute("BuildPackages", Required = false)]
		public string BuildPackages { get; set; } = String.Empty;

		/// <summary>A version string for this build.
		/// </summary>
		[TaskAttribute("BuildVersion", Required = false)]
		public string BuildVersion { get; set; } = String.Empty;

		public struct BuildMachineInfo
		{
			public string machineName;
			public string osVersion;
			public bool is64Bit;
			public ulong ram;
			public uint processors;
			public uint cores;
			public uint logicalCores;
			public uint maxclockspeed;
		}

		/// <summary>Initializes the task.</summary>
		protected override void InitializeTask(XmlNode taskNode) 
		{
				if (!String.IsNullOrEmpty(taskNode.InnerText)) 
				{
					_extraHeader = Project.ExpandProperties(taskNode.InnerText);
				}
		}

		protected override void ExecuteTask()
		{
			Execute(this, Log, _extraHeader);
		}

		public static void Execute(PrintBuildStats inst, Log log, string extraHeader= null)
		{

			using (var writer = new MakeWriter())
			{
				DateTime buildStatCollectTime = DateTime.Now;

				writer.FileName = Path.Combine(Environment.CurrentDirectory, "BuildStats.txt");
				log.Status.WriteLine();
				log.Status.WriteLine();
				log.Status.WriteLine("-----------------------------------------------------------------");
				log.Status.WriteLine("---      Collected build statistics. {0}", buildStatCollectTime.ToShortDateString());
				log.Status.WriteLine("-----------------------------------------------------------------");
				log.Status.WriteLine();
				writer.WriteLine("Collected build statistics. {0}", buildStatCollectTime.ToShortDateString());

				string buildpackages = inst.BuildPackages;
				string buildversion = inst.BuildVersion;

				BuildMachineInfo machineInfo = new BuildMachineInfo();
				GetBuildMachineInfo(out machineInfo);
				WriteSystemStats(log, writer, machineInfo);
				bool buildpackagesExtractedFromExtraHeader = false;
				bool buildversionExtractedFromExtraHeader = false;
				foreach (var line in extraHeader.LinesToArray())
				{
					log.Status.WriteLine(line);
					writer.WriteLine(line);

					// We parse these lines to get the info for backward compatibility.  The prefer method now is to use the task's attributes.
					System.Text.RegularExpressions.Match pkgMatch = System.Text.RegularExpressions.Regex.Match(line, @"Licensee packages:\s*(?<pkgNames>.+)");
					if (pkgMatch.Success)
					{
						buildpackages = pkgMatch.Groups["pkgNames"].Value.Trim(new char[]{' ', '\'', '\t'});
						buildpackagesExtractedFromExtraHeader = true;
					}

					System.Text.RegularExpressions.Match verMatch = System.Text.RegularExpressions.Regex.Match(line, @"Perforce Version:\s*(?<verStr>.+)");
					if (verMatch.Success)
					{
						buildversion = verMatch.Groups["verStr"].Value.Trim(new char[] { ' ', '\'', '\t' });
						buildversionExtractedFromExtraHeader = true;
					}
				}
				if (!buildpackagesExtractedFromExtraHeader)
				{
					log.Status.WriteLine("Packages:      '{0}'", buildpackages);
					writer.WriteLine("Packages:      '{0}'", buildpackages);
				}
				if (!buildversionExtractedFromExtraHeader)
				{
					log.Status.WriteLine("Version:       '{0}'", buildversion);
					writer.WriteLine("Version:       '{0}'", buildversion);
				}
				log.Status.WriteLine();
				writer.WriteLine();

				if (String.IsNullOrEmpty(buildpackages))
					buildpackages = inst.Properties.GetPropertyOrDefault("package.name", "Unknown");

				if (String.IsNullOrEmpty(buildversion))
					buildversion = inst.Properties.GetPropertyOrDefault("package.version", "Unknown");

				System.Text.StringBuilder dbErrMsg = new System.Text.StringBuilder();

				if (!CollectBuildStats._BuildStats.IsNullOrEmpty())
				{

					int maxLabelLen = CollectBuildStats._BuildStats.Max(e => e.Key.Length);
					int maxStatusLen = Enum.GetNames(typeof(CollectBuildStats.BuildStats.BuildStatus)).Max(name => name.Length);

					foreach (var entry in CollectBuildStats._BuildStats.OrderBy(e => e.Key))
					{
						log.Status.WriteLine("{0} [{1}]     time: {2}", entry.Key.PadRight(maxLabelLen), entry.Value.Status.ToString().PadRight(maxStatusLen), FormatTime(entry.Value.Elapsed));
						writer.WriteLine("{0}\t;\t{1}\t;\t{2}", entry.Key.PadRight(maxLabelLen), entry.Value.Status.ToString().PadRight(maxStatusLen), FormatTime(entry.Value.Elapsed));					
					}
				}
				else
				{
					log.Status.WriteLine("No data collected.");
					writer.WriteLine("No data collected");
				}

				log.Status.WriteLine();
				log.Status.WriteLine("-----------------------------------------------------------------");
				log.Status.WriteLine();
				log.Status.WriteLine();
			}
		}

		private static void GetBuildMachineInfo(out BuildMachineInfo machineInfo)
		{
			BuildMachineInfo info = new BuildMachineInfo();
			GetProcessor(out info.processors, out info.cores, out info.logicalCores, out info.maxclockspeed);
			info.osVersion = Environment.OSVersion.ToString();
			info.is64Bit = Environment.Is64BitOperatingSystem;
			info.ram = GetTotalRam();
			info.machineName = System.Environment.MachineName;
			string hostname = System.Net.Dns.GetHostName();
			if (!String.IsNullOrEmpty(hostname))
			{
				System.Net.IPHostEntry hostinfo = System.Net.Dns.GetHostEntry(hostname);
				if (hostinfo != null)
				{
					if (!String.IsNullOrEmpty(hostinfo.HostName))
					{
						info.machineName = hostinfo.HostName;
					}
				}
			}
			machineInfo = info;
		}

		private static void WriteSystemStats(Log log, IMakeWriter writer, BuildMachineInfo info)
		{
			log.Status.WriteLine("CommandLine: {0}", System.Environment.CommandLine);
			log.Status.WriteLine("{0}", info.osVersion);
			log.Status.WriteLine("{0}", info.is64Bit ? "64 bit OS" : "32 bit OS");
			log.Status.WriteLine("RAM:           {0} GB", info.ram);
			log.Status.WriteLine("Processors:    {0}", info.processors);
			log.Status.WriteLine("Cores:         {0}", info.cores);
			log.Status.WriteLine("Logical Cores: {0}", info.logicalCores);
			log.Status.WriteLine("MaxClockSpeed: {0:f2} GHz", info.maxclockspeed / 1000.0);

			writer.WriteLine("CommandLine: {0}", System.Environment.CommandLine);
			writer.WriteLine("{0}", info.osVersion);
			writer.WriteLine("{0}", info.is64Bit ? "64 bit OS" : "32 bit OS");
			writer.WriteLine("RAM:           {0} GB", info.ram);
			writer.WriteLine("Processors:    {0}", info.processors);
			writer.WriteLine("Cores:         {0}", info.cores);
			writer.WriteLine("Logical Cores: {0}", info.logicalCores);
			writer.WriteLine("MaxClockSpeed: {0:f2} GHz", info.maxclockspeed / 1000.0);
		}

		private static UInt64 GetTotalRam()
		{
			if(PlatformUtil.IsWindows)
			{
				return GetTotalRamWindows();
			}
			return 0;
		}


		private static void GetProcessor(out uint processors, out uint cores, out uint logicalCores, out uint maxclockspeed)
		{
			logicalCores = (uint)Environment.ProcessorCount;
			processors = logicalCores;
			cores = logicalCores;
			maxclockspeed = 0;

			if (PlatformUtil.IsWindows)
			{
				GetProcessorsWindows(out processors, out cores, out maxclockspeed);
			}
		}

		private static void GetProcessorsWindows(out uint processors, out uint cores, out uint maxclockspeed)
		{
			processors = 0;
			cores = 0;
			maxclockspeed = 0;

			foreach (var item in new System.Management.ManagementObjectSearcher("Select * from Win32_ComputerSystem").Get())
			{
				processors += Convert.ToUInt32(item["NumberOfProcessors"]);
			}

			foreach (var item in new System.Management.ManagementObjectSearcher("Select * from Win32_Processor").Get())
			{
				cores += Convert.ToUInt32(item["NumberOfCores"]);
			}

			foreach (var item in new System.Management.ManagementObjectSearcher("Select MaxClockSpeed from Win32_Processor").Get())
			{
				var clockSpeed = (uint)item["MaxClockSpeed"];
				maxclockspeed = Math.Max(clockSpeed, maxclockspeed);
			}
		}

		private static UInt64 GetTotalRamWindows()
		{
			UInt64 sizeinGB = 0;

			var searcher = new System.Management.ManagementObjectSearcher("SELECT Capacity FROM Win32_PhysicalMemory");
			foreach (var WniPART in searcher.Get())
			{
				UInt64 sizeinB = Convert.ToUInt64(WniPART.Properties["Capacity"].Value);
				sizeinGB += ((sizeinB / 1024) / 1024) /1024 ;
			}
			return sizeinGB;
		}

		private static string FormatTime(TimeSpan elapsed)
		{
			return String.Format("({0:D2}:{1:D2}:{2:D2})", elapsed.Hours, elapsed.Minutes, elapsed.Seconds);
		}

	}

}
