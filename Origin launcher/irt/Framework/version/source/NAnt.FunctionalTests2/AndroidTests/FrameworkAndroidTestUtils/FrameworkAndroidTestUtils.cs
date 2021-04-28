using System;
using System.IO;

using ICSharpCode.SharpZipLib.Zip;
using NAnt.Core;
using NAnt.Core.Attributes;

namespace FrameworkAndroidTestUtils
{
	[FunctionClass("Android Test Utility Functions")]
	public static class FrameworkAndroidTestUtils
	{
		[Function]
		public static string ApkContains(Project project, string zipPath, string filePath)
		{
			filePath = filePath.Replace('\\', '/').TrimStart('/');
			using (Stream zipStream = File.OpenRead(zipPath))
			using (ZipFile zipFile = new ZipFile(zipStream))
			{
				foreach (ZipEntry zipEntry in zipFile)
				{
					if (zipEntry.Name == filePath)
					{
						return Boolean.TrueString;
					}
				}
			}
			return Boolean.FalseString;
		}
	}
}