// (c) Electronic Arts. All rights reserved.

using System;
using System.Collections.Generic;
using System.Text;
using System.IO;

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Attributes;
using NAnt.Core.Writers;
using NAnt.Core.Reflection;
using NAnt.Core.Events;
using EA.FrameworkTasks.Model;

using EA.Eaconfig.Core;
using EA.Eaconfig.Modules;
using EA.Eaconfig.Build;
using EA.Eaconfig.Backends.VisualStudio;

namespace EA.Eaconfig
{
	[TaskName("pc-common-visualstudio-extension", XmlSchema = false)]
	public class EAConfig_PC_Common_VisualStudioExtension : IMCPPVisualStudioExtension
	{
		public override void SetVCPPDirectories(VCPPDirectories directories)
		{
			if (Generator.Interface.BuildGenerator.IsPortable)
			{
				// If we are generating portable solution, don't need to set environment variables.
				return;
			}

			bool use64BitTools = false;
			if (Environment.Is64BitOperatingSystem)
			{
				// In the same file, we always set vc compiler to have "UserNativeEnvironment=true".
				use64BitTools = true;
			}

			string winSdkKitDir = Project.Properties.GetPropertyOrDefault("package.WindowsSDK.kitdir", null);
			if (!string.IsNullOrEmpty(winSdkKitDir))
			{
				string targetPlatformVersion = Project.Properties.GetPropertyOrDefault("package.WindowsSDK.TargetPlatformVersion", null);
				bool useTargetPlatformSubFolder = !String.IsNullOrEmpty(targetPlatformVersion) && targetPlatformVersion.StrCompareVersions("10.0.15063.0") >= 0;
				if (use64BitTools)
				{
					string winSdkKitBinDir64 = Path.Combine(winSdkKitDir, "bin", "x64");
					if (useTargetPlatformSubFolder)
					{
						// The 'targetPlatformVersion' sub-folder under bin only start showing up on SDK 10.0.15063.0 and up.
						winSdkKitBinDir64 = Path.Combine(winSdkKitDir, "bin", targetPlatformVersion, "x64");
					}
					directories.ExecutableDirectories.Add(new PathString(winSdkKitBinDir64).Normalize());
				}
				string winSdkKitBinDir32 = Path.Combine(winSdkKitDir, "bin", "x86");
				if (useTargetPlatformSubFolder)
				{
					// The 'targetPlatformVersion' sub-folder under bin only start showing up on SDK 10.0.15063.0 and up.
					winSdkKitBinDir32 = Path.Combine(winSdkKitDir, "bin", targetPlatformVersion, "x86");
				}
				directories.ExecutableDirectories.Add(new PathString(winSdkKitBinDir32).Normalize());
			}

			string winSdkDotNetToolsDir = Project.Properties.GetPropertyOrDefault("package.WindowsSDK.dotnet.tools.dir", null);
			if (!string.IsNullOrEmpty(winSdkDotNetToolsDir))
			{
				if (use64BitTools)
				{
					string winSdkDotNetToolsDir64 = Path.Combine(winSdkDotNetToolsDir, "x64");
					directories.ExecutableDirectories.Add(new PathString(winSdkDotNetToolsDir64).Normalize());
				}
				directories.ExecutableDirectories.Add(new PathString(winSdkDotNetToolsDir).Normalize());
			}
		}

