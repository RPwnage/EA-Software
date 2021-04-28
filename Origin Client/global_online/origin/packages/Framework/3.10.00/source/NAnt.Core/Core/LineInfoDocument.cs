// NAnt - A .NET build tool
// Copyright (C) 2001-2003 Gerry Shaw
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
// 
// As a special exception, the copyright holders of this software give you 
// permission to link the assemblies with independent modules to produce 
// new assemblies, regardless of the license terms of these independent 
// modules, and to copy and distribute the resulting assemblies under terms 
// of your choice, provided that you also meet, for each linked independent 
// module, the terms and conditions of the license of that module. An 
// independent module is a module which is not derived from or based 
// on these assemblies. If you modify this software, you may extend 
// this exception to your version of the software, but you are not 
// obligated to do so. If you do not wish to do so, delete this exception 
// statement from your version. 
// 
// 
// 
// The events described in this file are based on the comments and structure of Ant.
// Copyright (C) Copyright (c) 2000,2002 The Apache Software Foundation.  All rights reserved.

// File Maintainers:
// Gerry Shaw (gerry_shaw@yahoo.com)
// Ian MacLean (ian_maclean@another.com)
// Scott Hernandez (ScottHernandez@hotmail.com)
// William E. Caputo (wecaputo@thoughtworks.com | logosity@yahoo.com)

using System;
using System.IO;
using System.Xml;
using System.Collections.Concurrent;

using NAnt.Core.Util;

namespace NAnt.Core
{

    internal class LineInfoElement : XmlElement, IXmlLineInfo
    {
        private int _lineNumber;
        private int _linePosition;

        internal LineInfoElement(string prefix, string localname, string nsURI, XmlDocument doc)
            : this(prefix, localname, nsURI, doc, 0, 0)
        {
        }

        internal LineInfoElement(string prefix, string localname, string nsURI, XmlDocument doc, int lineNumber, int linePosition)
            : base(prefix, localname, nsURI, doc)
        {
            _lineNumber = lineNumber;
            _linePosition = linePosition;
        }

        public int LineNumber
        {
            get
            {
                return _lineNumber;
            }
        }
        public int LinePosition
        {
            get
            {
                return _linePosition;
            }
        }

        public bool HasLineInfo() { return _lineNumber != 0 && _linePosition != 0; }

        public override string BaseURI
        {
            get
            {
                string baseUri = base.BaseURI;
                if (String.IsNullOrEmpty(baseUri) && OwnerDocument != null)
                {
                    baseUri = OwnerDocument.BaseURI;
                }
                return baseUri;
            }
        }

        public override XmlNode CloneNode(bool deep)
        {
            throw new NotImplementedException();
        }


    }

    public class LineInfoDocument : XmlDocument
    {
        private XmlReader reader;
        private string filename = null;

        private static ConcurrentDictionary<string, Lazy<LineInfoDocument>> _xmlDocCache = new ConcurrentDictionary<string, Lazy<LineInfoDocument>>();

        public static new LineInfoDocument Load(string filename)
        {
            //IM do I need thios woodoo with URI?

            // if the filename is not a valid uri, pass it thru.
            // if the source is a file uri, pass the localpath of it thru.
            string path = filename;
            try
            {
                Uri testURI = UriFactory.CreateUri(filename);

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
                    path = filename;
            }

            if (String.IsNullOrEmpty(path))
            {
                throw new BuildException("Can not load Xml document, file name is empty.", BuildException.StackTraceType.XmlOnly);
            }

            LineInfoDocument doc = _xmlDocCache.GetOrAddBlocking(path, key => 
            {
                var d = new LineInfoDocument(); 
                d.Load(new XmlTextReader(key));
                return d;
            });

            return doc;
        }

        public override void Load(XmlReader reader)
        {
            this.reader = reader;
            try
            {
                base.Load(this.reader);
            }
            catch (System.IO.FileNotFoundException fe)
            {
                IXmlLineInfo iLineInfo = reader as IXmlLineInfo;

                var location = (iLineInfo != null) ?
                        new Location(reader.BaseURI, iLineInfo.LineNumber, iLineInfo.LinePosition)
                        : new Location(reader.BaseURI);
                throw new BuildException(String.Empty, location, fe, BuildException.StackTraceType.XmlOnly);
            }
            catch (XmlException xe)
            {
                IXmlLineInfo iLineInfo = reader as IXmlLineInfo;

                var location = (iLineInfo != null) ?
                        new Location(reader.BaseURI, iLineInfo.LineNumber, iLineInfo.LinePosition)
                        : new Location(reader.BaseURI);

                var msg = String.Format("Error in XML document {0}", location.ToString());

                throw new BuildException(msg, location, xe, BuildException.StackTraceType.XmlOnly);
            }

            catch (Exception e)
            {
                IXmlLineInfo iLineInfo = reader as IXmlLineInfo;

                var location = (iLineInfo != null) ?
                        new Location(reader.BaseURI, iLineInfo.LineNumber, iLineInfo.LinePosition)
                        : new Location(reader.BaseURI);

                var msg = String.Format("Error reading file {0}", location.ToString());

                throw new BuildException(msg, location, e, BuildException.StackTraceType.XmlOnly);
            }
            finally
            {
                //
                // whatever happens, close the reader asap. this is mainly
                // not to break self-tester because it tries to delete working folder
                // immediately after the test build is completed. it is unimportant
                // for real-life projects
                //
                reader.Close();
                this.reader = null;
            }
        }

        public override XmlElement CreateElement(string prefix, string localname, string namespaceuri)
        {
            int line = 0;
            int pos = 0;
            IXmlLineInfo iLineInfo = reader as IXmlLineInfo;
            if (iLineInfo != null)
            {
                line = iLineInfo.LineNumber;
                pos = iLineInfo.LinePosition;

            }
            return new LineInfoElement(prefix, localname, namespaceuri, this, line, pos);
        }

        public override XmlNode CreateNode(string prefix, string localname, string namespaceuri)
        {
            return base.CreateNode(prefix, localname, namespaceuri);
        }

        public XmlElement CreateElement(string localname, Location location)
        {
            filename = location.FileName;
            return new LineInfoElement(Prefix, localname, NamespaceURI, this, location.LineNumber, location.ColumnNumber);
        }
        public override string BaseURI 
        {
            get 
            { 
                return filename ?? base.BaseURI; 
            }
        }
    }
}