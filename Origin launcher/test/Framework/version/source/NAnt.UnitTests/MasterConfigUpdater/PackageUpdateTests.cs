using System;
using System.Collections.Generic;
using System.Linq;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using NAnt.Core.PackageCore;

namespace NAnt.MasterConfigUpdaterTest
{
	[TestClass]
	public class PackageUpdateTest
	{
		[TestMethod]
		public void UnchangedSimple()
		{
			// typical masterconfig
			Utils.VerifyUnchanged
			(
				@"<project>" + Environment.NewLine +
				@"   <masterversions>" + Environment.NewLine +
				@"      <package name=""OrderingShouldRemainA"" version=""UnchangedA"" />" + Environment.NewLine +
				@"      <package name=""OrderingShouldRemainB"" version=""UnchangedB"" />" + Environment.NewLine +
				@"      <package name=""OrderingShouldRemainC"" version=""UnchangedC"" />" + Environment.NewLine +
				@"    <package    name=""WierdSpacing""     version=""1.00.00""    " + Environment.NewLine +
				@"              uri=""p4://server:1234/location/WierdSpacing/1.00.00""  >" + Environment.NewLine +
				@"          </package>" + Environment.NewLine +
				@" </masterversions>" + Environment.NewLine +
				@" <config package=""eaconfig"" />" + Environment.NewLine +
				@"</project>"
			);
		}

		[TestMethod]
		public void UnchangedWithLeadingComment()
		{
			// comment before root
			Utils.VerifyUnchanged
			(
				@"   <!-- top of file comment -->    " + Environment.NewLine +
				@"<project>" + Environment.NewLine +
				@"   <masterversions>" + Environment.NewLine +
				@"      <package name=""OrderingShouldRemainA"" version=""UnchangedA"" />" + Environment.NewLine +
				@"      <package name=""OrderingShouldRemainB"" version=""UnchangedB"" />" + Environment.NewLine +
				@"      <package name=""OrderingShouldRemainC"" version=""UnchangedC"" />" + Environment.NewLine +
				@"    <package    name=""WierdSpacing""     version=""1.00.00""    " + Environment.NewLine +
				@"              uri=""p4://server:1234/location/WierdSpacing/1.00.00""  >" + Environment.NewLine +
				@"          </package>" + Environment.NewLine +
				@" </masterversions>" + Environment.NewLine +
				@" <config package=""eaconfig"" />" + Environment.NewLine +
				@"</project>"
			);
		}

		[TestMethod]
		public void UnchangedWithXmlDeclaration()
		{
			// xml declaration
			Utils.VerifyUnchanged
			(
				@"<?xml version=""1.0""?>" + Environment.NewLine +
				@"<project>" + Environment.NewLine +
				@"   <masterversions>" + Environment.NewLine +
				@"      <package name=""OrderingShouldRemainA"" version=""UnchangedA"" />" + Environment.NewLine +
				@"      <package name=""OrderingShouldRemainB"" version=""UnchangedB"" />" + Environment.NewLine +
				@"      <package name=""OrderingShouldRemainC"" version=""UnchangedC"" />" + Environment.NewLine +
				@"    <package    name=""WierdSpacing""     version=""1.00.00""    " + Environment.NewLine +
				@"              uri=""p4://server:1234/location/WierdSpacing/1.00.00""  >" + Environment.NewLine +
				@"          </package>" + Environment.NewLine +
				@" </masterversions>" + Environment.NewLine +
				@" <config package=""eaconfig"" />" + Environment.NewLine +
				@"</project>"
			);
		}

		[TestMethod]
		public void UnchangedXmlDeclarationVersionAndEncoding()
		{
			// just xml declaration with version and ecnoding
			Utils.VerifyUnchanged
			(
				@"<?xml version=""1.0"" encoding=""UTF-8""?>" + Environment.NewLine +
				@"<project>" + Environment.NewLine +
				@" <config package=""eaconfig"" />" + Environment.NewLine +
				@"</project>"
			);
		}

