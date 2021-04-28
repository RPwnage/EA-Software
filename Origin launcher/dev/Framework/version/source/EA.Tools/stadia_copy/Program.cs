using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text.RegularExpressions;
using Newtonsoft.Json;

namespace stadia_copy
{
    class Program
	{
		private class OutputHelper : IDisposable
        {
			private static object outputHelperLock = new object();
			public static bool inUse = false;
			public static string stdout = "";
			public static string stderr = "";

			public OutputHelper()
			{
				Start();
			}

			~OutputHelper()
			{
				End();
			}

			public void Dispose()
			{
				End();
			}

			public static void CaptureOutput(string str)
			{
				if (!String.IsNullOrEmpty(str))
				{
					stdout += str + Environment.NewLine;
				}
			}
			public static void CaptureAndWriteOutput(string str)
			{
				if (!String.IsNullOrEmpty(str))
				{
					CaptureOutput(str);
					Console.WriteLine(str);
				}
			}
			public static void CaptureError(string str)
			{
				if (!String.IsNullOrEmpty(str))
				{
					stderr += str + Environment.NewLine;
				}
			}
			public static void CaptureAndWriteError(string str)
			{
				if (!String.IsNullOrEmpty(str))
				{
					CaptureError(str);
					Console.WriteLine(str);
				}
			}

			private static void Start()
            {
				lock (outputHelperLock)
                {
					if (!inUse)
					{
						stdout = "";
						stderr = "";
						inUse = true;
					}
					else
                    {
						throw new Exception($"Output Helper is already in use, cannot run parallel processes that are using the Output Helper!");
					}
				}
            }

			private static void End()
            {
				lock (outputHelperLock)
				{
					inUse = false;
				}
			}
		}
	
		private class SSHInit
		{
			public string User { get; set; }
			public string Host { get; set; }
			public string Port { get; set; }
			public string KeyPath { get; set; }
			public string KnownHostsPath { get; set; }
		}

		private class InstanceInfo
		{
			public string devkit { get; set; }
			public string id { get; set; }
		}

		static private bool SupressError = false;
		static private bool Silent = false;
		static private bool Help = false;
		
		static private string Src = null;
		static private string Dest = "/mnt/developer";
		static private string InstanceId = null;
		static private string GGPSDKPath = null;

		static private string PlayerId = null;
		static private string UserExt = ".user";

		static private string[] ExtsToIgnore = new string[] { ".sig", ".debug" };

		static private int ret = 0;

		static int Main(string[] args)
		{
			try
			{
				ParseCommandLine(args);

				if (Help)
				{
					PrintHelp();
				}

				if (Src == null)
				{
					throw new ArgumentException(String.Format("Invalid command line: source is not set"));
				}

				if (String.IsNullOrEmpty(GGPSDKPath))
				{
					GGPSDKPath = Environment.GetEnvironmentVariable("GGP_SDK_PATH");
					if (String.IsNullOrEmpty(GGPSDKPath))
						throw new Exception($"Can't invoke ggp as Environment variable for GGP_SDK_PATH path has not been set");
				}

				// make sure we don't have multiple instances leased if an instance wasn't specified
				if (String.IsNullOrEmpty(InstanceId))
				{
					if (!Silent)
						Console.WriteLine(String.Format("Retrieving instance list."));

					var output = RunGGPProcess("instance list -s");
					var instances = JsonConvert.DeserializeObject<List<InstanceInfo>>(output);
					if (instances.Count > 1)
						throw new Exception("You have more than one instance reserved.  You must specify the instance you wish to deploy to.");
				}

				Copy();
			}
			catch (Exception ex)
			{
				SetError(ex);
			}

			return ret;
		}

		private static readonly char[] TRIM_CHARS = new char[] { '"', ' ', '\n', '\r' };

		private static void ParseCommandLine(string[] args)
		{
			for(int argIndex = 0; argIndex < args.Length; argIndex++)
			{
				string arg = args[argIndex];
				if (arg[0] == '-' || arg[0] == '/')
				{
					if (arg.Substring(1) == "debug")
					{
						Debugger.Launch();
						continue;
					}

					var option = arg[1];
					switch (option)
					{
						case 'e':
							SupressError = true;
							break;
						case 's':
							Silent = true;
							break;
						case 'h':
							Help = true;
							break;
						case 'i':
							InstanceId = args[++argIndex];
							break;
						case 'g':
							GGPSDKPath = args[++argIndex];
							break;
						case 'x':
							++argIndex; // no longer needed
							break;
						default:
							if (!Silent)
							{
								Console.WriteLine("copy: unknown option '-{0}'", option);
							}
							break;
					}
				}
				else
				{
					if(Src == null)
					{
						Src = Path.GetFullPath(arg.Trim(TRIM_CHARS));
						if ((Src[Src.Length - 1] != Path.DirectorySeparatorChar) && (Src[Src.Length - 1] != Path.AltDirectorySeparatorChar))
							Src += Path.DirectorySeparatorChar;
					}
					else
					{
						Dest = arg.Trim(TRIM_CHARS);
					}
				}
			}
		}

		private static void SetError(Exception e)
		{
			if (!SupressError)
			{
				if (!Silent)
				{
					Console.WriteLine("copy error: '{0}'", e.Message);
				}
				ret = 1;
			}
		}

