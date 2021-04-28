// (c) Electronic Arts. All rights reserved.

using System;
using System.Xml;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Tasks;
using NAnt.Core.Attributes;
using NAnt.Core.Functions;
using NAnt.Core.Logging;
using NAnt.Core.PackageCore;
using EA.CPlusPlusTasks;
using EA.FrameworkTasks.Model;

using EA.Eaconfig;
using EA.Eaconfig.Core;
using EA.Eaconfig.Build;
using EA.Eaconfig.Modules;
using EA.Eaconfig.Modules.Tools;

namespace EA.Eaconfig
{
	[TaskName("IPhonePackageTask")]
	public class IPhonePackageTask : Task
	{
		public IPhonePackageTask()
		{
		}

		[TaskAttribute("ExeName", Required = true)]
		public string ExeName
		{
			get { return _exename; }
			set { _exename = value; }
		}
		private string _exename;

		[TaskAttribute("ExeBinDir", Required = true)]
		public string ExeBinDir
		{
			get { return _exebindir; }
			set { _exebindir = value.Replace('\\', '/'); }
		}
		private string _exebindir;

		[TaskAttribute("BundleName", Required = true)]
		public string BundleName // Without the .app extension
		{
			get { return _bundlename; }
			set { _bundlename = value.Replace('\\', '/'); }
		}
		private string _bundlename;

		[TaskAttribute("XCodeProjDir", Required = true)]
		public string XCodeProjDir
		{
			get { return _xcodeprojdir; }
			set { _xcodeprojdir = value.Replace('\\', '/'); }
		}
		private string _xcodeprojdir;

		[TaskAttribute("GroupModuleName", Required = true)]
		public string GroupModuleName
		{
			get { return _groupmodulename; }
			set { _groupmodulename = value; }
		}
		private string _groupmodulename;