		[TestMethod]
		public void UnchangedWithXmlDeclarationAndComment()
		{
			// xml declaration with whitespace after
			Utils.VerifyUnchanged
			(
				@"<?xml version=""1.0""?>" + Environment.NewLine +
				@"   <!-- whitespace between prolog and root -->" + Environment.NewLine +
				@"" + Environment.NewLine +
				@"<project>" + Environment.NewLine +
				@"   <masterversions>" + Environment.NewLine +
				@"      <package name=""OrderingShouldRemainA"" version=""UnchangedA"" />" + Environment.NewLine +
				@"      <package name=""OrderingShouldRemainB"" version=""UnchangedB"" />" + Environment.NewLine +
				@"      <package name=""OrderingShouldRemainC"" version=""UnchangedC"" />" + Environment.NewLine +
				@"    <package    name=""WierdSpacing""     version=""1.00.00""    " + Environment.NewLine +
				@"              uri=""p4://server:1234/location/WierdSpacing/1.00.00""  >" + Environment.NewLine +
				@"          </package>" + Environment.NewLine +
				@" </masterversions>" + Environment.NewLine +
				@" <config package=""eaconfig"" />" + Environment.NewLine +
				@"</project>"
			);
		}

		[TestMethod]
		public void UnchangedFragment()
		{
			// simple fragment
			Utils.VerifyUnchanged
			(
				@"<project>" + Environment.NewLine +
				@"   <masterversions>" + Environment.NewLine +
				@"      <package name=""TestPackageA"" version=""A"" />" + Environment.NewLine +
				@"      <package name=""TestPackageB"" version=""B"" />" + Environment.NewLine +
				@"   </masterversions>" + Environment.NewLine +
				@"   <fragments>" + Environment.NewLine +
				@"      <include name=""./*/masterconfig_fragment.xml""/>" + Environment.NewLine +
				@"   </fragments>" + Environment.NewLine +
				@" <config package=""eaconfig"" />" + Environment.NewLine +
				@"</project>",
				new Dictionary<string, string>()
				{
					{
						"test_wildcard_folder\\masterconfig_fragment.xml",
							@"<project>" + Environment.NewLine +
							@"   <masterversions>" + Environment.NewLine +
							@"      <package name=""TestPackageC"" version=""C"" />" + Environment.NewLine +
							@"   </masterversions>" + Environment.NewLine +
							@"</project>"
					}
				}
			);
		}

		[TestMethod]
		public void UpdateSimple()
		{
			// straight forward update
			Utils.VerifyChanged
			(
				inputContents:
				@"<project>" + Environment.NewLine +
				@"   <masterversions>" + Environment.NewLine +
				@"      <package name=""OrderingShouldRemainA"" version=""UnchangedA"" />" + Environment.NewLine +
				@"      <package name=""OrderingShouldRemainB"" version=""UnchangedB"" />" + Environment.NewLine +
				@"      <package name=""OrderingShouldRemainC"" version=""UnchangedC"" />" + Environment.NewLine +
				@"    <package    name=""WierdSpacing""     version=""1.00.00""    " + Environment.NewLine +
				@"              uri=""p4://server:1234/location/WierdSpacing/1.00.00""  >" + Environment.NewLine +
				@"          </package>" + Environment.NewLine +
				@" </masterversions>" + Environment.NewLine +
				@" <config package=""eaconfig"" />" + Environment.NewLine +
				@"</project>",
				updateFunction: (MasterConfigUpdater updater) =>
				{
					updater.GetPackage("OrderingShouldRemainA").AddOrUpdateAttribute("version", "ChangedA");
					updater.GetPackage("OrderingShouldRemainB").AddOrUpdateAttribute("version", "ChangedB");
					updater.GetPackage("OrderingShouldRemainC").AddOrUpdateAttribute("version", "ChangedC");
					updater.GetPackage("WierdSpacing").AddOrUpdateAttribute("version", "2.34.56");
				},
				expectedContents:
				@"<project>" + Environment.NewLine +
				@"   <masterversions>" + Environment.NewLine +
				@"      <package name=""OrderingShouldRemainA"" version=""ChangedA"" />" + Environment.NewLine +
				@"      <package name=""OrderingShouldRemainB"" version=""ChangedB"" />" + Environment.NewLine +
				@"      <package name=""OrderingShouldRemainC"" version=""ChangedC"" />" + Environment.NewLine +
				@"    <package    name=""WierdSpacing""     version=""2.34.56""    " + Environment.NewLine +
				@"              uri=""p4://server:1234/location/WierdSpacing/1.00.00""  >" + Environment.NewLine +
				@"          </package>" + Environment.NewLine +
				@" </masterversions>" + Environment.NewLine +
				@" <config package=""eaconfig"" />" + Environment.NewLine +
				@"</project>"
			);
		}

