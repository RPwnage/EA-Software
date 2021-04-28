// Originally based on NAnt - A .NET build tool
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
// Gerry Shaw (gerry_shaw@yahoo.com)
// Ian MacLean (ian_maclean@another.com)
// Electronic Arts (Frostbite.Team.CM@ea.com)

using System.Xml;

namespace NAnt.Core.Writers
{
	public class XmlWriter : CachedWriter, IXmlWriter
	{
		private System.Xml.XmlWriter m_xmlWriterImpl;

		public XmlWriter(IXmlWriterFormat format)
		{
			XmlWriterSettings writerSettings = new XmlWriterSettings
			{
				Encoding = format.Encoding,
				Indent = true,
				IndentChars = new string(format.IndentChar, format.Indentation),
				NewLineOnAttributes = format.NewLineOnAttributes,
				CloseOutput = false
			};
			m_xmlWriterImpl = System.Xml.XmlWriter.Create(Data, writerSettings);
		}

		public virtual void WriteStartElement(string localName)
		{
			m_xmlWriterImpl.WriteStartElement(localName);
		}

		public virtual void WriteStartElement(string localName, string ns)
		{
			m_xmlWriterImpl.WriteStartElement(localName, ns);
		}

		public virtual void WriteAttributeString(string localName, string value)
		{
			m_xmlWriterImpl.WriteAttributeString(localName, value);
		}

		public virtual void WriteAttributeString(string prefix, string localName, string ns, string value)
		{
			m_xmlWriterImpl.WriteAttributeString(prefix, localName, ns, value);
		}

		public virtual void WriteAttributeString(string localName, bool value)
		{
			m_xmlWriterImpl.WriteAttributeString(localName, value.ToString().ToLowerInvariant());
		}

		public virtual void WriteEndElement()
		{
			m_xmlWriterImpl.WriteEndElement();
		}

		public virtual void WriteFullEndElement()
		{
			m_xmlWriterImpl.WriteFullEndElement();
		}

		public virtual void WriteElementString(string name, string value)
		{
			m_xmlWriterImpl.WriteElementString(name, value);
		}

		public virtual void WriteElementString(string prefix, string localName, string ns, string value)
		{
			m_xmlWriterImpl.WriteElementString(prefix, localName, ns, value);
		}

		public virtual void WriteElementString(string name, bool value)
		{
			m_xmlWriterImpl.WriteElementString(name, value.ToString().ToLowerInvariant());
		}

		public virtual void WriteString(string text)
		{
			m_xmlWriterImpl.WriteString(text);
		}

		public virtual void WriteStartDocument()
		{
			m_xmlWriterImpl.WriteStartDocument();
		}

		public void Close()
		{
			m_xmlWriterImpl.Close();
		}

		public new void Flush()
		{
			m_xmlWriterImpl.Flush();
			base.Flush();
		}
		protected override void Dispose(bool disposing)
		{
			m_xmlWriterImpl.Dispose();
			base.Dispose(disposing);
		}
	}
}