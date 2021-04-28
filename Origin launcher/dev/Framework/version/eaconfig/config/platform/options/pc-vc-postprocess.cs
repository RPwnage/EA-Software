// (c) Electronic Arts. All rights reserved.

using System;
using System.IO;
using System.Linq;
using System.Text;

using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Functions;
using NAnt.Core.PackageCore;
using NAnt.Core.Util;

using EA.Eaconfig.Build;
using EA.Eaconfig.Modules;
using EA.Eaconfig.Modules.Tools;
using EA.FrameworkTasks.Model;
using EA.Tasks.Model;

namespace EA.Eaconfig
{
	[TaskName("pc-vc-postprocess", XmlSchema = false)]
	public class PC_VC_PostprocessOptions : VC_Common_PostprocessOptions
	{
		protected virtual string DefaultNtddiVersion
		{
			// By default, we want to let the sdkddkver.h define this based on WINVER's setup.
			// We only need to use this if we have specific restriction for Service Pack level.
			// HOWEVER, if NTDDI_VERSION is defined, WINVER must be defined and the first 4 digit 
			// NTDDI_VERSION MUST match up with WINVER.  For example, we cannot define 
			// NTDDI_VERSION to 0x06000100 for Vista SP1 and then define WINVER as 0x0601 meaning Windows 7!  
			// Also, note that NTDDI_VERSION's conversion is "[WINVER][2 digit SP#][2 digit sub ver]"

			get { return String.Empty; }
		}

		// Note that this default is NOT used if user provided a define for NTDDI_VERSION through 
		// property eaconfig.defines.ntddi_version or [group].[module].defines.ntddi_version.
		protected virtual string DefaultWinver
		{
			get
			{
				if (Module.Configuration.System == "pc")
				{
					string WindowsSDKVersion = Properties["package.WindowsSDK.MajorVersion"];

					if (WindowsSDKVersion != null)
					{
						int WinSDKver = 0;

						Int32.TryParse(WindowsSDKVersion, out WinSDKver);

						if (WinSDKver >= 8)
							return "0x0601";    // Windows 8 SDK does not support Windows XP, so minimum OS target is Windows 7.
					}

					return "0x0600";            // If not using Windows SDK, or using Windows SDK < 8, set the minimum target OS to Windows Vista.
				}
				else
				{
					return "0x0601";            // Non-PC platforms (Mainly pc64. The winrt config has its own override) target Windows 7 or later.
				}
			}
		}
		
		protected virtual string DefaultWinIeVersion
		{
            // By default, we want to let the sdkddkver.h define this based on
            // WINVER's setup.  We only need to use this if we have specific
            // restriction for Internet Explorer level.  When _WIN32_IE is
            // unset, sdkddkver.h will define it based on the minimum level
            // provided by the release of Windows corresponding to _WIN32_WINNT
			get { return String.Empty; }
		}

		public override void ProcessTool(CcCompiler cc)
		{
			SetWinVersionDefines(cc);
			ClearUsingDirsIfC(cc);

			base.ProcessTool(cc);
		}

		public override void ProcessCustomTool(CcCompiler cc)
		{
			SetWinVersionDefines(cc);
			ClearUsingDirsIfC(cc);

			base.ProcessCustomTool(cc);
		}

