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

using System.Collections.Generic;
using System.IO;

namespace PowerCode.BuildTools
{
	public class ArchiveMember
	{
		private readonly byte[] _buf;
		public ArchiveHeader Header { get; private set; }
		public FileHeader FileHeader { get; private set; }
		public ImportHeader ImportHeader { get; private set; }
		

		public ArchiveMember(BinaryReader reader)
		{
			 _offset = reader.BaseStream.Position;
			Header = new ArchiveHeader(reader.BaseStream);
			var peek = reader.ReadUInt16();
			reader.BaseStream.Position -= 2;
			if (peek == 0)
			{
				ImportHeader = new ImportHeader(reader);
			}
			else {
				FileHeader = new FileHeader(reader);
			}
			
			//ImportHeader = new MemberHeader(reader);
			_buf = reader.ReadBytes(Header.Size - 20); // sizeof FileHeader || sizeof ImportHeader
		}        

		private sealed class ArchiveMemberTimeInsensitiveEqualityComparer : IEqualityComparer<ArchiveMember>
		{
			public bool Equals(ArchiveMember x, ArchiveMember y)
			{
				if (ReferenceEquals(x, y)) return true;
				if (ReferenceEquals(x, null)) return false;
				if (ReferenceEquals(y, null)) return false;
				if (x.GetType() != y.GetType()) return false;
				var archiveHeaderEquals = ArchiveHeader.TimestampInsensitiveComparer.Equals(x.Header, y.Header);
				var fileHeaderEquals = FileHeader.TimestampInsensitiveFileHeaderComparer.Equals(x.FileHeader, y.FileHeader);
				var importHeaderEquals = ImportHeader.TimestampInsensitiveComparer.Equals(x.ImportHeader, y.ImportHeader);
				var arrayEquals = x._buf.ArrayEquals(y._buf);
				var retval = archiveHeaderEquals && fileHeaderEquals && importHeaderEquals && arrayEquals;
				return retval;
			}

			public int GetHashCode(ArchiveMember obj)
			{
				unchecked
				{
					return ((obj._buf != null ? obj._buf.GetHashCode() : 0) * 397) ^ (obj.FileHeader != null ? obj.FileHeader.GetHashCode() : 0);
				}
			}
		}

		private long _offset;

		public static IEqualityComparer<ArchiveMember> ArchiveMemberComparer { get; } = new ArchiveMemberTimeInsensitiveEqualityComparer();
	}

	public class MemberHeader
	{
		private readonly long _preAmple;        
		private readonly int _postAmble;        

		public MemberHeader(BinaryReader reader)
		{
			_preAmple = reader.ReadInt64();
			Timestamp = reader.ReadInt32();
			Size = reader.ReadInt32();
			_postAmble = reader.ReadInt32();            
		}        

		public int Size { get; private set; }
		public int Timestamp { get; private set; }

		private sealed class TimestampInsensitiveEqualityComparer : IEqualityComparer<MemberHeader>
		{
			public bool Equals(MemberHeader x, MemberHeader y)
			{
				if (ReferenceEquals(x, y)) return true;
				if (ReferenceEquals(x, null)) return false;
				if (ReferenceEquals(y, null)) return false;
				if (x.GetType() != y.GetType()) return false;
				return x._preAmple == y._preAmple && x._postAmble == y._postAmble && x.Size == y.Size;
			}

			public int GetHashCode(MemberHeader obj)
			{
				unchecked
				{
					var hashCode = obj._preAmple.GetHashCode();
					hashCode = (hashCode*397) ^ obj._postAmble;
					hashCode = (hashCode*397) ^ obj.Size;
					return hashCode;
				}
			}
		}

		public static IEqualityComparer<MemberHeader> TimeInsensitiveComparer { get; } = new TimestampInsensitiveEqualityComparer();
	}
}
