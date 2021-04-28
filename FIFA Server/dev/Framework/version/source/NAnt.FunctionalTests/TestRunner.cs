using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading;
using System.Xml;

using NAnt.Core;
using NAnt.Core.Logging;
using NAnt.Core.PackageCore;
using NAnt.Core.Util;

using EA.Eaconfig;

namespace RegressNant
{
	// TestRuner executes external test program and manages input/output streams and test results.
	class TestRunner : IDisposable
	{
		// Build farm has a 10 minute time out for all sub processes, 
		// which causes the entire test procedure to crash if one of the tests runs longer than 10 minutes.
		// We set the timeout to be slightly less than 10 minutes so we can continue to run the remaining tests.
		private const int TEST_TIMEOUT = 580000;
		private const int TIMEOUT_EXIT_CODE = -600; // some arbitrary number that should collide with any other exit code

		private bool m_disposed = false;

		private string m_globalMasterConfigPath;
		private MasterConfig m_globalMasterConfig;
		private Project project;

		private string m_workingDirectory = null;

		private int m_errorCount = 0;
		private int m_testCount = 0;
		private int m_knownBroken = 0;
		private int m_timedOut = 0;

		private OrderedDictionary<string, List<string>> m_failures = new OrderedDictionary<string, List<string>>();

		private StreamWriter m_nantLogFile = null;

		private string m_masterconfigLogFilePath;
		private string m_frameworkDir = null;
		private string m_buildRoot = null;
		private string m_nantConfig = null;
		private OpenCoverContext m_openCover = null;
		private Dictionary<string,string> m_startupProperties = new Dictionary<string,string>();

		private int m_counter = 0;
		private bool m_echo = false;
		private bool m_verbose = false;
		private bool m_unix = false;
		private bool m_retestFailures = false;
		private bool m_showOutput = false;

		private string m_frameworkErrorMessages = String.Empty;

		public TestRunner(
			string logFilePath,
			string frameworkDir,
			string buildRoot,
			bool retestFailures,
			bool verbose,
			bool unix,
			string config,
			OpenCoverContext openCover,
			Dictionary<string,string> startupProperties,
			bool showOutput)
		{
			if (logFilePath != null)
			{
				m_nantLogFile = new StreamWriter(logFilePath);
				if (Path.IsPathRooted(logFilePath))
				{
					m_masterconfigLogFilePath = Path.Combine(
						Path.GetDirectoryName(logFilePath),
						String.Format("masterconfigs_{0}", Path.GetFileName(logFilePath)));
				}
				else
				{
					m_masterconfigLogFilePath = string.Format("masterconfigs_{0}", logFilePath);
				}

				if (File.Exists(m_masterconfigLogFilePath))
				{
					File.Delete(m_masterconfigLogFilePath);
				}
			}

			m_frameworkDir = frameworkDir;
			m_buildRoot = buildRoot;
			m_verbose = verbose;
			m_unix = unix;
			m_retestFailures = retestFailures;
			m_nantConfig = config;
			m_openCover = openCover;
			m_startupProperties = startupProperties;
			m_showOutput = showOutput;
		}

		private string MasterConfigToString(MasterConfig masterconfig, bool hideFragments)
		{
			NAnt.Core.Writers.XmlFormat xmlFormat = new NAnt.Core.Writers.XmlFormat(
				identChar: ' ',
				identation: 2,
				newLineOnAttributes: false,
				encoding: new UTF8Encoding(false, false));

			using (var xmlWriter = new NAnt.Core.Writers.XmlWriter(xmlFormat))
			{
				// The default use case for masterconfigWriter is the -showmasterconfig option but this feature makes several
				// changes to the masterconfig that ultimate causes it to produce an invalid masterconfig.
				// In order to use this writer for merging we need to toggle certain options.
				MasterConfigWriter masterconfigWriter = new MasterConfigWriter(xmlWriter, masterconfig, project);
				// The masterconfig that is written already has all of the fragments merged together so we don't need the fragments section
				masterconfigWriter.HideFragments = hideFragments;
				// Resolve the various package roots to get their full roots since the relative paths will be broken
				masterconfigWriter.ResolveRoots = true;
				// Evaluate the master versions based on the startupProperties
				masterconfigWriter.EvaluateMasterVersions = true;
				// Need to preserve the global properties the way they are in the masterconfig
				masterconfigWriter.ResolveGlobalProperties = false;
				// Need to make sure that group types are not evaluated ahead of time
				masterconfigWriter.EvaluateGroupTypes = false;
				// Turn off attribute added to show which fragment a package/property came from
				masterconfigWriter.ShowFragmentSources = false;
				// Remove metapackage attributes
				masterconfigWriter.RemoveMetaPackages = true;

				masterconfigWriter.Write();
				xmlWriter.Flush();
				string generatedMasterconfig = new UTF8Encoding(false).GetString(xmlWriter.Internal.ToArray());

				return generatedMasterconfig;
			}
		}

