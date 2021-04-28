using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace NAnt.Core.Logging
{
    class ConsoleListener : ILogListener
    {

        public ConsoleListener()
        {            
        }
        public virtual void WriteLine(string arg)
        {         
            Console.WriteLine(arg);
        }

        public virtual void Write(string arg)
        {
            Console.Write(arg);
        }

        public string GetBuffer()
        {
            return null;
        }
    }

    class ConsoleStdErrListener : ILogListener
    {

        public ConsoleStdErrListener()
        {
        }

        public virtual void WriteLine(string arg)
        {
            Console.Error.WriteLine(arg);
        }

        public virtual void Write(string arg)
        {
            Console.Error.Write(arg);
        }

        public string GetBuffer()
        {
            return null;
        }
    }

}
