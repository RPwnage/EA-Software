// NAnt - A .NET build tool
// Copyright (C) 2003-2015 Electronic Arts Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
// 
// As a special exception, the copyright holders of this software give you 
// permission to link the assemblies with independent modules to produce 
// new assemblies, regardless of the license terms of these independent 
// modules, and to copy and distribute the resulting assemblies under terms 
// of your choice, provided that you also meet, for each linked independent 
// module, the terms and conditions of the license of that module. An 
// independent module is a module which is not derived from or based 
// on these assemblies. If you modify this software, you may extend 
// this exception to your version of the software, but you are not 
// obligated to do so. If you do not wish to do so, delete this exception 
// statement from your version. 
// 
// Electronic Arts (Frostbite.Team.CM@ea.com)

using System;
using System.Collections.Generic;
using System.IO;
using System.Security.Principal;

using Microsoft.VisualStudio.TestTools.UnitTesting;

using NAnt.Core.Util;
using NAnt.Tests.Util;

namespace NAnt.Shared.Tests
{
	[TestClass]
	public class LinkOrCopyTests
	{
		private string m_testDir;

		[TestInitialize]
		public void TestSetup()
		{
			m_testDir = TestUtil.CreateTestDirectory(GetType().Name);
		}

		[TestCleanup]
		public void TestCleanUp()
		{
			if (Directory.Exists(m_testDir))
			{
				File.SetAttributes(m_testDir, File.GetAttributes(m_testDir) & ~FileAttributes.ReadOnly);
				foreach (string fileSystemEntry in Directory.GetFileSystemEntries(m_testDir, "*", SearchOption.AllDirectories))
				{
					File.SetAttributes(fileSystemEntry, File.GetAttributes(fileSystemEntry) & ~FileAttributes.ReadOnly);
				}

				// try catch with double delete is slightly hacky way to deal with Directory.Delete failing
				// if deleting a junction the function will throw an exception, but will remove it so calling
				// again succeeds
				try
				{
					Directory.Delete(m_testDir, true);
				}
				catch
				{
					Directory.Delete(m_testDir, true);
				}
			}
		}

		// file tests (no junction, cannot junction files)
		[TestMethod] public void HardlinkFileTest() => FileTest(LinkOrCopyMethod.Hardlink, LinkOrCopyStatus.Hardlinked, LinkOrCopyStatus.AlreadyHardlinked, LinkOrCopyCommon.HardLink.IsSupported());
		[TestMethod] public void CopyFileTest() => FileTest(LinkOrCopyMethod.Copy, LinkOrCopyStatus.Copied, LinkOrCopyStatus.AlreadyCopied, true);
		[TestMethod]
		public void SymlinkFileTest()
		{
			if (IsWindows() && !RunningAsWindowsAdmin())
			{
				Assert.Inconclusive("Creating symlinks cannot be tested unless runnning as admin.");
			}
			FileTest(LinkOrCopyMethod.Symlink, LinkOrCopyStatus.Symlinked, LinkOrCopyStatus.AlreadySymlinked, LinkOrCopyCommon.SymLink.IsSupported());
		}

		// directory tests (no hardlink, cannot hardlink directories)
		[TestMethod] public void CopyDirectoryTest() => DirectoryTest(LinkOrCopyMethod.Copy, LinkOrCopyStatus.Copied, LinkOrCopyStatus.AlreadyCopied, true);
		[TestMethod] public void JunctionDirectoryTest() => DirectoryTest(LinkOrCopyMethod.Junction, LinkOrCopyStatus.Junctioned, LinkOrCopyStatus.AlreadyJunctioned, LinkOrCopyCommon.Junction.IsSupported());
		[TestMethod] public void SymlinkDirectoryTest()
		{
			if (IsWindows() && !RunningAsWindowsAdmin())
			{
				Assert.Inconclusive("Creating symlinks cannot be tested unless runnning as admin.");
			}
			DirectoryTest(LinkOrCopyMethod.Symlink, LinkOrCopyStatus.Symlinked, LinkOrCopyStatus.AlreadySymlinked, LinkOrCopyCommon.SymLink.IsSupported());
		}