		private void WriteMasterConfig(MasterConfig masterconfig, string path, bool hideFragments)
		{
			File.WriteAllText(path, MasterConfigToString(masterconfig, hideFragments));
		}

		public int RunTests(string tests, string masterconfig)
		{
			DateTime started = DateTime.Now;

			m_echo = true;
			WriteLineToLogFile("\n----------------------------------------------------------------------------");
			WriteLineToLogFile("Framework regression tests, started {0}.", started);
			WriteLineToLogFile("----------------------------------------------------------------------------");
			m_echo = false;

			WriteLineToMasterconfigLogFile(string.Format("---------- Framework regression tests, started {0} ----------\r\n", started));

			if (!String.IsNullOrEmpty(masterconfig))
			{
				project = new Project(Log.Silent);


				foreach (KeyValuePair<string, string> property in m_startupProperties)
				{
					project.Properties[property.Key] = property.Value;
				}

				m_globalMasterConfig = MasterConfig.Load(project, masterconfig);

				// Add startup properties to the masterconfig
				if (m_globalMasterConfig.GlobalProperties != null)
				{
					foreach (KeyValuePair<string, string> startupProperty in m_startupProperties)
					{
						bool propertyReplaced = false;
						foreach (MasterConfig.GlobalProperty globalProperty in m_globalMasterConfig.GlobalProperties)
						{
							if (globalProperty.Name == startupProperty.Key && globalProperty.Condition.IsNullOrEmpty())
							{
								globalProperty.Value = startupProperty.Value;
								propertyReplaced = true;
								break;
							}
						}
						if (!propertyReplaced)
						{
							m_globalMasterConfig.GlobalProperties.Add(new MasterConfig.GlobalProperty(startupProperty.Key, startupProperty.Value, String.Empty));
						}
					}
				}

				PackageMap.Init(project, m_globalMasterConfig, buildRootOverride: m_buildRoot);

				// Save a copy of the potentially changed masterconfig for future reference.
				m_globalMasterConfigPath = Path.Combine(m_buildRoot, "masterconfig", "global_masterconfig.xml");
				Directory.CreateDirectory(Path.GetDirectoryName(m_globalMasterConfigPath));
				WriteMasterConfig(m_globalMasterConfig, m_globalMasterConfigPath, true);
			}
			else
			{
				throw new ApplicationException("Masterconfig not provided.");
			}

			if (!m_retestFailures)
			{
				ClearSuccessTracker();
			}

			Parser parser = new Parser(tests);
			foreach (TestSet test in parser.Tests)
			{
				RunTestSet(test);
			}

			DateTime finished = DateTime.Now;
			TimeSpan elapsed = finished - started;

			WriteTotalResults(elapsed);

			return m_errorCount;
		}

		private void WriteTotalResults(TimeSpan elapsed)
		{
			m_echo = true;
			int passed = m_testCount - m_errorCount - m_knownBroken - m_timedOut;
			string details = "";
			if (m_knownBroken > 0)
			{
				details += string.Format(", {0} KNOWN BROKEN", m_knownBroken);
			}
			if (m_timedOut > 0)
			{
				details += string.Format(", {0} TIMED OUT", m_timedOut);
			}
			WriteLineToLogFile(string.Format("{0}TOTAL {1} tests: {2} PASSED, {3} FAILED{4}. DURATION {5:D2}:{6:D2}:{7:D2}{0}", Environment.NewLine, m_testCount, passed, m_errorCount, details, elapsed.Hours, elapsed.Minutes, elapsed.Seconds));
			if (m_failures.Any())
			{
				WriteLineToLogFile("FAILURES:");
				foreach (KeyValuePair<string, List<string>> failedSet in m_failures)
				{
					WriteLineToLogFile($"\t{failedSet.Key}");
					foreach (string failedTest in failedSet.Value)
					{
						WriteLineToLogFile($"\t\t{failedTest}");
					}
				}
			}
			m_echo = false;
		}

		private void ClearSuccessTracker()
		{
			string failureTrackerFilePath = GetSuccessTrackerPath();
			if (File.Exists(failureTrackerFilePath))
			{
				File.Delete(failureTrackerFilePath);
			}
		}

		private string GetSuccessTrackerPath()
		{
			return Path.Combine(m_buildRoot, "test_nant_passes.txt");
		}

		private void TrackTestSuccess(string testSetPackage, string test)
		{
			string failureTrackerFilePath = GetSuccessTrackerPath();
			File.AppendAllText(failureTrackerFilePath, testSetPackage + "::" + test + Environment.NewLine);
		}

		private string[] GetPreviousSuccesses(string testSetPackage)
		{
			string failureTrackerFilePath = GetSuccessTrackerPath();
			if (File.Exists(failureTrackerFilePath))
			{		
				string setPrefix = testSetPackage + "::";
				return File.ReadAllLines(failureTrackerFilePath).Where(line => line.StartsWith(setPrefix)).Select(line => line.Remove(0, setPrefix.Length)).ToArray();
			}
			return new string[] { };
		}

