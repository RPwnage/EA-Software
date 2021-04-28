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

using NAnt.Perforce.Internal;

namespace NAnt.Perforce
{
	public partial class P4Stream
	{
		public enum StreamType
		{
			mainlinetype, 
			virtualtype, 
			releasetype, 
			developmenttype, 
			tasktype
		}

		public const string NoParent = "none";

		private P4Record _StreamRecord = null;

        public DateTime Access { get { return _StreamRecord.GetDateField("access"); } }
        public DateTime Update { get { return _StreamRecord.GetDateField("update"); } }
		public P4Server Server { get; private set; }

		public string Name
		{
			get { return _StreamRecord.GetField("name"); }
			private set { _StreamRecord.SetField("name", value); }
		}

		public string Stream
		{
			get { return _StreamRecord.GetField("stream"); }
			private set { _StreamRecord.SetField("stream", value); }
		}

		public string Owner
		{
			get { return _StreamRecord.GetField("owner"); }
			set { _StreamRecord.SetField("owner", value); }
		}

		public string Description
		{
			get { return _StreamRecord.GetField("desc", null); }
			set { _StreamRecord.SetField("desc", value); }
		}

		public string Parent
		{
			get { return _StreamRecord.GetField("parent"); }
			set { _StreamRecord.SetField("parent", value); }
		}

		public StreamType Type 
		{ 
			get { return (StreamType)Enum.Parse(typeof(StreamType), _StreamRecord.GetField("type") + "type"); } 
			set { _StreamRecord.SetField("type", value.ToString().Substring(0, value.ToString().Length - 4)); } 
		}

		internal P4Stream(P4Server server, string stream, string descriptiom, string displayName, string parent, StreamType type, params Path[] paths)
		{
			_StreamRecord = new P4Record();

			Server = server;
			Stream = stream;
			Description = descriptiom;
			Name = displayName;
			Parent = parent;
			Type = type;
			Owner = server.User;

			// default value
			if (paths == null || paths.Length == 0)
			{
				paths = new Path[] { new Path(Path.PathType.share, "...") };
			}

			SetPaths(paths);
		}

		internal P4Stream(P4Server server, P4Record p4Record)
		{
			Server = server;
			_StreamRecord = p4Record;
		}

		public Path[] GetPaths()
		{
			return _StreamRecord.GetArrayField("paths").Select(p => Path.Parse(p)).ToArray();
		}

		public void SetPaths(Path[] paths)
		{
			_StreamRecord.SetArrayField("paths", paths.Select(p => p.ToString()).ToArray());
		}

		public void Save()
		{
			Server.EnsureLoggedIn();
			P4Caller.Run(Server, "stream", new string[] { "-i" }, input: _StreamRecord.AsForm(), timeoutMillisec: P4GlobalOptions.DefaultTimeoutMs);
			_StreamRecord = P4Caller.Run(Server, "stream", new string[] { "-o", Stream }, timeoutMillisec: P4GlobalOptions.DefaultTimeoutMs).Records.First(); // read back stream to get modified dates, etc
		}
	}
}
