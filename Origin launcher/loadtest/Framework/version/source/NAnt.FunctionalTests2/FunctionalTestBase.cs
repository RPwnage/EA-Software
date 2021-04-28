using System;
using System.IO;
using System.Reflection;
using System.Runtime.CompilerServices;

using Microsoft.VisualStudio.TestTools.UnitTesting;

using NAnt.Console;
using NAnt.Core;
using NAnt.Core.PackageCore;
using NAnt.Tests.Util;

namespace NAnt.FunctionalTests2
{
	public abstract class FunctionalTestBase : NAntTestClassBase
	{
		// assume the tests are located relative to this assembly always - not
		// robust but easy to maintain
		private static string s_testsLocation = new Func<string>(() =>
		{
			Assembly thisAssembly = Assembly.GetExecutingAssembly();
			return Path.GetFullPath(Path.Combine
			(
				Path.GetDirectoryName(thisAssembly.Location),
				"../../../source",
				thisAssembly.GetName().Name
			));
		})();

		private static string s_testCommonFragment = new Func<string>(() =>
		{
			Assembly thisAssembly = Assembly.GetExecutingAssembly();
			return Path.GetFullPath(Path.Combine
			(
				Path.GetDirectoryName(thisAssembly.Location),
				"functional-test-common-fragment.xml"
			));
		})();

		// TODO this executing assembly stuff is getting nasty, think of a better way
		private static string s_frameworkLocalDir = new Func<string>(() =>
		{
			return Path.GetFullPath(Path.Combine
			(
				Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location),
				"../../../Local"
			));
		})();

		protected void RunFunctionalTest([CallerMemberName] string testName = null)
		{
			string suiteName = GetType().Name;

			string testOutputDir = Path.Combine(s_frameworkLocalDir, testName);
			string testSourceDir = Path.Combine(s_testsLocation, suiteName, testName);
			string testBuildFile = Path.Combine(testSourceDir, testName + ".build");

			// explicitly clean up package map and doc cache to reset anything done by above masterconfig merging
			LineInfoDocument.ClearCache();
			PackageMap.Reset();

			// reset command line option state
			Option.Reset();

			int returnCode = RunNant
			(
				$"-buildfile:{testBuildFile}",
				$"-buildroot:{testOutputDir}",
				$"-masterconfigfragments:{s_testCommonFragment}",
				"-nometrics"
			);

			Assert.AreEqual(0, returnCode, "Functional test did not return zero.");
		}
	}
}
