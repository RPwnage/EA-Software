using System;
using System.IO;
using System.Reflection;

namespace NAnt.Authentication.Test
{
	public class CredStoreTest : IDisposable
	{
		private string m_tempCredStoreFilePath;

		public CredStoreTest(string fileName)
		{
            // clear environment for each test, test set this explicitly
            Environment.SetEnvironmentVariable("EAT_FRAMEWORK_NET_LOGIN", null);
            Environment.SetEnvironmentVariable("EAT_FRAMEWORK_NET_DOMAIN", null);
            Environment.SetEnvironmentVariable("EAT_FRAMEWORK_NET_PASSWORD", null);

            m_tempCredStoreFilePath = fileName;
			DeleteIfExists();
		}

		public void Dispose()
		{
			DeleteIfExists();
		}

		private void DeleteIfExists()
		{
			if (File.Exists(m_tempCredStoreFilePath))
			{
				File.Delete(m_tempCredStoreFilePath);
			}
		}

	}
}