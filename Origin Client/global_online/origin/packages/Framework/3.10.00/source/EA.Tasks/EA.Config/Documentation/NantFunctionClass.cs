using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Xml;

namespace EA.Eaconfig
{
    public class NantFunctionClass
    {
        public string Name;
        public XmlNode Description;
        public List<NantFunction> Functions = new List<NantFunction>();
    }
}
