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
using System.IO.MemoryMappedFiles;
using System.Linq;
using System.Text;

namespace PowerCode.BuildTools
{
	/// <summary>
	/// See "Microsoft PE and COFF Specification", last found at http://msdn.microsoft.com/en-us/library/windows/hardware/gg463119.aspx
	/// for a description of the file format
	/// </summary>
	public class Archive : IDisposable
	{
		private sealed class ArchiveTimestampInsensitiveEqualityComparer : IEqualityComparer<Archive>
		{
			public bool Equals(Archive x, Archive y)
			{
				if (ReferenceEquals(x, y)) return true;
				if (ReferenceEquals(x, null)) return false;
				if (ReferenceEquals(y, null)) return false;
				if (x.GetType() != y.GetType()) return false;
				var firstLinkerEquals = ArchiveFirstLinker.TimeInsensitiveComparer.Equals(x.FirstLinkerMember, y.FirstLinkerMember);
				var secondLinkerEquals = ArchiveSecondLinker.TimestampInsensitiveComparer.Equals(x.SecondLinkerMember, y.SecondLinkerMember);
				var sequenceEqual = x.Members.SequenceEqual(y.Members, ArchiveMember.ArchiveMemberComparer);
				return firstLinkerEquals && secondLinkerEquals && sequenceEqual;
			}

			public int GetHashCode(Archive obj)
			{
				unchecked
				{
					return ((obj.Members != null ? obj.Members.GetHashCode() : 0)*397) ^ (obj.FirstLinkerMember != null ? obj.FirstLinkerMember.GetHashCode() : 0);
				}
			}
		}

		private readonly Stream _stream;
		private readonly MemoryMappedFile _mappedFile;
		private bool _disposed;

		private List<ArchiveMember> _members;
		public static IEqualityComparer<Archive> ArchiveTimestampInsensitiveComparer { get; } = new ArchiveTimestampInsensitiveEqualityComparer();

		public Archive(string filename)
		{
			FileStream fs = new FileStream(filename, FileMode.Open, FileAccess.Read, FileShare.Read);
#if NETFRAMEWORK
			_mappedFile = MemoryMappedFile.CreateFromFile(fs, null, 0, MemoryMappedFileAccess.Read, null, HandleInheritability.None, false);
#else
			_mappedFile = MemoryMappedFile.CreateFromFile(fs, null, 0, MemoryMappedFileAccess.Read, HandleInheritability.None, false);
#endif
			_stream = _mappedFile.CreateViewStream(0, 0, MemoryMappedFileAccess.Read);
			
			ReadFromStream(_stream);
		}

		internal void ReadFromStream(Stream stream)
		{
			var buffer = new byte[8];
			stream.Read(buffer, 0, 8);
			var signature = Encoding.ASCII.GetString(buffer, 0, 8);
			if (signature != "!<arch>\n")
			{
				throw new ArgumentException("Invalid file format.");
			}
			FirstLinkerMember = new ArchiveFirstLinker(stream);
			if (stream.Position % 2 != 0)
			{
				stream.ReadByte();    
			}
			
			var peek = stream.ReadByte();
			stream.Position--;
			if (peek == 0x2F)
			{
				SecondLinkerMember = new ArchiveSecondLinker(stream);    
			}
			_members = new List<ArchiveMember>(SecondLinkerMember.NumberOfMembers);            
			_members.AddRange(GetMembers(_stream, FirstLinkerMember));
		}

		public IEnumerable<ArchiveMember> Members
		{
			get
			{
				return _members;
			}
		}

		static IEnumerable<ArchiveMember> GetMembers(Stream stream, ILinkerMember firstLinker)
		{
			var reader = new BinaryReader(stream, Encoding.ASCII, true);
			var numberOfSymbols = firstLinker.Offsets.Length;

			for (int i = 0; i < numberOfSymbols; i++)
			{
				var offset = firstLinker.Offsets[i];
				stream.Position = offset;
				yield return new ArchiveMember(reader);                  
			}
			
		} 
		public ArchiveSecondLinker SecondLinkerMember { get; private set; }
		public ArchiveFirstLinker FirstLinkerMember { get; set; }
		public void Dispose()
		{
			Dispose(true);
			GC.SuppressFinalize(this);
		}

		protected virtual void Dispose(bool disposing)
		{
			if (_disposed)
			{
				return;
			}

			if (disposing)
			{
				_stream.Dispose();
				_mappedFile.Dispose();
			}

			_disposed = true;
		}
	}
}