		static void WriteUnbuffered(string message, params object[] args)
		{
			Console.Write(message, args);
			Console.Out.Flush();
		}

		static void WriteLineUnbuffered(string message, params object[] args)
		{
			Console.WriteLine(message, args);
			Console.Out.Flush();
		}

		private void RunTestSet(TestSet testSet)
		{
			try
			{
				Console.WriteLine("\r\n--- testing: {0} ---", testSet.package);

				string testsRoot = Directory.GetCurrentDirectory();

				foreach (string test in GetTestsList(testSet.package, testSet.tests))
				{
					string testDir = Path.Combine(testSet.package, test);

					// Make sure that the directory still exists
					if (!Directory.Exists(testDir))
					{
						WriteLineUnbuffered("Directory {0} missing.  Skipping this test.", testDir);
						continue;
					}

					// Make sure that and there are indeed files in that directory.
					// If you deleted a test case on perforce, you could be left with
					// an empty directory on the computer.  We should still see a masterconfig
					// file in this directory.
					string localMasterconfigFilePath = Path.Combine(testDir, "masterconfig.xml");
					if (!File.Exists(localMasterconfigFilePath))
					{
						WriteLineUnbuffered("Directory {0} missing masterconfig.xml, test case deleted on perforce?  Skipping this test.", testDir);
						continue;
					}

					string buildRootTestDir = Path.Combine(m_buildRoot, "masterconfig", testDir);
					if (!Directory.Exists(buildRootTestDir))
					{
						Directory.CreateDirectory(buildRootTestDir);
					}

					// Replace the special variable ${test_location} which appears in some packageroots and fragments
					// TODO: check to see if using an xml reader/writer or some other approach is faster than this basic string replace
					string contents = File.ReadAllText(localMasterconfigFilePath);
					contents = contents.Replace("${test_location}", testsRoot);
					string copiedLocalMasterconfigPath = Path.Combine(buildRootTestDir, "local_masterconfig.xml");
					File.WriteAllText(copiedLocalMasterconfigPath, contents);

					CleanTestEnvironment(testDir);

					m_frameworkErrorMessages = String.Empty;
					m_workingDirectory = testDir;
					m_testCount++;

					string testCaseName = Path.GetFileName(testDir);
					WriteUnbuffered("EXECUTING {0,-35} ", testCaseName);

					string mergedMasterconfigPath = Path.Combine(m_buildRoot, "masterconfig", testDir, "masterconfig.xml");
					MergeWithGlobalMasterConfigFile(copiedLocalMasterconfigPath, mergedMasterconfigPath, testDir);

					string args = "";
					string program = "";
					BuildCommandLineArguments(Path.Combine(testsRoot, testDir, testSet.package + ".build"), out program, out args, mergedMasterconfigPath);

					int errorCode = ExecuteTest(program, args);
					if (errorCode != 0)
					{
						WriteUnbuffered("FAILURE RUNNING {0} {1} ", program, args);
					}

					CompleteTest(testSet.package, test, errorCode);
				}

				if (m_openCover != null)
				{
					ExecuteOpenCoverReportGenerator();
				}
			}
			catch (Exception ex)
			{
				Console.WriteLine("FAILED: internal error, details: " + ex.Message);
				m_errorCount++;
			}
		}

		public void CopyFileOverBase(string copyfile, string basefile)
		{
			if (File.Exists(basefile))
			{
				File.SetAttributes(basefile, FileAttributes.Normal);
				File.Delete(basefile);
			}
			if (File.Exists(copyfile))
			{
				string targetDir = Path.GetDirectoryName(basefile);
				if (!Directory.Exists(targetDir))
				{
					Directory.CreateDirectory(targetDir);
				}
				File.Copy(copyfile, basefile);
			}
		}

