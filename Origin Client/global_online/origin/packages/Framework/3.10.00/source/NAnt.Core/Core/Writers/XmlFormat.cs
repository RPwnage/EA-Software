using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace NAnt.Core.Writers
{
    // Default XML format;

    public class XmlFormat : IXmlWriterFormat
    {
        public XmlFormat(char identChar, int identation, bool newLineOnAttributes, Encoding encoding)
        {
            _identChar = identChar;
            _identation = identation;
            _newLineOnAttributes = newLineOnAttributes;
            _encoding = encoding;
        }

        public char IndentChar 
        {
            get { return _identChar; } 
        }

        public int Indentation 
        {
            get { return _identation; } 
        }

        public bool NewLineOnAttributes 
        {
            get { return _newLineOnAttributes; } 
        }

        public Encoding Encoding 
        {
            get { return _encoding; } 
        }

        private readonly char _identChar;
        private readonly int _identation;
        private readonly bool _newLineOnAttributes;
        private readonly Encoding _encoding;
    }
}