		protected override void ExecuteTask()
		{
			// The packaging step requires that the build machine has set up the code signing / provisioning stuff with XCode.
			// Created this property to allow user to skip this packaging step.
			bool skipPackagingStep = (Project.GetPropertyOrDefault("iphone-nant-build-skip-app-packaging", "false").ToLowerInvariant() == "true");
			if (skipPackagingStep)
			{
				Log.Status.WriteLine("Skipping iphone app bundle packaging as iphone-nant-build-skip-app-packaging property is provided and set to 'true'.");
				return;
			}

			bool isAppExtension = Project.Properties.GetBooleanPropertyOrDefault(GroupModuleName + ".is-app-extension", false);
			string bundleExtension = isAppExtension ? ".appex" : ".app";

			string exeName = ExeName;
			string appBundleName = BundleName + bundleExtension;
			string exePath = ExeBinDir;
			string exeFullPath = System.IO.Path.Combine(exePath,exeName).Replace('\\','/');
			string appBundleFullPath = System.IO.Path.Combine(exePath, appBundleName).Replace('\\', '/');
			string deploymentTargetVersion = null;
			if (!Project.Properties.Contains("iphone-deployment-target-version"))
			{
				throw new BuildException("Unable to create xcode wrapper to create the app bundle!  The property 'iphone-deployment-target-version' is missing!");
			}
			else
			{
				deploymentTargetVersion = Project.GetPropertyValue("iphone-deployment-target-version");
			}
			string baseSdkVersion = Project.GetPropertyOrDefault("iphone-base-sdk-version", "");  // Default to empty means use latest version on the system!
			string xcodeConfigName = "Debug";
			if (Project.NamedOptionSets.ContainsKey("config-options-common"))
			{
				OptionSet cmnOptSet = Project.NamedOptionSets["config-options-common"];
				string debugFlagOpt = cmnOptSet.GetOptionOrDefault("debugflags", "on").ToLowerInvariant();
				if (debugFlagOpt != "on")
				{
					xcodeConfigName = "Release";
				}
			}
			bool enterpriseProvisioning = (Project.GetPropertyOrDefault("enterpriseProvisioning", "false").ToLowerInvariant() == "true");
			string sdkname = "iphoneos";
			bool isIphoneSim = Project.Properties.GetBooleanPropertyOrDefault("config.iphone-sim",false);
			bool is64Bit = (Project.Properties.GetPropertyOrDefault("config-processor", "") == "arm64");
			if (isIphoneSim)
			{
				sdkname = "iphonesimulator";
				is64Bit = (Project.Properties.GetPropertyOrDefault("config-processor", "") == "x64");
			}
			string codesignIdentity = QuoteStringIfNecessary(Project.Properties.GetPropertyOrDefault("iphone-codesign-identity", null));
			string provisioningProfile = QuoteStringIfNecessary(Project.Properties.GetPropertyOrDefault("iphone-provisioning-profile", null));
			string developmentTeam = QuoteStringIfNecessary(Project.Properties.GetPropertyOrDefault("iphone-development-team", null));

			PbxprojData pbxprojData = new PbxprojData();
			string xcodeProjName = exeName + ".xcodeproj";
			string xcodeProjPackage = System.IO.Path.Combine(XCodeProjDir,xcodeProjName);

			string infoPListFile = Project.GetPropertyOrDefault(GroupModuleName + ".iphone-info.plist", null);
			if (infoPListFile == null)
			{
				infoPListFile = Project.GetPropertyOrDefault("iphone-info.plist", null);
			}
			if (infoPListFile == null)
			{
				infoPListFile = System.IO.Path.Combine(XCodeProjDir, exeName + "Info.plist");
				string defaultBundleIdentifier = String.Format("com.ea.{0}", exeName);
				string bundleIdentifier = Project.GetPropertyOrDefault(GroupModuleName + ".iphone-bundle-id", 
											Project.GetPropertyOrDefault("iphone-bundle-id", 
											defaultBundleIdentifier));
				pbxprojData.BuildDefaultInfoPListFile(infoPListFile, exeName, BundleName, bundleIdentifier, deploymentTargetVersion);
			}

			// Now populate the data needed to create the pbxproj file.
			pbxprojData.CreatePbxFileReference(System.IO.Path.GetFileName(infoPListFile), infoPListFile);
			pbxprojData.CreatePbxFileReference(exeName, exeFullPath);
			pbxprojData.CreatePbxFileReference(appBundleName, appBundleFullPath);
			if (isAppExtension)
				pbxprojData.PBXFileReferences[appBundleFullPath].explicitFileType = "wrapper.app-extension";
			else
				pbxprojData.PBXFileReferences[appBundleFullPath].explicitFileType = "wrapper.application";

			pbxprojData.CreatePbxGroup("ROOT");
			pbxprojData.CreatePbxGroup("misc", "ROOT");
			pbxprojData.PBXGroups["misc"].AddChild(pbxprojData.PBXFileReferences[infoPListFile]);

			pbxprojData.CreatePbxGroup("resources", "ROOT");
			pbxprojData.PBXGroups["resources"].AddChild(pbxprojData.PBXFileReferences[exeFullPath]);
			pbxprojData.PBXGroups["resources"].AddChild(pbxprojData.PBXFileReferences[appBundleFullPath]);

			pbxprojData.CreateProjBuildConfig(xcodeConfigName);
			pbxprojData.ProjBuildConfig[xcodeConfigName].BuildSettings.Add("PRODUCT_NAME", String.Format("\"{0}\"", BundleName));
			// $(ARCHS_STANDARD) evaluate to something different on different Xcode.  On XCode 5.1.1, it evaluates to armv7, armv7s, arm64 for iphoneos and i383, x86_64 for iphonesimulator
			if (is64Bit)
				pbxprojData.ProjBuildConfig[xcodeConfigName].BuildSettings.Add("ARCHS", "\"$(ARCHS_STANDARD)\"");
			else
				pbxprojData.ProjBuildConfig[xcodeConfigName].BuildSettings.Add("ARCHS", "\"$(ARCHS_STANDARD_32_BIT)\"");
			if (!String.IsNullOrEmpty(codesignIdentity))
			{
				pbxprojData.ProjBuildConfig[xcodeConfigName].BuildSettings.Add("CODE_SIGN_IDENTITY", codesignIdentity);
				pbxprojData.ProjBuildConfig[xcodeConfigName].BuildSettings.Add(String.Format("\"CODE_SIGN_IDENTITY[sdk={0}*]\"", sdkname), codesignIdentity);
			}
			else
			{
				if (enterpriseProvisioning)
					pbxprojData.ProjBuildConfig[xcodeConfigName].BuildSettings.Add(String.Format("\"CODE_SIGN_IDENTITY[sdk={0}*]\"", sdkname), "\"iPhone Distribution\"");
				else
					pbxprojData.ProjBuildConfig[xcodeConfigName].BuildSettings.Add(String.Format("\"CODE_SIGN_IDENTITY[sdk={0}*]\"", sdkname), "\"iPhone Developer\"");
			}
			if (!String.IsNullOrEmpty(provisioningProfile))
			{
				pbxprojData.ProjBuildConfig[xcodeConfigName].BuildSettings.Add("PROVISIONING_PROFILE", provisioningProfile);
				pbxprojData.ProjBuildConfig[xcodeConfigName].BuildSettings.Add(String.Format("\"PROVISIONING_PROFILE[sdk={0}*]\"", sdkname), provisioningProfile);
				pbxprojData.ProjBuildConfig[xcodeConfigName].BuildSettings.Add("PROVISIONING_PROFILE_SPECIFIER", provisioningProfile);
				pbxprojData.ProjBuildConfig[xcodeConfigName].BuildSettings.Add(String.Format("\"PROVISIONING_PROFILE_SPECIFIER[sdk={0}*]\"", sdkname), provisioningProfile);
			}
			if (!String.IsNullOrEmpty(developmentTeam))
			{
				pbxprojData.ProjBuildConfig[xcodeConfigName].BuildSettings.Add("DEVELOPMENT_TEAM", developmentTeam);
			}
			pbxprojData.ProjBuildConfig[xcodeConfigName].BuildSettings.Add("COMPRESS_PNG_FILES", "NO");
			pbxprojData.ProjBuildConfig[xcodeConfigName].BuildSettings.Add("PREBINDING", "NO");
			pbxprojData.ProjBuildConfig[xcodeConfigName].BuildSettings.Add("SDKROOT", sdkname + baseSdkVersion);
			pbxprojData.ProjBuildConfig[xcodeConfigName].BuildSettings.Add("INFOPLIST_FILE", PbxprojData.RelativePathTo(XCodeProjDir, infoPListFile));
			pbxprojData.ProjBuildConfig[xcodeConfigName].BuildSettings.Add("IPHONEOS_DEPLOYMENT_TARGET", deploymentTargetVersion);
			pbxprojData.ProjBuildConfig[xcodeConfigName].BuildSettings.Add("COPY_PHASE_STRIP", "NO");
			pbxprojData.ProjBuildConfig[xcodeConfigName].BuildSettings.Add("SYMROOT", String.Format("\"{0}\"", exePath));
			pbxprojData.ProjBuildConfig[xcodeConfigName].BuildSettings.Add("OBJROOT", String.Format("\"{0}\"", XCodeProjDir));
			pbxprojData.ProjBuildConfig[xcodeConfigName].BuildSettings.Add("CONFIGURATION_BUILD_DIR", "\"$(BUILD_DIR)\"");
			pbxprojData.ProjBuildConfig[xcodeConfigName].BuildSettings.Add("CONFIGURATION_TEMP_DIR", "\"$(PROJECT_TEMP_DIR)\"");
			pbxprojData.CreateProjConfigList(xcodeConfigName);
			pbxprojData.ProjConfigList[xcodeConfigName].DefaultConfigName = xcodeConfigName;
			pbxprojData.ProjConfigList[xcodeConfigName].BuildConfigs.Add(pbxprojData.ProjBuildConfig[xcodeConfigName]);

			pbxprojData.CreateTargetBuildConfig(xcodeConfigName);
			pbxprojData.CreateTargetConfigList(xcodeConfigName);
			pbxprojData.TargetConfigList[xcodeConfigName].DefaultConfigName = xcodeConfigName;
			pbxprojData.TargetConfigList[xcodeConfigName].BuildConfigs.Add(pbxprojData.TargetBuildConfig[xcodeConfigName]);

			pbxprojData.CreateBuildFile(exeName + " in Resources");
			pbxprojData.BuildFiles[exeName + " in Resources"].FileRef = pbxprojData.PBXFileReferences[exeFullPath];

			pbxprojData.CreateBuildPhases("Resources");
			pbxprojData.BuildPhases["Resources"].BuildFiles.Add(pbxprojData.BuildFiles[exeName + " in Resources"]);

			pbxprojData.NativeTarget.Name = BundleName;
			pbxprojData.NativeTarget.ProductName = BundleName;
			pbxprojData.NativeTarget.ProductReference = pbxprojData.PBXFileReferences[appBundleFullPath];
			pbxprojData.NativeTarget.BuildConfigurationList = pbxprojData.TargetConfigList[xcodeConfigName];
			pbxprojData.NativeTarget.BuildPhases.Add(pbxprojData.BuildPhases["Resources"]);

			pbxprojData.Project.BuildConfigList = pbxprojData.ProjConfigList[xcodeConfigName];
			pbxprojData.Project.MainGroup = pbxprojData.PBXGroups["ROOT"];
			pbxprojData.Project.Targets.Add(pbxprojData.NativeTarget);

			// Now write out the project file.
			pbxprojData.WriteOutProjectFile(xcodeProjPackage);

			// Now build this xcodeproj!
			if (!System.IO.Directory.Exists(xcodeProjPackage))
			{
				throw new BuildException("Failed to create the wrapper XCode project!");
			}
			string xcodeConfigSwitch = String.Format(" -configuration {0}", xcodeConfigName);
			string sdkVersionSwitch = String.Format(" -sdk {0}{1}", sdkname, baseSdkVersion);

			// Get full path to xcodebuild executable.
			string xcodebuild_app = "xcodebuild";
			string xcodeToolsBinPath = Project.GetPropertyOrDefault("package.iphonesdk.toolbindir", Project.GetPropertyOrDefault("package.iphonesdk.xcrundir", null));
			if (!String.IsNullOrEmpty(xcodeToolsBinPath))
			{
				xcodebuild_app = Path.Combine(xcodeToolsBinPath,xcodebuild_app).Replace("\\","/");
			}
			ExecTask execTask = new ExecTask();
			execTask.Project = Project;
			execTask.Parent = this;
			execTask.Program = xcodebuild_app;
			execTask.WorkingDirectory = XCodeProjDir;
			execTask.Arguments = String.Format("-project \"{0}\"{1}{2}", xcodeProjPackage, xcodeConfigSwitch, sdkVersionSwitch);
			execTask.Execute();
		}