		public void MergeWithGlobalMasterConfigFile(string localMasterconfigFilePath, string mergedMasterconfigPath, string testDir)
		{
			// if there is no local masterconfig file we don't need to merge, just copy the global masterconfig
			if (!File.Exists(localMasterconfigFilePath))
			{
				CopyFileOverBase(m_globalMasterConfigPath, mergedMasterconfigPath);
				WriteLineToMasterconfigLogFile(string.Format("\r\n{0}\r\n{1}\r\n", testDir, "(no local masterconfig - using global masterconfig)"));
				return;
			}

			MasterConfig localMasterconfig = MasterConfig.Load(project, localMasterconfigFilePath);

			// load a new copy of the global masterconfig which is the base that we will make the local changes to
			MasterConfig mergedMasterconfig = MasterConfig.Load(project, m_globalMasterConfigPath);

			// add group types from the local masterconfig
			// add group types from the local masterconfig
			foreach (KeyValuePair<string, MasterConfig.GroupType> nameToGroup in localMasterconfig.GroupMap)
			{
				if (!mergedMasterconfig.GroupMap.TryGetValue(nameToGroup.Key, out MasterConfig.GroupType existingGroup))
				{
					mergedMasterconfig.GroupMap[nameToGroup.Key] = nameToGroup.Value;
				}
				else
				{
					existingGroup.Merge(nameToGroup.Value);
				}
			}
			foreach (MasterConfig.GroupType groupType in mergedMasterconfig.GroupMap.Values)
			{
				groupType.UpdateRefs(mergedMasterconfig.GroupMap);
			}

			// merge package versions
			foreach (MasterConfig.Package localMasterconfig_Package in localMasterconfig.Packages.Values)
			{
				// we don't want to take versions from fragments in the local masterconfig since the fragment conditions in the merged masterconfig will be evaluated later
				if (!localMasterconfig_Package.IsFromFragment)
				{
					if (mergedMasterconfig.Packages.ContainsKey(localMasterconfig_Package.Name))
					{
						// If the package is in the global masterconfig replace it with the version in the local masterconfig.
						// This will also move that package out of the group and remove any of the exceptions from the global masterconfig
						mergedMasterconfig.Packages[localMasterconfig_Package.Name] = localMasterconfig_Package;
					}
					else
					{
						mergedMasterconfig.Packages.Add(localMasterconfig_Package.Name, localMasterconfig_Package);
					}
				}
			}

			// replace the fragment blocks with the ones from the local masterconfig
			// only affects the fragments section when writing the new masterconfig, the original fragments were loaded and the settings resolved during the Load function
			mergedMasterconfig.FragmentDefinitions = localMasterconfig.FragmentDefinitions;

			// Override any global property/define with the value from the local masterconfig or add it if it is not in the global masterconfig
			if (localMasterconfig.GlobalProperties != null)
			{
				foreach (MasterConfig.GlobalProperty localMasterconfig_globalProperty in localMasterconfig.GlobalProperties)
				{
					bool propertyReplaced = false;
					if (mergedMasterconfig.GlobalProperties != null)
					{
						foreach (MasterConfig.GlobalProperty globalMasterconfig_GlobalProperty in mergedMasterconfig.GlobalProperties)
						{
							if (globalMasterconfig_GlobalProperty.ConditionsMatch(localMasterconfig_globalProperty))
							{
								globalMasterconfig_GlobalProperty.Value = localMasterconfig_globalProperty.Value;
								globalMasterconfig_GlobalProperty.UseXmlElement = localMasterconfig_globalProperty.UseXmlElement;
								propertyReplaced = true;
								break;
							}
						}
					}
					if (!propertyReplaced)
					{
						mergedMasterconfig.GlobalProperties.Add(localMasterconfig_globalProperty);
					}
				}
			}
			if (localMasterconfig.GlobalDefines != null)
			{
				foreach (MasterConfig.GlobalDefine localMasterconfig_globalDefine in localMasterconfig.GlobalDefines)
				{
					bool propertyReplaced = false;
					if (mergedMasterconfig.GlobalDefines != null)
					{
						foreach (MasterConfig.GlobalDefine globalMasterconfig_GlobalDefine in mergedMasterconfig.GlobalDefines)
						{
							if (globalMasterconfig_GlobalDefine.ConditionsMatch(localMasterconfig_globalDefine))
							{
								globalMasterconfig_GlobalDefine.Value = localMasterconfig_globalDefine.Value;
								propertyReplaced = true;
								break;
							}
						}
					}
					if (!propertyReplaced)
					{
						mergedMasterconfig.GlobalDefines.Add(localMasterconfig_globalDefine);
					}
				}
			}

			// append additional package roots and sort
			if (localMasterconfig.PackageRoots != null)
			{
				mergedMasterconfig.PackageRoots.AddRange(localMasterconfig.PackageRoots);
			}

			// merge warnings
			foreach (KeyValuePair<Log.WarningId, MasterConfig.WarningDefinition> warning in localMasterconfig.Warnings)
			{
				mergedMasterconfig.Warnings[warning.Key] = warning.Value;
			}

			// use the config element that is define in the local masterconfig
			// but for extensions we want the ones from the global masterconfig (we don't have tests that change these and we want to test with what the global masterconfig is using)
			Dictionary<string, string> includes = new Dictionary<string, string>();
			Dictionary<string, string> excludes = new Dictionary<string, string>();
			foreach (string cfgHost in MasterConfig.ConfigInfo.SupportedConfigHosts)
            {
				includes.Add(cfgHost, null);
				excludes.Add(cfgHost, null);
				if (!localMasterconfig.Config.IncludesByHost[cfgHost].IsNullOrEmpty())
					includes[cfgHost] = string.Join(" ", localMasterconfig.Config.IncludesByHost[cfgHost]);
				if (!localMasterconfig.Config.ExcludesByHost[cfgHost].IsNullOrEmpty())
					excludes[cfgHost] = string.Join(" ", localMasterconfig.Config.ExcludesByHost[cfgHost]);
			}
			mergedMasterconfig.Config = new MasterConfig.ConfigInfo(
				localMasterconfig.Config.Package, 
				localMasterconfig.Config.DefaultByHost["generic"],
				includes["generic"], excludes["generic"],
				mergedMasterconfig.Config.Extensions,
				localMasterconfig.Config.DefaultByHost["osx"],
				includes["osx"], excludes["osx"],
				localMasterconfig.Config.DefaultByHost["unix"],
				includes["unix"], excludes["unix"]
				);

			WriteMasterConfig(mergedMasterconfig, mergedMasterconfigPath, false);

			WriteLineToMasterconfigLogFile(string.Format("\r\n{0}\r\n{1}\r\n", testDir, MasterConfigToString(mergedMasterconfig, false)));
		}

