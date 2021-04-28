// (c) Electronic Arts. All rights reserved.

using System;
using System.Collections.Generic;
using System.IO;

using NAnt.Core;
using NAnt.Core.Functions;
using NAnt.Core.Util;
using NAnt.Core.Attributes;
using NAnt.Core.Logging;

using EA.Eaconfig;
using EA.Eaconfig.Core;
using Model=EA.FrameworkTasks.Model;
using EA.Eaconfig.Backends.VisualStudio;

namespace EA.CapilanoConfig
{
	[TaskName("capilano-vc-visualstudio-extension", XmlSchema = false)]
	public class CapilanoVisualStudioExtension : IMCPPVisualStudioExtension
	{
		private bool DoNotInjectEnvironmentOverride
		{
			get
			{
				// Checks if we're using a version of CapilanoSDK that has added support to provide the property file.
				return Project.Properties.GetBooleanPropertyOrDefault("package.CapilanoSDK.DoNotInjectEnvironmentOverride", false);
			}
		}

		private bool NeedLegacyPropertyOverrideSupport
		{
			get
			{
				if (Project.Properties.GetBooleanPropertyOrDefault("gdk.enabled", false))
				{
					return false;
				}
				
				// If we are not using the correct version of CapilanoSDK and Framework, we'll do things the old way
				// and setup property override here.  Otherwise, in the future, we'll let the CapilanoSDK provide
				// the property override file and have all changes in one single import file instead of having this spread
				// out over all generated .vcxproj.
				string nantVersion = Project.Properties.GetPropertyOrDefault("nant.version", "0.0.0.0");
				return (nantVersion.StrCompareVersions("5.07.00.00") < 0 || DoNotInjectEnvironmentOverride == false);
			}
		}

		public override void SetVCPPDirectories(VCPPDirectories directories)
		{
		}

		public override void ProjectGlobals(IDictionary<string, string> projectGlobals)
		{
			if (!projectGlobals.ContainsKey("DefaultLanguage"))
			{
				projectGlobals.Add("DefaultLanguage", "en-US");
			}
			if (!projectGlobals.ContainsKey("ApplicationEnvironment"))
			{
				projectGlobals.Add("ApplicationEnvironment", "title");
			}

			if (NeedLegacyPropertyOverrideSupport)
			{
				Log.Error.WriteLine("ERROR: Visual Studio 2015 or later is required to build Xbox One!");
			}

			AddWindowsProjectGlobals(projectGlobals);

			if (!projectGlobals.ContainsKey("DisableConfigurationRefresh") && Project.Properties.GetBooleanPropertyOrDefault("package.CapilanoSDK.DisableConfigurationRefresh", false))
			{
				projectGlobals.Add("DisableConfigurationRefresh", "true");
			}

			// This is to allow users to add arbitrary elements to project globals
			if (Project.Properties["package.CapilanoSDK.AdditionalProjectGlobals"] != null)
			{
				string[] elements = Project.Properties["package.CapilanoSDK.AdditionalProjectGlobals"].Split();
				foreach (string element in elements)
				{
					string element_value = Project.Properties["package.CapilanoSDK.AdditionalProjectGlobals." + element];
					if (!projectGlobals.ContainsKey(element) && element_value != null)
					{
						projectGlobals.Add(element, element_value);
					}
				}
			}

			var osname = Project.Properties["package.CapilanoXDK.osname"];
			if (String.IsNullOrEmpty(osname) && Project.Properties.Contains("build.env.MSBuildExtensionsPath")) // Check build.env.MSBuildExtensionsPath because we only want to do this for non-proxy package;
			{
				// TODO: remove this if block which sets the default value once more of the CapilanoSDK packages have this property defined. 
				//       and make sure build farm removes this property as a global property from their masterconfig files.

				// the default value for osname comes from the registry, but for non-proxy packages we don't want to use the registry value
				// because we don't want it to be affected by the installed version which could be different, so we set a default value.
				osname = "era";
			}
			if (!String.IsNullOrEmpty(osname) && !projectGlobals.ContainsKey("DurangoInstalledOSName"))
			{
				projectGlobals.Add("DurangoInstalledOSName", osname);
			}

			if(Project.Properties.GetBooleanPropertyOrDefault("gdk.enabled", false))
			{
				var edition = Project.Properties["package.GDK.gdkVersion"];
				if (!String.IsNullOrEmpty(edition) && !projectGlobals.ContainsKey("XdkEditionTarget"))
				{
					projectGlobals.Add("XdkEditionTarget", edition);
				}
			}
		}

