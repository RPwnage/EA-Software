using System;
using System.Collections.Generic;
using System.IO;
using System.Xml;

using NAnt.Core;
using NAnt.Core.Logging;
using NAnt.Core.PackageCore;
using NAnt.Core.Util;

namespace NAnt.MasterConfigUpdaterTest
{
    // simple disposable class for writing out masterconfig file for the test
    internal sealed class MasterConfigUpdateTestCase : IDisposable
    {
        internal readonly string MasterConfigPath;
        internal readonly string Contents;
        internal readonly Project Project;
        internal readonly Dictionary<string, Tuple<string, string>> Fragments = new Dictionary<string, Tuple<string, string>>();

        private bool m_diposed = false;

        internal MasterConfigUpdateTestCase(string contents, Dictionary<string, Tuple<string, string>> fragments)
        {
            string baseDir = Path.Combine(Path.GetTempPath(), "masterconfigtest", Path.GetFileNameWithoutExtension(Path.GetRandomFileName()));
            if (!Directory.Exists(baseDir))
            {
                Directory.CreateDirectory(baseDir);
            }

            // write to temp file
            MasterConfigPath = Path.Combine(baseDir, "masterconfig.xml");
            File.WriteAllText(MasterConfigPath, contents);
            Contents = contents;

            // write fragments to temp files
            fragments = fragments ?? new Dictionary<string, Tuple<string, string>>();
            foreach (KeyValuePair<string, Tuple<string, string>> relativePathToContents in fragments)
            {
                string fragmentPath = Path.Combine(baseDir, relativePathToContents.Key);
                string fragmentDir = Path.GetDirectoryName(fragmentPath);
                if (!Directory.Exists(fragmentDir))
                {
                    Directory.CreateDirectory(fragmentDir);
                }
                File.WriteAllText(fragmentPath, relativePathToContents.Value.Item1);
                Fragments.Add(fragmentPath, relativePathToContents.Value);
            }

			Project = new Project(Log.Silent);
        }

        public void VerifyXmlLoad()
        {
            if (m_diposed)
            {
                throw new InvalidOperationException("Cannot verify disposed test case!");
            }

            // check document is valid by standard library's reckoning
            XmlDocument document = new XmlDocument();
            document.Load(MasterConfigPath);
        }

        public void VerifyNantLoad()
        {
            if (m_diposed)
            {
                throw new InvalidOperationException("Cannot verify disposed test case!");
            }

            // check masterconfig is valid by framework's reckoning by trting to load it - we'll see exceptions if something is wrong
            MasterConfig.Load(new Project(Log.Silent), MasterConfigPath);
        }

        public void Dispose()
		{
			if (!m_diposed)
            {
                File.Delete(MasterConfigPath);
                foreach (string fragmentPath in Fragments.Keys)
                {
                    File.Delete(fragmentPath);
                }
                m_diposed = true;
            }
        }
    }
}