using System;

namespace NAnt.Core.Logging
{
    public interface ILog
    {
        void WriteLine();
        void WriteLine(string arg);
        void WriteLine(string format, params object[] args);
        void Write(string arg);
        void Write(string format, params object[] args);

    }
}
