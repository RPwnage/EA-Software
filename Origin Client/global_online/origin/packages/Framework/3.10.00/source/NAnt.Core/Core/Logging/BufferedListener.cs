using System;
using System.Text;

namespace NAnt.Core.Logging
{

    public class BufferedListener : ILogListener
    {
        public BufferedListener(StringBuilder sb = null)
        {
            _sb = _sb ?? new StringBuilder();
        }
        public virtual void WriteLine(string arg)
        {
            _sb.AppendLine(arg);
        }

        public virtual void Write(string arg)
        {
            _sb.Append(arg);
        }

        public override string ToString()
        {
            return _sb.ToString();
        }

        public string GetBuffer()
        {
            return _sb.ToString();
        }

        private readonly StringBuilder _sb;
    }
}
