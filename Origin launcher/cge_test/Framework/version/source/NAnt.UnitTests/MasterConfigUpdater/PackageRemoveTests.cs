using System;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using NAnt.Core.PackageCore;

namespace NAnt.MasterConfigUpdaterTest
{
    [TestClass]
    public class PackageRemoveTest
    {
        [TestMethod]
        public void RemoveSimple()
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
                    bool removed = updater.RemovePackage("OrderingShouldRemainB");
                    Assert.IsTrue(removed, "Should have removed a package!");
                },
                expectedContents:
                @"<project>" + Environment.NewLine +
                @"   <masterversions>" + Environment.NewLine +
                @"      <package name=""OrderingShouldRemainA"" version=""A"" />" + Environment.NewLine +
                @"      <package name=""OrderingShouldRemainC"" version=""C"" />" + Environment.NewLine +
                @"    <package    name=""WierdSpacing""     version=""1.00.00""    " + Environment.NewLine +
                @"              uri=""p4://server:1234/location/WierdSpacing/1.00.00""  >" + Environment.NewLine +
                @"          </package>" + Environment.NewLine +
                @" </masterversions>" + Environment.NewLine +
                @" <config package=""eaconfig"" />" + Environment.NewLine +
                @"</project>"
            );
        }

        [TestMethod]
        public void RemoveFromGroup()
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
                    bool removed = updater.RemovePackage("GroupPackage");
                    Assert.IsTrue(removed, "Should have removed a package!");
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
                @"      </grouptype>" + Environment.NewLine +
                @" </masterversions>" + Environment.NewLine +
                @" <config package=""eaconfig"" />" + Environment.NewLine +
                @"</project>"
            );
        }
    }
}
