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

using System;

namespace NAnt.Core
{
	public class ScopedElement : IDisposable
	{
		readonly IXmlWriter writer;

		public ScopedElement(IXmlWriter writer, string elementName)
		{
			this.writer = writer;
			writer.WriteStartElement(elementName);
		}

		public ScopedElement(IXmlWriter writer, string elementName, string nameSpace)
		{
			this.writer = writer;
			writer.WriteStartElement(elementName, nameSpace);
		}

		public ScopedElement Attribute(string attrName, string attrValue)
		{
			writer.WriteAttributeString(attrName, attrValue);
			return this;
		}

		public ScopedElement Attribute(string attrName, bool attrValue)
		{
			writer.WriteAttributeString(attrName, attrValue);
			return this;
		}

		// value is semantically different from the implementation "WriteString".
		// the API of ScopedElement allows for setting the 'value' of the XML node,
		// and we only allow it to be set once.
		public ScopedElement Value(string value)
		{
			if (valueWritten_ == false)
			{
				writer.WriteString(value);
			}
			else
			{
				throw new ExpressionException("XML Value already written");
			}

			valueWritten_ = true;
			return this;
		}

		public ScopedElement Value(bool value)
		{
			return Value(value.ToString().ToLowerInvariant());
		}

		public void Dispose()
		{
			writer.WriteEndElement();
		}

		private bool valueWritten_ = false;
	}

	public static class WriterExtensions
	{

		public static void WriteNonEmptyElementString(this IXmlWriter writer, string name, string value)
		{
			if (writer != null && !String.IsNullOrWhiteSpace(value))
			{
				writer.WriteElementString(name, value);
			}
		}

		public static void WriteNonEmptyAttributeString(this IXmlWriter writer, string localName, string value)
		{
			if (writer != null && !String.IsNullOrWhiteSpace(value))
			{
				writer.WriteAttributeString(localName, value);
			}
		}

		public static ScopedElement ScopedElement(this IXmlWriter writer, string elementName)
		{
			return new ScopedElement(writer, elementName);
		}

		public static ScopedElement ScopedElement(this IXmlWriter writer, string elementName, string nameSpace)
		{
			return new ScopedElement(writer, elementName, nameSpace);
		}
	}
}
