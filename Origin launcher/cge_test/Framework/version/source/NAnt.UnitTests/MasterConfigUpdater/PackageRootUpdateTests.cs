using System;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using NAnt.Core.PackageCore;

namespace NAnt.MasterConfigUpdaterTest
{
    [TestClass]
    public class PackageRootUpdateTest
    {
        [TestMethod]
        public void UnchangedSimple()
        {
            // typical masterconfig
            Utils.VerifyUnchanged
            (
                @"<project>" + Environment.NewLine +
                @"   <masterversions>" + Environment.NewLine +
                @"      <package name=""TestPackage"" version=""dev"" />" + Environment.NewLine +
                @" </masterversions>" + Environment.NewLine +
                @" <packageroots>" + Environment.NewLine +
                @"      <packageroot>package/root</packageroot>" + Environment.NewLine +
                @" </packageroots>" + Environment.NewLine +
                @" <config package=""eaconfig"" />" + Environment.NewLine +
                @"</project>"
            );
        }

        [TestMethod]
        public void AddPackageRootTest()
        {
            Utils.VerifyChanged
            (
                inputContents:
                @"<project>" + Environment.NewLine +
                @"   <masterversions>" + Environment.NewLine +
                @"      <package name=""TestPackage"" version=""dev"" />" + Environment.NewLine +
                @" </masterversions>" + Environment.NewLine +
                @" <packageroots>" + Environment.NewLine +
                @"      <packageroot>package/root</packageroot>" + Environment.NewLine +
                @" </packageroots>" + Environment.NewLine +
                @" <config package=""eaconfig"" />" + Environment.NewLine +
                @"</project>",
                updateFunction: (MasterConfigUpdater updater) =>
                {
                    updater.AddPackageRoot("package/root/folder");
                },
                expectedContents:
                @"<project>" + Environment.NewLine +
                @"   <masterversions>" + Environment.NewLine +
                @"      <package name=""TestPackage"" version=""dev"" />" + Environment.NewLine +
                @" </masterversions>" + Environment.NewLine +
                @" <packageroots>" + Environment.NewLine +
                @"      <packageroot>package/root</packageroot>" + Environment.NewLine +
                @"      <packageroot>package/root/folder</packageroot>" + Environment.NewLine +
                @" </packageroots>" + Environment.NewLine +
                @" <config package=""eaconfig"" />" + Environment.NewLine +
                @"</project>"
            );
        }
    }
}