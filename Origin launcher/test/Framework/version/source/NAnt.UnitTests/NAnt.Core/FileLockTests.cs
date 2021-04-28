using System;
using System.IO;
using System.Threading;
using Microsoft.VisualStudio.TestTools.UnitTesting;

using NAnt.Core.Util;
using NAnt.Tests.Util;

namespace NAnt.Core.Tests
{
	[TestClass]
	public class FileLockTests
	{
		private string TestDir;
		private string TestDestDir;
		private string TestFile;
		private string TestFile2;
		private Thread FileLockerThread;
		private CancellationTokenSource AbortToken;

		[TestInitialize]
		public void TestInitialize()
		{
			TestDir = TestUtil.CreateTestDirectory("PathUtilFileLockTest");
			TestDestDir = TestUtil.CreateTestDirectory("PathUtilFileLockTestDest");
			TestFile = Path.Combine(TestDir, "dummyfile.txt");
			TestFile2 = Path.Combine(TestDir, "dummyfile2.txt");

			// Initialization data before running test
			using (StreamWriter strm = File.CreateText(TestFile))
			{
				strm.WriteLine("This is a dummy text file.");
			}
			if (!File.Exists(TestFile))
			{
				throw new Exception(string.Format("Failed to create test file: {0}", TestFile));
			}
			using (StreamWriter strm2 = File.CreateText(TestFile2))
			{
				strm2.WriteLine("This is a dummy text file2.");
			}
			if (!File.Exists(TestFile2))
			{
				throw new Exception(string.Format("Failed to create test file: {0}", TestFile2));
			}

			AbortToken = new CancellationTokenSource();
			var token = AbortToken.Token;
			FileLockerThread = new Thread(new ThreadStart(() =>
			{
				int fileLockerTimeoutMs = 12000;
				try
				{
					using (FileStream strm = File.Open(TestFile, FileMode.Open, FileAccess.Read, FileShare.None))
					{
						token.WaitHandle.WaitOne(fileLockerTimeoutMs);
					}
				}
				catch
				{
					//throw new Exception(string.Format("Unable to open file!"));
				}
			}));
			FileLockerThread.Start();

			// wait some time to ensure the file is locked
			Thread.Sleep(10);
		}

		[TestCleanup]
		public void TestCleanup()
		{
			AbortToken.Cancel();
			FileLockerThread.Join();
			if (File.Exists(TestFile2)) PathUtil.DeleteFile(TestFile2);
			if (File.Exists(TestFile)) PathUtil.DeleteFile(TestFile);
			if (Directory.Exists(TestDir)) Directory.Delete(TestDir);
		}

		[TestMethod]
		public void IsFileNotAccessibleTest()
		{
			Assert.IsTrue(PathUtil.IsFileNotAccessible(TestFile));
		}

		[TestMethod]
		public void FindFirstNotAccessibleFileInDirectoryTest()
		{
			string firstFile = null;
			Assert.IsTrue(PathUtil.FindFirstNotAccessibleFileInDirectory(TestDir, out firstFile));
			Assert.IsTrue(string.Compare(TestFile, firstFile, true) == 0);
		}

		[TestMethod]
		public void MoveDirectoryWithFileLockTest()
		{
			bool receivedException = false;
			string exceptionMessage = "";
			try
			{
				PathUtil.MoveDirectory(TestDir, TestDestDir);
				// Shouldn't get here.  We should have received an exception.  For now just move it back so that it won't affect other tests
				Thread.Sleep(1000);
				PathUtil.MoveDirectory(TestDestDir, TestDir);
			}
			catch (Exception ex)
			{
				receivedException = true;
				exceptionMessage = ex.Message;
			}
			Assert.IsTrue(receivedException);
		}
	}
}