		static string ExtractDefaultConfiguration(string masterconfigFile)
		{
			using (TextReader tr = new StreamReader(masterconfigFile))
			{
				using (XmlReader xr = XmlReader.Create(tr))
				{
					xr.MoveToContent();
					if (!xr.ReadToDescendant("config"))
					{
						return null;
					}

					string defaultConfiguration = xr.GetAttribute("default");

					return defaultConfiguration;
				}
			}
		}

		// Most tests provide a default configuration in the specified masterconfig
		// file. This is fine if the test is intended to only run on one platform
		// however if the test is to be run on multiple platforms it may be necessary
		// to translate this to an appropriate default.
		//
		// Specifically this is meant to make sure that tests run on OSX default to
		// osx-* configs as default and Linux tests default to unix64-* configs as
		// default.
		//
		// The reason this complexity is necessary, as opposed to just specifying
		// the host platform as the default configuration, is that there are tests
		// that specify a default configuration that is not the host platform, 
		// such as OSX-only tests that require a default configuration of iphone-*
		private string TranslateDefaultConfiguration(string masterconfigfile)
		{
			if (Environment.OSVersion.Platform == PlatformID.Unix)
			{
				bool isMac = NAnt.Core.Util.PlatformUtil.IsOSX;

				string defaultConfiguration = ExtractDefaultConfiguration(masterconfigfile);

				if (isMac)
				{
					if (defaultConfiguration != null && (defaultConfiguration.StartsWith("iphone") || defaultConfiguration.StartsWith("osx")))
					{
						return String.Empty;
					}

					return "osx-x64-clang-debug";
				}
				else
				{
					if (defaultConfiguration != null && defaultConfiguration.StartsWith("unix64"))
					{
						return String.Empty;
					}

					return "unix64-clang-debug";
				}
			}
			else
			{
				// Assume default platform is set correctly for Windows hosts.
				return String.Empty;
			}
		}

		private void BuildCommandLineArguments(string buildFile, out string program, out string args, string masterconfigfile)
		{
			args = " -buildfile:\"" + buildFile + "\"";
			if (!String.IsNullOrEmpty(m_nantConfig))
			{
				args += " -D:config=" + m_nantConfig;
			}
			else
			{
				string translatedConfig = TranslateDefaultConfiguration(masterconfigfile);
				if (!String.IsNullOrEmpty(translatedConfig))
				{
					args += " -D:config=" + translatedConfig;
				}
			}

			if (!String.IsNullOrEmpty(masterconfigfile))
			{
				args += " -masterconfigfile:\"" + Path.GetFullPath(masterconfigfile) + "\"";
			}
			if (!String.IsNullOrEmpty(m_buildRoot))
			{
				args += " -buildroot:\"" + m_buildRoot + "\"";
			}
			if (m_verbose)
			{
				args += " -verbose ";
			}

			foreach (var prop in m_startupProperties)
			{
				args += string.Format(" -D:{0}={1}",prop.Key,prop.Value);
			}

			program = (m_frameworkDir != null) ? Path.Combine(m_frameworkDir, "nant.exe") : "nant.exe";
			if (m_unix)
			{
				args = string.Format("{0} {1}", program, args);
				program = "mono";
			}

			if (m_openCover != null)
			{
				int reportNum = m_counter++;
				string reportPath = Path.Combine(m_openCover.OutputDir, string.Format("report_{0}.xml", reportNum.ToString("D4")));
				string openCoverArgs = string.Format("\"-target:{0}\" \"-targetargs:{1}\" -output:{2} -register", program, args, reportPath);

				if (m_workingDirectory != null)
				{
					openCoverArgs += string.Format(" \"-targetdir:{0}\"", Path.Combine(Environment.CurrentDirectory, m_workingDirectory));
				}

				program = m_openCover.Exe;
				args = openCoverArgs;
			}
		}

		private int ExecuteTest(string program, string args)
		{
			DateTime testCaseStartTime = DateTime.Now;

			int errorCode = ExecuteSync(program, args);

			DateTime testCaseStopTime = DateTime.Now;

			// Test case run time
			TimeSpan testCaseElapsedTime = testCaseStopTime - testCaseStartTime;
			WriteUnbuffered("({0:D2}:{1:D2}:{2:D2}) ", testCaseElapsedTime.Hours, testCaseElapsedTime.Minutes, testCaseElapsedTime.Seconds);

			return errorCode;
		}

