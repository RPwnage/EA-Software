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
		public static Location GetLocationFromNode(XmlNode node)
		{
			if (node is IXmlLineInfo lineInfo && lineInfo.HasLineInfo())
			{
				return new Location(node.OwnerDocument.BaseURI, lineInfo.LineNumber, lineInfo.LinePosition);
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
					sb.Append('(');
					sb.Append(LineNumber);
					sb.Append(',');
					sb.Append(ColumnNumber);
					sb.Append(')');
				}
				sb.Append(":");
			}

			return sb.ToString();
		}

		public override bool Equals(object obj)
		{
			if (obj is Location otherLoc)
			{
				return _fileName == otherLoc._fileName &&
					LineNumber == otherLoc.LineNumber &&
					ColumnNumber == otherLoc.ColumnNumber;
			}

			return base.Equals(obj);
		}

		public override int GetHashCode()
		{
			int hash = 17;
			hash = hash * 31 + _fileName.GetHashCode();
			hash = hash * 31 + LineNumber.GetHashCode();
			hash = hash * 31 + ColumnNumber.GetHashCode();
			return hash;
		}
	}
}
