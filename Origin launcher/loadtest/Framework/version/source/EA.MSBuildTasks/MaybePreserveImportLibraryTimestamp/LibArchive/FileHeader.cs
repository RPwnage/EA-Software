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

namespace PowerCode.BuildTools
{
	using System.Collections.Generic;
	using System.IO;

	public sealed class FileHeader
	{
		public ushort Machine { get; private set; }
		public ushort NumberOfSections { get; private set; }
		public uint Timestamp { get; private set; }
		public uint PointerToSymbolTable { get; private set; }
		public ulong Rest { get; private set; }
		

		public FileHeader(BinaryReader reader)
		{
			Machine = reader.ReadUInt16();
			NumberOfSections = reader.ReadUInt16();
			Timestamp = reader.ReadUInt32();
			PointerToSymbolTable = reader.ReadUInt32();
			Rest = reader.ReadUInt64();
		}

		private sealed class FileHeaderEqualityComparer : IEqualityComparer<FileHeader>
		{
			public bool Equals(FileHeader x, FileHeader y)
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
				return x.Machine == y.Machine && x.NumberOfSections == y.NumberOfSections && x.PointerToSymbolTable == y.PointerToSymbolTable && x.Rest == y.Rest;
			}

			public int GetHashCode(FileHeader obj)
			{
				unchecked
				{
					var hashCode = obj.Machine.GetHashCode();
					hashCode = (hashCode * 397) ^ obj.NumberOfSections.GetHashCode();
					hashCode = (hashCode * 397) ^ (int)obj.PointerToSymbolTable;
					hashCode = (hashCode * 397) ^ obj.Rest.GetHashCode();
					return hashCode;
				}
			}
		}

		public static IEqualityComparer<FileHeader> TimestampInsensitiveFileHeaderComparer { get; } = new FileHeaderEqualityComparer();
	}
}