		private void CompleteTest(string testSetPackage, string test, int errorCode)
		{
			bool isKnownBroken = File.Exists(Path.Combine(m_workingDirectory, "KnownBroken.txt"));

			if (isKnownBroken)
			{
				if (errorCode == 0)
				{
					WriteLineUnbuffered("[PASS] (KNOWN-BROKEN)");
				}
				else
				{
					string brokenReason = File.ReadAllText(Path.Combine(m_workingDirectory, "KnownBroken.txt"));
					WriteLineUnbuffered("[FAIL-KB]  Reason for fail: {0}", brokenReason);
				}

				if (errorCode > 0)
				{
					m_knownBroken++;
				}
			}
			else if (errorCode == TIMEOUT_EXIT_CODE)
			{
				// This 10 minute timeout is here because originally when we ran the tests on escher it would trigger a build farm timeout
				// after 10 minutes of no output and that would prevent tests after this one from running.
				WriteLineUnbuffered("[TIMEOUT] Exceeded test_nant's 10 minute timeout");
				m_timedOut++;
			}
			else
			{
				WriteLineUnbuffered("{0}", errorCode == 0 ? "[pass]" : "[FAIL]  !");

				if (errorCode > 0)
				{
					m_errorCount++;
					if (!m_failures.TryGetValue(testSetPackage, out List<string> packageFailures))
					{
						packageFailures = new List<string>();
						m_failures[testSetPackage] = packageFailures;
					}
					packageFailures.Add(test);
					Console.WriteLine(m_frameworkErrorMessages + Environment.NewLine);
				}
				else
				{
					TrackTestSuccess(testSetPackage,  test);
				}
			}
		}

		private void ExecuteOpenCoverReportGenerator()
		{
			// OpenCover generates coverage data but this is not readable
			// on its own. The ReportGenerator tool will merge the results
			// from all OpenCover invocations and generate a readable 
			// HTML report in the directory specified (ReportDir).
			StringBuilder interestingAssemblies = new StringBuilder();

			string[] dlls = Directory.GetFiles(m_frameworkDir, "*.dll");
			string[] exes = new string[] { "eapm.exe", "nant.exe" };

			// Reporting is limited to just the DLLs in Framework to keep out
			// noise for assemblies we do not care about (ie: 
			// FusionBuildHelpers). It would be nice to have coverage
			// information on, say, eaconfig but its debug information cannot
			// be resolved by OpenCover.
			foreach (string dll in dlls)
			{
				// SharpZipLib is largely external code; we would like our 
				// code coverage 
				if (dll.Contains("ICSharpCode.SharpZipLib"))
				{
					continue;
				}

				interestingAssemblies.Append('+').Append(Path.GetFileNameWithoutExtension(dll)).Append(';');
			}

			foreach (string exe in exes)
			{
				interestingAssemblies.Append('+').Append(Path.GetFileNameWithoutExtension(exe)).Append(';');
			}

			string reportGeneratorArgs = string.Format("\"-reports:{0}\" \"-targetdir:{1}\" -filters:{2}", Path.Combine(m_openCover.OutputDir, "*.xml"), m_openCover.ReportDir, interestingAssemblies.ToString());
			ExecuteSync(m_openCover.ReportGenerator, reportGeneratorArgs);

			Console.WriteLine();
			Console.WriteLine("OpenCover report generated to {0}", m_openCover.ReportDir);
		}

		private void CleanTestEnvironment(string test)
		{
			string testbuildroot = Path.Combine(m_buildRoot, test);
			string parentroot = Path.GetFullPath(Path.Combine(testbuildroot, @".."));

			try
			{
				var process = Process.GetCurrentProcess();

				if (process != null)
				{
					ProcessUtil.KillChildProcessTree(process.Id, process.ProcessName, killInstantly: true);
				}
			}
			catch (Exception e)
			{
				Console.WriteLine("Deleting process subtree failed: {0}", e.Message);
			}

			bool success = false;
			string exception_message = "";
			for (int i = 1; i <= 5; ++i)
			{
				try
				{
					if (Directory.Exists(parentroot))
					{
						PathUtil.DeleteDirectory(parentroot, true);
					}
					success = true;
					break;
				}
				catch (Exception ex)
				{
					exception_message = ex.ToString();
					Console.WriteLine("Delete directory failed, waiting 100ms then retrying ({0}/5)...", i);
					Thread.Sleep(100);
				}
			}
			if (!success)
			{
				Console.WriteLine("Could not delete directory {0} due to: {1}", parentroot, exception_message);

				ExecuteHandle(testbuildroot);
			}

			// something in the test_nant procedure is deleting the files in the android sdk packages
			// if the packages are missing a lot of files delete them before the test is run and the test will re-sync them
			DeleteAndroidSDKIfEmpty("AndroidSDK", 5);
			DeleteAndroidSDKIfEmpty("AndroidAPI", 5);
			DeleteAndroidSDKIfEmpty("AndroidPlatformTools", 5);
			DeleteAndroidSDKIfEmpty("AndroidBuildTools", 5);
		}

