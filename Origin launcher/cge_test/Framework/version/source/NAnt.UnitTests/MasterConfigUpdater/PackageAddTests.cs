using System;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using NAnt.Core.PackageCore;

namespace NAnt.MasterConfigUpdaterTest
{
	[TestClass]
	public class PackageAddTest
    {
        [TestMethod]
        public void AddSimple()
        {
            // straight forward add
            Utils.VerifyChanged
            (
                inputContents:
                @"<project>" + Environment.NewLine +
                @"   <masterversions>" + Environment.NewLine +
                @"      <package name=""OrderingShouldRemainA"" version=""A"" />" + Environment.NewLine +
                @"      <package name=""OrderingShouldRemainB"" version=""B"" />" + Environment.NewLine +
                @"      <package name=""OrderingShouldRemainC"" version=""C"" />" + Environment.NewLine +
                @"    <package    name=""WierdSpacing""     version=""1.00.00""    " + Environment.NewLine +
                @"              uri=""p4://server:1234/location/WierdSpacing/1.00.00""  >" + Environment.NewLine +
                @"          </package>" + Environment.NewLine +
                @" </masterversions>" + Environment.NewLine +
                @" <config package=""eaconfig"" />" + Environment.NewLine +
                @"</project>",
                updateFunction: (MasterConfigUpdater updater) =>
                {
                    updater.AddPackage("NewPackage", "newVersion");
                },
                expectedContents:
                @"<project>" + Environment.NewLine +
                @"   <masterversions>" + Environment.NewLine +
                @"      <package name=""OrderingShouldRemainA"" version=""A"" />" + Environment.NewLine +
                @"      <package name=""OrderingShouldRemainB"" version=""B"" />" + Environment.NewLine +
                @"      <package name=""OrderingShouldRemainC"" version=""C"" />" + Environment.NewLine +
                @"    <package    name=""WierdSpacing""     version=""1.00.00""    " + Environment.NewLine +
                @"              uri=""p4://server:1234/location/WierdSpacing/1.00.00""  >" + Environment.NewLine +
                @"          </package>" + Environment.NewLine +
                @"      <package name=""NewPackage"" version=""newVersion"" />" + Environment.NewLine +
                @" </masterversions>" + Environment.NewLine +
                @" <config package=""eaconfig"" />" + Environment.NewLine +
                @"</project>"
            );
        }

        [TestMethod]
        public void AddToGroup()
        {
            Utils.VerifyChanged
            (
                inputContents:
                @"<project>" + Environment.NewLine +
                @"   <masterversions>" + Environment.NewLine +
                @"      <package name=""OrderingShouldRemainA"" version=""A"" />" + Environment.NewLine +
                @"      <package name=""OrderingShouldRemainB"" version=""B"" />" + Environment.NewLine +
                @"      <package name=""OrderingShouldRemainC"" version=""C"" />" + Environment.NewLine +
                @"    <package    name=""WierdSpacing""     version=""1.00.00""    " + Environment.NewLine +
                @"              uri=""p4://server:1234/location/WierdSpacing/1.00.00""  >" + Environment.NewLine +
                @"          </package>" + Environment.NewLine +
                @"      <grouptype name=""Group"">" + Environment.NewLine +
                @"          <package name=""GroupPackage"" version=""group"" />" + Environment.NewLine +
                @"      </grouptype>" + Environment.NewLine +
                @" </masterversions>" + Environment.NewLine +
                @" <config package=""eaconfig"" />" + Environment.NewLine +
                @"</project>",
                updateFunction: (MasterConfigUpdater updater) =>
                {
                    updater.AddPackage("NewPackage", "newVersion", groupType: "Group");
                },
                expectedContents:
                @"<project>" + Environment.NewLine +
                @"   <masterversions>" + Environment.NewLine +
                @"      <package name=""OrderingShouldRemainA"" version=""A"" />" + Environment.NewLine +
                @"      <package name=""OrderingShouldRemainB"" version=""B"" />" + Environment.NewLine +
                @"      <package name=""OrderingShouldRemainC"" version=""C"" />" + Environment.NewLine +
                @"    <package    name=""WierdSpacing""     version=""1.00.00""    " + Environment.NewLine +
                @"              uri=""p4://server:1234/location/WierdSpacing/1.00.00""  >" + Environment.NewLine +
                @"          </package>" + Environment.NewLine +
                @"      <grouptype name=""Group"">" + Environment.NewLine +
                @"          <package name=""GroupPackage"" version=""group"" />" + Environment.NewLine +
                @"          <package name=""NewPackage"" version=""newVersion"" />" + Environment.NewLine +
                @"      </grouptype>" + Environment.NewLine +
                @" </masterversions>" + Environment.NewLine +
                @" <config package=""eaconfig"" />" + Environment.NewLine +
                @"</project>"
            );
        }