		private bool IsProjectWithSdkReferences()
		{
			var sdkrefprop = (Module.Package.Project.Properties[Module.GroupName + ".sdkreferences." + Module.Configuration.System] ?? Module.Package.Project.Properties[Module.GroupName + ".sdkreferences"]).TrimWhiteSpace();
			var globalsdkrefprop = (Module.Package.Project.Properties["package.sdkreferences." + Module.Configuration.System] ?? Module.Package.Project.Properties["package.sdkreferences"]).TrimWhiteSpace();

			return false == (String.IsNullOrEmpty(sdkrefprop) && String.IsNullOrEmpty(globalsdkrefprop));
		}

		public override void ProjectConfiguration(IDictionary<string, string> configurationElements)
		{
			// Need to remove WindowsAppContainer:
			configurationElements.Remove("WindowsAppContainer");

			// Utility projects cause a bug with the December XDK where they recursively generate Layout directories for 
			// app packaging. Setting the LayoutDir property to the default layout dir works around this problem.
			if (Module.IsKindOf(Model.Module.Utility))
			{
				configurationElements.Add("LayoutDir", @"$(Platform)\Layout\");
			}

			bool usedebuglibs = OptionSetUtil.GetOptionSetOrFail(Project, Module.BuildType.Name).GetBooleanOptionOrDefault("usedebuglibs", false);

			configurationElements["UseDebugLibraries"] = usedebuglibs.ToString();
		}

		public override void WriteExtensionToolProperties(IXmlWriter writer)
		{
			//if (Generator.Interface.BuildGenerator.GenerateSingleConfig)
			if (Project.Properties.GetBooleanPropertyOrDefault("generate-single-config", false))
			{
				var layoutDir = CapilanoFunctions.GetModuleLayoutDir(Module.Package.Project, "capilano", Module.Name, Module.BuildGroup.ToString());
				layoutDir = Path.GetDirectoryName(Path.GetDirectoryName(layoutDir));
				layoutDir = PathUtil.RelativePath(layoutDir, Generator.Interface.OutputDir.Path).EnsureTrailingSlash();

				writer.WriteNonEmptyElementString("LayoutDir", layoutDir);
			}
			else
			{
				//IM : When we add support for nant builds we need to add postprocess step and set default layout dir in postprocess step.
				writer.WriteNonEmptyElementString("LayoutDir", PropGroupValue("layoutdir", @"$(MSBuildProjectName)\$(PlatformName)\Layout\"));
			}
			writer.WriteNonEmptyElementString("RemoveExtraDeployFiles", PropGroupBoolValue("remove-extra-deploy-files"));
			writer.WriteNonEmptyElementString("IsolateConfigurationsOnDeploy", PropGroupBoolValue("isolate-configurations-on-deploy"));
			writer.WriteNonEmptyElementString("LayoutExtensionFilter", PropGroupValue("layout-extension-filter"));
			// April GDK has a bug if you write this value to the project file it gets messed up because the same value is written to the .user file. so only write this to the user file from now on in the UserData() function.
			// See https://forums.xboxlive.com/questions/100549/index.html
			if (Project.Properties.GetBooleanPropertyOrDefault("gdk.enabled", false))
			{
				if (StringFunctions.StrVersionLess(null, Project.Properties["package.GDK.gdkVersion"], "200400") == bool.TrueString)
				{
					writer.WriteNonEmptyElementString("DeployMode", PropGroupValue("deploy-mode"));
				}
			}
			else if (!Project.Properties.GetBooleanPropertyOrDefault("gdk.enabled", false))
			{
				writer.WriteNonEmptyElementString("DeployMode", PropGroupValue("deploy-mode"));
			}
			writer.WriteNonEmptyElementString("PullMappingFile", PropGroupValue("pullmappingfile"));
			writer.WriteNonEmptyElementString("PullTemporaryFolder", PropGroupValue("pulltempfolder"));
			writer.WriteNonEmptyElementString("NetworkSharePath", PropGroupValue("networksharepath"));
			writer.WriteNonEmptyElementString("GameOSFilePath", PropGroupValue("gameos"));

			if (Project.Properties["package.CapilanoSDK.use32bitBuildTools"] == "true")
			{
				writer.WriteNonEmptyElementString("_IsNativeEnvironment", "false");
			}
		}


		public override void UserData(IDictionary<string, string> userData)
		{
			if (Module.IsKindOf(Model.Module.Program))
			{
				userData.Clear();
				// Novemeber 2015 and beyond uses XboxOneVCppDebugger instead of DurangoVCppDebugger
				if (Project.Properties.Contains("package.CapilanoSDK.XdkEdition"))
				{
					userData["DebuggerFlavor"] = "XboxOneVCppDebugger";
				}
				else
				{
					userData["DebuggerFlavor"] = "DurangoVCppDebugger";
				}
				
				if(Project.Properties.GetBooleanPropertyOrDefault("gdk.enabled", false))
				{
					userData["DebuggerFlavor"] = "XboxGamingVCppDebugger";
				}

				var deploymode = PropGroupValue("deploy-mode");
				if (!String.IsNullOrEmpty(deploymode))
				{
					userData["DeployMode"] = deploymode;
				}

				var aumidoverride = PropGroupValue("aumidoverride");
				if (!String.IsNullOrEmpty(aumidoverride))
				{
					userData["AumidOverride"] = aumidoverride;
				}
			}
		}

		public override void WriteMsBuildOverrides(IXmlWriter writer)
		{
			string doOnceKey = Generator.Interface.ProjectFileName;

			NAnt.Core.Tasks.DoOnce.Execute(Project, doOnceKey + "GeneratePackageRuntimeMetadata_Target", () =>
			{
				if (Module.IsKindOf(Model.Module.MakeStyle))
				{
					// Without this Make Style projects will fail with
					//  "  Making package application data '....\APPDATA.BIN' from '....\AppxManifest.xml' "
					//  "  Invalid manifest detected - The system cannot locate the object specified.      "
					writer.WriteStartElement("Target");
					writer.WriteAttributeString("Name", "GeneratePackageRuntimeMetadata");
					writer.WriteAttributeString("AfterTargets", "_GenerateCurrentProjectAppxManifest");
					writer.WriteEndElement();
				}
			},
			isblocking: true);

			if (Project.Properties["package.CapilanoSDK.use32bitBuildTools"] == "true")
			{
				Log.Warning.WriteLine("WARNING: Property 'package.CapilanoSDK.use32bitBuildTools' was specified but 32-bit tools are not supported under Visual Studio 2015.  This property is being ignored!");
			}

			string deployTargetName = "_DurangoDeploy";

			// As of the September 2019 GDK, the MSBuild deploy target we want to override is named "_GameCoreDeploy"
			bool isGdkEnabled = Project.Properties.GetBooleanPropertyOrDefault("gdk.enabled", false);
			if (isGdkEnabled)
			{
				string gdkVersion = Project.Properties.GetPropertyOrDefault("package.GDK.gdkVersion", "");
				if (!String.IsNullOrEmpty(gdkVersion))
				{
					int gdkVersionInt = int.Parse(gdkVersion);
					if (gdkVersionInt >= 190900)
					{
						deployTargetName = "_GameCoreDeploy";
					}
				}
				else
				{
					Log.Warning.WriteLine("WARNING: Property 'gdk.enabled' is true, but we couldn't get the GDK's version information. We may end up overriding the wrong MSBuild targets.");
				}
			}

			NAnt.Core.Tasks.DoOnce.Execute(Project, doOnceKey + deployTargetName + "_Target", () =>
			{
				if (Module.Package.Project.Properties.GetBooleanPropertyOrDefault(Module.GroupName + ".CapilanoSDK.skipDeployment", false)
				|| Module.Package.Project.Properties.GetBooleanPropertyOrDefault("package.CapilanoSDK.skipDeployment", false)
				|| Module.IsKindOf(Model.Module.Utility))
				{
					writer.WriteStartElement("Target");
					writer.WriteAttributeString("Name", deployTargetName);
					writer.WriteAttributeString("AfterTargets", "AfterBuild");
					writer.WriteEndElement();
				}
			},
			isblocking: true);

			if (!Generator.Interface.BuildGenerator.IsPortable)
			{
				NAnt.Core.Tasks.DoOnce.Execute(Project, doOnceKey + "_InjectMakePriPath_Target", () =>
				{
					// Microsoft's XDK MSBuild files setup of MakePriExeFullPath only works on Windows 8 SDK.  
					// But we've added properties to reset this to use Windows 10 SDK (if selected in masterconfig),
					// we need to override the MakePriExeFullPath setup as well.
					string winsdk_version = Project.GetPropertyOrDefault("package.WindowsSDK.MajorVersion", "0");
					if (winsdk_version == "10")
					{
						string winSdkKitBinDir = Project.Properties.GetPropertyOrDefault("package.WindowsSDK.kitbin.dir", null);
						if (!string.IsNullOrEmpty(winSdkKitBinDir))
						{
							writer.WriteStartElement("Target");
							writer.WriteAttributeString("Name", "_InjectMakePriPath");
							writer.WriteAttributeString("BeforeTargets", "_GetSdkToolPaths");
							writer.WriteStartElement("PropertyGroup");
							writer.WriteElementString("MakePriExeFullPath", Path.Combine(winSdkKitBinDir, "makepri.exe"));
							writer.WriteEndElement();
							writer.WriteEndElement();
						}
					}
				},
				isblocking: true);
			}

			if (isGdkEnabled)
			{
				NAnt.Core.Tasks.DoOnce.Execute(Project, doOnceKey + "Layout_Target", () =>
				{
					if (Module.Package.Project.Properties.GetBooleanPropertyOrDefault(Module.GroupName + ".CapilanoSDK.skipLayout", false)
					|| Module.Package.Project.Properties.GetBooleanPropertyOrDefault("package.CapilanoSDK.skipLayout", false)
					|| Module.IsKindOf(Model.Module.MakeStyle)
					|| Module.IsKindOf(Model.Module.Utility))
					{
						writer.WriteStartElement("Target");
						writer.WriteAttributeString("Name", "Layout");
						writer.WriteStartElement("Message");
						writer.WriteAttributeString("Text", "Skipping Layout Target since this is a Make or Utility project");
						writer.WriteEndElement();
						writer.WriteEndElement();
					}
				},
				isblocking: true);
			}
		}

		// For now, this is just copied from eaconfig.  We should probably consider refactor this and
		// move these stuff to WindowsSDK package in the future and just have solution generation import
		// a WindowsSDK property override file like the way we did it in CapilanoSDK package.
		private void AddWindowsProjectGlobals(IDictionary<string, string> projectGlobals)
		{
			if (Generator.Interface.BuildGenerator.IsPortable)
			{
				// Only do this for non-portable solution.  For portable solution, we should just use the default MSBuild environment setup.
				// NOTE that currently for portable solution, Microsoft's MSBuild script has a bug that you must have WindowsSDK 8.1 installed.
				// For non-portable solution, we added the Windows 10 SDK's MSBuild properties to properly override to use Windows 10 SDK.
				return;
			}

			string winsdk_version = Project.GetPropertyOrDefault("package.WindowsSDK.MajorVersion", "0");
			if (winsdk_version == "10")
			{
				string targetPlatformVer = Project.Properties.GetPropertyOrDefault("package.WindowsSDK.TargetPlatformVersion", null);
				if (!projectGlobals.ContainsKey("TargetPlatformVersion") && !string.IsNullOrEmpty(targetPlatformVer))
				{
					projectGlobals.Add("TargetPlatformVersion", targetPlatformVer);
				}

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

					// We need to setup the following for Visual Studio to find rc.exe
					string winSdkKitBin64Dir = null;
					if (targetPlatformVer != null)
					{
						winSdkKitBin64Dir = Path.Combine(winSdkKitDir,"bin", targetPlatformVer, "x64");
					}
					else
					{
						winSdkKitBin64Dir = Path.Combine(winSdkKitDir, "bin", "x64");
					}
					if (!projectGlobals.ContainsKey("RCToolPath"))
					{
						projectGlobals.Add("RCToolPath", winSdkKitBin64Dir);
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
}
