using System;
using System.Linq;
using System.Text;

using Microsoft.VisualStudio.TestTools.UnitTesting;

using NAnt.Core.Logging;

namespace NAnt.Core.Tests
{
    [TestClass]
    public class LoggingTests
    {
        [TestMethod]
        public void WarningLevelTest()
        {
            StringBuilder builder = new StringBuilder();
            BufferedListener listener = new BufferedListener(builder);

            Log warningLog = new Log(warningLevel: Log.WarnLevel.Normal, listeners: new ILogListener[] { listener });

            // test a simple minimal warning, should show up in log
            warningLog.ThrowWarning(Log.WarningId.BuildFailure, Log.WarnLevel.Minimal, "minimal warning test");
            Assert.IsTrue(warningLog.IsWarningEnabled(Log.WarningId.BuildFailure, Log.WarnLevel.Minimal));
            Assert.IsTrue(builder.ToString().TrimEnd().EndsWith("minimal warning test"));
            builder.Clear();

            // test a simple normal warning, should show up in log
            warningLog.ThrowWarning(Log.WarningId.BuildFailure, Log.WarnLevel.Normal, "normal warning test");
            Assert.IsTrue(warningLog.IsWarningEnabled(Log.WarningId.BuildFailure, Log.WarnLevel.Normal));
            Assert.IsTrue(builder.ToString().TrimEnd().EndsWith("normal warning test"));
            builder.Clear();

            // test an advise level warning, should not be thrown
            warningLog.ThrowWarning(Log.WarningId.BuildFailure, Log.WarnLevel.Advise, "advise warning test");
            Assert.IsFalse(warningLog.IsWarningEnabled(Log.WarningId.BuildFailure, Log.WarnLevel.Advise));
            Assert.AreEqual(builder.ToString(), String.Empty);
            builder.Clear();
        }

		[TestMethod]
		public void DeprecationLevelTest()
		{
			StringBuilder builder = new StringBuilder();
			BufferedListener listener = new BufferedListener(builder);

			Log deprecationLog = new Log(deprecationLevel: Log.DeprecateLevel.Normal, listeners: new ILogListener[] { listener });

			// test a simple minimal deprecation, should show up in log
			deprecationLog.ThrowDeprecation(Log.DeprecationId.SysInfo, Log.DeprecateLevel.Minimal, "minimal deprecation test");
			Assert.IsTrue(deprecationLog.IsDeprecationEnabled(Log.DeprecationId.SysInfo, Log.DeprecateLevel.Minimal));
			Assert.IsTrue(builder.ToString().TrimEnd().EndsWith("minimal deprecation test"));
			builder.Clear();

			// test a simple normal deprecation, should show up in log
			deprecationLog.ThrowDeprecation(Log.DeprecationId.SysInfo, Log.DeprecateLevel.Normal, "normal deprecation test");
			Assert.IsTrue(deprecationLog.IsDeprecationEnabled(Log.DeprecationId.SysInfo, Log.DeprecateLevel.Normal));
			Assert.IsTrue(builder.ToString().TrimEnd().EndsWith("normal deprecation test"));
			builder.Clear();

			// test an advise level deprecation, should not be thrown
			deprecationLog.ThrowDeprecation(Log.DeprecationId.SysInfo, Log.DeprecateLevel.Advise, "advise deprecation test");
			Assert.IsFalse(deprecationLog.IsDeprecationEnabled(Log.DeprecationId.SysInfo, Log.DeprecateLevel.Advise));
			Assert.AreEqual(builder.ToString(), String.Empty);
			builder.Clear();
		}