		public void DeleteAndroidSDKIfEmpty(string name, int expectedFiles)
		{
			try
			{
				TaskUtil.Dependent(project, name, initialize: false);
				string installedDir = Path.Combine(project.Properties["package." + name + ".dir"], "installed");
				if (Directory.GetFiles(installedDir, "*", SearchOption.AllDirectories).Count() < expectedFiles)
				{
					Console.WriteLine("[warning] SDK package '{0}' is missing files. Cleaning...", name);
					PathUtil.DeleteDirectory(project.Properties["package." + name + ".dir"]);
				}
			} catch { }
		}

		private void ExecuteHandle(string testbuildroot)
		{
			if (PlatformUtil.IsWindows)
			{
				// use handle.exe to find out if the directory is being locked and by what
				// note: handle.exe requires administrative privileges.
				Process tool = new Process();
				tool.StartInfo.FileName = "handle.exe";
				tool.StartInfo.Arguments = testbuildroot + " /accepteula";
				tool.StartInfo.UseShellExecute = false;
				tool.StartInfo.RedirectStandardOutput = true;
				tool.Start();
				tool.WaitForExit();
				string outputTool = tool.StandardOutput.ReadToEnd();

				Console.WriteLine("Directory Lock Info: {0}", outputTool);
			}
		}

		static void WriteLineToLogFile(StreamWriter writer, string str)
		{
			if (writer != null)
			{
				lock (writer)
				{
					writer.WriteLine(str);
				}
			}
		}

		static void WriteLineToLogFile(StreamWriter writer, string fmt, params object[] par)
		{
			if (writer != null)
			{
				lock (writer)
				{
					writer.WriteLine(fmt, par);
				}
			}
		}

		public void WriteLineToLogFile(string str)
		{
			WriteLineToLogFile(m_nantLogFile, str);

			if (m_nantLogFile == null || m_echo == true)
			{
				Console.WriteLine(str);
			}
		}

		public void WriteLineToLogFile(string fmt, params object[] par)
		{
			WriteLineToLogFile(m_nantLogFile, fmt, par);

			if (m_nantLogFile == null || m_echo == true)
			{
				System.Console.WriteLine(fmt, par);
			}
		}

		public void WriteLineToMasterconfigLogFile(string str)
		{
			using (StreamWriter w = File.AppendText(m_masterconfigLogFilePath))
			{
				w.WriteLine(str);
			}
		}

		private string[] GetTestsList(string package, string[] filters)
		{
			// filters should be null or array of directory patterns to match
			if (filters == null)
			{
				filters = new string[] { "*" };
			}

			// get exclude tests if running in retest mode
			string[] excludeTests = m_retestFailures ? GetPreviousSuccesses(package) : new string[] { };

			// scan for directories matching filters excluding previous successes if applicable
			List<string> results = new List<string>();
			foreach (string filter in filters)
			{
				string[] tests = Directory.GetDirectories(package, filter.Trim(), SearchOption.TopDirectoryOnly).Select(testDir => testDir.Remove(0, package.Length + 1)).ToArray();
				IEnumerable<string> unexcludedTests = tests.Except(excludeTests);
				if (tests.Length == 0)
				{
					Console.WriteLine("*** WARNING: no tests found for {0}[{1}]", package, filter);
				}
				else if (!unexcludedTests.Any())
				{
					Console.WriteLine("*** WARNING: no retests required for {0}[{1}]", package, filter);
				}
				else
				{
					results.AddRange(unexcludedTests);
				}
			}

			return results.ToArray();
		}

		void LogFileWriter(object sender, DataReceivedEventArgs e, StreamWriter writer)
		{
			if (!String.IsNullOrEmpty(e.Data))
			{
				WriteLineToLogFile(writer, e.Data);

				if (m_showOutput)
				{
					Console.WriteLine(e.Data);
				}

				// builds an error string that gets printed after failing unit tests
				if (e.Data.Contains("[error]") || e.Data.Contains("[regress_nant]"))
				{
					m_frameworkErrorMessages += Environment.NewLine + e.Data;
				}
			}
		}

		static string GetLogFileName()
		{
			string tempDirectory = Path.GetTempPath();
			int processId = Process.GetCurrentProcess().Id;
			string randomId = Path.GetRandomFileName();

			return Path.Combine(tempDirectory, string.Format("nant.{0}.{1}.log", processId, randomId));
		}
				