        [TestMethod]
        public void AddToNewGroup()
        {
            Utils.VerifyChanged
            (
                inputContents:
                @"<project>" + Environment.NewLine +
                @"   <masterversions>" + Environment.NewLine +
                @"      <package name=""OrderingShouldRemainA"" version=""A"" />" + Environment.NewLine +
                @"      <package name=""OrderingShouldRemainB"" version=""B"" />" + Environment.NewLine +
                @"      <package name=""OrderingShouldRemainC"" version=""C"" />" + Environment.NewLine +
                @"    <package    name=""WierdSpacing""     version=""1.00.00""    " + Environment.NewLine +
                @"              uri=""p4://server:1234/location/WierdSpacing/1.00.00""  >" + Environment.NewLine +
                @"          </package>" + Environment.NewLine +
                @" </masterversions>" + Environment.NewLine +
                @" <config package=""eaconfig"" />" + Environment.NewLine +
                @"</project>",
                updateFunction: (MasterConfigUpdater updater) =>
                {
                    updater.AddPackage("NewPackage", "newVersion", groupType: "Group");
                },
                expectedContents:
                @"<project>" + Environment.NewLine +
                @"   <masterversions>" + Environment.NewLine +
                @"      <package name=""OrderingShouldRemainA"" version=""A"" />" + Environment.NewLine +
                @"      <package name=""OrderingShouldRemainB"" version=""B"" />" + Environment.NewLine +
                @"      <package name=""OrderingShouldRemainC"" version=""C"" />" + Environment.NewLine +
                @"    <package    name=""WierdSpacing""     version=""1.00.00""    " + Environment.NewLine +
                @"              uri=""p4://server:1234/location/WierdSpacing/1.00.00""  >" + Environment.NewLine +
                @"          </package>" + Environment.NewLine +
                @"      <grouptype name=""Group""><package name=""NewPackage"" version=""newVersion"" /></grouptype>" + Environment.NewLine +
                @" </masterversions>" + Environment.NewLine +
                @" <config package=""eaconfig"" />" + Environment.NewLine +
                @"</project>"
            );
        }

		[TestMethod]
		public void EncodeTest()
		{
			Utils.VerifyChanged
			(
				inputContents:
				@"<project>" + Environment.NewLine +
				@"   <masterversions>" + Environment.NewLine +
				@"      <package name=""testOne"" version=""testOne"" />" + Environment.NewLine +
				@"   </masterversions>" + Environment.NewLine +
				@"   <some typical=""'${looking}'=='framework' &amp;&amp; ${string.with?lots}""/> <!-- of (symbols) -->" + Environment.NewLine +
				@"</project>",
				updateFunction: (MasterConfigUpdater updater) =>
				{
					updater.AddPackage("testTwo", "testTwo");
				},
				expectedContents:
				@"<project>" + Environment.NewLine +
				@"   <masterversions>" + Environment.NewLine +
				@"      <package name=""testOne"" version=""testOne"" />" + Environment.NewLine +
				@"      <package name=""testTwo"" version=""testTwo"" />" + Environment.NewLine +
				@"   </masterversions>" + Environment.NewLine +
				@"   <some typical=""'${looking}'=='framework' &amp;&amp; ${string.with?lots}""/> <!-- of (symbols) -->" + Environment.NewLine +
				@"</project>"
			);
		}
    }
}
