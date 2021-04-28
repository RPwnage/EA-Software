using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace NAnt.Core.Logging
{
    public class BufferedLog : Log
    {
        public BufferedLog(Log log) : base(log.Level, log.IndentLevel, log.Id)
        {
            _log = log;
            Listeners.AddRange(LoggerFactory.Instance.CreateLogger(LoggerType.Buffer));
        }

        public void Flash()
        {
            _log.Append(this);
        }

        public override string ToString()
        {
            StringBuilder sb = new StringBuilder();

            foreach (ILogListener listener in Listeners)
            {
                sb.Append(listener.GetBuffer());
            }
            return sb.ToString();
        }


        private readonly Log _log;
    }
}