		[TestMethod]
		public void UpdateComplexWhiteSpace()
		{
			// adding attribute preverses whitespace
			Utils.VerifyChanged
			(
				inputContents:
				@"<project>" + Environment.NewLine +
				@"   <masterversions>" + Environment.NewLine +
				@"      <package" + Environment.NewLine + 
				@"          name=""TestPackage""" + Environment.NewLine +
				@"          version=""dev""" + Environment.NewLine +
				@"      />" + Environment.NewLine +
				@"  </masterversions>" + Environment.NewLine +
				@"  <config package=""eaconfig"" />" + Environment.NewLine +
				@"</project>",
				updateFunction: (MasterConfigUpdater updater) =>
				{
					updater.GetPackage("TestPackage").AddOrUpdateAttribute("uri", "p4://server.domain.com:1000/depot/path/TestPackag/dev?cl=1234");
				},
				expectedContents:
				@"<project>" + Environment.NewLine +
				@"   <masterversions>" + Environment.NewLine +
				@"      <package" + Environment.NewLine +
				@"          name=""TestPackage""" + Environment.NewLine +
				@"          version=""dev""" + Environment.NewLine +
				@"          uri=""p4://server.domain.com:1000/depot/path/TestPackag/dev?cl=1234""" + Environment.NewLine +
				@"      />" + Environment.NewLine +
				@"  </masterversions>" + Environment.NewLine +
				@"  <config package=""eaconfig"" />" + Environment.NewLine +
				@"</project>"
			);
		}

		[TestMethod]
		public void UpdateFragment()
		{
			string masterConfigWithFragment =
				@"<project>" + Environment.NewLine +
				@"   <masterversions>" + Environment.NewLine +
				@"      <package name=""TestPackage"" version=""A"" />" + Environment.NewLine +
				@"   </masterversions>" + Environment.NewLine +
				@"   <fragments>" + Environment.NewLine +
				@"      <include name=""./*/masterconfig_fragment.xml""/>" + Environment.NewLine +
				@"   </fragments>" + Environment.NewLine +
				@" <config package=""eaconfig"" />" + Environment.NewLine +
				@"</project>";

			// fragment update
			Utils.VerifyChanged
			(
				inputContents: masterConfigWithFragment,
				updateFunction: (MasterConfigUpdater updater) =>
				{
					updater.GetPackage("OnlyInFragment").AddOrUpdateAttribute("version", "2.00.00");
					Assert.IsTrue(updater.GetPathsToModifyOnSave().Count() == 1); // expecting just fragment to be modified
				},
				expectedContents: masterConfigWithFragment,
				fragments:
				new Dictionary<string, Tuple<string, string>>()
				{
					{
						"test_wildcard_folder\\masterconfig_fragment.xml",
						Tuple.Create
						(
							@"<project>" + Environment.NewLine +
							@"   <masterversions>" + Environment.NewLine +
							@"      <package name=""OnlyInFragment"" version=""1.00.00"" />" + Environment.NewLine +
							@"   </masterversions>" + Environment.NewLine +
							@"</project>",

							@"<project>" + Environment.NewLine +
							@"   <masterversions>" + Environment.NewLine +
							@"      <package name=""OnlyInFragment"" version=""2.00.00"" />" + Environment.NewLine +
							@"   </masterversions>" + Environment.NewLine +
							@"</project>"
						)
					}
				}
			);
		}

