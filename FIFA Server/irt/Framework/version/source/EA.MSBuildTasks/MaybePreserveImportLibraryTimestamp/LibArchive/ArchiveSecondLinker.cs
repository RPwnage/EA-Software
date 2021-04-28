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
using System.IO;

namespace PowerCode.BuildTools
{
	using System.Collections.Generic;
	using System.Linq;

	public class ArchiveSecondLinker : ILinkerMember
	{
		private readonly byte[] _buf;
		public ArchiveHeader Header { get; private set; }

		public ArchiveSecondLinker(Stream stream)
		{
			Header = new ArchiveHeader(stream);
			_buf = new byte[Header.Size - ArchiveHeader.StructSize];
			stream.Read(_buf, 0, _buf.Length);

			NumberOfMembers = BitConverter.ToInt32(_buf, 0);
			int offset = NumberOfMembers*4 + 4;
			NumberOfSymbols = BitConverter.ToInt32(_buf, offset);

			var offsets = new int[NumberOfMembers];
			for (int i = 0; i < offsets.Length; ++i)
			{
				offsets[i] = BitConverter.ToInt32(_buf, 4 * i + 4);
			}
			Array.Sort(offsets);
			Offsets = offsets;

			var indices = new ushort[NumberOfSymbols];
			var indiciesStart = 4 + 4*NumberOfSymbols + 4;
			for (int i = 0; i < indices.Length; ++i)
			{
				indices[i] = BitConverter.ToUInt16(_buf, 2 * i + indiciesStart);
			}
			Indicies = indices;
			var stringTableStart = indiciesStart + 2*NumberOfSymbols;
			_stringTable = new byte[Header.Size - stringTableStart - ArchiveHeader.StructSize];
			Array.Copy(_buf, stringTableStart, _stringTable, 0, _stringTable.Length);
		}

		private sealed class ArchiveSecondLinkerEqualityComparer : IEqualityComparer<ArchiveSecondLinker>
		{
			public bool Equals(ArchiveSecondLinker x, ArchiveSecondLinker y)
			{
				if (ReferenceEquals(x, y))
				{
					return true;
				}
				if (ReferenceEquals(x, null))
				{
					return false;
				}
				if (ReferenceEquals(y, null))
				{
					return false;
				}
				if (x.GetType() != y.GetType())
				{
					return false;
				}
				return x.NumberOfMembers == y.NumberOfMembers
					   && x.NumberOfSymbols == y.NumberOfSymbols && x.Indicies.SequenceEqual(y.Indicies)
					   && x.Offsets.SequenceEqual(y.Offsets)
					   && ArchiveHeader.TimestampInsensitiveComparer.Equals(x.Header, y.Header)
					   && x._stringTable.SequenceEqual(y._stringTable);
			}

			public int GetHashCode(ArchiveSecondLinker obj)
			{
				unchecked
				{
					var hashCode = (obj._buf != null ? obj._buf.GetHashCode() : 0);
					hashCode = (hashCode * 397) ^ obj.NumberOfMembers;
					hashCode = (hashCode * 397) ^ obj.NumberOfSymbols;
					hashCode = (hashCode * 397) ^ (obj.Indicies != null ? obj.Indicies.GetHashCode() : 0);
					hashCode = (hashCode * 397) ^ (obj.Offsets != null ? obj.Offsets.GetHashCode() : 0);
					hashCode = (hashCode * 397) ^ (obj.Header != null ? obj.Header.GetHashCode() : 0);
					hashCode = (hashCode * 397) ^ (obj._stringTable != null ? obj._stringTable.GetHashCode() : 0);
					return hashCode;
				}
			}
		}

		public static IEqualityComparer<ArchiveSecondLinker> TimestampInsensitiveComparer { get; } = new ArchiveSecondLinkerEqualityComparer();

		private byte[] _stringTable;

		public int[] Offsets { get; private set; }
		public ushort[] Indicies { get; private set; }
		public int NumberOfSymbols { get; private set; }
		public int NumberOfMembers { get; private set; }
	}
}