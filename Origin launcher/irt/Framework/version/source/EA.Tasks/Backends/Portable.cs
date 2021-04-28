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
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
using System.IO;

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Logging;

namespace EA.Eaconfig.Backends
{
	public class Portable
	{
		public readonly List<SDKLocationEntry> SdkEnvInfo = new List<SDKLocationEntry>();

		public class SDKLocationEntry
		{
			public SDKLocationEntry(string sdkName, string sdkVar, string[] properties)
			{
				SdkName = sdkName;
				EnvVarName = sdkVar;
				Properties = properties;
				SdkPath = null;
			}
			public readonly string EnvVarName;
			public readonly string SdkName;
			public readonly string[] Properties;
			public string SdkPath;
		}

		public Portable()
		{
			// We really shouldn't be using these.  The generated vcproj should setup appropriate environment override.  And if we have user
			// build script explicitly referencing WindowsSDK full path (or any SDK full path), we should try to fix those build script
			// instead of silently fix these for them.  User build script should have referenced the libraries using "asis" in their fileset
			// attribute.  If there are includedirs, they should have used relative path based on default SDK include path setup that is
			// already provided.
			
			// PC
			SdkEnvInfo.Add(new SDKLocationEntry("VisualStudio", "VsInstallRoot", new string[] { "package.VisualStudio.appdir" }));
			SdkEnvInfo.Add(new SDKLocationEntry("WindowsSDK", "WindowsSdkDir", new string[] { "package.WindowsSDK.kitdir", "eaconfig.PlatformSDK.dir" }));
			SdkEnvInfo.Add(new SDKLocationEntry("WindowsSDK", "NETFXSDKDir", new string[] { "package.WindowsSDK.appdir" }));
			SdkEnvInfo.Add(new SDKLocationEntry("WindowsSDK", "NETFXKitsDir", new string[] { "package.WindowsSDK.netfxkitdir" }));
		}

		string PortableRoot { get; set; }

		private void FillDynamicTable(Project project)
		{
			OptionSet portableData;
            var portableRoot = project.Properties["eaconfig.portable-root"];
            PortableRoot = portableRoot != null ? Path.GetFullPath(Path.Combine(Path.GetDirectoryName(project.Properties["nant.project.masterconfigfile"]), portableRoot)) : null;
            if (!string.IsNullOrEmpty(PortableRoot))
            {
                PortableRoot = project.GetFullPath(PortableRoot);
            }
			if(project.NamedOptionSets.TryGetValue("sdk.portable-gen-data", out portableData))
			{
				foreach(var option in portableData.Options)
				{
					var val = option.Value.ToArray();

					if(!SdkEnvInfo.Any(e=>e.EnvVarName == option.Key))
					{
						SdkEnvInfo.Insert(0, new SDKLocationEntry("dynamic", option.Key, option.Value.ToArray().ToArray()));
					}
				}
			}
		}

		public void InitConfig(Project project)
		{
			FillDynamicTable(project);

			foreach (SDKLocationEntry sdk in SdkEnvInfo)
			{
				if (sdk.SdkPath == null)
				{
					foreach (string prop in sdk.Properties)
					{
						int trim = prop.IndexOf('/');
						string propName = prop;

						if (trim >= 0)
						{
							propName = prop.Substring(0, trim);
							trim = prop.Length - trim;
						}


						string val = project.Properties[propName];
						if (val != null)
						{
							val = val.Trim(new char[] { '\n', '\r', '\t', ' ', '/', '\\', '"' });

							if (trim > 0)
							{
								val = val.Substring(0, val.Length - trim);
							}
							if (!String.IsNullOrEmpty(val))
							{
								sdk.SdkPath = PathNormalizer.Normalize(val,false);
								break;
							}
						}
					}
				}
			}
		}

		public string FixPath(string path, out bool isSDK)
		{
			isSDK = false;

			if (!String.IsNullOrEmpty(path))
			{
				path = path.TrimWhiteSpace();

				if (path.TrimStart('"').StartsWith("$("))
				{
					isSDK = true;
				}
				else
				{
					path = PathNormalizer.Normalize(path, false);

					foreach (SDKLocationEntry sdk in SdkEnvInfo)
					{
						if (sdk.SdkPath != null)
						{
							if (path.TrimStart('"').StartsWith(sdk.SdkPath, StringComparison.OrdinalIgnoreCase))
							{
								path = path.Replace(sdk.SdkPath, "$(" + sdk.EnvVarName + ")");
								isSDK = true;
								break;
							}
						}
					}
				}
			}

			return path;
		}

		public string NormalizeCommandLineWithPathStrings(string line, string root)
		{
			if (!String.IsNullOrEmpty(line))
			{
				string regexPattern = "\\s*(\"(?:[^\"]*)\"|(?:[^\\s]+))\\s*";
				string[] lines = line.Split(new char[] { '\n', '\r' });
				StringBuilder sb = new StringBuilder();
				foreach (string cmd_line in lines)
				{
					string normalized = NormalizeOptionsWithPathStrings(cmd_line, root, regexPattern);
					sb.AppendLine(normalized);
				}
				return sb.ToString().TrimEnd(new char[] { '\n', '\r' });
			}
			return line;
		}

