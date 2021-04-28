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
using System.IO;

namespace PowerCode.BuildTools
{
	public interface ILinkerMember
	{
		int[] Offsets { get; }
		int NumberOfSymbols { get; }
	}

	public class ArchiveFirstLinker : ILinkerMember
	{

		private readonly byte[] _buf;
		public ArchiveHeader Header { get; private set; }

		public ArchiveFirstLinker(Stream stream)
		{
			Header = new ArchiveHeader(stream);
			_buf = new byte[Header.Size];
			stream.Read(_buf, 0, _buf.Length);
			NumberOfSymbols = BitConverter.ToInt32(_buf, 0).SwapByteOrder();
			var offsets = new int[NumberOfSymbols];
			for (int i = 0; i < NumberOfSymbols; ++i)
			{
				offsets[i] = BitConverter.ToInt32(_buf, 4*i + 4).SwapByteOrder();
			}
			Offsets = offsets;
		}

		public int[] Offsets { get; private set; }

		public int NumberOfSymbols { get; private set; }

		#region Comparer
		private sealed class BufHeaderEqualityComparer : IEqualityComparer<ArchiveFirstLinker>
		{
			public bool Equals(ArchiveFirstLinker x, ArchiveFirstLinker y)
			{
				if (ReferenceEquals(x, y)) return true;
				if (ReferenceEquals(x, null)) return false;
				if (ReferenceEquals(y, null)) return false;
				if (x.GetType() != y.GetType()) return false;
				return ArchiveHeader.TimestampInsensitiveComparer.Equals(x.Header, y.Header) && x._buf.ArrayEquals(y._buf);
			}

			public int GetHashCode(ArchiveFirstLinker obj)
			{
				unchecked
				{
					return ((obj._buf != null ? obj._buf.GetHashCode() : 0) * 397) ^ (obj.Header != null ? obj.Header.GetHashCode() : 0);
				}
			}
		}

		public static IEqualityComparer<ArchiveFirstLinker> TimeInsensitiveComparer { get; } = new BufHeaderEqualityComparer();
		#endregion

	}
}