		[TestMethod]
        public void TestWarningEnableDisable()
        {
            StringBuilder builder = new StringBuilder();
            BufferedListener listener = new BufferedListener(builder);

            Log warningLog = new Log(warningLevel: Log.WarnLevel.Normal, listeners: new ILogListener[] { listener });

            warningLog.ThrowWarning(Log.WarningId.BuildFailure, Log.WarnLevel.Normal, "default enabled warning test");
            Assert.IsTrue(warningLog.IsWarningEnabled(Log.WarningId.BuildFailure, Log.WarnLevel.Normal));
            Assert.IsTrue(builder.ToString().TrimEnd().EndsWith("default enabled warning test"));
            builder.Clear();

            try
            {
                // disable warning globally
                Log.SetGlobalWarningEnabledIfNotSet(warningLog, Log.WarningId.BuildFailure, enabled: false, enabledSettingSource: Log.SettingSource.CommandLine);
                warningLog.ThrowWarning(Log.WarningId.BuildFailure, Log.WarnLevel.Normal, "global disabled warning test");
                Assert.IsFalse(warningLog.IsWarningEnabled(Log.WarningId.BuildFailure, Log.WarnLevel.Normal));
                Assert.AreEqual(builder.ToString(), String.Empty);
                builder.Clear();

                // try to enabled warning globally, should have no effect as it can only be set once
                Log.SetGlobalWarningEnabledIfNotSet(warningLog, Log.WarningId.BuildFailure, enabled: true, enabledSettingSource: Log.SettingSource.CommandLine);
                warningLog.ThrowWarning(Log.WarningId.BuildFailure, Log.WarnLevel.Normal, "global re-enabled warning test");
                Assert.IsFalse(warningLog.IsWarningEnabled(Log.WarningId.BuildFailure, Log.WarnLevel.Normal));
                Assert.IsTrue(builder.ToString().Contains("already explicitly disabled"));
                Assert.IsFalse(builder.ToString().Contains("global re-enabled warning test"));
                builder.Clear();

                // now enabled locally, this should trump global setting
                warningLog.SetWarningEnabledIfNotSet(Log.WarningId.BuildFailure, enabled: true, enabledSettingSource: Log.SettingSource.CommandLine);
                warningLog.ThrowWarning(Log.WarningId.BuildFailure, Log.WarnLevel.Normal, "local enabled warning test");
                Assert.IsTrue(warningLog.IsWarningEnabled(Log.WarningId.BuildFailure, Log.WarnLevel.Normal));
                Assert.IsTrue(builder.ToString().TrimEnd().EndsWith("local enabled warning test"));
                builder.Clear();

                // should be enabled, even if it's warning level is above the default level
                warningLog.ThrowWarning(Log.WarningId.BuildFailure, Log.WarnLevel.Advise, "local advise enabled warning test");
                Assert.IsTrue(warningLog.IsWarningEnabled(Log.WarningId.BuildFailure, Log.WarnLevel.Advise));
                Assert.IsTrue(builder.ToString().TrimEnd().EndsWith("local advise enabled warning test"));
                builder.Clear();

                // test legacy properties
                Project warningProject = new Project(warningLog);

                warningProject.Properties["nant.warningsuppression"] = Log.WarningId.BuildFailure.ToString("d");
                warningLog.ApplyLegacyProjectSuppressionProperties(warningProject);

                // legacy properties stomp other settings, so waring should be disabled despite local previous local enabled
                warningLog.ThrowWarning(Log.WarningId.BuildFailure, Log.WarnLevel.Normal, "legacy suppressed warning test");
                Assert.IsFalse(warningLog.IsWarningEnabled(Log.WarningId.BuildFailure, Log.WarnLevel.Normal));
                Assert.IsFalse(builder.ToString().Contains("legacy suppressed warning test"));
                builder.Clear();
            }
            finally
            {
                Log.ResetGlobalWarningStates();
            }
        }