		private static void PrintHelp()
		{
			Console.WriteLine("Usage: stadia_copy [-tgesh...] src dest");
			Console.WriteLine();
			Console.WriteLine("  -e     Do not return error code");
			Console.WriteLine("  -s     Silent");
			Console.WriteLine("  -i     Instance ID");
			Console.WriteLine("  -g     GGP SDK Path");
		}

		static void Copy()
		{
			// get the file log from the instance
			var instanceTimeStamps = new Dictionary<string, DateTime>();
			{
				if (!Silent)
					Console.WriteLine(String.Format("Retrieving {0} info from \"{1}\" instance.", Dest, InstanceId ?? "default"));

				try
				{
					var output = RunGGPProcess(String.Format("ssh shell{0} -- find {1}/* -type f -printf %p:%T@\\\\n", (InstanceId != null) ? " --instance " + InstanceId : "", Dest));
					Regex regex = new Regex(@"(.*):(\d+\.?\d*)", RegexOptions.Multiline);
					string currentDirectory = string.Empty;
					foreach (Match match in regex.Matches(output))
					{
						string file = match.Groups[1].Value;
						double secondsSinceEpoch = Double.Parse(match.Groups[2].Value);
						DateTime fileDataTime = new DateTime(1970, 1, 1, 0, 0, 0, 0, DateTimeKind.Utc) + TimeSpan.FromSeconds(secondsSinceEpoch);
						instanceTimeStamps[file] = fileDataTime;
					}
				}
				catch { }
			}

			// if this is a new instance then we need to upload the user file
			bool isNewInstance = IsInstanceNew(instanceTimeStamps.Keys);
			var userFilePath = Path.Combine(Path.GetTempPath(), PlayerId + UserExt);

			try
			{
				string args = String.Empty;

				var argList = new List<string>();
				argList.Add("--progress");
				if (ExtsToIgnore.Length > 0)
				{
					var excludeArgList = new List<string>();
					foreach (var excludeExt in ExtsToIgnore)
						excludeArgList.Add("exclude:*" + excludeExt);
					argList.Add($"--filter={String.Join(",", excludeArgList)}");
				}

				if (isNewInstance)
					argList.Add("--delete");

				args = String.Join(" ", argList);

				RunGGPProcess($"ssh sync {Src}/* {Dest}/ -r {args}");

				if (isNewInstance)
				{
					if (!File.Exists(userFilePath))
						File.Create(userFilePath).Close();
					RunGGPProcess($"ssh sync {userFilePath} {Dest}/");
				}
			}
			finally
			{
				// delete the temp file
				if (isNewInstance)
					File.Delete(userFilePath);
			}
		}

		class ProfileInfo
		{
			public string playerId { get; set; }
		}

		/// <summary>
		/// Make sure the devnode doesn't have any stale data from a previous user
		/// <summary>
		/// <param name="files">List of files on the instance</param>
		/// <returns>Returns true/false on whether or not this instance is new to the user</returns>
		private static bool IsInstanceNew(IEnumerable<string> files)
		{
			// get the player id
			{
				if (!Silent)
					Console.WriteLine($"Getting player ID.");

				var output = RunGGPProcess("profile describe -s");
				var profileInfo = JsonConvert.DeserializeObject<ProfileInfo>(output);
				PlayerId = profileInfo.playerId;
			}

			return String.IsNullOrEmpty(files.FirstOrDefault(file => file.Contains(PlayerId)));
		}

		private static string RunGGPProcess(string arguments, bool throwExceptionOnError = true)
		{
			var process = CreateGGPProcess();
			process.StartInfo.Arguments = arguments;
			return RunProcess(process, throwExceptionOnError);
		}

		private static string RunProcess(System.Diagnostics.Process process, bool throwExceptionOnError = true)
		{
			string error = String.Empty;
			string output = String.Empty;

			Console.WriteLine("[stadia_copy] Running: " + process.StartInfo.FileName + " " + process.StartInfo.Arguments ?? "");

			using (var outputHelper = new OutputHelper())
			{
				process.OutputDataReceived += (sender, dataArgs) => OutputHelper.CaptureAndWriteOutput(dataArgs.Data);
				process.ErrorDataReceived += (sender, dataArgs) => OutputHelper.CaptureAndWriteError(dataArgs.Data);

				bool processStarted = process.Start();
				if (processStarted)
				{
					process.BeginOutputReadLine();
					process.BeginErrorReadLine();
					process.WaitForExit();
				}
				error = OutputHelper.stderr;
				output = OutputHelper.stdout;

				if (throwExceptionOnError && (process.ExitCode != 0 || !processStarted || error.Length != 0))
				{
					throw new Exception($"Failed to run \"{process.StartInfo.FileName} {process.StartInfo.Arguments}\". {error}");
				}
			}

			return output;
		}

		private static System.Diagnostics.Process CreateProcess()
		{
			System.Diagnostics.Process p = new System.Diagnostics.Process();
			p.StartInfo.RedirectStandardOutput = true;
			p.StartInfo.RedirectStandardError = true;
			p.StartInfo.RedirectStandardInput = false;
			p.StartInfo.CreateNoWindow = true;
			p.StartInfo.UseShellExecute = false;
			return p;
		}

		private static System.Diagnostics.Process CreateGGPProcess()
		{
			var p = CreateProcess();
			p.StartInfo.FileName = Path.Combine(GGPSDKPath, "dev", "bin") + Path.DirectorySeparatorChar;
			p.StartInfo.FileName += "ggp";
			if (InstanceId != null)
				p.StartInfo.Arguments = $"--instance {InstanceId} ";
			return p;
		}
	}
}