		[TestMethod]
		public void UpdateFragmentAndMainMasterConfig()
		{
			// fragment update
			Utils.VerifyChanged
			(
				inputContents:
				@"<project>" + Environment.NewLine +
				@"   <masterversions>" + Environment.NewLine +
				@"      <package name=""TestPackageA"" version=""A"" />" + Environment.NewLine +
				@"      <package name=""TestPackageB"" version=""B"" />" + Environment.NewLine +
				@"      <package name=""AlsoInFragment"" version=""1.00.00"" />" + Environment.NewLine +
				@"   </masterversions>" + Environment.NewLine +
				@"   <fragments>" + Environment.NewLine +
				@"      <include name=""./*/masterconfig_fragment.xml""/>" + Environment.NewLine +
				@"   </fragments>" + Environment.NewLine +
				@" <config package=""eaconfig"" />" + Environment.NewLine +
				@"</project>",
				updateFunction: (MasterConfigUpdater updater) =>
				{
					updater.GetPackage("AlsoInFragment").AddOrUpdateAttribute("version", "2.00.00");
					Assert.IsTrue(updater.GetPathsToModifyOnSave().Count() == 2); // expecting both main config and fragment to be modified
				},
				expectedContents:
				@"<project>" + Environment.NewLine +
				@"   <masterversions>" + Environment.NewLine +
				@"      <package name=""TestPackageA"" version=""A"" />" + Environment.NewLine +
				@"      <package name=""TestPackageB"" version=""B"" />" + Environment.NewLine +
				@"      <package name=""AlsoInFragment"" version=""2.00.00"" />" + Environment.NewLine +
				@"   </masterversions>" + Environment.NewLine +
				@"   <fragments>" + Environment.NewLine +
				@"      <include name=""./*/masterconfig_fragment.xml""/>" + Environment.NewLine +
				@"   </fragments>" + Environment.NewLine +
				@" <config package=""eaconfig"" />" + Environment.NewLine +
				@"</project>",
				fragments:
				new Dictionary<string, Tuple<string, string>>()
				{
					{
						"test_wildcard_folder\\masterconfig_fragment.xml",
						Tuple.Create
						(
							@"<project>" + Environment.NewLine +
							@"   <masterversions>" + Environment.NewLine +
							@"      <package name=""AlsoInFragment"" version=""1.00.00"" />" + Environment.NewLine +
							@"   </masterversions>" + Environment.NewLine +
							@"</project>",

							@"<project>" + Environment.NewLine +
							@"   <masterversions>" + Environment.NewLine +
							@"      <package name=""AlsoInFragment"" version=""2.00.00"" />" + Environment.NewLine +
							@"   </masterversions>" + Environment.NewLine +
							@"</project>"
						)
					}
				}
			);
		}

		[TestMethod]
		public void UpdateOverriddingFragment()
		{
			// overidding fragments
			Utils.VerifyChanged
			(
				inputContents:
				@"<project>" + Environment.NewLine +
				@"   <masterversions>" + Environment.NewLine +
				@"      <package name=""AlsoInFragment"" version=""1.00.00"" />" + Environment.NewLine +
				@"      <package name=""OverriddenInFragment"" version=""1.00.00"" />" + Environment.NewLine +
				@"   </masterversions>" + Environment.NewLine +
				@"   <fragments>" + Environment.NewLine +
				@"      <include name=""./*/masterconfig_fragment.xml""/>" + Environment.NewLine +
				@"   </fragments>" + Environment.NewLine +
				@" <config package=""eaconfig"" />" + Environment.NewLine +
				@"</project>",
				updateFunction: (MasterConfigUpdater updater) =>
				{
					updater.GetPackage("AlsoInFragment").AddOrUpdateAttribute("version", "2.00.00");
					updater.GetPackage("OverriddenInFragment").AddOrUpdateAttribute("version", "2.00.00");
					Assert.IsTrue(updater.GetPathsToModifyOnSave().Count() == 2); // expecting both main config and fragment to be modified
				},
				expectedContents:
				@"<project>" + Environment.NewLine +
				@"   <masterversions>" + Environment.NewLine +
				@"      <package name=""AlsoInFragment"" version=""2.00.00"" />" + Environment.NewLine +
				@"      <package name=""OverriddenInFragment"" version=""1.00.00"" />" + Environment.NewLine +
				@"   </masterversions>" + Environment.NewLine +
				@"   <fragments>" + Environment.NewLine +
				@"      <include name=""./*/masterconfig_fragment.xml""/>" + Environment.NewLine +
				@"   </fragments>" + Environment.NewLine +
				@" <config package=""eaconfig"" />" + Environment.NewLine +
				@"</project>",
				fragments:
				new Dictionary<string, Tuple<string, string>>()
				{
					{
						"test_wildcard_folder\\masterconfig_fragment.xml",
						Tuple.Create
						(
							@"<project>" + Environment.NewLine +
							@"   <masterversions>" + Environment.NewLine +
							@"      <package name=""AlsoInFragment"" version=""1.00.00"" />" + Environment.NewLine +
							@"      <package name=""OverriddenInFragment"" version=""dev"" allowoverride=""true"" />" + Environment.NewLine +
							@"   </masterversions>" + Environment.NewLine +
							@"</project>",

							@"<project>" + Environment.NewLine +
							@"   <masterversions>" + Environment.NewLine +
							@"      <package name=""AlsoInFragment"" version=""2.00.00"" />" + Environment.NewLine +
							@"      <package name=""OverriddenInFragment"" version=""2.00.00"" allowoverride=""true"" />" + Environment.NewLine +
							@"   </masterversions>" + Environment.NewLine +
							@"</project>"
						)
					}
				}
			);
		}