		[TestMethod]
		public void TestDeprecationEnableDisable()
		{
			StringBuilder builder = new StringBuilder();
			BufferedListener listener = new BufferedListener(builder);

			Log deprecationLog = new Log(deprecationLevel: Log.DeprecateLevel.Normal, listeners: new ILogListener[] { listener });

			deprecationLog.ThrowDeprecation(Log.DeprecationId.SysInfo, Log.DeprecateLevel.Normal, "default enabled deprecation test");
			Assert.IsTrue(deprecationLog.IsDeprecationEnabled(Log.DeprecationId.SysInfo, Log.DeprecateLevel.Normal));
			Assert.IsTrue(builder.ToString().TrimEnd().EndsWith("default enabled deprecation test"));
			builder.Clear();

			try
			{
				// disable warning globally
				Log.SetGlobalDeprecationEnabledIfNotSet(deprecationLog, Log.DeprecationId.SysInfo, enabled: false, enabledSettingSource: Log.SettingSource.CommandLine);
				deprecationLog.ThrowDeprecation(Log.DeprecationId.SysInfo, Log.DeprecateLevel.Normal, "global disabled deprecation test");
				Assert.IsFalse(deprecationLog.IsDeprecationEnabled(Log.DeprecationId.SysInfo, Log.DeprecateLevel.Normal));
				Assert.AreEqual(builder.ToString(), String.Empty);
				builder.Clear();

				// try to enabled warning globally, should have no effect as it can only be set once
				Log.SetGlobalDeprecationEnabledIfNotSet(deprecationLog, Log.DeprecationId.SysInfo, enabled: true, enabledSettingSource: Log.SettingSource.CommandLine);
				deprecationLog.ThrowDeprecation(Log.DeprecationId.SysInfo, Log.DeprecateLevel.Normal, "global re-enabled deprecation test");
				Assert.IsFalse(deprecationLog.IsDeprecationEnabled(Log.DeprecationId.SysInfo, Log.DeprecateLevel.Normal));
				Assert.IsTrue(builder.ToString().Contains("already explicitly disabled"));
				Assert.IsFalse(builder.ToString().Contains("global re-enabled deprecation test"));
				builder.Clear();

				// now enabled locally, this should trump global setting
				deprecationLog.SetDeprecationEnabledIfNotSet(Log.DeprecationId.SysInfo, enabled: true, enabledSettingSource: Log.SettingSource.CommandLine);
				deprecationLog.ThrowDeprecation(Log.DeprecationId.SysInfo, Log.DeprecateLevel.Normal, "local enabled deprecation test");
				Assert.IsTrue(deprecationLog.IsDeprecationEnabled(Log.DeprecationId.SysInfo, Log.DeprecateLevel.Normal));
				Assert.IsTrue(builder.ToString().TrimEnd().EndsWith("local enabled deprecation test"));
				builder.Clear();

				// should be enabled, even if it's warning level is above the default level
				deprecationLog.ThrowDeprecation(Log.DeprecationId.SysInfo, Log.DeprecateLevel.Advise, "local advise enabled deprecation test");
				Assert.IsTrue(deprecationLog.IsDeprecationEnabled(Log.DeprecationId.SysInfo, Log.DeprecateLevel.Advise));
				Assert.IsTrue(builder.ToString().TrimEnd().EndsWith("local advise enabled deprecation test"));
				builder.Clear();
			}
			finally
			{
				Log.ResetGlobalDeprecationStates();
			}
		}

		[TestMethod]
        public void BufferLogTest()
        {
            StringBuilder builder = new StringBuilder();
            BufferedListener listener = new BufferedListener(builder);

            StringBuilder errorBuilder = new StringBuilder();
            BufferedListener errorListener = new BufferedListener(errorBuilder);

            Log flushLog = new Log(Log.LogLevel.Normal, listeners: new ILogListener[] { listener }, errorListeners: new ILogListener[] { errorListener });

            using (BufferedLog bufferLog = new BufferedLog(flushLog))
            {
                // interleave error and regular writes
                bufferLog.Status.WriteLine("line 1");
                bufferLog.Error.WriteLine("error line 1");
                bufferLog.Status.WriteLine("line 2");
                bufferLog.Error.WriteLine("error line 2");
                bufferLog.Status.WriteLine("line 3");

                // nothing should have been logged to flush log yet
                Assert.AreEqual(builder.ToString(), String.Empty);

                bufferLog.Dispose();

                // check buffered log has now flushed and regular listener has no error lines
                string[] lines = builder.ToString().Split(new string[] { Environment.NewLine }, StringSplitOptions.RemoveEmptyEntries);
                Assert.IsTrue((lines.ElementAtOrDefault(0) ?? String.Empty).EndsWith("line 1"));
                Assert.IsTrue((lines.ElementAtOrDefault(1) ?? String.Empty).EndsWith("line 2"));
                Assert.IsTrue((lines.ElementAtOrDefault(2) ?? String.Empty).EndsWith("line 3"));

                // check error only recieved error lines
                string[] errorLines = errorBuilder.ToString().Split(new string[] { Environment.NewLine }, StringSplitOptions.RemoveEmptyEntries);
                Assert.IsTrue((errorLines.ElementAtOrDefault(0) ?? String.Empty).EndsWith("error line 1"));
                Assert.IsTrue((errorLines.ElementAtOrDefault(1) ?? String.Empty).EndsWith("error line 2"));
            }

            // check second dispose didn't double log
            string[] secondDisposeLines = builder.ToString().Split(new string[] { Environment.NewLine }, StringSplitOptions.RemoveEmptyEntries);
            Assert.AreEqual(secondDisposeLines.Length, 3);

            string[] secondDisposeErrorLines = errorBuilder.ToString().Split(new string[] { Environment.NewLine }, StringSplitOptions.RemoveEmptyEntries);
            Assert.AreEqual(secondDisposeErrorLines.Length, 2);
        }
    }
}
