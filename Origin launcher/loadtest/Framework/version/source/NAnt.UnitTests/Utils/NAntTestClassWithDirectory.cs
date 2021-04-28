using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;

using Microsoft.VisualStudio.TestTools.UnitTesting;

using NAnt.Core;
using NAnt.Core.PackageCore;
using NAnt.Core.Util;

namespace NAnt.Tests.Util
{
    public abstract class NAntTestClassWithDirectory : NAntTestClassBase
    {
		protected string m_testDir;

		[TestInitialize]
		public override void TestSetup()
		{
			base.TestSetup();

			m_testDir = TestUtil.CreateTestDirectory(GetType().Name);
		}

		[TestCleanup]
		public override void TestCleanUp()
		{
			if (Directory.Exists(m_testDir))
			{
				PathUtil.DeleteDirectory(m_testDir);
			}

			base.TestCleanUp();
		}

		protected override void InitializePackageMapMasterConfig(Project project, params string[] masterConfigContents)
		{
			// Can't initialize package map if there is a previous instance. Run the reset first
			// in case we still have an instance dangling from some unit test.
			PackageMap.Reset();

			string masterConfigPath = Path.Combine(m_testDir, "masterconfig.xml");
			SeedXmlDocumentCache
			(
				masterConfigPath,
				masterConfigContents
			);
			MasterConfig masterConfig = MasterConfig.Load(project, masterConfigPath);
			PackageMap.Init(project, masterConfig);
			PackageMap.Instance.SetBuildRoot(project, Path.Combine(m_testDir, "build"));
		}

		// returns path to the package build file
		protected virtual string CreatePackage(string package, string version, string buildFileContents = null, string initializeFileContents = null)
		{
			string packageDirectory = PackageDirectory(package, version);
			string buildFile = Path.Combine(packageDirectory, package + ".build");
			
			WriteFile
			(
				buildFile, 
				buildFileContents ?? FileContents
				(
					$"<project>",
					$"	<package name=\"{package}\"/>",
					$"</project>"
				)
			);

			
			WriteFile
			(
				Path.Combine(packageDirectory, "Manifest.xml"),
				$"<package>",
				$"	<packageName>{package}</packageName>",
				$"	<versionName>{version}</versionName>",
				$"	<frameworkVersion>2</frameworkVersion>",
				$"	<buildable>true</buildable>",
				$"</package>"
			);

			WriteFile
			(
				Path.Combine(packageDirectory, "Initialize.xml"),
				initializeFileContents ?? FileContents
				(
					"<package/>"
				)
			);

			return buildFile;
		}

		protected virtual void CreateConfigPackage(string package, string version, string buildFileContents = null, string initializeFileContents = null, bool createTopLevelInit = true, params string[] configs)
		{
			CreatePackage
			(
				package,
				version,
				buildFileContents: buildFileContents,
				initializeFileContents: initializeFileContents
			);

			if (createTopLevelInit)
			{
				CreateTopLevelConfigGlobalInit(package, version);
			}

			foreach (string config in configs)
			{
				CreateConfig(package, version, config);
			}
		}

		protected virtual void CreateTopLevelConfigGlobalInit(string configPackage, string configPackageVersion)
		{
			WriteFile
			(
				Path.Combine(PackageDirectory(configPackage, configPackageVersion), "config", "global", "init.xml"),
				$"<project>",
				$"	<optionset name='config-options-common'>",
				$"		<option name=\"buildset.protobuildtype\" value=\"true\"/>",
				$"	</optionset>",
				$"	<optionset name='config-options-program'>",
				$"		<option name=\"buildset.name\" value=\"StdProgram\"/>",
				$"		<option name=\"buildset.tasks\" value=\"asm cc link\"/>",
				$"	</optionset>",
				$"	<optionset name='config-options-library'>",
				$"		<option name=\"buildset.name\" value=\"StdLibrary\"/>",
				$"		<option name=\"buildset.tasks\" value=\"asm cc lib\"/>",
				$"	</optionset>",
				$"	<optionset name='config-options-dynamiclibrary'>",
				$"		<option name=\"generatedll\" value=\"on\"/>",
				$"		<option name=\"buildset.name\" value=\"DynamicLibrary\"/>",
				$"		<option name=\"buildset.tasks\" value=\"asm cc link\"/>",
				$"	</optionset>",
				$"	<optionset name='MakeStyle'>",
				$"		<option name=\"build.tasks\" value=\"makestyle\"/>",
				$"	</optionset>",
				$"",
				$"	<target name=\"load-package\" style=\"build\" hidden=\"true\">",
				$"		<load-package build-group-names=\"${{eaconfig.build.group.names??runtime}}\" autobuild-target=\"load-package\"/>",
				$"	</target>",
				$"</project>"
			);
		}

