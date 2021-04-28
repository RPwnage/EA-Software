using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace NAnt.Core
{
    public interface ITextWriter : ICachedWriter
    {
        void Write(string arg);

        void Write(string format, params object[] args);

        void WriteLine();

        void WriteLine(string arg);

        void WriteLine(string format, params object[] args);

        void WriteNonEmpty(string arg);

        void WriteNonEmpty(string format, params object[] args);

        void WriteNonEmptyLine(string arg);

        void WriteNonEmptyLine(string format, params object[] args);

        int Indent { get; set; }

        int IndentSize { get; set; }

        Encoding Encoding { get; }
    }
}