		[TestMethod]
		public void UpdateFragmentInsideFragment()
		{
			// fragment update
			Utils.VerifyChanged
			(
				inputContents:
				@"<project>" + Environment.NewLine +
				@"   <masterversions>" + Environment.NewLine +
				@"      <package name=""TestPackageA"" version=""A"" />" + Environment.NewLine +
				@"      <package name=""TestPackageB"" version=""B"" />" + Environment.NewLine +
				@"   </masterversions>" + Environment.NewLine +
				@"   <fragments>" + Environment.NewLine +
				@"      <include name=""./masterconfig_fragment.xml""/>" + Environment.NewLine +
				@"   </fragments>" + Environment.NewLine +
				@" <config package=""eaconfig"" />" + Environment.NewLine +
				@"</project>",
				updateFunction: (MasterConfigUpdater updater) =>
				{
					updater.GetPackage("OnlyInSubFragment").AddOrUpdateAttribute("version", "2.00.00");
					Assert.IsTrue(updater.GetPathsToModifyOnSave().Count() == 1); // expecting just the sub fragment to be updated
			},
				expectedContents:
				@"<project>" + Environment.NewLine +
				@"   <masterversions>" + Environment.NewLine +
				@"      <package name=""TestPackageA"" version=""A"" />" + Environment.NewLine +
				@"      <package name=""TestPackageB"" version=""B"" />" + Environment.NewLine +
				@"   </masterversions>" + Environment.NewLine +
				@"   <fragments>" + Environment.NewLine +
				@"      <include name=""./masterconfig_fragment.xml""/>" + Environment.NewLine +
				@"   </fragments>" + Environment.NewLine +
				@" <config package=""eaconfig"" />" + Environment.NewLine +
				@"</project>",
				fragments:
				new Dictionary<string, Tuple<string, string>>()
				{
					{
						"masterconfig_fragment.xml",
						Tuple.Create
						(
							@"<project>" + Environment.NewLine +
							@"   <masterversions>" + Environment.NewLine +
							@"      <package name=""OnlyInFragment"" version=""2.00.00"" />" + Environment.NewLine +
							@"   </masterversions>" + Environment.NewLine +
							@"   <fragments>" + Environment.NewLine +
							@"      <include name=""./masterconfig_sub_fragment.xml""/>" + Environment.NewLine +
							@"   </fragments>" + Environment.NewLine +
							@"</project>",

							@"<project>" + Environment.NewLine +
							@"   <masterversions>" + Environment.NewLine +
							@"      <package name=""OnlyInFragment"" version=""2.00.00"" />" + Environment.NewLine +
							@"   </masterversions>" + Environment.NewLine +
							@"   <fragments>" + Environment.NewLine +
							@"      <include name=""./masterconfig_sub_fragment.xml""/>" + Environment.NewLine +
							@"   </fragments>" + Environment.NewLine +
							@"</project>"
						)
					},
					{
						"masterconfig_sub_fragment.xml",
						Tuple.Create
						(
							@"<project>" + Environment.NewLine +
							@"   <masterversions>" + Environment.NewLine +
							@"      <package name=""OnlyInSubFragment"" version=""1.00.00"" />" + Environment.NewLine +
							@"   </masterversions>" + Environment.NewLine +
							@"</project>",

							@"<project>" + Environment.NewLine +
							@"   <masterversions>" + Environment.NewLine +
							@"      <package name=""OnlyInSubFragment"" version=""2.00.00"" />" + Environment.NewLine +
							@"   </masterversions>" + Environment.NewLine +
							@"</project>"
						)
					}
				}
			);
		}

