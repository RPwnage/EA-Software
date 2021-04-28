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
using System.Linq;

using NAnt.Perforce.Internal;

namespace NAnt.Perforce
{
	public class P4Depot
	{
		public enum DepotType
		{
			local, 
			remote, 
			spec, 
			stream, 
			unload,
			archive
		}

		private P4Record _DepotRecord = null;

		public P4Server Server { get; private set; }

		public string Name
		{
			get { return _DepotRecord.GetField("depot"); }
			private set { _DepotRecord.SetField("depot", value); }
		}

		public string Description
		{
			get { return _DepotRecord.GetField("desc", null); }
			set { _DepotRecord.SetField("desc", value); }
		}

		public string Map
		{
			get { return _DepotRecord.GetField("map"); }
			set { _DepotRecord.SetField("map", value); }
		}

		public DepotType Type
		{
			get { return _DepotRecord.GetEnumField<DepotType>("type"); }
			set { _DepotRecord.SetField("type", value.ToString()); }
		}

		public string Owner
		{
			get { return _DepotRecord.GetField("owner", null); } // depots don't necessarily have an owener so allow default null
			set { _DepotRecord.SetField("owner", value); }
		}

        public uint StreamDepth
        {
            get { return _DepotRecord.GetUint32Field("streamdepth", 1); } // depots don't necessarily have an owener so allow default null
            set { _DepotRecord.SetField("streamdepth", value.ToString()); }
        }

		internal P4Depot(P4Server server, string name, string descriptiom, string map, DepotType type = DepotType.local, uint streamDepth = 1)
		{
			_DepotRecord = new P4Record();

			Server = server;
			Name = name;
			Description = descriptiom;
			Map = map;
			Type = type;
			Owner = server.User;

            if (Type == DepotType.stream)
            {
                StreamDepth = 1;
            }
		}

		internal P4Depot(P4Server server, P4Record p4Record)
		{
			Server = server;
			_DepotRecord = p4Record;
			if (_DepotRecord.HasField("name"))
			{
				_DepotRecord.SetField("depot", _DepotRecord.GetField("name")); // depots returns "name", depot returns "depot", le sigh, we internally convert to always use "depot" for 
				_DepotRecord.SetField("name", null);						   // consistency (because this is what form will expect if we use .Save())
			}
		}

		public void Save()
		{
			Server.EnsureLoggedIn();

            List<string> formExclusions = new List<string>();
            formExclusions.Add("time"); // will error if we try and save this field //TODO: not strictly true - it won't let us save *time* field, but time is actually a bastardized *date* field (timestamp rather than date format)
            if (Type != DepotType.stream)
            {
                formExclusions.Add("streamdepth"); // only valid for stream depots
            }
            P4Caller.Run
            (
                Server, 
                "depot",
                args: new string[] { "-i" }, 
                input: _DepotRecord.AsForm(formExclusions.ToArray()), 
                timeoutMillisec: P4GlobalOptions.DefaultTimeoutMs
            );

			_DepotRecord = P4Caller.Run(Server, "depot", new string[] { "-o", Name }, timeoutMillisec: P4GlobalOptions.DefaultTimeoutMs).Records.First(); // read back depot to get modified dates, etc
		}
	}
}