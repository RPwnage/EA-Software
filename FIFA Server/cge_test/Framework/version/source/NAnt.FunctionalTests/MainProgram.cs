using System;
using System.Collections.Generic;

using System.Diagnostics;
using System.IO;

namespace RegressNant
{
	public class OpenCoverContext
	{
		public string Exe;
		public string OutputDir;
		public string ReportGenerator;
		public string ReportDir;

		public OpenCoverContext Validate()
		{
			if (Exe == null && OutputDir == null && ReportDir == null && ReportGenerator == null)
			{
				return null;
			}

			if (Exe == null || OutputDir == null || ReportDir == null || ReportGenerator == null)
			{
				throw new Exception("Partial open cover settings passed! Requires -opencover:, -opencoveroutputdir:, -reportgenerator:, -opencoverreportdir:");
			}

			return this;
		}
	}

	class MainProgram
	{
		private static ConsoleCtrl m_consoleControl;

		private static void ConsoleEventHandler(ConsoleCtrl.ConsoleEvent consoleEvent)
		{
			// allow nant to run as a service
			if (consoleEvent == ConsoleCtrl.ConsoleEvent.CTRL_LOGOFF)
			{
				return;
			}

			try
			{
				var process = Process.GetCurrentProcess();

				if (process != null)
				{
					ProcessUtil.KillChildProcessTree(process.Id, process.ProcessName, killInstantly: true);
					process.Kill();
				}
			}
			catch (System.Exception /*e*/) { }
		}

		static int Main(string[] args)
		{
			if (ProcessUtil.IsWindows)
			{
				m_consoleControl = new ConsoleCtrl();
				m_consoleControl.ControlEvent += new ConsoleCtrl.ControlEventHandler(ConsoleEventHandler);
			}

			MainProgram program = new MainProgram();

			return program.Run(args);
		}

		class ReturnCode
		{
			public static readonly int Ok = 0;
			public static readonly int Error = 1;
		}

		class Options
		{
			public static readonly string TestList = "-tests:";
            public static readonly string ReTestFailures = "-retestfailures";
			public static readonly string MasterConfigFile = "-masterconfigfile:";
			public static readonly string Logfile = "-logfile:";
			public static readonly string Help = "-help";
			public static readonly string FrameworkDir = "-frameworkdir:";
			public static readonly string BuildRoot = "-buildroot:";
			public static readonly string Verbose = "-verbose";
			public static readonly string Unix = "-unix";
			public static readonly string Config = "-D:config=";
			public static readonly string OpenCover = "-opencover:";
			public static readonly string OpenCoverOutputDir = "-opencoveroutputdir:";
			public static readonly string ReportGenerator = "-reportgenerator:";
			public static readonly string OpenCoverReportDir = "-opencoverreportdir:";
			public static readonly string StartupProperties = "-startup_properties:";
			public static readonly string ShowOutput = "-show_output";
		}

		public bool InputHelp = false;
		public string TestString = null;
		public string MasterConfigFilePath = null;
		public string LogFilePath = null;
		public string FrameworkDir = null;
		public string BuildRoot = null;
		public string Config = null;
		public bool Verbose = false;
        public bool ReTestFailures = false;
        public bool Unix = false;
		public string UnlockerDir = null;
		public bool UnlockerDebug = false;
		public OpenCoverContext OpenCover = new OpenCoverContext();
		public Dictionary<string,string> StartupProperties = new Dictionary<string,string>();
		public bool ShowOutput = false;

