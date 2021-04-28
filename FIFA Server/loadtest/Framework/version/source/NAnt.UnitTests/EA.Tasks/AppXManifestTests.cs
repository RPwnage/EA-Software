using System;
using System.IO;
using System.Text;

using Microsoft.VisualStudio.TestTools.UnitTesting;

using NAnt.Core.PackageCore;
using NAnt.Core.Util;
using NAnt.Core;
using NAnt.Tests.Util;

using EA.Eaconfig;
using EA.Eaconfig.Backends.VisualStudio;
using EA.Eaconfig.Modules;
using EA.FrameworkTasks.Model;

namespace EA.Tasks.Tests
{
	[TestClass]
	public class AppXManifestTests : NAntTestClassBase
    {
		[TestMethod]
		public void AppXNetworkManifestValidTest()
		{
			string outputDir = TestUtil.CreateTestDirectory("AppXNetworkManifestTest");
			try
			{
				StringBuilder logRecorder = new StringBuilder();
				Module_Native module = AppXTestCommonSetup(ref logRecorder, outputDir);

				// first test we don't get any exception or warnings from a valid setup
				string netValidManifestPath = Path.Combine(outputDir, "net-valid.xml");
				File.WriteAllText
				(
					netValidManifestPath,
					String.Join
					(
						Environment.NewLine,
						"<Package xmlns=\"http://schemas.microsoft.com/appx/2010/manifest\"",
						"	xmlns:mx=\"http://schemas.microsoft.com/appx/2013/xbox/manifest\" IgnorableNamespaces=\"mx\" >",
						"	<mx:XboxNetworkingManifest>",
						"		<mx:SocketDescriptions>",
						"			<mx:SocketDescription Name=\"insecure-tcp-10000-20000\" SecureIpProtocol=\"Tcp\" BoundPort=\"10000-20000\" >",
						"				<mx:AllowedUsages>",
						"					<mx:SecureDeviceSocketUsage Type=\"Accept\" />",
						"					<mx:SecureDeviceSocketUsage Type=\"ReceiveInsecure\" />",
						"					<mx:SecureDeviceSocketUsage Type=\"SendInsecure\" />",
						"				</mx:AllowedUsages>",
						"			</mx:SocketDescription>",
						"			<mx:SocketDescription Name=\"insecure-udp-10000-20000\" SecureIpProtocol=\"Udp\" BoundPort=\"10000-20000\" >",
						"				<mx:AllowedUsages>",
						"					<mx:SecureDeviceSocketUsage Type=\"Accept\" />",
						"					<mx:SecureDeviceSocketUsage Type=\"ReceiveInsecure\" />",
						"					<mx:SecureDeviceSocketUsage Type=\"SendInsecure\" />",
						"				</mx:AllowedUsages>",
						"			</mx:SocketDescription>",
						"		</mx:SocketDescriptions>",
						"	</mx:XboxNetworkingManifest>",
						"</Package>"
					)
				);
				module.Package.Project.Properties["runtime.TestModule.networking-manifest-files"] = netValidManifestPath;
				AppXManifest.Generate(module.Package.Project.Log, module, new PathString(outputDir), "TestModule", "TestModule");

				Assert.IsFalse(logRecorder.ToString().Contains("duplicate BoundPort + Protocol"));

				// test warnings is thrown from duplicate protocol (both tcp)
				string netDuplicateBoundAndProtocolManifestPath = Path.Combine(outputDir, "net-duplicate-bound-and-protocol.xml");
				File.WriteAllText
				(
					netDuplicateBoundAndProtocolManifestPath,
					String.Join
					(
						Environment.NewLine,
						"<Package xmlns=\"http://schemas.microsoft.com/appx/2010/manifest\"",
						"	xmlns:mx=\"http://schemas.microsoft.com/appx/2013/xbox/manifest\" IgnorableNamespaces=\"mx\" >",
						"	<mx:XboxNetworkingManifest>",
						"		<mx:SocketDescriptions>",
						"			<mx:SocketDescription Name=\"insecure-tcp-10000-20000\" SecureIpProtocol=\"Tcp\" BoundPort=\"10000-20000\" >",
						"				<mx:AllowedUsages>",
						"					<mx:SecureDeviceSocketUsage Type=\"Accept\" />",
						"					<mx:SecureDeviceSocketUsage Type=\"ReceiveInsecure\" />",
						"					<mx:SecureDeviceSocketUsage Type=\"SendInsecure\" />",
						"				</mx:AllowedUsages>",
						"			</mx:SocketDescription>",
						"			<mx:SocketDescription Name=\"insecure-tcp2-10000-20000\" SecureIpProtocol=\"Tcp\" BoundPort=\"10000-20000\" >",
						"				<mx:AllowedUsages>",
						"					<mx:SecureDeviceSocketUsage Type=\"Accept\" />",
						"					<mx:SecureDeviceSocketUsage Type=\"ReceiveInsecure\" />",
						"					<mx:SecureDeviceSocketUsage Type=\"SendInsecure\" />",
						"				</mx:AllowedUsages>",
						"			</mx:SocketDescription>",
						"		</mx:SocketDescriptions>",
						"	</mx:XboxNetworkingManifest>",
						"</Package>"
					)
				);
				module.Package.Project.Properties["runtime.TestModule.networking-manifest-files"] = netDuplicateBoundAndProtocolManifestPath;
				AppXManifest.Generate(module.Package.Project.Log, module, new PathString(outputDir), "TestModule", "TestModule");

				Assert.IsTrue(logRecorder.ToString().Contains("duplicate BoundPort + Protocol"));
				logRecorder.Clear();

				// test warnings is thrown from duplicate range (no protocol specified)
				string netNoProtocolManifestPath = Path.Combine(outputDir, "net-no-protocol.xml");
				File.WriteAllText
				(
					netNoProtocolManifestPath,
					String.Join
					(
						Environment.NewLine,
						"<Package xmlns=\"http://schemas.microsoft.com/appx/2010/manifest\"",
						"	xmlns:mx=\"http://schemas.microsoft.com/appx/2013/xbox/manifest\" IgnorableNamespaces=\"mx\" >",
						"	<mx:XboxNetworkingManifest>",
						"		<mx:SocketDescriptions>",
						"			<mx:SocketDescription Name=\"insecure-tcp-10000-20000\" BoundPort=\"10000-20000\" >",
						"				<mx:AllowedUsages>",
						"					<mx:SecureDeviceSocketUsage Type=\"Accept\" />",
						"					<mx:SecureDeviceSocketUsage Type=\"ReceiveInsecure\" />",
						"					<mx:SecureDeviceSocketUsage Type=\"SendInsecure\" />",
						"				</mx:AllowedUsages>",
						"			</mx:SocketDescription>",
						"			<mx:SocketDescription Name=\"insecure-udp-10000-20000\" BoundPort=\"10000-20000\" >",
						"				<mx:AllowedUsages>",
						"					<mx:SecureDeviceSocketUsage Type=\"Accept\" />",
						"					<mx:SecureDeviceSocketUsage Type=\"ReceiveInsecure\" />",
						"					<mx:SecureDeviceSocketUsage Type=\"SendInsecure\" />",
						"				</mx:AllowedUsages>",
						"			</mx:SocketDescription>",
						"		</mx:SocketDescriptions>",
						"	</mx:XboxNetworkingManifest>",
						"</Package>"
					)
				);
				module.Package.Project.Properties["runtime.TestModule.networking-manifest-files"] = netNoProtocolManifestPath;
				AppXManifest.Generate(module.Package.Project.Log, module, new PathString(outputDir), "TestModule", "TestModule");

				Assert.IsTrue(logRecorder.ToString().Contains("duplicate BoundPort + Protocol"));
			}
			finally
			{
				if (Directory.Exists(outputDir))
				{
					Directory.Delete(outputDir, recursive: true);
				}
			}
		}

