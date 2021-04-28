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
using System.Linq;

namespace NAnt.Perforce
{
	// this class breaks the whole "read and write record in place" approach used everywhere else however this seems worth it in order
	// to allow free standing options objects i.e:
	//
	// 
	// P4ClientStub.ClientOptions options = new P4ClientStub.ClientOptions(allwrite: true, modTime: true); // free standing options class
	// foreach (P4Client client in GetMyClients())
	// {
	//     client.Options = options;
	//     client.Save();
	// }

	public partial class P4Client : P4ClientStub
	{
		public class ClientOptions
		{
			private static readonly string[] TrueValues = { "allwrite", "clobber", "compress", "locked", "modtime", "rmdir" };
			private static readonly string[] FalseValues = { "noallwrite", "noclobber", "nocompress", "unlocked", "nomodtime", "normdir" };

			private readonly P4ClientStub _Client = null;
			private readonly bool[] _Options = { false, true, false, false, false, true };

			public bool AllWrite
			{
				get { return _Options[0]; }
				set { _Options[0] = value; SetClientOptions(); }
			}

			public bool Clobber
			{
				get { return _Options[1]; }
				set { _Options[1] = value; SetClientOptions(); }
			}

			public bool Compress
			{
				get { return _Options[2]; }
				set { _Options[2] = value; SetClientOptions(); }
			}

			public bool Locked
			{
				get { return _Options[3]; }
				set { _Options[3] = value; SetClientOptions(); }
			}

			public bool ModTime
			{
				get { return _Options[4]; }
				set { _Options[4] = value; SetClientOptions(); }
			}

			public bool RmDir
			{
				get { return _Options[5]; }
				set { _Options[5] = value; SetClientOptions(); }
			}

			public ClientOptions(bool allwrite = false, bool clobber = true, bool compress = false, bool locked = false, bool modTime = false, bool rmdir = true)
			{
				_Options[0] = allwrite;
				_Options[1] = clobber;
				_Options[2] = compress;
				_Options[3] = locked;
				_Options[4] = modTime;
				_Options[5] = rmdir;
			}

			internal ClientOptions(P4ClientStub client) 
			{
				_Client = client;
				ParseOptionsString(_Client.ClientRecord.GetField("options").ToLowerInvariant());
			}

			internal ClientOptions(P4ClientStub client, ClientOptions options)
			{
				_Client = client;
				for (int i = 0; i < _Options.Length; ++i)
				{
					_Options[i] = options._Options[i];
				}
				SetClientOptions();
			}

			private string ToOptionsString()
			{
				return String.Join(" ", (from index in Enumerable.Range(0, _Options.Length) select _Options[index] ? TrueValues[index] : FalseValues[index]));
			}

			private void ParseOptionsString(string optionsString)
			{
				string[] fragments = optionsString.Split(' ');
				for (int i = 0; i < fragments.Length; ++i)
				{
					if (fragments[i] == TrueValues[i])
					{
						_Options[i] = true;
					}
					else if (fragments[i] == FalseValues[i])
					{
						_Options[i] = false;
					}
					else
					{
						throw new ClientOptionsParsingException(optionsString);
					}
				}
			}

			private void SetClientOptions()
			{
				if (_Client != null)
				{
					_Client.ClientRecord.SetField("options", ToOptionsString());
				}
			}
		}
	}
}
