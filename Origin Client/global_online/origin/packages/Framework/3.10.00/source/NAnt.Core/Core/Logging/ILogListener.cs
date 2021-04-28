using System;

namespace NAnt.Core.Logging
{
    public interface ILogListener
    {        
        void WriteLine(string arg);        
        void Write(string arg);
        string GetBuffer();
    }
}
