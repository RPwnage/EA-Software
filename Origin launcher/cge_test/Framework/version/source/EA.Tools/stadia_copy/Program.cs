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
		static private string ExecutablePath = null;

		// properties for scp
		static private SSHInit SSHInitInfo = null;

		static private string PlayerId = null;
		static private string UserExt = ".user";

		static private bool sCalledSSHInit = false;

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

				// make sure we don't have multiple instances leased if an instance wasn't specified
				if(String.IsNullOrEmpty(InstanceId))
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
							ExecutablePath = args[++argIndex];
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
			var stadiaAssets = Directory.GetFiles(Src, "*.*", SearchOption.AllDirectories);
			var instanceToLocalMap = new Dictionary<string, string>();

			// get the local timestamps
			var assetTimeStamps = new Dictionary<string, DateTime>();
			foreach (var asset in stadiaAssets)
			{
				try
				{
					// prune any files that have an extension that match our ignore list
					if (ExtsToIgnore.Contains(Path.GetExtension(asset)))
						continue;

					// grab utc time but only to seconds accuracy, this seems to be the accuracy with which 
					// we will get stadia file times and we don't want to copy because our files are a few
					// ms newer
					DateTime modTime = File.GetCreationTimeUtc(asset);
					modTime = new DateTime(modTime.Year, modTime.Month, modTime.Day, modTime.Hour, modTime.Minute, modTime.Second, DateTimeKind.Utc);

					string fileName = Path.Combine(Dest, asset.Replace(Src, "")).Replace("\\", "/");
					instanceToLocalMap[asset] = fileName;
					assetTimeStamps[fileName] = modTime;
				}
				catch (Exception ex)
				{
					throw new Exception($"Failed to get file info {asset}. " + ex.ToString());
				}
			}

			// get the file log from the instance
			HashSet<string> instanceDirectories = new HashSet<string>();
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

			var newInstance = CleanInstanceIfNew(instanceTimeStamps.Keys);
			if (!newInstance)
			{
				// compare timings and prune anything that is the same
				if (!Silent)
				{
					foreach (var assetTimeStamp in assetTimeStamps)
					{
						if (!instanceTimeStamps.ContainsKey(assetTimeStamp.Key))
						{
							Console.WriteLine($"File \"{assetTimeStamp.Key}\" not present on instance");
						}
					}
				}

				foreach (var instanceTimeStamp in instanceTimeStamps)
				{
					if (assetTimeStamps.TryGetValue(instanceTimeStamp.Key, out DateTime timestamp))
					{
						// NOTE: The stadia sync tool used for the main application in VS does not persist the datetime 
						// to the instance so we must assume that if the date on the instance is later than the datetime 
						// we have locally then it is intentional.
						if (timestamp.CompareTo(instanceTimeStamp.Value) <= 0)
							assetTimeStamps.Remove(instanceTimeStamp.Key);
						else if (!Silent)
							Console.WriteLine($"File \"{instanceTimeStamp.Key}\" out of date relative to local timestamp (local: {timestamp}, instance {instanceTimeStamp.Value})");
					}
				}
			}

			// create directories if they don't exist and populate asset list
			List<Tuple<string, string, bool>> assets = new List<Tuple<string, string, bool>>();
			foreach (var kvp in instanceToLocalMap)
			{
				if (assetTimeStamps.ContainsKey(kvp.Value))
				{
					bool setExecBit = ExecutablePath != null ?
						Path.GetFullPath(kvp.Key).Equals(Path.GetFullPath(ExecutablePath), StringComparison.OrdinalIgnoreCase) :
						false;
					assets.Add(Tuple.Create(kvp.Key, kvp.Value, setExecBit));

					var instanceDir = Path.GetDirectoryName(kvp.Value).Replace("\\", "/");
					if (!instanceDirectories.Contains(instanceDir))
					{
						if (!Silent)
							Console.WriteLine($"Creating \"{instanceDir}\"");

						RunGGPProcess(String.Format("ssh shell{0} -- mkdir -p {1}", (InstanceId != null) ? " --instance " + InstanceId : "", instanceDir));

						instanceDirectories.Add(instanceDir);
					}
				}
			}

			// if this is a new instance then we need to upload the user file
			var filePath = Path.Combine(Path.GetTempPath(), PlayerId + UserExt);
			if (newInstance)
			{
				if (!File.Exists(filePath))
					File.Create(filePath).Close();
				assets.Add(new Tuple<string, string, bool>(filePath, Dest + "/", false));
			}

			try
			{
				// push assets that have changed
				foreach (var asset in assets)
				{
					const int maxRetries = 5;
					int retry = 0;
					for (; retry < maxRetries; retry++)
					{
						var instanceDir = Path.GetDirectoryName(asset.Item2).Replace("\\", "/") + "/";
						if (PushAsset(asset.Item1, instanceDir, asset.Item3))
							break;
					}

					if (retry == maxRetries)
						throw new Exception($"Failed to upload asset: \"{asset.Item1}");
				}
			}
			finally
			{
				// delete the temp file
				if (newInstance)
					File.Delete(filePath);
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
		private static bool CleanInstanceIfNew(IEnumerable<string> files)
		{
			// get the player id
			{
				if (!Silent)
					Console.WriteLine($"Getting player ID.");

				var output = RunGGPProcess("profile describe -s");
				var profileInfo = JsonConvert.DeserializeObject<ProfileInfo>(output);
				PlayerId = profileInfo.playerId;
			}

			var userFile = files.FirstOrDefault(file => file.Contains(PlayerId));
			if (String.IsNullOrEmpty(userFile))
			{
				if (!Silent)
					Console.WriteLine($"Deleting {Dest}.");

				try
				{
					RunGGPProcess(String.Format("ssh shell{0} \"rm -r {1}/*\"", (InstanceId != null) ? " --instance " + InstanceId : "", Dest));
				}
				catch {}

				return true;
			}

			return false;
		}

		private static bool PushAsset(string localAsset, string dest, bool setExecutable)
		{
			if(!sCalledSSHInit)
			{
				if (!Silent)
					Console.WriteLine(String.Format("Retrieving ssh info from \"{0}\" instance.", InstanceId != null ? InstanceId : "default"));

				var output = RunGGPProcess(String.Format("ssh init{0} -s", (InstanceId != null) ? " --instance " + InstanceId : ""));
				SSHInitInfo = JsonConvert.DeserializeObject<SSHInit>(output);

				sCalledSSHInit = true;
			}

			try
			{
				var pushProcess = CreateProcess();
				if (GGPSDKPath != null)
					pushProcess.StartInfo.FileName = Path.Combine(GGPSDKPath, "tools", "OpenSSH-Win64") + Path.DirectorySeparatorChar;  
				pushProcess.StartInfo.FileName += "scp";
				pushProcess.StartInfo.Arguments = $"-p -C -P{SSHInitInfo.Port} -FNUL -i{SSHInitInfo.KeyPath} -oStrictHostKeyChecking=yes -oUserKnownHostsFile=\\\"{SSHInitInfo.KnownHostsPath}\\\" {localAsset} {SSHInitInfo.User}@{SSHInitInfo.Host}:{dest}";

				using (var outputHelper = new OutputHelper())
				{
					pushProcess.OutputDataReceived += (sender, dataArgs) => OutputHelper.CaptureAndWriteOutput(dataArgs.Data);
					pushProcess.ErrorDataReceived += (sender, dataArgs) => OutputHelper.CaptureAndWriteError(dataArgs.Data);

					if (!Silent)
					{
						Console.WriteLine($"Uploading \"{localAsset}\" to \"{dest}\"");
					}
					if (pushProcess.Start())
					{
						pushProcess.BeginOutputReadLine();
						pushProcess.BeginErrorReadLine();
						pushProcess.WaitForExit();
						if (pushProcess.ExitCode != 0)
						{
							throw new Exception($"Failed to run: \"{pushProcess.StartInfo.FileName} {pushProcess.StartInfo.Arguments}\" {OutputHelper.stderr}");
						}
					}
					else
					{
						throw new Exception($"Failed to run: \"{pushProcess.StartInfo.FileName} {pushProcess.StartInfo.Arguments}\"");
					}
				}

				if (setExecutable)
				{
					try
					{
						string destFile = $"{dest}/{Path.GetFileName(localAsset)}";

						if (!Silent)
							Console.WriteLine($"Setting executable flag on \"{destFile}\"");

						RunGGPProcess(String.Format("ssh shell{0} -- chmod a+x {1}", (InstanceId != null) ? " --instance " + InstanceId : "", destFile));
					}
					catch (Exception ex)
					{
						if (!Silent)
						{
							Console.WriteLine($"Failed to set executable bit on file \"{dest}\". " + ex.ToString());
						}
						return false;
					}
				}
			}
			catch (Exception ex)
			{
				if (!Silent)
				{
					Console.WriteLine($"Failed to upload file \"{localAsset}\" to \"{dest}\". " + ex.ToString());
				}
				return false;
			}
			return true;
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
			if (GGPSDKPath != null)
				p.StartInfo.FileName = Path.Combine(GGPSDKPath, "dev", "bin") + Path.DirectorySeparatorChar;
			p.StartInfo.FileName += "ggp";
			if (InstanceId != null)
				p.StartInfo.Arguments = $"--instance {InstanceId} ";
			return p;
		}
	}
}
