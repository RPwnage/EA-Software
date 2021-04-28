using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace NAnt.Core
{
	public interface IXmlWriter : ICachedWriter
	{
		void WriteStartElement(string localName);

		void WriteStartElement(string localName, string ns);

		void WriteAttributeString(string localName, string value);

		void WriteAttributeString(string prefix, string localName, string ns, string value);

		void WriteAttributeString(string localName, bool value);

		void WriteEndElement();

		void WriteFullEndElement();

		void WriteElementString(string localName, string value);

		void WriteElementString(string prefix, string localName, string ns, string value);

		void WriteElementString(string localName, bool value);

		void WriteString(string text);

		void WriteStartDocument();
	}
}