		[TestMethod]
		public void UpdatePackageInMultiplyDefinedGroup()
		{
			Utils.VerifyChanged
			(
				inputContents:
				@"<project>" + Environment.NewLine +
				@"   <masterversions>" + Environment.NewLine +
				@"      <grouptype name=""group"">" + Environment.NewLine +
				@"          <package name=""TestPackageA"" version=""A"" />" + Environment.NewLine +
				@"      </grouptype>" + Environment.NewLine +
				@"      <grouptype name=""group"">" + Environment.NewLine +
				@"          <package name=""TestPackageB"" version=""B"" />" + Environment.NewLine +
				@"      </grouptype>" + Environment.NewLine +
				@"   </masterversions>" + Environment.NewLine +
				@" <config package=""eaconfig"" />" + Environment.NewLine +
				@"</project>",
				updateFunction: (MasterConfigUpdater updater) =>
				{
					updater.GetPackage("TestPackageB").AddOrUpdateAttribute("version", "B2");
				},
				expectedContents:
				@"<project>" + Environment.NewLine +
				@"   <masterversions>" + Environment.NewLine +
				@"      <grouptype name=""group"">" + Environment.NewLine +
				@"          <package name=""TestPackageA"" version=""A"" />" + Environment.NewLine +
				@"      </grouptype>" + Environment.NewLine +
				@"      <grouptype name=""group"">" + Environment.NewLine +
				@"          <package name=""TestPackageB"" version=""B2"" />" + Environment.NewLine +
				@"      </grouptype>" + Environment.NewLine +
				@"   </masterversions>" + Environment.NewLine +
				@" <config package=""eaconfig"" />" + Environment.NewLine +
				@"</project>"
			);
		}

		[TestMethod]
		public void OverriddingFragmentInsideFragment()
		{
			// fragment update
			Utils.VerifyChanged
			(
				inputContents:
				@"<project>" + Environment.NewLine +
				@"   <masterversions>" + Environment.NewLine +
				@"      <package name=""TestPackageA"" version=""A"" />" + Environment.NewLine +
				@"      <package name=""TestPackageB"" version=""B"" />" + Environment.NewLine +
				@"      <package name=""AlsoInSubFragment"" version=""1.00.00"" />" + Environment.NewLine +
				@"   </masterversions>" + Environment.NewLine +
				@"   <fragments>" + Environment.NewLine +
				@"      <include name=""./masterconfig_fragment.xml""/>" + Environment.NewLine +
				@"   </fragments>" + Environment.NewLine +
				@" <config package=""eaconfig"" />" + Environment.NewLine +
				@"</project>",
				updateFunction: (MasterConfigUpdater updater) =>
				{
					updater.GetPackage("AlsoInSubFragment").AddOrUpdateAttribute("version", "2.00.00");
					Assert.IsTrue(updater.GetPathsToModifyOnSave().Count() == 2); // expecting to update fragment and subfragment
				},
				expectedContents:
				@"<project>" + Environment.NewLine +
				@"   <masterversions>" + Environment.NewLine +
				@"      <package name=""TestPackageA"" version=""A"" />" + Environment.NewLine +
				@"      <package name=""TestPackageB"" version=""B"" />" + Environment.NewLine +
				@"      <package name=""AlsoInSubFragment"" version=""2.00.00"" />" + Environment.NewLine +
				@"   </masterversions>" + Environment.NewLine +
				@"   <fragments>" + Environment.NewLine +
				@"      <include name=""./masterconfig_fragment.xml""/>" + Environment.NewLine +
				@"   </fragments>" + Environment.NewLine +
				@" <config package=""eaconfig"" />" + Environment.NewLine +
				@"</project>",
				fragments:
				new Dictionary<string, Tuple<string, string>>()
				{
					{
						"masterconfig_fragment.xml",
						Tuple.Create
						(
							@"<project>" + Environment.NewLine +
							@"   <masterversions>" + Environment.NewLine +
							@"      <package name=""OnlyInFragment"" version=""2.00.00"" />" + Environment.NewLine +
							@"   </masterversions>" + Environment.NewLine +
							@"   <fragments>" + Environment.NewLine +
							@"      <include name=""./masterconfig_sub_fragment.xml""/>" + Environment.NewLine +
							@"   </fragments>" + Environment.NewLine +
							@"</project>",

							@"<project>" + Environment.NewLine +
							@"   <masterversions>" + Environment.NewLine +
							@"      <package name=""OnlyInFragment"" version=""2.00.00"" />" + Environment.NewLine +
							@"   </masterversions>" + Environment.NewLine +
							@"   <fragments>" + Environment.NewLine +
							@"      <include name=""./masterconfig_sub_fragment.xml""/>" + Environment.NewLine +
							@"   </fragments>" + Environment.NewLine +
							@"</project>"
						)
					},
					{
						"masterconfig_sub_fragment.xml",
						Tuple.Create
						(
							@"<project>" + Environment.NewLine +
							@"   <masterversions>" + Environment.NewLine +
							@"      <package name=""AlsoInSubFragment"" version=""1.00.00"" />" + Environment.NewLine +
							@"   </masterversions>" + Environment.NewLine +
							@"</project>",

							@"<project>" + Environment.NewLine +
							@"   <masterversions>" + Environment.NewLine +
							@"      <package name=""AlsoInSubFragment"" version=""2.00.00"" />" + Environment.NewLine +
							@"   </masterversions>" + Environment.NewLine +
							@"</project>"
						)
					}
				}
			);
		}

