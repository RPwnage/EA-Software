using System;

using System.Text;
using System.IO;

using NAnt.Core;
using NAnt.Core.Util;

namespace NAnt.Core.Logging
{
    class FileListener : ILogListener
    {
        private const string DEFAULT_LOG_FILE_NAME = "BuildLog.txt";
        private string _logFileName;
        private StreamWriter _writer = null;

        public readonly string LogFilePath;

        public FileListener(string logFileName = null)
        {
            try
            {
                _logFileName = logFileName ?? DEFAULT_LOG_FILE_NAME;
                LogFilePath = Path.GetFullPath(_logFileName);
                _writer = File.CreateText(LogFilePath);
                _writer.AutoFlush = true;
            }
            catch (Exception e)
            {
                throw new BuildException(String.Format("Failed to create buildlog file: '{0}'", _logFileName), e);
            }
        }
        public virtual void WriteLine(string arg)
        {
            lock (_writer)
            {
                _writer.WriteLine(arg);
            }
        }

        public virtual void Write(string arg)
        {
            lock (_writer)
            {
                _writer.Write(arg);
            }
        }

        public string GetBuffer()
        {
            return null;
        }
    }
}