		private string QuoteStringIfNecessary(string input)
		{
			string output = input;

			if (!String.IsNullOrEmpty(input) && input.Contains(" ") && !input.StartsWith("\""))
			{
				output = "\"" + input + "\"";
			}

			return output;
		}
	}


	class PbxprojData
	{
		// Classes structure for each individual component of the PBXPROJ file.
		public class PbxBaseObject
		{
			protected readonly string mId;
			protected string mName;
			private const string mFormatter = "10181018DEADBEEF{0}";
			private static long mCounter = 1;

			public string Identifier { get { return mId; } }
			public string Name { get { return mName; } }

			public PbxBaseObject(string name)
			{
				mId = GetNextId();
				mName = name;
			}
			public static string GetNextId()
			{
				string id = String.Format(mFormatter, mCounter.ToString("X8"));
				++mCounter;
				return id;
			}
		}

		public class PbxFileReference : PbxBaseObject
		{
			public string explicitFileType = null;
			public string FullPath = null;
			public PbxFileReference(string name, string fullpath)
				: base(name)
			{
				FullPath = fullpath;
			}
		}

		public class PbxGroup : PbxBaseObject
		{
			public List<PbxBaseObject> Children = new List<PbxBaseObject>();
			public string SourceTree = "<group>";
			public PbxGroup(string name) : base(name) { }
			public void AddChild(PbxFileReference fileRef) { Children.Add(fileRef); }
		}