		[TestMethod]
		public void EmptyMasterConfigError()
		{
			// empty masterconfig
			Exception emptyException = Utils.VerifyThrows<MasterConfigUpdater.MasterConfigException>("");
			Assert.IsTrue(emptyException.Message == "Unexpected end of file!");
		}

		[TestMethod]
		public void PackageMissingName()
		{
			// package element with no name   
			Exception missingNameException = Utils.VerifyThrows<MasterConfigUpdater.MasterConfigException>
			(
				@"<project>" + Environment.NewLine +
				@"   <masterversions>" + Environment.NewLine +
				@"      <package/>" + Environment.NewLine +
				@"   </masterversions>" + Environment.NewLine +
				@"   <config package=""eaconfig"" />" + Environment.NewLine +
				@"</project>"
			);
			Assert.IsTrue(missingNameException.Message.StartsWith("Package element missing required attribute 'name' at line 3 (column 7)"));
		}

		[TestMethod]
		public void InvalidPackageName()
		{
			// package element with invalid name   
			Exception invalidNameExcepion = Utils.VerifyThrows<MasterConfigUpdater.MasterConfigException>
			(
				@"<project>" + Environment.NewLine +
				@"   <masterversions>" + Environment.NewLine +
				@"      <package name=""????""/>" + Environment.NewLine +
				@"   </masterversions>" + Environment.NewLine +
				@"   <config package=""eaconfig"" />" + Environment.NewLine +
				@"</project>"
			);
			Assert.IsTrue(invalidNameExcepion.Message.Contains("Package name '????'"));
			Assert.IsTrue(invalidNameExcepion.Message.Contains("is invalid"));
		}

		[TestMethod]
		public void PackageMissingVersion()
		{
			// package element with no version   
			Exception missingVersionException = Utils.VerifyThrows<MasterConfigUpdater.MasterConfigException>
			(
				@"<project>" + Environment.NewLine +
				@"   <masterversions>" + Environment.NewLine +
				@"      <package name=""TestPackage""/>" + Environment.NewLine +
				@"   </masterversions>" + Environment.NewLine +
				@"   <config package=""eaconfig"" />" + Environment.NewLine +
				@"</project>"
			);
			Assert.IsTrue(missingVersionException.Message.Contains("missing required attribute 'version'"));
			Assert.IsTrue(missingVersionException.Message.Contains("at line 3 (column 7)"));
		}

		[TestMethod]
		public void InvalidPackageVersion()
		{
			// package element with invalid version   
			Exception invalidNameExcepion = Utils.VerifyThrows<MasterConfigUpdater.MasterConfigException>
			(
				@"<project>" + Environment.NewLine +
				@"   <masterversions>" + Environment.NewLine +
				@"      <package name=""TestPackage"" version=""????""/>" + Environment.NewLine +
				@"   </masterversions>" + Environment.NewLine +
				@"   <config package=""eaconfig"" />" + Environment.NewLine +
				@"</project>"
			);
			Assert.IsTrue(invalidNameExcepion.Message.Contains("has invalid version name '????'"));
		}
	}
}
