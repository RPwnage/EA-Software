using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace NAnt.Core.Writers
{
    internal class MakeFormat
    {
        internal readonly static char COMMENT = '#';
        internal readonly static char SPACE = ' ';
        internal readonly static char TAB = '\t';
        internal readonly static string HEADER_SEP_LINE = COMMENT + new String('-', 80);
        internal readonly static string EOL = "\r\n";
    }
}