		public string NormalizeOptionsWithPathStrings(string inputPath, string projectRoot, string regexPattern)
		{
			if (String.IsNullOrEmpty(inputPath))
			{
				return String.Empty;
			}

			StringBuilder cleanString = new StringBuilder();
			string[] splitStrings = null;		//the array that stores the tokens (separated by ;)

			//Go through each line
			splitStrings = Regex.Split(inputPath, regexPattern);
			for (int index = 0; index < splitStrings.Length; index++)
			{
				if (!String.IsNullOrEmpty(splitStrings[index]) && splitStrings[index].IndexOfAny(new char[] { Path.DirectorySeparatorChar, PathNormalizer.AltDirectorySeparatorChar }) >= 0)
				{
					string option = splitStrings[index];

					int ind = option.LastIndexOfAny(new char[] { ',', '=' });

					if (ind < 0 && (option.StartsWith("-") || option.StartsWith("/")))
					{
						ind = option.IndexOf(':');
						if (ind > 0 && option.Length > ind + 1 && !(option[ind + 1] == '\\' || option[ind + 1] == '/'))
						{
							// Use this ind
						}
						else
						{
							ind = -1;
						}
					}

					if (ind < 0 && option.StartsWith("-"))
					{
						// Just append option;
					}
					else if (ind >= 0 && ind >= option.Length - 1)
					{
						// Just append option;
					}
					else
					{
						if (ind >= 0 && ind < option.Length - 1)
						{
							cleanString.Append(option.Substring(0, ind + 1));
							option = option.Substring(ind + 1);
						}
						if (!String.IsNullOrEmpty(option))
						{
							bool isSdk;
							string fixedPath = FixPath(option, out isSdk);
							if (option.StartsWith("%") || option.StartsWith("\"%"))
							{
								isSdk = true;
							}
							if (!isSdk)
							{
								string relativePath = PathUtil.RelativePath(fixedPath, projectRoot);
								if (relativePath != fixedPath)
								{
									option = relativePath;
								}
							}
							else
							{
								option = fixedPath;
							}
						}
					}
					cleanString.Append(option);

					if (index != splitStrings.Length - 1)
					{
						cleanString.Append(' ');
					}
				}
				else if (!String.IsNullOrEmpty(splitStrings[index]))
				{
					cleanString.Append(splitStrings[index]);
					cleanString.Append(' ');
				}
			}
			return cleanString.ToString();
		}


		public string NormalizePathStrings(string inputPath, string projectRoot, string regexPattern, char sep)
		{
			var cleanString = new StringBuilder();

			var splitStrings = Regex.Split(inputPath, regexPattern);

			for (int index = 0; index < splitStrings.Length; index++)
			{
				var normalized_path = NormalizeIfPathString(splitStrings[index], projectRoot);

				cleanString.Append(normalized_path);

				if (index != splitStrings.Length - 1)
				{
					cleanString.Append(sep);
				}
			}
			return cleanString.ToString();
		}


		public string NormalizeIfPathString(string path, string rootDir, bool quote=true)
		{
			path = path.TrimWhiteSpace();

			// Note that the path.TrimQuotes() function only trim double quote and the pathString.Quote() function only add double quote.
			bool quoted = path.Length > 0 && path[0] == '"';

			string pathString  = path.TrimQuotes();

			// Microsoft's MSBuild script are quite sensitive to requiring path ending with directory separator character.
			// So if the input has this character, we need to make sure we keep it.
			bool isPathEndsWithSeparator = path.Length > 0 ? pathString[ pathString.Length-1 ] == Path.DirectorySeparatorChar : false;

			if (IsRootedPath(pathString))
			{
				bool isSdk;
				pathString = FixPath(pathString, out isSdk);

				if (!isSdk)
				{
                    if (PathIsWithinPortableRoot(pathString) == false)
                    {
                        Log.Default.Warning.WriteLine($"Portable path escaped root: {path}");
                    }

					pathString = PathUtil.RelativePath(pathString, rootDir);
				}

				if (isPathEndsWithSeparator)
				{
					if (pathString.Length > 0 && pathString[ pathString.Length-1 ] != Path.DirectorySeparatorChar)
					{
						// Add back in the directory separator only if FixPath() and RelativePath() removed it.
						pathString += Path.DirectorySeparatorChar;
					}
				}

				if (quote || quoted)
				{
					path = pathString.Quote();
				}
				else
				{
					path = pathString;
				}
			}

			return path;
		}


		public bool IsRootedPath(string path)
		{
			bool res = false;
			if (!String.IsNullOrEmpty(path) && PathUtil.IsValidPathString(path))
			{
				res = (-1 != path.IndexOfAny(new char[] { Path.DirectorySeparatorChar, PathNormalizer.AltDirectorySeparatorChar } ))
						&& Path.IsPathRooted(path);
			}
			return res;
		}

        public bool? PathIsWithinPortableRoot(string pathString)
        {
            if (PortableRoot == null)
                return null;
            return Path.GetFullPath(pathString.Trim('"')).StartsWith(PortableRoot, StringComparison.OrdinalIgnoreCase);
        }
    }
}