		public class XCBuildConfig : PbxBaseObject
		{
			public Dictionary<string, string> BuildSettings = new Dictionary<string, string>();
			public XCBuildConfig(string name) : base(name) { }
		}

		public class XCConfigList : PbxBaseObject
		{
			public string DefaultConfigName;
			public List<XCBuildConfig> BuildConfigs = new List<XCBuildConfig>();
			public XCConfigList(string name) : base(name) { }
		}

		public class PbxBuildFile : PbxBaseObject
		{
			public PbxFileReference FileRef;
			public PbxBuildFile(string name) : base(name) { FileRef = null; }
		}

		public class PbxBuildPhase : PbxBaseObject
		{
			public List<PbxBuildFile> BuildFiles = new List<PbxBuildFile>();
			public PbxBuildPhase(string name) : base(name) { }
		}

		public class PbxTargetBase : PbxBaseObject
		{
			public PbxTargetBase(string name) : base(name) { }
		}

		public class PbxNativeTarget : PbxTargetBase
		{
			public string ProductName;
			public PbxFileReference ProductReference;
			public XCConfigList BuildConfigurationList;
			public List<PbxBuildPhase> BuildPhases = new List<PbxBuildPhase>();
			public PbxNativeTarget(string name) : base(name) { }
			public new string Name { get { return mName; } set { mName = value; } }
		}