		public override void ProcessTool(Linker link)
		{

			base.ProcessTool(link);

			if (Module.BuildType.IsCLR)
			{
				string keyfile = PropGroupValue("keyfile").TrimWhiteSpace();
				if (!String.IsNullOrEmpty(keyfile))
				{
					link.Options.Add(String.Format("-KEYFILE:\"{0}\"", PathNormalizer.Normalize(keyfile, false)));
				}
				string delaysign = PropGroupValue("delaysign").TrimWhiteSpace();
				if (!String.IsNullOrEmpty(delaysign))
				{
					if (delaysign.OptionToBoolean())
					{
						link.Options.Add("-DELAYSIGN");
					}
					else
					{
						link.Options.Add("-DELAYSIGN:NO");
					}
				}

			}

			// --- Set manifest flags ---:
			string embedManifest = null;
			var embed = PropGroupValue("embedmanifest", embedManifest);
			if (embed != null)
			{
				embedManifest = embed.ToBooleanOrDefault(true).ToString().ToLowerInvariant();
			}
			else
			{
				OptionSet buildOptionSet = OptionSetUtil.GetOptionSetOrFail(Project, Module.BuildType.Name);
				
				if (buildOptionSet.Options.ContainsKey("embedmanifest"))
				{
					embedManifest = buildOptionSet.GetBooleanOptionOrDefault("embedmanifest", true).ToString().ToLowerInvariant();
				}
			}
			if (!String.IsNullOrEmpty(embedManifest))
			{
				Module.MiscFlags.Add("embedmanifest", embedManifest);
			}

		}

		public override void Process(Module_Native module)
		{
			base.Process(module);

			if (module.Link != null && !PathString.IsNullOrEmpty(module.Link.ImportLibFullPath))
			{
                // TODO use newer API
                // -dvaliant 2017/01/10
                /* should use ModulePath.Private.GetModuleLibPath(module but that isn't available in FW 7.00.00 and older, using older API for now */
                string libPath = ModulePath.Private.GetModuleLibPath(module.Package.Project, module.Package.Name, module.Name, module.BuildGroup.ToString(), module.BuildType).OutputDir;
                PathString outputpath = module.Link.ImportLibOutputDir;

                // create import lib path before linker is run otherwise linker will error
                if (0 != String.Compare(outputpath.NormalizedPath, module.Link.LinkOutputDir.NormalizedPath, true))
				{
					var prestep = new BuildStep("Create import library directory", BuildStep.PreBuild);
                    prestep.Commands.Add(new Command(String.Empty, createdirectories: new PathString[] { outputpath }));
					module.BuildSteps.Add(prestep);
				}

                // if the lib outputdir for the module is not the same as the importlib output dir for the linker, copy lib to final destination
                // it should be very unlikely that this is true
                if (!outputpath.NormalizedPath.StartsWith(libPath))
				{
                    string postBuildStep = String.Format("if exist {0} {2} {0} {1} -t", 
                        PathFunctions.PathToWindows(module.Package.Project, Path.Combine(module.Link.LinkOutputDir.Path, Path.GetFileName(module.Link.ImportLibFullPath.Path))), 
                        module.Package.Project.GetPropertyValue(Project.NANT_PROPERTY_COPY),
                        libPath);
                    BuildStep step = new BuildStep("copy import library to the lib folder", BuildStep.PostBuild);
                    step.Commands.Insert(0, new Command(postBuildStep));
                    module.BuildSteps.Add(step);
				}
			}

			AddDefaultAssemblies(module);

			AddAssetDeployment(module);
		}

		public override void Process(Module_DotNet module)
		{
			base.Process(module);
			AddDefaultAssemblies(module);
			AddAssetDeployment(module);
		}

		protected virtual void AddDefaultAssemblies(Module_Native module)
		{
			if (module.IsKindOf(EA.FrameworkTasks.Model.Module.Managed))
			{
				if (!module.GetModuleBooleanPropertyValue("skip_defaultassemblies", false)
					|| module.GetModuleBooleanPropertyValue(module.PropGroupName("usedefaultassemblies"), false))
				{
					if (!(module.TargetFrameworkFamily == DotNetFrameworkFamilies.Standard || module.TargetFrameworkFamily == DotNetFrameworkFamilies.Core))
					{
						module.Cc.Assemblies.Include("mscorlib.dll", asIs: true);
						module.Cc.Assemblies.Include("System.dll", asIs: true);
						module.Cc.Assemblies.Include("System.Data.dll", asIs: true);

						// Make sure it is Windows program before adding the followings
						// We can either test for our Framework's optionset has "pcconsole" turned off or
						// test for module.Link.Options (compiler build options) has -subsystem:windows in the list
						OptionSet moduleBuildOption = module.GetModuleBuildOptionSet();
						if (OptionSetUtil.GetBooleanOption(moduleBuildOption, "pcconsole") == false)
						{
							module.Cc.Assemblies.Include("System.Drawing.dll", asIs: true);
							module.Cc.Assemblies.Include("System.Drawing.Design.dll", asIs: true);
							module.Cc.Assemblies.Include("System.Windows.Forms.dll", asIs: true);
							module.Cc.Assemblies.Include("System.XML.dll", asIs: true);
						}
					}
				}
			}
		}

