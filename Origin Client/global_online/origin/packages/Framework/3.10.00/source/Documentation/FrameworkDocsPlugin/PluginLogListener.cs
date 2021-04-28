using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

using SandcastleBuilder.Utils.BuildEngine;
using NAnt.Core.Logging;

namespace FrameworkDocsPlugin
{
    public class PluginLogListener : ILogListener
    {
        public PluginLogListener(BuildProcess buildProcess)
        {
            _buildProcess = buildProcess;
        }
        public virtual void WriteLine(string arg)
        {
             _buildProcess.ReportProgress(arg);
        }

        public virtual void Write(string arg)
        {
            _buildProcess.ReportProgress(arg);
        }

        public string GetBuffer()
        {
            return null;
        }

        private readonly BuildProcess _buildProcess;
    }

}
