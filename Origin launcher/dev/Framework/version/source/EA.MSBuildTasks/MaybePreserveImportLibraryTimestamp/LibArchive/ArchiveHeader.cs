// Originally based on NAnt - A .NET build tool
// Copyright (C) 2003-2018 Electronic Arts Inc.
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
// Electronic Arts (Frostbite.Team.CM@ea.com)

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Text;

namespace PowerCode.BuildTools
{
	public class ArchiveHeader
	{        
		private readonly string _mode;
		private readonly string _group;
		private readonly string _user;

		public const int StructSize = 60;

		public ArchiveHeader(Stream stream)
		{
			if (stream.Position % 2 != 0)
			{
				stream.ReadByte();
			}
			var buffer = new byte[StructSize];
			stream.Read(buffer, 0, StructSize);

			var header = Encoding.ASCII.GetString(buffer, 0, StructSize);

			Name = header.Substring(0, 16).TrimEnd();   
			Date = Int32.Parse(header.Substring(16, 12));
			_user = header.Substring(28, 6);
			_group = header.Substring(34, 6);
			_mode = header.Substring(40, 8);
			Size = Int32.Parse(header.Substring(48, 10).TrimEnd());
			var end = header.Substring(58, 2);
			Debug.Assert(end == "`\n");

		}

		private sealed class ArchiveHeaderEqualityComparer : IEqualityComparer<ArchiveHeader>
		{
			public bool Equals(ArchiveHeader x, ArchiveHeader y)
			{
				if (ReferenceEquals(x, y)) return true;
				if (ReferenceEquals(x, null)) return false;
				if (ReferenceEquals(y, null)) return false;
				if (x.GetType() != y.GetType()) return false;
				return string.Equals(x.Name, y.Name) && x.Size == y.Size && string.Equals(x._mode, y._mode) && string.Equals(x._group, y._group) && string.Equals(x._user, y._user);
			}

			public int GetHashCode(ArchiveHeader obj)
			{
				unchecked
				{
					var hashCode = (obj._mode != null ? obj._mode.GetHashCode() : 0);
					hashCode = (hashCode*397) ^ (obj._group != null ? obj._group.GetHashCode() : 0);
					hashCode = (hashCode*397) ^ (obj._user != null ? obj._user.GetHashCode() : 0);
					hashCode = (hashCode*397) ^ (obj.Name != null ? obj.Name.GetHashCode() : 0);
					hashCode = (hashCode*397) ^ obj.Size;
					return hashCode;
				}
			}
		}

		public static IEqualityComparer<ArchiveHeader> TimestampInsensitiveComparer { get; } = new ArchiveHeaderEqualityComparer();
		public string Name { get; internal set; }
		public int Size { get; private set; }

		public int Date { get; internal set; }

	}
}