		public class PbxProject : PbxBaseObject
		{
			public XCConfigList BuildConfigList;
			public PbxGroup MainGroup;
			public List<PbxTargetBase> Targets = new List<PbxTargetBase>();
			public PbxProject(string name) : base(name) { }
		}

		// PBXPROJ's data.
		public Dictionary<string, PbxGroup> PBXGroups = new Dictionary<string, PbxGroup>();
		public Dictionary<string, PbxFileReference> PBXFileReferences = new Dictionary<string, PbxFileReference>();
		public Dictionary<string, XCBuildConfig> ProjBuildConfig = new Dictionary<string, XCBuildConfig>();
		public Dictionary<string, XCConfigList> ProjConfigList = new Dictionary<string, XCConfigList>();
		public Dictionary<string, XCBuildConfig> TargetBuildConfig = new Dictionary<string, XCBuildConfig>();
		public Dictionary<string, XCConfigList> TargetConfigList = new Dictionary<string, XCConfigList>();
		public Dictionary<string, PbxBuildFile> BuildFiles = new Dictionary<string, PbxBuildFile>();
		public Dictionary<string, PbxBuildPhase> BuildPhases = new Dictionary<string, PbxBuildPhase>();
		public PbxNativeTarget NativeTarget = new PbxNativeTarget("");
		public PbxProject Project = new PbxProject("");

		public PbxprojData()
		{
		}

		public void CreatePbxFileReference(string filename, string fullpath)
		{
			if (!PBXFileReferences.ContainsKey(fullpath))
			{
				PBXFileReferences.Add(fullpath, new PbxFileReference(filename, fullpath));
			}
		}

		public void CreatePbxGroup(string groupName, string parentGroupName = null)
		{
			if (!PBXGroups.ContainsKey(groupName))
			{
				PbxGroup newGrp = new PbxGroup(groupName);
				PBXGroups.Add(groupName, newGrp);
				if (parentGroupName != null)
				{
					if (!PBXGroups.ContainsKey(parentGroupName))
					{
						throw new Exception(String.Format("PBXGroup for '{0}' has not been added yet!", parentGroupName));
					}
					PBXGroups[parentGroupName].Children.Add(newGrp);
				}
			}
		}

		public void CreateProjBuildConfig(string configName)
		{
			if (!ProjBuildConfig.ContainsKey(configName))
			{
				ProjBuildConfig.Add(configName, new XCBuildConfig(configName));
			}
		}

		public void CreateProjConfigList(string configName)
		{
			if (!ProjConfigList.ContainsKey(configName))
			{
				ProjConfigList.Add(configName, new XCConfigList(configName));
			}
		}

		public void CreateTargetBuildConfig(string configName)
		{
			if (!TargetBuildConfig.ContainsKey(configName))
			{
				TargetBuildConfig.Add(configName, new XCBuildConfig(configName));
			}
		}

		public void CreateTargetConfigList(string configName)
		{
			if (!TargetConfigList.ContainsKey(configName))
			{
				TargetConfigList.Add(configName, new XCConfigList(configName));
			}
		}