		protected virtual void CreateConfig(string configPackage, string configPackageVersion, string config, bool writeConfigSetFile = true)
		{
			string[] configComponents = config.Split('-');
			int compIdx = 0;
			string configSystem = configComponents[compIdx++];
			string configProcessor = configComponents.Length == 4 ? configComponents[compIdx++] : "test";
			string configCompiler = configComponents[compIdx++];
			string configName = configComponents[compIdx++];

			// take a best guess at how these should be set that should be good enough for testing
			Dictionary<string, string> options = new Dictionary<string, string>()
			{
				{ "debug", "off" },
				{ "optimization", "off" },
				{ "profile", "off" },
				{ "retail", "off" }
			};
			if (configName.Contains("debug"))
			{
				options["debug"] = "on";
			}
			else if (configName.Contains("opt") || configName.Contains("release"))
			{
				options["optimization"] = "on";
			}
			else if (configName.Contains("profile"))
			{
				options["profile"] = "on";
			}
			else if(configName.Contains("retail"))
			{
				options["retail"] = "on";
				options["optimization"] = "on";
			}
			string configOptions = String.Join
			(
				Environment.NewLine,
				options.OrderBy(kvp => kvp.Key).Select(kvp => $"		<option name='{kvp.Key}' value='{kvp.Value}'/>")
			);

			string configPlatform = $"{configSystem}-{configProcessor}-{configCompiler}";

			WriteFile
			(
				Path.Combine(PackageDirectory(configPackage, configPackageVersion), "config", $"{config}.xml"),
				$"<package>",
				$"	<optionset name='configuration-definition'>",
				$"		<option name='config-system' value='{configSystem}'/>",
				$"		<option name='config-processor' value='{configProcessor}'/>",
				$"		<option name='config-compiler' value='{configCompiler}'/>",
				$"		<option name='config-name' value='dev-{configCompiler}'/>",
				$"		<option name='config-platform' value='{configPlatform}'/>",
				$"		<option name='config-os' value='testOS'/>",
				configOptions,
				$"	</optionset>",
				$"</package>"
			);



			CreatePlatformFile(configPackage, configPackageVersion, configPlatform);

			if (writeConfigSetFile)
			{
				CreateConfigSetFile(configPackage, configPackageVersion, configName);
			}
		}

		protected virtual void CreateConfigSetFile(string configPackage, string configPackageVersion, string configName)
		{
			WriteFile
			(
				Path.Combine(PackageDirectory(configPackage, configPackageVersion), "config", "configset", $"{configName}.xml"),
				$"<project/>"
			);
		}

		protected virtual void CreatePlatformFile(string configPackage, string configPackageVersion, string platformFile)
		{
			WriteFile 
			(
				Path.Combine(PackageDirectory(configPackage, configPackageVersion), "config", "platform", $"{platformFile}.xml"),
				"<project/>"
			);
		}

		protected string PackageDirectory(string package, string version)
		{
			return Path.Combine(m_testDir, package, version);
		}

		protected string PackageRoot()
		{
			return m_testDir;
		}

		protected static void WriteFile(string path, params string[] contents)
		
  {
			string dir = Path.GetDirectoryName(path);
			Directory.CreateDirectory(dir);

			File.WriteAllText(path, FileContents(contents));
		}

		protected static string FileContents(params string[] contents)
		{
			return String.Join (Environment.NewLine, contents);
		}
	}
}