		void ParseCommandLine(string[] args)
		{
			StartupProperties.Clear();
			foreach (string arg in args)
			{
				if (arg.StartsWith(Options.TestList))
				{
					if (TestString != null)
					{
						throw new ApplicationException("Test list has already been specified.");
					}
					try
					{
						TestString = arg.Substring(Options.TestList.Length);

						if (TestString.Length == 0)
						{
							throw new ApplicationException("Invalid '-tests:' parameter value. Looks like parameter value is an empty string");
						}
					}
					catch
					{
						throw new ApplicationException("Invalid test list specified.");
					}

				}
				else if (arg.StartsWith(Options.OpenCover))
				{
					OpenCover.Exe = arg.Substring(Options.OpenCover.Length);
				}
				else if (arg.StartsWith(Options.OpenCoverOutputDir))
				{
					OpenCover.OutputDir = arg.Substring(Options.OpenCoverOutputDir.Length);
				}
				else if (arg.StartsWith(Options.ReportGenerator))
				{
					OpenCover.ReportGenerator = arg.Substring(Options.ReportGenerator.Length);
				}
				else if (arg.StartsWith(Options.OpenCoverReportDir))
				{
					OpenCover.ReportDir = arg.Substring(Options.OpenCoverReportDir.Length);
				}
				else if (arg.StartsWith(Options.MasterConfigFile))
				{
					if (MasterConfigFilePath != null)
					{
						throw new ApplicationException("MasterConfigFile has already been specified.");
					}
					try
					{
						MasterConfigFilePath = arg.Substring(Options.MasterConfigFile.Length);

						if (MasterConfigFilePath.Length == 0)
						{
							throw new ApplicationException("Masterconfig parameter value is empty");
						}
					}
					catch
					{
						throw new ApplicationException("Invalid MasterConfigFile specified.");
					}

				}
				else if (arg.StartsWith(Options.Logfile))
				{
					string msg = null;

					LogFilePath = arg.Substring(Options.Logfile.Length);

					if (String.IsNullOrEmpty(LogFilePath))
					{
						msg = "Log file unspecified.";
						throw new ApplicationException(msg);
					}
					string dirName = Path.GetDirectoryName(LogFilePath);
					if (!String.IsNullOrEmpty(dirName) && !Directory.Exists(dirName))
					{
						// try to create log file dir:
						Directory.CreateDirectory(dirName);

						if (!Directory.Exists(dirName))
						{
							msg = String.Format("Given log file path does not exist: {0}", dirName);
							throw new ApplicationException(msg);
						}
					}
				}
				else if (arg.StartsWith(Options.FrameworkDir))
				{
					string msg = null;

					FrameworkDir = arg.Substring(Options.FrameworkDir.Length);

					if (String.IsNullOrEmpty(FrameworkDir))
					{
						msg = String.Format("FrameworkDir path unspecified.");
						throw new ApplicationException(msg);
					}
					if (!String.IsNullOrEmpty(FrameworkDir) && !Directory.Exists(FrameworkDir))
					{
						msg = String.Format("Given Framework directory does not exist: {0}", FrameworkDir);
						throw new ApplicationException(msg);
					}
				}
				else if (arg.StartsWith(Options.BuildRoot))
				{
					string msg = null;

					BuildRoot = arg.Substring(Options.BuildRoot.Length);

					if (String.IsNullOrEmpty(BuildRoot))
					{
						msg = String.Format("BuildRoot path unspecified.");
						throw new ApplicationException(msg);
					}
				}
				else if (arg.StartsWith(Options.Config))
				{
					string msg = null;

					Config = arg.Substring(Options.Config.Length);

					if (String.IsNullOrEmpty(Config))
					{
						msg = String.Format(Options.Config + "value unspecified.");
						throw new ApplicationException(msg);
					}
				}
				else if (arg.StartsWith(Options.Help))
				{
					InputHelp = true;
				}
				else if (arg.StartsWith(Options.Verbose))
				{
					Verbose = true;
				}
				else if (arg.StartsWith(Options.ShowOutput))
				{
					ShowOutput = true;
				}
                else if (arg.StartsWith(Options.ReTestFailures))
                {
                    ReTestFailures = true;
                }
				else if (arg.StartsWith(Options.Unix))
				{
					Unix = true;
				}
				else if (arg.StartsWith(Options.StartupProperties))
				{
					string[] propList = arg.Substring(Options.StartupProperties.Length).Split(';');
					foreach (string propPair in propList)
					{
						int idx = propPair.IndexOf('=');
						if (idx > 0) // We need to have at least 1 character for the property name.
						{
							string key = propPair.Substring(0, idx);
							string value = propPair.Substring(idx+1);	// Skipping over the = character.
							StartupProperties.Add(key, value);
						}
					}
				}
				else if (arg.Length > 0)
				{
					if (arg.StartsWith("-"))
					{
						string msg = String.Format("Unknown command line option '{0}'.", arg);
						throw new ApplicationException(msg);
					}
					else
					{
						string msg = String.Format("Unknown command line option '{0}'.{1}{1}- Use quotes if parameter values contain strings with spaces.", arg, Environment.NewLine);
						throw new ApplicationException(msg);
					}

				}
			}

			if (TestString == null)
			{
				System.Console.WriteLine("ERROR: Parameter '-tests:' is missing");
				System.Console.WriteLine("");
				System.Console.WriteLine("");

				InputHelp = true;
			}
			if (BuildRoot == null)
			{
				System.Console.WriteLine("ERROR: Parameter '-buildroot:' is missing");
				System.Console.WriteLine("");
				System.Console.WriteLine("");

				InputHelp = true;
			}

		}