		[TestMethod]
		public void InvalidJunctionTests()
		{
			if (LinkOrCopyCommon.Junction.IsSupported())
			{
				// junctions are invalid for files, we should get an argument exception if the requested method was junction for a file
				string sourcePath = Path.Combine(m_testDir, "source.txt");
				File.WriteAllText(sourcePath, "source");

				string destPath = Path.Combine(m_testDir, "dest.txt");
				Assert.ThrowsException<ArgumentException>(() => LinkOrCopyCommon.LinkOrCopy(sourcePath, destPath, OverwriteMode.NoOverwrite, methods: LinkOrCopyMethod.Junction));
			}
		}

		[TestMethod]
		public void InvalidHardlinkTests()
		{
			if (LinkOrCopyCommon.HardLink.IsSupported())
			{
				// hardlinking is a file concept, argument exception should be thrown if hardlinking is the method given for a directory
				string sourcePath = Path.Combine(m_testDir, "source");
				Directory.CreateDirectory(sourcePath);

				string destPath = Path.Combine(m_testDir, "dest");
				Assert.ThrowsException<ArgumentException>(() => LinkOrCopyCommon.LinkOrCopy(sourcePath, destPath, OverwriteMode.NoOverwrite, methods: LinkOrCopyMethod.Hardlink));
			}
		}

		private void FileTest(LinkOrCopyMethod testMethod, LinkOrCopyStatus expectedChangeResult, LinkOrCopyStatus expectedNoChangeResult, bool isSupportedOnThisPlatform)
		{
			// test initial link / copy
			string fileContents = "file";
			string sourcePath = Path.Combine(m_testDir, "source.txt");
			File.WriteAllText(sourcePath, fileContents);

			string destPath = Path.Combine(m_testDir, "dest.txt");

			LinkOrCopyStatus result = LinkOrCopyStatus.Failed;
			Func<LinkOrCopyStatus> firstLinkOperation = () => LinkOrCopyCommon.LinkOrCopy(sourcePath, destPath, OverwriteMode.NoOverwrite, methods: testMethod);
			if (!isSupportedOnThisPlatform)
			{
				Assert.ThrowsException<PlatformNotSupportedException>(() => firstLinkOperation());
				return; // skip the rest of the tests
			}
			else
			{
				result = firstLinkOperation();
			}
			Assert.AreEqual(expectedChangeResult, result);
			Assert.AreEqual(fileContents, File.ReadAllText(destPath));

			// link / copy again, even with NoOverwrite we should detect that the link / file already exists and succeed
			result = LinkOrCopyCommon.LinkOrCopy(sourcePath, destPath, OverwriteMode.NoOverwrite, methods: testMethod);
			Assert.AreEqual(expectedNoChangeResult, result);

			if (!PathUtil.IsCaseSensitive)
			{
				// link / copy one more time if filesystem is case sensitive - see if detection still works with mismatched case
				result = LinkOrCopyCommon.LinkOrCopy(sourcePath.ToUpperInvariant(), destPath.ToUpperInvariant(), OverwriteMode.NoOverwrite, methods: testMethod);
				Assert.AreEqual(expectedNoChangeResult, result);
			}

			// trying to link to a new source / copy from new source with NoOverwrite should fail
			string newFileContents = "new file";
			string newSourcePath = Path.Combine(m_testDir, "newSource.txt");
			File.WriteAllText(newSourcePath, newFileContents);
			Assert.ThrowsException<IOException>(() => LinkOrCopyCommon.LinkOrCopy(newSourcePath, destPath, OverwriteMode.NoOverwrite, methods: testMethod));

			// test re-link / copy with new source and Overwrite
			result = LinkOrCopyCommon.LinkOrCopy(newSourcePath, destPath, OverwriteMode.Overwrite, methods: testMethod);
			Assert.AreEqual(expectedChangeResult, result);
			Assert.AreEqual(newFileContents, File.ReadAllText(destPath));

			if (PlatformUtil.IsWindows) // setting read only this way does work on unix
			{
				// test overwriting a pre-existing read-only destination, without OverwriteReadOnly
				string newDestPath = Path.Combine(m_testDir, "newDest.txt");
				File.WriteAllText(newDestPath, "overwrite");
				File.SetAttributes(newDestPath, File.GetAttributes(newDestPath) | FileAttributes.ReadOnly);
				Assert.ThrowsException<UnauthorizedAccessException>(() => LinkOrCopyCommon.LinkOrCopy(sourcePath, newDestPath, OverwriteMode.Overwrite, methods: testMethod));

				// should work with OverwriteReadOnly
				result = LinkOrCopyCommon.LinkOrCopy(sourcePath, newDestPath, OverwriteMode.OverwriteReadOnly, methods: testMethod);
				Assert.AreEqual(expectedChangeResult, result);
			}
		}

