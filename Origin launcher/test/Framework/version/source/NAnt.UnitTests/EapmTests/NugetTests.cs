using System;
using System.IO;

using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace EapmTests
{
	[TestClass]
	public class NugetTests : EapmTestBase
	{
		// TODO this is a nice area for quickly testing package download but it doesn't really test anything about how
		// we've wrapped the package - a Project instance that loads the initialize.xml would be a good testing context

		[TestMethod] // test nuget install from nuget v2 uri
		public void NewtonsoftJsonV2ApiTest() => TestNugetInstall("Newtonsoft.Json", "7.0.1", "https://www.nuget.org/api/v2/");

		[TestMethod] // testing v3 api, but also this package requires 3.x *client* version
		public void MicrosoftEntityFrameworkCoreSqliteV3Api() => TestNugetInstall("Microsoft.EntityFrameworkCore.Sqlite.Core", "2.2.6", "https://api.nuget.org/v3/index.json");

		[TestMethod] // test installing from internal arftifactory nuget
		public void DevilCoreArtifactory() => TestNugetInstall("Devil.Core", "1.0.0", "https://artifactory.ap.ea.com/artifactory/api/nuget/firemonkeysqe-nuget-local");

		[TestMethod] // NEST package has dependencies
		public void NESTTest() => TestNugetInstall("NEST", "5.4.0", "https://www.nuget.org/api/v2/");

		[TestMethod] // iron python has a lot of content files
		public void IronPythonTest() => TestNugetInstall("IronPython.StdLib", "2.7.5", "https://www.nuget.org/api/v2/");

		[TestMethod] // this version of log4net has some "any" assemblies for old version which we need to handle
		public void Log4NetTest() => TestNugetInstall("log4net", "1.2.10", "https://www.nuget.org/api/v2/");

		[TestMethod] // this package has framework assemblies (i.e dependencies on build in system assemblies)
		public void WpfToolkitTest() => TestNugetInstall("Extended.Wpf.Toolkit", "3.0.0", "https://www.nuget.org/api/v2/");

		[TestMethod] // rxmain is an unlisted package that proxies for it's dependencies
		public void RxMain() => TestNugetInstall("Rx-Main", "2.2.5", "https://www.nuget.org/api/v2/");

		private void TestNugetInstall(string nugetPackage, string nugetPackageVersion, string nugetSource)
		{
			// setup masterconfig with a nuget package uri
			string masterConfigPath = Path.Combine(m_testDir, "masterconfig.xml");
			File.WriteAllText
			(
				Path.Combine(m_testDir, "masterconfig.xml"),
				$@"<project>
					<masterversions>
						<package name=""{nugetPackage}"" version=""{nugetPackageVersion}"" uri=""nuget-{nugetSource}""/>
					</masterversions>
					<ondemand>true</ondemand>
					<ondemandroot>{m_testDir}</ondemandroot>
				</project>"
			);

			string nugetDownloadDirectory = Path.Combine(m_testDir, nugetPackage, nugetPackageVersion);
			Assert.IsFalse(Directory.Exists(nugetDownloadDirectory), "Download folder existed before install. Test invalid!");

			string recordedOutput = RunEapm("install", nugetPackage, String.Format("-masterconfigfile:{0}", masterConfigPath));

			ValidateInstall(nugetPackage, nugetPackageVersion, nugetDownloadDirectory, recordedOutput);
		}
	}
}