		public void CreateBuildFile(string key)
		{
			if (!BuildFiles.ContainsKey(key))
			{
				BuildFiles.Add(key, new PbxBuildFile(key));
			}
		}

		public void CreateBuildPhases(string name)
		{
			if (!BuildPhases.ContainsKey(name))
			{
				BuildPhases.Add(name, new PbxBuildPhase(name));
			}
		}

		public void BuildDefaultInfoPListFile(string filename, string exeFileName, string bundleName, string bundleIdentifier, string deploymentTargetVersion)
		{
			System.IO.File.WriteAllText(filename, String.Format(
@"{{
	""CFBundleDevelopmentRegion"" = English;
	CFBundleName = ""{0}"";
	CFBundleDisplayName = ""{0}"";
	CFBundleExecutable = {1};
	CFBundleIdentifier = ""{2}"";
	""CFBundleInfoDictionaryVersion"" = ""6.0"";
	CFBundlePackageType = APPL;
	""CFBundleSupportedPlatforms"" = ( iPhoneOS, );
	CFBundleVersion = ""2.0"";
	DTPlatformName = iphoneos;
	DTSDKName = ""iphoneos{3}"";
	LSRequiresIPhoneOS = YES;
	MinimumOSVersion = ""{3}"";
}}", bundleName, exeFileName, bundleIdentifier, deploymentTargetVersion).Replace("\r", ""));
		}

		public static string RelativePathTo(string from, string to)
		{
			const char ourCrossPlatformDirSeperator = '/';
			from = from.Replace(System.IO.Path.DirectorySeparatorChar, ourCrossPlatformDirSeperator);
			to = to.Replace(System.IO.Path.DirectorySeparatorChar, ourCrossPlatformDirSeperator);

			bool isRooted = System.IO.Path.IsPathRooted(from) && System.IO.Path.IsPathRooted(to);

			if (isRooted)
			{
				bool isDifferentRoot = string.Compare(System.IO.Path.GetPathRoot(from), System.IO.Path.GetPathRoot(to), true) != 0;
				if (isDifferentRoot)
					return to;
			}

			List<String> relativePath = new List<String>();
			string[] froms = from.Split(ourCrossPlatformDirSeperator);

			string[] tos = to.Split(ourCrossPlatformDirSeperator);

			int length = Math.Min(
				froms.Length,
				tos.Length);

			int lastCommonRoot = -1;

			// find common root
			for (int x = 0; x < length; x++)
			{
				if (string.Compare(froms[x],
					tos[x], true) != 0)
					break;

				lastCommonRoot = x;
			}
			if (lastCommonRoot == -1)
				return to;

			// add relative folders in from path
			for (int x = lastCommonRoot + 1; x < froms.Length; x++)
				if (froms[x].Length > 0)
					relativePath.Add("..");

			// add to folders to path
			for (int x = lastCommonRoot + 1; x < tos.Length; x++)
				relativePath.Add(tos[x]);

			// create relative path
			string[] relativeParts = new string[relativePath.Count];
			relativePath.CopyTo(relativeParts, 0);

			string newPath = string.Join(
				ourCrossPlatformDirSeperator.ToString(),
				relativeParts);

			return newPath;
		}