		private void DirectoryTest(LinkOrCopyMethod testMethod, LinkOrCopyStatus expectedChangeResult, LinkOrCopyStatus expectedNoChangeResult, bool isSupportedOnThisPlatform)
		{
			// test initial link / copy
			string sourceDirPath = Path.Combine(m_testDir, "source");
			Directory.CreateDirectory(sourceDirPath);

			string sourceOnePath = Path.Combine(sourceDirPath, "fileOne.txt");
			string fileOneContents = "source one";
			File.WriteAllText(sourceOnePath, fileOneContents);

			string sourceTwoPath = Path.Combine(sourceDirPath, "fileTwo.txt");
			string fileTwoContents = "source two";
			File.WriteAllText(sourceTwoPath, fileTwoContents);

			// nested file test
			string sourceThreePath = Path.Combine(sourceDirPath, "nested", "fileThree.txt");
			Directory.CreateDirectory(Path.GetDirectoryName(sourceThreePath));
			string fileThreeContents = "source three";
			File.WriteAllText(sourceThreePath, fileThreeContents);

			string destDirPath = Path.Combine(m_testDir, "dest");

			LinkOrCopyStatus result = LinkOrCopyStatus.Failed;
			Func<LinkOrCopyStatus> firstLinkOperation = () => LinkOrCopyCommon.LinkOrCopy(sourceDirPath, destDirPath, OverwriteMode.NoOverwrite, methods: testMethod);
			if (!isSupportedOnThisPlatform)
			{
				Assert.ThrowsException<PlatformNotSupportedException>(() => firstLinkOperation());
				return; // skip the rest of the tests
			}
			else
			{
				result = firstLinkOperation();
			}
			
			Assert.AreEqual(expectedChangeResult, result);

			string destOnePath = Path.Combine(destDirPath, "fileOne.txt");
			Assert.AreEqual(fileOneContents, File.ReadAllText(destOnePath));
			string destTwoPath = Path.Combine(destDirPath, "fileTwo.txt");
			Assert.AreEqual(fileTwoContents, File.ReadAllText(destTwoPath));
			string destThreePath = Path.Combine(destDirPath, "nested", "fileThree.txt");
			Assert.AreEqual(fileThreeContents, File.ReadAllText(destThreePath));

			// link / copy again, even with NoOverwrite we should detect that the link / dir and contained files already exist and succeed
			result = LinkOrCopyCommon.LinkOrCopy(sourceDirPath, destDirPath, OverwriteMode.NoOverwrite, methods: testMethod);
			Assert.AreEqual(expectedNoChangeResult, result);

			if (!PathUtil.IsCaseSensitive) // this method of setting file attributes does not work on unix
			{
				// link / copy one more time if filesystem is case sensitive - see if detection still works with mismatched case
				result = LinkOrCopyCommon.LinkOrCopy(sourceDirPath.ToUpperInvariant(), destDirPath.ToUpperInvariant(), OverwriteMode.NoOverwrite, methods: testMethod);
				Assert.AreEqual(expectedNoChangeResult, result);
			}

			// link from a new source, without overwrite - should fail
			string newSourceDirPath = Path.Combine(m_testDir, "newSource");
			Directory.CreateDirectory(newSourceDirPath);

			string newSourceOnePath = Path.Combine(newSourceDirPath, "fileOne.txt");
			string newFileOneContents = "new source one";
			File.WriteAllText(newSourceOnePath, newFileOneContents);

			string newSourceTwoPath = Path.Combine(newSourceDirPath, "fileTwo.txt");
			string newFileTwoContents = "new source two";
			File.WriteAllText(newSourceTwoPath, newFileTwoContents);

			Assert.ThrowsException<IOException>(() => LinkOrCopyCommon.LinkOrCopy(newSourceDirPath, destDirPath, OverwriteMode.NoOverwrite, methods: testMethod));

			// try again with override
			result = LinkOrCopyCommon.LinkOrCopy(newSourceDirPath, destDirPath, OverwriteMode.Overwrite, methods: testMethod);
			Assert.AreEqual(expectedChangeResult, result);

			// make sure when we deleted the old link we didn't recursively go through the old source and delete nested files, we should have just deleted the link
			Assert.IsTrue(File.Exists(sourceOnePath));
			Assert.IsTrue(File.Exists(sourceTwoPath));
			Assert.IsTrue(File.Exists(sourceThreePath));

			// try to override a new path with readonly dir and file
			string newDestDirPath = Path.Combine(m_testDir, "newDest");
			Directory.CreateDirectory(newDestDirPath);
			File.SetAttributes(newDestDirPath, File.GetAttributes(newDestDirPath) | FileAttributes.ReadOnly);

			string destReadOnlyPath = Path.Combine(newDestDirPath, "fileOne.txt");
			string destReadOnlyContents = "readonly overwrite";
			File.WriteAllText(destReadOnlyPath, destReadOnlyContents);
			File.SetAttributes(destReadOnlyPath, File.GetAttributes(destReadOnlyPath) | FileAttributes.ReadOnly);

			if (PlatformUtil.IsWindows) // this method of setting file attributes does not work on unix
			{
				Assert.ThrowsException<UnauthorizedAccessException>(() => LinkOrCopyCommon.LinkOrCopy(sourceDirPath, newDestDirPath, OverwriteMode.Overwrite, methods: testMethod));

				// should work with OverwriteReadOnly
				result = LinkOrCopyCommon.LinkOrCopy(sourceDirPath, newDestDirPath, OverwriteMode.OverwriteReadOnly, methods: testMethod);
				Assert.AreEqual(expectedChangeResult, result);

				// set current source files to readonly to test attribute preservation
				File.SetAttributes(sourceOnePath, File.GetAttributes(sourceOnePath) | FileAttributes.ReadOnly);
				File.SetAttributes(sourceTwoPath, File.GetAttributes(sourceTwoPath) | FileAttributes.ReadOnly);
				File.SetAttributes(sourceThreePath, File.GetAttributes(sourceThreePath) | FileAttributes.ReadOnly);

				// overwrite link to point to a different source, this shouldn't modify file attributes of current target
				result = LinkOrCopyCommon.LinkOrCopy(newSourceDirPath, newDestDirPath, OverwriteMode.OverwriteReadOnly, methods: testMethod);
				Assert.AreEqual(expectedChangeResult, result);

				// check we didn't recursively go through a symlink modifying file attributes of the old target when trying to delete
				Assert.IsTrue((File.GetAttributes(sourceOnePath) & FileAttributes.ReadOnly) == FileAttributes.ReadOnly);
				Assert.IsTrue((File.GetAttributes(sourceTwoPath) & FileAttributes.ReadOnly) == FileAttributes.ReadOnly);
				Assert.IsTrue((File.GetAttributes(sourceThreePath) & FileAttributes.ReadOnly) == FileAttributes.ReadOnly);
			}

			// test that copying / creating links inside non-existent folders works
			string nestedDestDirPath = Path.Combine(m_testDir, "nested", "dest");
			result = LinkOrCopyCommon.LinkOrCopy(sourceDirPath, nestedDestDirPath, OverwriteMode.OverwriteReadOnly, methods: testMethod);
			Assert.AreEqual(expectedChangeResult, result);
		}

		private static bool RunningAsWindowsAdmin()
		{
			WindowsIdentity identity = WindowsIdentity.GetCurrent();
			WindowsPrincipal principal = new WindowsPrincipal(identity);
			return principal.IsInRole(WindowsBuiltInRole.Administrator);
		}

		private static bool IsWindows()
		{
			return Environment.OSVersion.Platform == PlatformID.Win32NT ||
				Environment.OSVersion.Platform == PlatformID.Win32S ||
				Environment.OSVersion.Platform == PlatformID.Win32Windows ||
				Environment.OSVersion.Platform == PlatformID.WinCE;
		}
	}
}