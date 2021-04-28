using System;
using System.IO;
using System.Reflection;
using System.Text;

using NAnt.Core.Logging;
using NAnt.Core.Util;

namespace NAnt.Tests.Util
{
	internal static class TestUtil
	{
		internal sealed class TestDirectory : IDisposable
		{
			private readonly string m_directoryPath;

			internal TestDirectory(string directoryName)
			{
				m_directoryPath = CreateTestDirectory(directoryName);
			}

			public static implicit operator string(TestDirectory directory) => directory.m_directoryPath;

			public void Dispose()
			{
				if (Directory.Exists(m_directoryPath))
				{
					PathUtil.DeleteDirectory(m_directoryPath);
				}
			}
		}

		// create a log that write all output to a StringBuilder - the builder can be used to check certain logging / warning message were
		// emitted. The builder can be safely cleared between calls to reset contents
		internal static Log CreateRecordingLog(ref StringBuilder builder, Log.LogLevel level = Log.LogLevel.Normal, Log.WarnLevel warningLevel = Log.WarnLevel.Normal)
		{
			BufferedListener listener = new BufferedListener(builder);
			return new Log
			(
				level: level,
				warningLevel: warningLevel,
				listeners: new ILogListener[] { listener },
				errorListeners: new ILogListener[] { listener }
			);
		}

		internal static string CreateTestDirectory(string directoryName)
		{
			string testDir = Path.GetFullPath(Path.Combine
			(
				Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location),
				"../../../Local",
				directoryName
			));
			if (Directory.Exists(testDir))
			{
				PathUtil.DeleteDirectory(testDir);
			}
			Directory.CreateDirectory(testDir);
			return testDir;
		}
	}
}