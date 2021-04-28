// NAnt - A .NET build tool
// Copyright (C) 2003-2015 Electronic Arts Inc.
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
// Copyright (C) 2003-2015 Electronic Arts Inc.
// Copyright (C) Copyright (c) 2000,2002 The Apache Software Foundation.  All rights reserved.

// File Maintainers:
// Electronic Arts (Frostbite.Team.CM@ea.com)
// Gerry Shaw (gerry_shaw@yahoo.com)
// Ian MacLean (ian_maclean@another.com)
// Scott Hernandez (ScottHernandez@hotmail.com)
// William E. Caputo (wecaputo@thoughtworks.com | logosity@yahoo.com)

using System;
using System.Collections.Concurrent;
using System.IO;
using System.Xml;

using NAnt.Core.Util;

namespace NAnt.Core
{

	internal class LineInfoElement : XmlElement, IXmlLineInfo
	{
        public int LineNumber { get; }
        public int LinePosition { get; }

        public bool HasLineInfo() { return LineNumber != 0 && LinePosition != 0; }

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

        internal LineInfoElement(string prefix, string localname, string nsURI, XmlDocument doc)
			: this(prefix, localname, nsURI, doc, 0, 0)
		{
		}

		internal LineInfoElement(string prefix, string localname, string nsURI, XmlDocument doc, int lineNumber, int linePosition)
			: base(prefix, localname, nsURI, doc)
		{
			LineNumber = lineNumber;
			LinePosition = linePosition;
		}

		public override XmlNode CloneNode(bool deep)
		{
			throw new NotImplementedException();
		}
	}

	public class LineInfoDocument : XmlDocument
	{
        public override string BaseURI
        {
            get
            {
                return m_fileName ?? base.BaseURI;
            }
        }

        private XmlReader m_reader;
		private string m_fileName = null;

		private static ConcurrentDictionary<string, Lazy<LineInfoDocument>> s_xmlDocCache = new ConcurrentDictionary<string, Lazy<LineInfoDocument>>();

        public override void Load(XmlReader reader)
        {
            m_reader = reader;
            try
            {
                base.Load(m_reader);
            }
            catch (FileNotFoundException fe)
            {
                IXmlLineInfo iLineInfo = reader as IXmlLineInfo;

                Location location = (iLineInfo != null) ?
                    new Location(reader.BaseURI, iLineInfo.LineNumber, iLineInfo.LinePosition) :
                    new Location(reader.BaseURI);
                throw new BuildException(String.Empty, location, fe, stackTrace: BuildException.StackTraceType.XmlOnly);
            }
            catch (XmlException xe)
            {
                IXmlLineInfo iLineInfo = reader as IXmlLineInfo;

                Location location = (iLineInfo != null) ?
                    new Location(reader.BaseURI, iLineInfo.LineNumber, iLineInfo.LinePosition) :
                    new Location(reader.BaseURI);

                string msg = String.Format("Error in XML document {0}", location.ToString());

                throw new BuildException(msg, location, xe, stackTrace: BuildException.StackTraceType.XmlOnly);
            }

            catch (Exception e)
            {
                IXmlLineInfo iLineInfo = reader as IXmlLineInfo;

                Location location = (iLineInfo != null) ?
                    new Location(reader.BaseURI, iLineInfo.LineNumber, iLineInfo.LinePosition) :
                    new Location(reader.BaseURI);

                string msg = String.Format("Error reading file {0}", location.ToString());

                throw new BuildException(msg, location, e, stackTrace: BuildException.StackTraceType.XmlOnly);
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
                m_reader = null;
            }
        }

        public override XmlElement CreateElement(string prefix, string localname, string namespaceuri)
        {
            int line = 0;
            int pos = 0;
            IXmlLineInfo iLineInfo = m_reader as IXmlLineInfo;
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

        public static new LineInfoDocument Load(string filename)
		{
			string path = PreparePath(filename);

			if (String.IsNullOrEmpty(path))
			{
				throw new BuildException("Can not load XML document, file name is empty.", stackTrace: BuildException.StackTraceType.XmlOnly);
			}

			LineInfoDocument doc = s_xmlDocCache.GetOrAddBlocking(path, key => 
			{
				var d = new LineInfoDocument();
				d.Load(new XmlTextReader(key));
				return d;
			});

			return doc;
		}

        public static LineInfoDocument UpdateCache(string filename, XmlReader reader)
        {
            string path = PreparePath(filename);
            if (String.IsNullOrEmpty(path))
            {
                throw new BuildException("Can not store XML document, file name is empty.", stackTrace: BuildException.StackTraceType.XmlOnly);
            }

			LineInfoDocument doc = s_xmlDocCache.AddOrUpdateBlocking
			(
				path,
				addValueFactory: key =>
				{
					LineInfoDocument d = new LineInfoDocument();
					d.Load(reader);
					return d;
				},
				updateValueFactory: (key, existing) =>
				{
					LineInfoDocument d = new LineInfoDocument();
					d.Load(reader);
					return d;
				}
			);

			return doc;
        }

        public static bool IsInCache(string filename)
        {
            string path = PreparePath(filename);
            if (!String.IsNullOrEmpty(path))
            {
                return s_xmlDocCache.ContainsKey(filename);
            }
            return false;
        }

        internal static LineInfoDocument Parse(string documentcontents)
		{
			LineInfoDocument document = new LineInfoDocument();
			document.LoadXml(documentcontents);
			return document;
		}

        // for testing only
        internal static void ClearCache()
        {
            s_xmlDocCache.Clear();
        }

		private static string PreparePath(string filename)
		{
			//IM do I need this magic with URI?

			// if the filename is not a valid URI, pass it though.
			// if the source is a file URI, pass the local path of it through.
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
                {
                    path = filename;
                }
			}
    		return path;
		}
	}
}