		protected virtual void AddDefaultAssemblies(Module_DotNet module)
		{
			if (module.Compiler != null && module.Package.Project.Properties.GetBooleanPropertyOrDefault(PropGroupName("usedefaultassemblies"), true))
			{
				bool isXaml = false;
				try
				{
					isXaml = module.Compiler.SourceFiles.FileItems.Any(fi => Path.GetExtension(fi.Path.Path) == ".xaml");
				}
				catch (Exception)
				{
				}

				if (module.TargetFrameworkFamily == DotNetFrameworkFamilies.Framework)
				{
					module.Compiler.Assemblies.Include("System.dll", asIs: true);
					module.Compiler.Assemblies.Include("System.Data.dll", asIs: true);
					module.Compiler.Assemblies.Include("System.XML.dll", asIs: true);
					module.Compiler.Assemblies.Include("System.Drawing.dll", asIs: true);
					module.Compiler.Assemblies.Include("System.Drawing.Design.dll", asIs: true);
					if (isXaml)
					{
						module.Compiler.Assemblies.Include("Microsoft.Windows.Design.Extensibility.dll", asIs: true);
					}
					if (!isXaml || module.Package.Project.Properties.GetBooleanPropertyOrDefault("xaml-add-windows-forms-dll", false))
					{
						module.Compiler.Assemblies.Include("System.Windows.Forms.dll", asIs: true);
					}
					if (PackageMap.Instance.GetMasterPackage(Project, "DotNet").Version.StrCompareVersions("3.0") >= 0)
					{
						module.Compiler.Assemblies.Include("System.Core.dll", asIs: true);
					}
				}
				else
				{
					AddDefaultPlatformIndependentDotNetCoreAssemblies(module);

					if (module.GetModuleBooleanPropertyValue("usewindowsforms", false))
					{
						module.Compiler.Assemblies.Include("System.Windows.Forms.dll", asIs: true);
					}
				}
			}
		}


		public virtual void AddAssetDeployment(Module module)
		{
			if (module.DeployAssets && module.Assets != null && module.Assets.Includes.Count > 0 && !module.Package.Project.Properties.GetBooleanPropertyOrDefault(PropGroupName("eaconfig.blastemu"), false))
			{
				var step = new BuildStep("postbuild", BuildStep.PostBuild);
				var targetInput = new TargetInput();
				step.TargetCommands.Add(new TargetCommand("copy-asset-files.pc", targetInput, nativeNantOnly: false));
				module.BuildSteps.Add(step);
			}
		}

