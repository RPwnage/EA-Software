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

	public class ImportHeader
	{
		
		public ImportHeader(BinaryReader reader)
		{
			Signature = reader.ReadUInt32();
			VersionMachine = reader.ReadUInt32();
			Timestamp = reader.ReadUInt32();
			Rest = reader.ReadUInt64();
		}

		private sealed class RestSignatureVersionMachineEqualityComparer : IEqualityComparer<ImportHeader>
		{
			public bool Equals(ImportHeader x, ImportHeader y)
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
				return x.Rest == y.Rest && x.Signature == y.Signature && x.VersionMachine == y.VersionMachine;
			}

			public int GetHashCode(ImportHeader obj)
			{
				unchecked
				{
					var hashCode = obj.Rest.GetHashCode();
					hashCode = (hashCode * 397) ^ (int)obj.Signature;
					hashCode = (hashCode * 397) ^ (int)obj.VersionMachine;
					return hashCode;
				}
			}
		}

		public static IEqualityComparer<ImportHeader> TimestampInsensitiveComparer { get; } = new RestSignatureVersionMachineEqualityComparer();
		public ulong Rest { get; private set; }

		public uint Signature { get; private set; }

		public uint VersionMachine { get; private set; }

		public uint Timestamp { get; private set; }


	}
}