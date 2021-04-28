using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace NAnt.Core.Writers
{
    public interface IXmlWriterFormat
    {
        char IndentChar {get;}

        int Indentation {get;}

        bool NewLineOnAttributes { get; }

        Encoding Encoding { get; }
    }
}