		void ShowHelp()
		{
			const int optionPadding = 23;
			Console.WriteLine("----------------------------------------------------------------------------");
			Console.WriteLine("NAnt regression test runner. Version 1.0");
			Console.WriteLine("----------------------------------------------------------------------------");
			Console.WriteLine();
			Console.WriteLine("Usage: ");
			Console.WriteLine();
			Console.WriteLine("   RegressNant [options] {0}<path> {1}package[test1;test2] [;package2[test3...]... ]", Options.BuildRoot, Options.TestList);
			Console.WriteLine();
			Console.WriteLine();
			Console.WriteLine("* NOTE * If test directory contains file named KnownBroken.txt error count");
			Console.WriteLine("         will not be increased even if test fails");
			Console.WriteLine();
			Console.WriteLine();
			Console.WriteLine("return value       - number of failed tests.");
			Console.WriteLine();
			Console.WriteLine("package            - name of the test package (absolute or relative path).");
			Console.WriteLine("                     NOTE - at least one test package must be specified.");
			Console.WriteLine();
			Console.WriteLine("[test1, test2,...] - version numbers of the test package. ");
			Console.WriteLine("                     If these are not specified all versions found in the");
			Console.WriteLine("                     package directory will be executed.");
			Console.WriteLine("                     When patrameters contain * and/or ? symbols, they are");
			Console.WriteLine("                     used as a filter");
			Console.WriteLine("                     Test parameters must be given in square brackets");
			Console.WriteLine();
			Console.WriteLine("  {0} buildroot passed to nant", Options.BuildRoot.PadRight(optionPadding));
			Console.WriteLine();
			Console.WriteLine();
			Console.WriteLine("Options:");
			Console.WriteLine();
			Console.WriteLine("  {0} print this message", Options.Help.PadRight(optionPadding));
			Console.WriteLine("  {0} use specified master configuration file", Options.MasterConfigFile.PadRight(optionPadding));
			Console.WriteLine("  {0} redirect output from tests into a log file", Options.Logfile.PadRight(optionPadding));
			Console.WriteLine("  {0} define directory for nant.exe", Options.FrameworkDir.PadRight(optionPadding));
			Console.WriteLine("  {0} verbose output from nant", Options.Verbose.PadRight(optionPadding));
			Console.WriteLine("  {0} indicates RegressNant is running under unix with mono", Options.Unix.PadRight(optionPadding));
			Console.WriteLine("  {0} indicates that only tests that failed or were not previously reached in last run should be tested", Options.ReTestFailures.PadRight(optionPadding));
			Console.WriteLine("  {0} print test output to the console as the test is running for debugging individual tests", Options.ShowOutput.PadRight(optionPadding));
			Console.WriteLine();
			Console.WriteLine("Only output from nant builds will be redirected to the this log file.");
			Console.WriteLine("Output from the RegressNant.exe itself is still to the console window");
			Console.WriteLine();
			Console.WriteLine();
			Console.WriteLine("----------------------------------------------------------------------------");
			Console.WriteLine();
			Console.WriteLine("EXAMPLES:");
			Console.WriteLine();
			Console.WriteLine("->RegressNant.exe  -tests:C:\\projects\\CMBugs\\fwckbug");
			Console.WriteLine("                       Executes all tests in the fwckbug package");
			Console.WriteLine();
			Console.WriteLine("->cd C:\\projects\\CMBugs");
			Console.WriteLine("->RegressNant.exe  -tests:fwckbug;eaconfbug");
			Console.WriteLine("                 Executes all tests in the fwckbug and eaconfbug packages");
			Console.WriteLine();
			Console.WriteLine("->RegressNant.exe  -tests:C:\\projects\\CMBugs\\fwckbug[1234;1122]");
			Console.WriteLine("                 Executes tests 1234 and 1122 in the fwckbug package");
			Console.WriteLine();
			Console.WriteLine("->RegressNant.exe  -tests:C:\\projects\\CMBugs\\fwckbug[12*];eaconfbug");
			Console.WriteLine("                 Executes all tests starting with 12 in the fwckbug package,");
			Console.WriteLine("                 and all tests in the eaconfbug test package");
			Console.WriteLine();
			Console.WriteLine("->RegressNant.exe -logfile:\"C:\\nant testing log.txt\" -masterconfigfile:\"C:\\packages\\testmasterconfig.xml\" -tests:C:\\projects\\CMBugs\\fwckbug");
			Console.WriteLine("                 Executes all tests in the fwckbug package ");
			Console.WriteLine("                 using C:\\packages\\testmasterconfig.xml masterconfig file.");
			Console.WriteLine("----------------------------------------------------------------------------");
		}


		int Run(string[] args)
		{
			try
			{
				ParseCommandLine(args);

				if (InputHelp)
				{
					ShowHelp();
					return ReturnCode.Ok;
				}

				using (TestRunner testRunner = new TestRunner
                (
					LogFilePath,
					FrameworkDir,
					BuildRoot,
                    ReTestFailures,
					Verbose,
					Unix,
					Config,
					OpenCover.Validate(),
					StartupProperties,
					ShowOutput
					))
				{
					return testRunner.RunTests(TestString, MasterConfigFilePath);
				}
			}
			catch (Exception ex)
			{
				Console.WriteLine("FAILED: " + ex.Message);
				return ReturnCode.Error;
			}
		}
	}
}
