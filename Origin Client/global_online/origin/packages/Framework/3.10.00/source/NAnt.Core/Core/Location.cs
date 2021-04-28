using System;
using System.Collections.Generic;
using System.Xml;
using System.Text;

using NAnt.Core.Util;

namespace NAnt.Core
{
    /// <summary>
    /// Stores the file name, line number and column number to record a position in a text file.
    /// </summary>
    public class Location
    {
        private readonly string _fileName;
        public readonly int LineNumber;
        public readonly int ColumnNumber;

        public static readonly Location UnknownLocation = new Location();

        public string FileName
        {
            get
            {
                if (_fileName != null)
                {
                    string path = _fileName;
                    try
                    {
                        Uri testURI = UriFactory.CreateUri(_fileName);

                        if (testURI.IsAbsoluteUri && testURI.IsFile)
                        {
                            path = testURI.LocalPath;
                        }
                    }
                    catch (Exception)
                    {
                        // do nothing
                    }
                    finally
                    {
                        if (path == null)
                            path = _fileName;
                    }
                    return path;
                }
                return _fileName;
            }

        }
        ///
        /// <summary>Location factory that return meaningful location from custom nodes that
        /// support position information.
        /// </summary>
        ///
        static public Location GetLocationFromNode(XmlNode node)
        {
            IXmlLineInfo iLineInfo = node as IXmlLineInfo;
            if (null != iLineInfo && iLineInfo.HasLineInfo())
            {
                return new Location(node.OwnerDocument.BaseURI, iLineInfo.LineNumber, iLineInfo.LinePosition);
            }
            return UnknownLocation;
        }

        /// <summary>Creates a location consisting of a file name, line number and column number.</summary>
        /// <remarks>fileName can be a local URI resource, e.g., file:///C:/WINDOWS/setuplog.txt</remarks>
        public Location(string fileName, int lineNumber, int columnNumber)
        {
            _fileName = fileName;
            LineNumber = lineNumber;
            ColumnNumber = columnNumber;
        }

        /// <summary>Creates a location consisting of a file name.</summary>
        /// <remarks>fileName can be a local URI resource, e.g., file:///C:/WINDOWS/setuplog.txt</remarks>
        public Location(string fileName) : this(fileName, 0, 0) 
        {
        }

        /// <summary>Creates an "unknown" location.</summary>
        private Location() : this(null, 0,0)
        {
        }

        /// <summary>
        /// Returns the file name, line number and a trailing space. An error
        /// message can be appended easily. For unknown locations, returns
        /// an empty string.
        ///</summary>
        public override string ToString()
        {
            StringBuilder sb = new StringBuilder("");

            if (FileName != null)
            {
                sb.Append(FileName);
                if (LineNumber != 0)
                {
                    sb.Append(String.Format("({0},{1})", LineNumber, ColumnNumber));
                }
                sb.Append(":");
            }

            return sb.ToString();
        }

    }
}
