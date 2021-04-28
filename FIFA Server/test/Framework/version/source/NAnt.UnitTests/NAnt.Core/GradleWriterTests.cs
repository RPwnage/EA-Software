// Copyright (C) 2019 Electronic Arts Inc.

using Microsoft.VisualStudio.TestTools.UnitTesting;
using NAnt.Core.Writers;
using System;
using System.IO;

namespace NAnt.Core.Tests
{
	[TestClass]
	public class GradleWriterTests
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

		[TestMethod]
		public void BasicGradleWriterTest()
		{
			string expectedFileContents = @"// This is a test .gradle file
buildscript {
    repositories {
        google()
        jcenter()
    }
    dependencies {
        classpath 'com.android.tools.build:gradle:3.3.0'
    }
}
allprojects {
    repositories {
        google()
        jcenter()
    }
}
task clean(type: Delete) {
    delete rootProject.buildDir
}
";

			using (WithRandomFile randomFile = new WithRandomFile())
			{
				using (GradleWriter writer = new GradleWriter(GradleWriterFormat.Default))
				{
					writer.FileName = randomFile.Name;

					writer.WriteComment("This is a test .gradle file");
					writer.WriteStartBlock("buildscript");
					{
						writer.WriteStartBlock("repositories");
						{
							writer.WriteRepository("google");
							writer.WriteRepository("jcenter");
						}
						writer.WriteEndBlock();

						writer.WriteStartBlock("dependencies");
						{
							writer.WriteDependencyString("classpath", "com.android.tools.build:gradle", "3.3.0");
						}
						writer.WriteEndBlock();
					}
					writer.WriteEndBlock();

					writer.WriteStartBlock("allprojects");
					{
						writer.WriteStartBlock("repositories");
						{
							writer.WriteRepository("google");
							writer.WriteRepository("jcenter");
						}
						writer.WriteEndBlock();
					}
					writer.WriteEndBlock();

					writer.WriteStartTask("clean(type: Delete)");
					{
						writer.WriteLine("delete rootProject.buildDir");
					}
					writer.WriteEndTask();
				}

				Assert.IsTrue(File.Exists(randomFile.Name));
				string writtenFileContents = File.ReadAllText(randomFile.Name);
				Assert.AreEqual(writtenFileContents, expectedFileContents);
			}
		}
	}
}