		public override void ProjectGlobals(IDictionary<string, string> projectGlobals)
		{
			if (Generator.Interface.BuildGenerator.IsPortable)
			{
				// Only do this for non-portable solution.  For portable solution, we should just use the default MSBuild environment setup.
				return;
			}

			// These type of setup probably should have live in WindowsSDK and during project generation, we just source the the property file
			// like the way we did it in CapilanoSDK.
			string winsdk_version = Project.GetPropertyOrDefault("package.WindowsSDK.MajorVersion", "0");
			if (winsdk_version == "10")
			{
				string pathSeparator = new string(Path.DirectorySeparatorChar, 1);
				// We need to set up these SDK properties so that the Visual Studio environment will set
				// up the correct exe path for the default properties in Visual Studio 2015's MSBuild.
				// Also, starting with Visual Studio 2015 Update 2, if we don't set up this property and 
				// people didn't installed the SDK and just use the non-proxy, the new MSBuild will
				// fail the build complaining about SDK not installed.
				string winSdkKitDir = Project.Properties.GetPropertyOrDefault("package.WindowsSDK.kitdir", null);
				if (!string.IsNullOrEmpty(winSdkKitDir))
				{
					if (!winSdkKitDir.EndsWith(pathSeparator))
					{
						winSdkKitDir = winSdkKitDir + pathSeparator;
					}
					string ucrtPropsFile = Path.Combine(winSdkKitDir, "DesignTime", "CommonConfiguration", "Neutral", "uCRT.props");
					if (!projectGlobals.ContainsKey("UniversalCRT_PropsPath") && File.Exists(ucrtPropsFile))
					{
						projectGlobals.Add("UniversalCRT_PropsPath", ucrtPropsFile);
					}
					// This UCRTContentRoot property is actually defined in ${package.WindowsSDK.kitdir}\DesignTime\CommonConfiguration\Neutral\uCRT.props
					// That file is loaded by MSBuild to define UniversalCRT_IncludePath, UniversalCRT_LibraryPath_xxx, etc.  We need to override
					// that property so that it can point to the non-proxy package.
					if (!projectGlobals.ContainsKey("UCRTContentRoot"))
					{
						projectGlobals.Add("UCRTContentRoot", winSdkKitDir);
					}
					// The following UniversalCRTSdkDir property help define UniversalCRT_PropsPath which specify the location of the
					// above uCRT.props file.
					if (!projectGlobals.ContainsKey("UniversalCRTSdkDir"))
					{
						projectGlobals.Add("UniversalCRTSdkDir", winSdkKitDir);
					}
					if (!projectGlobals.ContainsKey("UniversalCRTSdkDir_10"))   // Looks like newer version of MSBuild now requires this property to set the above.
					{
						projectGlobals.Add("UniversalCRTSdkDir_10", winSdkKitDir);
					}
					// For WindowsSDK 10, we need to define WindowsSdkDir_10 in order to override the property WindowsSdkDir which in turn is used
					// in defining other include/libs/exe paths.
					if (!projectGlobals.ContainsKey("WindowsSdkDir"))
					{
						projectGlobals.Add("WindowsSdkDir", winSdkKitDir);
					}
					if (!projectGlobals.ContainsKey("WindowsSdkDir_10"))
					{
						projectGlobals.Add("WindowsSdkDir_10", winSdkKitDir);
					}
				}

				// Note that setting the following two properties will currently have no effect until you
				// are updated to Visual Studio 2015 Update 2 (there is a bug in previous MSBuild where you cannot
				// override the location.  Also, in Update 1 (or older), MSBuild is hard coded to use $(NETFXSDKDir)bin\NETFX 4.6 Tools,
				// In Update 2, MSBuild is hard coded to use $(NETFXSDKDir)\bin\NETFX 4.6.1 Tools
				string netFxKitsDir = Project.Properties.GetPropertyOrDefault("package.WindowsSDK.netfxkitdir", null);
				if (!string.IsNullOrEmpty(netFxKitsDir))
				{
					if (!netFxKitsDir.EndsWith(pathSeparator))
					{
						netFxKitsDir = netFxKitsDir + pathSeparator;
					}
					if (!projectGlobals.ContainsKey("NETFXKitsDir"))
					{
						projectGlobals.Add("NETFXKitsDir", netFxKitsDir);
					}
					if (!projectGlobals.ContainsKey("DotNetSdkRoot"))
					{
						projectGlobals.Add("DotNetSdkRoot", netFxKitsDir);
					}
				}

				string netFxSdkDir = Project.Properties.GetPropertyOrDefault("package.WindowsSDK.appdir", null);
				if (!string.IsNullOrEmpty(netFxSdkDir))
				{
					if (!netFxSdkDir.EndsWith(pathSeparator))
					{
						netFxSdkDir = netFxSdkDir + pathSeparator;
					}
					if (!projectGlobals.ContainsKey("NETFXSDKDir"))
					{
						projectGlobals.Add("NETFXSDKDir", netFxSdkDir);
					}
					if (!projectGlobals.ContainsKey("FrameworkSDKRoot"))
					{
						projectGlobals.Add("FrameworkSDKRoot", netFxSdkDir);
					}
				}

				string dotNetToolsDir = Project.Properties.GetPropertyOrDefault("package.WindowsSDK.dotnet.tools.dir", null);
				if (!string.IsNullOrEmpty(dotNetToolsDir))
				{
					if (!dotNetToolsDir.EndsWith(pathSeparator))
					{
						dotNetToolsDir = dotNetToolsDir + pathSeparator;
					}

					// Seems like this environment variable only points to 32-bit path.
					if (!projectGlobals.ContainsKey("SDK40ToolsPath"))
					{
						projectGlobals.Add("SDK40ToolsPath", dotNetToolsDir);
					}
				}
			}
		}
	}

	// Framework work will look for extension 
	// ${config-platform}-visualstudio-extension + extension names in property '${config-platform}-visualstudio-extension

	[TaskName("pc-vc-visualstudio-extension", XmlSchema = false)]
	public class EAConfig_PC_VC_VisualStudioExtension : EAConfig_PC_Common_VisualStudioExtension
	{
	}

	[TaskName("pc64-vc-visualstudio-extension", XmlSchema = false)]
	public class EAConfig_PC64_VC_VisualStudioExtension : EAConfig_PC_Common_VisualStudioExtension
	{
	}
}