		private static Module_Native AppXTestCommonSetup(ref StringBuilder logRecorder, string outputDir)
		{
			Project project = new Project(TestUtil.CreateRecordingLog(ref logRecorder));

			string masterconfigpath = Environment.GetEnvironmentVariable("masterconfig_path");
			if (masterconfigpath.IsNullOrEmpty() || !File.Exists(masterconfigpath))
			{
				masterconfigpath = string.Join(outputDir, "masterconfig.xml");
				File.WriteAllText(masterconfigpath,
					string.Join(
						Environment.NewLine,
						"<project>",
						"  <masterversions>",
						"    <package name=\"MSBuildTools\" version=\"dev16\" uri=\"p4://dice-p4-one.dice.ad.ea.com:2001/SDK/MSBuildTools/dev16\" />",
						"  </masterversions>",
						"  <ondemand>true</ondemand>",
						"  <ondemandroot>ondemand</ondemandroot>",
						"  <config default=\"pc-vc-dev\" includes=\"pc-vc-debug\"/>",
						"</project>"
						));
			}
			MasterConfig masterconfigFile = MasterConfig.Load(project, masterconfigpath);

			// Use msbuildtools, specifically for when this runs on the buildfarm where visual studio isn't installed
			project.Properties["package.VisualStudio.use-non-proxy-build-tools"] = "true";

			PackageMap.Init(project, masterConfig: masterconfigFile);
			PackageMap.Instance.SetBuildRoot(project, outputDir); // appx razor template requires VisualStudio package - we can init the package with out setting a root for it even if we never use the root

			// do a whole bunch of setup to create a module
			IPackage package = new Package(new Release(new PackageMap.PackageSpec("TestPackage", "dev"), Environment.CurrentDirectory), "capilano-vc-debug")
			{
				Project = project
			};
			package.SetType(Package.Buildable);

			Module_Native module = new Module_Native
			(
				name: "TestModule",
				groupname: "runtime.TestModule",
				groupsourcedir: Environment.CurrentDirectory,
				configuration: new Configuration("capilano-vc-debug", "capilano", "vc", "capilano-vc", "debug", "x64"),
				buildgroup: BuildGroups.runtime,
				buildType: new BuildType("Program", "Program"),
				package: package
			)
			{
				IntermediateDir = new PathString(outputDir)
			};
			OptionSet manifestOptions = new OptionSet();
			manifestOptions.Options["application.executable"] = "TestModule";
			manifestOptions.Options["application.id"] = "TestPackage";
			manifestOptions.Options["capabilities"] = "internetClientServer";
			manifestOptions.Options["devicecapabilities"] = "";
			manifestOptions.Options["identity.name"] = "TestPackage";
			manifestOptions.Options["identity.publisher"] = "Electronic Arts Inc.";
			manifestOptions.Options["identity.version"] = "1.0.0.0";
			manifestOptions.Options["manifest.filename"] = "package.appxmanifest";
			manifestOptions.Options["mx.capability.list"] = String.Join
			(
				Environment.NewLine,
				"contentRestrictions",
				"kinectAudio",
				"kinectExpressions",
				"kinectFace",
				"kinectGamechat",
				"kinectRequired",
				"kinectVision"
			);
			manifestOptions.Options["properties.description"] = "TestPackage";
			manifestOptions.Options["properties.displayname"] = "TestPackage";
			manifestOptions.Options["properties.logo"] = "StoreLogo.png";
			manifestOptions.Options["properties.publisherdisplayname"] = "Electronic Arts Inc.";
			manifestOptions.Options["visualelements.description"] = "TestPackage";
			manifestOptions.Options["visualelements.displayname"] = "TestPackage";
			manifestOptions.Options["visualelements.logo"] = "Logo.png";
			manifestOptions.Options["visualelements.smalllogo"] = "SmallLogo.png";
			manifestOptions.Options["visualelements.splashscreen"] = "SplashScreen.png";
			manifestOptions.Options["visualelements.widelogo"] = "WideLogo.png";
			project.NamedOptionSets["runtime.TestModule.appxmanifestoptions"] = manifestOptions;

			return module;
		}
	}
}