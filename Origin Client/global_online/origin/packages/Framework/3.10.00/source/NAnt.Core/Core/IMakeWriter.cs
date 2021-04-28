using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace NAnt.Core
{
    public interface IMakeWriter : ICachedWriter
    {
        void Write(string arg);

        void Write(string format, params object[] args);

        void WriteLine();

        void WriteLine(string arg);

        void WriteLine(string format, params object[] args);

        void WriteTab(string arg="");

        void WriteTab(string format, params object[] args);

        void WriteTabLine(string arg);

        void WriteTabLine(string format, params object[] args);

        void WriteComment(string arg);

        void WriteComment(string format, params object[] args);

        void WriteHeader(string arg);

        void WriteHeader(string format, params object[] args);

        Encoding Encoding { get; }
    }
}
