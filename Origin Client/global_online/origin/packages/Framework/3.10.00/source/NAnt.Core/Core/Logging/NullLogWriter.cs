using System;

namespace NAnt.Core.Logging
{
    internal class NullLogWriter : ILog
    {
        internal NullLogWriter()
        {
        }

        public virtual void WriteLine()
        {
        }


        public virtual void WriteLine(string arg)
        {
        }

        public virtual void WriteLine(string format, params object[] args)
        {
        }

        public virtual void Write(string arg)
        {
        }

        public virtual void Write(string format, params object[] args)
        {
        }
    }
}
