using System;
using System.IO;
using Microsoft.VisualStudio.TestTools.UnitTesting;

using NAnt.Core.Writers;

namespace NAnt.Core.Tests
{
	[TestClass]
	public class CachedWriterTests
	{
		internal class WithRandomFile : IDisposable
		{
			public string Name { get; private set; }

			public WithRandomFile()
			{
				string tempPath = Path.GetTempPath();
				string tempFile = Path.GetRandomFileName();
				Name = Path.Combine(tempPath, tempFile + ".test");
			}

			public void Dispose()
			{
				if (File.Exists(Name))
				{
					File.Delete(Name);
				}
			}
		}

		internal class WithFile : IDisposable
		{
			public string Name { get; private set; }

			public WithFile(string name)
			{
				Name = name;
			}

			public WithFile(string name, bool delete)
				: this(name)
			{
				if (delete && File.Exists(name))
				{
					File.Delete(name);
				}
			}

			public void Dispose()
			{
				if (File.Exists(Name))
				{
					File.Delete(Name);
				}
			}
		}

		internal class BasicCachedWriter : CachedWriter
		{
			public BasicCachedWriter(string fileName)
			{
				FileName = fileName;
			}
		}

		internal class Helpers
		{
			static Random s_random = new Random();

			public static bool Bcmp(byte[] lhs, byte[] rhs)
			{
				if (lhs.Length != rhs.Length)
				{
					return false;
				}

				for (int i = 0; i < lhs.Length; ++i)
				{
					if (lhs[i] != rhs[i])
					{
						return false;
					}
				}

				return true;
			}

			public static void RandomFill(byte[] array)
			{
				s_random.NextBytes(array);
			}
		}

		[TestMethod]
		public void BasicCachedWriterTest()
		{
			using (WithRandomFile randomFile = new WithRandomFile())
			{
				byte[] data = new byte[1];

				using (BasicCachedWriter writer = new BasicCachedWriter(randomFile.Name))
				{
					writer.Data.Write(data, 0, data.Length);
				}

				byte[] writtenData = File.ReadAllBytes(randomFile.Name);
				Assert.IsTrue(Helpers.Bcmp(data, writtenData));
			}

			using (WithRandomFile randomFile = new WithRandomFile())
			{
				byte[] data = new byte[262147];
				Helpers.RandomFill(data);

				using (BasicCachedWriter writer = new BasicCachedWriter(randomFile.Name))
				{
					writer.Data.Write(data, 0, data.Length);
				}

				byte[] writtenData = File.ReadAllBytes(randomFile.Name);
				Assert.IsTrue(Helpers.Bcmp(data, writtenData));
			}
		}

		[TestMethod]
		public void BackupFileTest()
		{
			try
			{
				using (WithRandomFile randomFile = new WithRandomFile())
				{
					byte[] data = new byte[131072];
					Helpers.RandomFill(data);

					CachedWriter.SetBackupGeneratedFilesPolicy(true);

					Assert.IsTrue(!File.Exists(randomFile.Name));

					using (WithFile backupFile = new WithFile(CachedWriter.GetBackupFileName(randomFile.Name), true))
					{
						Assert.IsTrue(!File.Exists(backupFile.Name));

						using (BasicCachedWriter writer = new BasicCachedWriter(randomFile.Name))
						{
							writer.Data.Write(data, 0, data.Length);
						}

						Assert.IsTrue(!File.Exists(backupFile.Name));

						byte[] writtenData = File.ReadAllBytes(randomFile.Name);
						Assert.IsTrue(Helpers.Bcmp(data, writtenData));

						byte[] newData = new Byte[65537];
						Helpers.RandomFill(newData);

						using (BasicCachedWriter writer = new BasicCachedWriter(randomFile.Name))
						{
							writer.Data.Write(newData, 0, newData.Length);
						}

						Assert.IsTrue(File.Exists(backupFile.Name));
						byte[] writtenNewData = File.ReadAllBytes(randomFile.Name);
						Assert.IsTrue(Helpers.Bcmp(newData, writtenNewData));

						byte[] backupData = File.ReadAllBytes(backupFile.Name);
						Assert.IsTrue(Helpers.Bcmp(data, backupData));
					}
				}
			}
			finally
			{
				CachedWriter.SetBackupGeneratedFilesPolicy(false);
			}
		}
	}
}