		// Executes Returns process result
		// [2016/06/27] This method has been changed to only emit output to the top-level log
		// in the case of the spawned child process failing. This is to keep log file size down
		// as the 200-someodd tests generaet a lot of verbose output. This can make it difficult
		// to view the log on the web dashboard.
		public int ExecuteSync(String program, String arguments)
		{
			int returnCode = 2;
			string logFileName = GetLogFileName();

			try
			{
				using (StreamWriter logWriter = new StreamWriter(logFileName))
				using (Process process = new Process())
				{
					WriteLineToLogFile(logWriter, "");
					WriteLineToLogFile(logWriter, "> {0} {1}{2}", program, arguments, Environment.NewLine);
					if (m_showOutput)
					{
						WriteLineUnbuffered("> {0} {1}", program, arguments);
					}
					string processName = "unknown";

					try
					{

						process.StartInfo.FileName = program;
						process.StartInfo.Arguments = arguments;
						if (m_workingDirectory != null)
						{
							process.StartInfo.WorkingDirectory = m_workingDirectory;
						}

						process.StartInfo.RedirectStandardOutput = true;
						process.StartInfo.RedirectStandardError = true;
						process.StartInfo.UseShellExecute = false;
						process.StartInfo.CreateNoWindow = true;

						// Set event handler to asynchronously read the test process output.
						process.OutputDataReceived += new DataReceivedEventHandler((obj, e) => { LogFileWriter(obj, e, logWriter); });
						process.ErrorDataReceived += new DataReceivedEventHandler((obj, e) => { LogFileWriter(obj, e, logWriter); });

						process.Start();

						// Start the asynchronous read of the process's output stream.
						process.BeginOutputReadLine();
						process.BeginErrorReadLine();

						processName = process.ProcessName;

						if (!process.WaitForExit(TEST_TIMEOUT))
						{
							process.Kill();
							returnCode = TIMEOUT_EXIT_CODE;
						}
						else
						{
							returnCode = process.ExitCode;
						}
					}
					finally
					{
						if (m_unix == false)
						{
							ProcessUtil.KillChildProcessTree(process.Id, processName, true);
						}
					}

					process.Close();
				}
			}
			catch (Exception ex)
			{
				Console.WriteLine("FAILED: '" + program + " " + arguments + "', details: " + ex.Message);
			}
			finally
			{
				// Slurp the log file and add it to the master log in the case of failure.
				if (returnCode != 0 && File.Exists(logFileName))
				{
					string logText = File.ReadAllText(logFileName);
					m_nantLogFile.Write(logText);
					File.Delete(logFileName);
				}
			}

			// The process success/error message is still emitted  for general progress information.
			WriteLineToLogFile("{0}TEST {1} {2}", Environment.NewLine, Path.GetFileName(m_workingDirectory), returnCode == 0 ? "SUCCEEDED" : "FAILED");

			return returnCode;
		}

		protected virtual void Dispose(bool userInvoked)
		{
			// don't dispose more than once
			if (!m_disposed)
			{
				if (userInvoked)
				{
					if (m_nantLogFile != null)
					{
						m_nantLogFile.Dispose();
					}
				}
				m_disposed = true;
			}
		}

		public void Dispose()
		{
			Dispose(true);
			// mark this object finalized (there is no need to call the finalizer on this instance anymore)
			GC.SuppressFinalize(this);
		}

		~TestRunner()
		{
			Dispose(false);
		}

		class TestSet
		{
			public string package;
			public string[] tests;
		}

		private class Parser
		{
			private enum State { NONE, PACKAGE, TEST };
			private List<TestSet> testSetList;

			public TestSet[] Tests
			{
				get { return testSetList.ToArray(); }
			}

			public Parser(string testString)
			{
				testSetList = new List<TestSet>();

				Parse(testString, 0);
			}

			private int Parse(string testString, int index)
			{
				int i, start = 0;

				if (String.IsNullOrEmpty(testString))
					throw new ArgumentNullException("INTERNAL ERROR null or empty string passed to argument string parser");

				start = index = SkipWhiteSpace(testString, index);

				for (i = index; i < testString.Length; i++)
				{
					if (testString[i] == '[' || testString[i] == ';' || i == testString.Length - 1)
					{
						int len = i - start + 1;

						if (testString[i] == '[' || testString[i] == ';')
						{
							len--;
						}

						if (len > 0)
						{
							TestSet testSet = new TestSet();
							testSet.package = testString.Substring(start, len).Trim();

							i = ParseTestNames(testSet, testString, i);

							start = i = SkipWhiteSpace(testString, i);

							testSetList.Add(testSet);
						}
						else
						{
							throw new ApplicationException("Invalid '-tests:' parameter value. Looks like parameter value is an empty string");
						}
					}

				}

				return i;
			}

			private int ParseTestNames(TestSet testSet, string testString, int index)
			{
				int i = SkipWhiteSpace(testString, index);

				if (i < testString.Length && testString[i] == '[')
				{
					for (i = index + 1; i < testString.Length; i++)
					{
						if (testString[i] == ']')
						{
							string tests = testString.Substring(index + 1, i - index - 1).Trim();

							Char[] SEPARATOR = new Char[] { ';' };

							testSet.tests = String.IsNullOrEmpty(tests) ? new string[] { "*" } : tests.Split(SEPARATOR);

							i++;

							break;
						}

						if (i == testString.Length)
						{
							throw new ApplicationException("Invalid -tests: parameter value. Missing closing bracket ']'");
						}
					}

				}
				return i;
			}

			private static int SkipWhiteSpace(string str, int index)
			{
				while (index < str.Length && (Char.IsWhiteSpace(str, index) || str[index] == ';'))
					++index;
				return index;
			}
		}
	}
}
