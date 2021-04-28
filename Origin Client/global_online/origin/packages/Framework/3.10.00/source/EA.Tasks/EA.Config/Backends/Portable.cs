using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
using System.IO;

using NAnt.Core;
using NAnt.Core.Util;

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
            // PC & Xenon
            SdkEnvInfo.Add(new SDKLocationEntry("VisualStudio", "VSInstallDir", new string[] { "package.VisualStudio.appdir" }));
            SdkEnvInfo.Add(new SDKLocationEntry("xenonsdk", "Xbox360InstallDir", new string[] { "package.xenonsdk.appdir" }));
            SdkEnvInfo.Add(new SDKLocationEntry("WindowsSDK", "WindowsSDKDir", new string[] { "package.WindowsSDK.appdir", "eaconfig.PlatformSDK.dir" }));
            SdkEnvInfo.Add(new SDKLocationEntry("DotNetSDK", "FrameworkSDKDir", new string[] { "eaconfig.DotNetSDK.libdir/lib", "package.DotNetSDK.appdir", "package.VisualStudio.dotnetsdkdir" }));
            SdkEnvInfo.Add(new SDKLocationEntry("DotNet", "FrameworkDir", new string[] { "package.DotNet.bindir", "package.DotNet.appdir" }));
            SdkEnvInfo.Add(new SDKLocationEntry("DirectX", "DXSDK_DIR", new string[] { "package.DirectX.appdir" }));

            //ps3
            SdkEnvInfo.Add(new SDKLocationEntry("ps3sdk", "SCE_PS3_ROOT", new string[] { "package.ps3sdk.appdir" }));
            SdkEnvInfo.Add(new SDKLocationEntry("snvsi", "SN_PS3_PATH", new string[] { "package.snvsi.appdir" }));

            // Rev
            SdkEnvInfo.Add(new SDKLocationEntry("RevolutionCodeWarrior", "CWFOLDER_RVL", new string[] { "package.RevolutionCodeWarrior.appdir" }));
            SdkEnvInfo.Add(new SDKLocationEntry("RevolutionSDK", "REVOLUTION_SDK_ROOT", new string[] { "package.RevolutionSDK.appdir" }));
            SdkEnvInfo.Add(new SDKLocationEntry("RevolutionNDev", "NDEV", new string[] { "package.RevolutionNDev.appdir" }));
        }

        public void InitConfig(Project project)
        {
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
                if (!String.IsNullOrEmpty(splitStrings[index]) && splitStrings[index].IndexOfAny(new char[] { Path.DirectorySeparatorChar, PathNormalizer.AltDirectorySeparatorChar }) > 0)
                {
                    string option = splitStrings[index];

                    int ind = option.LastIndexOfAny(new char[] { ',', '=' });

                    if (ind < 0 && option.StartsWith("-"))
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
                        // Just append ioption;
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
                            option = FixPath(option, out isSdk);
                            if (option.StartsWith("%") || option.StartsWith("\"%"))
                            {
                                isSdk = true;
                            }
                            if (!isSdk)
                            {
                                option = PathUtil.RelativePath(option, projectRoot);
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


        public string NormalizeIfPathString(string path, string rootDir)
        {
            path = path.TrimWhiteSpace();

            var pathString  = path.TrimQuotes();

            if (IsRootedPath(pathString))
            {
                bool isSdk;
                pathString = FixPath(pathString, out isSdk);

                if (!isSdk)
                {
                    pathString = PathUtil.RelativePath(pathString, rootDir);
                }
                path = pathString.Quote();
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
    }
}