		private void SetWinVersionDefines(CcCompiler cc)
		{
			// ------ defines ----- 

			//If the end-user has specified a "winver" property, then use that to add WINVER and _WIN32_WINNT
			//#defines.   Otherwise, we default to the settings specified in about DefaultVinver variable.
			//
			//See this blog entry for more information on these defines.
			//
			//"What's the difference between WINVER, _WIN32_WINNT, _WIN32_WINDOWS, and _WIN32_IE?"
			//http://blogs.msdn.com/oldnewthing/archive/2007/04/11/2079137.aspx 
			//
			//Apprarently microsoft has removed some of the forced _WIN32_IE define in windowssdk, which used to exist in VC8 platformsdk, see the following lines existing in the original platformsdk but not in the new windowssdk
			//  C:\Program Files\Microsoft Visual Studio 8\VC\PlatformSDK\Include\CommCtrl.h   
			//      #ifndef _WINRESRC_
			//      #ifndef _WIN32_IE
			//      #define _WIN32_IE 0x0501
			//      #else
			//      #if (_WIN32_IE < 0x0400) && defined(_WIN32_WINNT) && (_WIN32_WINNT >= 0x0500)
			//      #error _WIN32_IE setting conflicts with _WIN32_WINNT setting
			//      #endif
			//      #endif
			//      #endif
			//
			// There's a new way to define version #'s just set NTDDI_VERSION.  See this page:
			// http://msdn.microsoft.com/en-us/library/aa383745.aspx
			//

			string ntddiVersionPropName = "defines.ntddi_version";
			string winverPropName = "defines.winver";
			string ieVersionPropName = "defines.win32_ie_version";

			string ntddi_version_group = PropGroupValue(ntddiVersionPropName, null);
			string ntddi_version_global = Project.Properties["eaconfig." + ntddiVersionPropName] ?? null;
			string ntddi_version = (ntddi_version_group != null ? ntddi_version_group : (ntddi_version_global != null ? ntddi_version_global : DefaultNtddiVersion)).TrimWhiteSpace();

			string winver_group = PropGroupValue(winverPropName, null);
			string winver_global = Project.Properties["eaconfig." + winverPropName] ?? null;
			string winver = (winver_group != null ? winver_group : (winver_global != null ? winver_global : DefaultWinver)).TrimWhiteSpace();

			string winie_version_group = PropGroupValue(ieVersionPropName, null);
			string winie_version_global = Project.Properties["eaconfig." + ieVersionPropName] ?? null;
			string winie_version = (winie_version_group != null ? winie_version_group : (winie_version_global != null ? winie_version_global : DefaultWinIeVersion)).TrimWhiteSpace();

			if (!String.IsNullOrEmpty(ntddi_version))
			{
				// Error check with syntax
				if (!ntddi_version.StartsWith("0x"))
				{
					if (!String.IsNullOrEmpty(ntddi_version_group))
						throw new BuildException(String.Format("The user defined module property '{0}' must start with '0x'.  It currently has the value {1}", PropGroupName(ntddiVersionPropName), ntddi_version_group));
					else if (!String.IsNullOrEmpty(ntddi_version_global))
						throw new BuildException(String.Format("The user defined global property '{0}' must start with '0x'.  It currently has the value {1}", "eaconfig." + ntddiVersionPropName, ntddi_version_global));
					else
						throw new BuildException(String.Format("INTERNAL ERROR: DefaultNtddiVersion must start with '0x'.  It currently has the value {0} for config-system {1}", DefaultNtddiVersion, Module.Configuration.System));
				}

				// Do some error check to make sure that there are no conflict between user defined NTDDI_VERSION and user defined WINVER
				if (!String.IsNullOrEmpty(winver))
				{
					// Error check with syntax
					if (!winver.StartsWith("0x"))
					{
						if (!String.IsNullOrEmpty(winver_group))
							throw new BuildException(String.Format("The user defined module property '{0}' must start with '0x'.  It currently has the value {1}", PropGroupName(winverPropName), winver_group));
						else if (!String.IsNullOrEmpty(winver_global))
							throw new BuildException(String.Format("The user defined global property '{0}' must start with '0x'.  It currently has the value {1}", "eaconfig." + winverPropName, winver_global));
						else
							throw new BuildException(String.Format("INTERNAL ERROR: DefaultVinver must start with '0x'.  It currently has the value {0} for config-system {1}", DefaultWinver, Module.Configuration.System));
					}
					// Check that NTDDI_VERSION setting doesn't conflict with WINVER setting!
					else if (ntddi_version.Substring(0, 6) != winver)
					{
						if (String.IsNullOrEmpty(winver_group) && String.IsNullOrEmpty(winver_global))
						{
							// The default value is no longer valid and needs to be reset!  WINVER's OS version needs to match up with NTDDI_VERSION's setting (if provided).
							winver = ntddi_version.Substring(0, 6);
						}
						else
						{
							StringBuilder errmsg = new StringBuilder();

							errmsg.Append("Mismatch between NTDDI_VERSION and WINVER!  ");

							if (!String.IsNullOrEmpty(ntddi_version_group))
								errmsg.Append(String.Format("User defined module property '{0}' is set to {1}.  ", PropGroupName(ntddiVersionPropName), ntddi_version_group));
							else if (!String.IsNullOrEmpty(ntddi_version_global))
								errmsg.Append(String.Format("User defined global property '{0}' is set to {1}.  ", "eaconfig." + ntddiVersionPropName, ntddi_version_global));
							else
								errmsg.Append(String.Format("Internal defined DefaultNtddiVersion is set to {0} for config-system {1}.  ", DefaultNtddiVersion, Module.Configuration.System));

							if (!String.IsNullOrEmpty(winver_group))
								errmsg.Append(String.Format("However, user defined module property '{0}' is set to {1}.  ", PropGroupName(winverPropName), winver_group));
							else if (!String.IsNullOrEmpty(winver_global))
								errmsg.Append(String.Format("However, user defined global property '{0}' is set to {1}.  ", "eaconfig." + winverPropName, winver_global));
							else
								errmsg.Append(String.Format("However, internal defined DefaultVinver is set to {0} for config-system {1}.  ", DefaultWinver, Module.Configuration.System));

							throw new BuildException(errmsg.ToString());
						}
					}
				}
				else
				{
					// winver is not defined.  We need to set WINVER with proper value based on NTDDI_VERSION info.
					winver = ntddi_version.Substring(0, 6);
				}

				cc.Defines.Add("NTDDI_VERSION=" + ntddi_version);

				// If we defined NTDDI_VERSION, we must define WINVER or we'll get build error with #error NTDDI_VERSION setting conflicts with _WIN32_WINNT setting.
				cc.Defines.Add("_WIN32_WINNT=" + winver);
				cc.Defines.Add("WINVER=" + winver);
			}
			else if (!String.IsNullOrEmpty(winver))
			{
				// If user didn't specify NTDDI_VERSION, we'll just set the basic WINVER defines and let sdkddkver.h define NTDDI_VERSION
				cc.Defines.Add("_WIN32_WINNT=" + winver);
				cc.Defines.Add("WINVER=" + winver);
			}

            // set the minimum required IE level override if provided
			if (!String.IsNullOrEmpty(winie_version))
            {
				// Error check with syntax
				if (!winie_version.StartsWith("0x"))
				{
					if (!String.IsNullOrEmpty(winie_version_group))
						throw new BuildException(String.Format("The user defined module property '{0}' must start with '0x'.  It currently has the value {1}", PropGroupName(ieVersionPropName), winie_version_group));
					else if (!String.IsNullOrEmpty(ntddi_version_global))
						throw new BuildException(String.Format("The user defined global property '{0}' must start with '0x'.  It currently has the value {1}", "eaconfig." + ieVersionPropName, winie_version_global));
					else
						throw new BuildException(String.Format("INTERNAL ERROR: DefaultWinIeVersion must start with '0x'.  It currently has the value {0} for config-system {1}", DefaultWinIeVersion, Module.Configuration.System));
				}

				cc.Defines.Add("_WIN32_IE=" + winie_version);
            }
		}

		private static void ClearUsingDirsIfC(CcCompiler cc)
		{
			// C compiler chocks on -AI option when it contains spaces. if cc options contain -TC clear usingDirs:
			// UsingDirs aren't used in C code anyway.
			if (null != cc.Options.FirstOrDefault(op => op == "-TC" || op == "/TC"))
			{
				cc.UsingDirs.Clear();
			}
		}
	}
}