		public void WriteOutProjectFile(string xcodeProj)
		{
			System.IO.Directory.CreateDirectory(xcodeProj);
			string xcodeProjName = System.IO.Path.GetFileName(xcodeProj);
			string xcodeProjDir = xcodeProj.Substring(0, xcodeProj.Length - xcodeProjName.Length - 1);
			using (System.IO.TextWriter tw = new System.IO.StreamWriter(System.IO.Path.Combine(xcodeProj, "project.pbxproj")))
			{
				System.CodeDom.Compiler.IndentedTextWriter itw = new System.CodeDom.Compiler.IndentedTextWriter(tw);
				itw.WriteLine("// !$*UTF8*$!");
				itw.WriteLine("{");
				++itw.Indent;
				itw.WriteLine("archiveVersion = 1;");
				itw.WriteLine("classes = {");
				itw.WriteLine("};");
				itw.WriteLine("objectVersion = 46;");
				itw.WriteLine("objects = {");
				++itw.Indent;

				// Write PBXBuildFile section
				foreach (KeyValuePair<string, PbxBuildFile> itm in BuildFiles)
				{
					PbxBuildFile bf = itm.Value;
					itw.WriteLine("{0} = {{", bf.Identifier);
					++itw.Indent;
					itw.WriteLine("isa = PBXBuildFile;");
					itw.WriteLine("fileRef = {0};", bf.FileRef.Identifier);
					--itw.Indent;
					itw.WriteLine("};");
				}
				bool isAppExtension = false;
				foreach (KeyValuePair<string, PbxFileReference> itm in PBXFileReferences)
				{
					PbxFileReference fref = itm.Value;
					itw.WriteLine("{0} = {{", fref.Identifier);
					++itw.Indent;
					itw.WriteLine("isa = PBXFileReference;");
					itw.WriteLine("includeInIndex = 0;");
					if (!String.IsNullOrEmpty(fref.explicitFileType))
					{
						itw.WriteLine("explicitFileType = \"{0}\";", fref.explicitFileType);
						isAppExtension = (fref.explicitFileType == "wrapper.app-extension");
					}
					itw.WriteLine("name = \"{0}\";", fref.Name);
					itw.WriteLine("path = \"{0}\";", RelativePathTo(xcodeProjDir, fref.FullPath));
					itw.WriteLine("sourceTree = \"<group>\";");
					--itw.Indent;
					itw.WriteLine("};");
				}
				foreach (KeyValuePair<string, PbxGroup> itm in PBXGroups)
				{
					PbxGroup grp = itm.Value;
					itw.WriteLine("{0} = {{", grp.Identifier);
					++itw.Indent;
					itw.WriteLine("isa = PBXGroup;");
					itw.WriteLine("children = (");
					++itw.Indent;
					foreach (PbxBaseObject obj in grp.Children)
					{
						itw.WriteLine("{0},", obj.Identifier);
					}
					--itw.Indent;
					itw.WriteLine(");");
					if (!String.IsNullOrEmpty(grp.Name) && grp.Name != "ROOT")
					{
						itw.WriteLine("name = \"{0}\";", grp.Name);
					}
					itw.WriteLine("sourceTree = \"{0}\";", grp.SourceTree);
					--itw.Indent;
					itw.WriteLine("};");
				}

				itw.WriteLine("{0} = {{", NativeTarget.Identifier);
				++itw.Indent;
				itw.WriteLine("isa = PBXNativeTarget;");
				itw.WriteLine("buildRules = (");
				itw.WriteLine(");");
				if (isAppExtension)
					itw.WriteLine("productType = \"com.apple.product-type.app-extension\";");
				else
					itw.WriteLine("productType = \"com.apple.product-type.application\";");
				itw.WriteLine("productReference = {0};", NativeTarget.ProductReference.Identifier);
				itw.WriteLine("buildConfigurationList = {0};", NativeTarget.BuildConfigurationList.Identifier);
				itw.WriteLine("buildPhases = (");
				++itw.Indent;
				foreach (PbxBuildPhase bf in NativeTarget.BuildPhases)
				{
					itw.WriteLine("{0},", bf.Identifier);
				}
				--itw.Indent;
				itw.WriteLine(");");
				itw.WriteLine("dependencies = (");
				itw.WriteLine(");");
				itw.WriteLine("name = \"{0}\";", NativeTarget.Name);
				itw.WriteLine("productName = \"{0}\";", NativeTarget.ProductName);
				--itw.Indent;
				itw.WriteLine("};");


				itw.WriteLine("{0} = {{", Project.Identifier);
				++itw.Indent;
				itw.WriteLine("isa = PBXProject;");
				itw.WriteLine("buildConfigurationList = {0};", Project.BuildConfigList.Identifier);
				itw.WriteLine("compatibilityVersion = \"Xcode 3.1\";");
				itw.WriteLine("hasScannedForEncodings = 1;");
				itw.WriteLine("mainGroup = {0};", Project.MainGroup.Identifier);
				itw.WriteLine("projectDirPath = \"\";");
				itw.WriteLine("projectRoot = \"\";");
				itw.WriteLine("targets = (");
				++itw.Indent;
				foreach (PbxTargetBase tg in Project.Targets)
				{
					itw.WriteLine("{0},", tg.Identifier);
				}
				--itw.Indent;
				itw.WriteLine(");");
				--itw.Indent;
				itw.WriteLine("};");

				itw.WriteLine("{0} = {{", BuildPhases["Resources"].Identifier);
				++itw.Indent;
				itw.WriteLine("isa = PBXResourcesBuildPhase;");
				itw.WriteLine("buildActionMask = 134217727;");
				itw.WriteLine("runOnlyForDeploymentPostprocessing = 0;");
				itw.WriteLine("files = (");
				++itw.Indent;
				foreach (PbxBuildFile bf in BuildPhases["Resources"].BuildFiles)
				{
					itw.WriteLine("{0},", bf.Identifier);
				}
				--itw.Indent;
				itw.WriteLine(");");
				--itw.Indent;
				itw.WriteLine("};");

				foreach (KeyValuePair<string, XCBuildConfig> itm in ProjBuildConfig)
				{
					XCBuildConfig bc = itm.Value;
					itw.WriteLine("{0} = {{", bc.Identifier);
					++itw.Indent;
					itw.WriteLine("isa = XCBuildConfiguration;");
					itw.WriteLine("name = \"{0}\";", bc.Name);
					itw.WriteLine("buildSettings = {");
					++itw.Indent;
					foreach (KeyValuePair<string, string> bsItm in bc.BuildSettings)
					{
						itw.WriteLine("{0} = {1};", bsItm.Key, bsItm.Value);
					}
					--itw.Indent;
					itw.WriteLine("};");
					--itw.Indent;
					itw.WriteLine("};");
				}
				foreach (KeyValuePair<string, XCBuildConfig> itm in TargetBuildConfig)
				{
					XCBuildConfig bc = itm.Value;
					itw.WriteLine("{0} = {{", bc.Identifier);
					++itw.Indent;
					itw.WriteLine("isa = XCBuildConfiguration;");
					itw.WriteLine("name = \"{0}\";", bc.Name);
					itw.WriteLine("buildSettings = {");
					++itw.Indent;
					foreach (KeyValuePair<string, string> bsItm in bc.BuildSettings)
					{
						itw.WriteLine("{0} = {1};", bsItm.Key, bsItm.Value);
					}
					--itw.Indent;
					itw.WriteLine("};");
					--itw.Indent;
					itw.WriteLine("};");
				}

				foreach (KeyValuePair<string, XCConfigList> itm in ProjConfigList)
				{
					XCConfigList cl = itm.Value;
					itw.WriteLine("{0} = {{", cl.Identifier);
					++itw.Indent;
					itw.WriteLine("isa = XCConfigurationList;");
					itw.WriteLine("defaultConfigurationIsVisible = 0;");
					itw.WriteLine("defaultConfigurationName = \"{0}\";", cl.DefaultConfigName);
					itw.WriteLine("buildConfigurations = (");
					++itw.Indent;
					foreach (XCBuildConfig bc in cl.BuildConfigs)
					{
						itw.WriteLine("{0},", bc.Identifier);
					}
					--itw.Indent;
					itw.WriteLine(");");
					--itw.Indent;
					itw.WriteLine("};");
				}
				foreach (KeyValuePair<string, XCConfigList> itm in TargetConfigList)
				{
					XCConfigList cl = itm.Value;
					itw.WriteLine("{0} = {{", cl.Identifier);
					++itw.Indent;
					itw.WriteLine("isa = XCConfigurationList;");
					itw.WriteLine("defaultConfigurationIsVisible = 0;");
					itw.WriteLine("defaultConfigurationName = \"{0}\";", cl.DefaultConfigName);
					itw.WriteLine("buildConfigurations = (");
					++itw.Indent;
					foreach (XCBuildConfig bc in cl.BuildConfigs)
					{
						itw.WriteLine("{0},", bc.Identifier);
					}
					--itw.Indent;
					itw.WriteLine(");");
					--itw.Indent;
					itw.WriteLine("};");
				}

				--itw.Indent;
				itw.WriteLine("};");
				itw.WriteLine("rootObject = {0};", Project.Identifier);

				--itw.Indent;
				itw.WriteLine("}");
			}
		}
	}
}

