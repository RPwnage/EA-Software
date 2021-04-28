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
using System.Linq;

using NAnt.Perforce.Internal;
using NAnt.Perforce.Extensions.String;
using NAnt.Perforce.Extensions.Dictionary;

namespace NAnt.Perforce
{
	public partial class P4Client : P4ClientStub
	{
		public enum SubmitOptionSet
		{
			submitunchanged,
			submitunchanged_reopen,
			revertunchanged,
			revertunchanged_reopen,
			leaveunchanged,
			leaveunchanged_reopen
		}

		public enum LineEndOption
		{
			local,
			unix,
			mac,
			win,
			share
		}

		public enum ClientTypes
		{
			clienttype_writeable,
			clienttype_readonly
		}

		public const LineEndOption DefaultLineEnd = LineEndOption.local;
		public const SubmitOptionSet DefaultSubmitOptions = SubmitOptionSet.submitunchanged;
		public const ClientTypes DefaultClientType = ClientTypes.clienttype_writeable;

		public readonly static ClientOptions DefaultOptions = new ClientOptions();

		public sealed override string Name
		{
			get { return base.Name; }
			set
			{
				Dictionary<string, string> viewMap = GetViewMap(); // cache view as viewmap to filter old name
				ClientRecord.SetField("client", value);
				SetViewMap(viewMap); // rebuild view from viewmap using new name
			}
		}

		public sealed override string Owner
		{
			get { return base.Owner; }
			set { ClientRecord.SetField("owner", value); }
		}

		public sealed override ClientOptions Options
		{
			get { return base.Options; }
			set { _Options = new ClientOptions(this, value); }
		}

		public sealed override LineEndOption LineEnd
		{
			get { return base.LineEnd; }
			set { ClientRecord.SetField("lineend", value.ToString()); }
		}

		public sealed override SubmitOptionSet SubmitOptions
		{
			get { return base.SubmitOptions; }
			set { ClientRecord.SetField("submitoptions", value.ToString().Replace("_", "+")); }
		}

		public sealed override string Root
		{
			get { return base.Root; }
			set { ClientRecord.SetField("root", value); }
		}

		public sealed override string Host
		{
			get { return base.Host; }
			set { ClientRecord.SetField("host", value); }
		}

		public sealed override string Stream
		{
			get { return base.Stream; }
			set { ClientRecord.SetField("stream", value); }
		}

		public sealed override ClientTypes ClientType
		{
			get { return base.ClientType; }
			set { ClientRecord.SetField("type", value.ToString().Replace("clienttype_", "")); }
		}

		internal P4Client(P4Server server, string name, string root, Dictionary<string, string> viewMap, ClientOptions options = null, LineEndOption lineEnd = DefaultLineEnd, SubmitOptionSet submitOptions = DefaultSubmitOptions, ClientTypes clientType = DefaultClientType)
			: base(server, name, root, options ?? DefaultOptions, lineEnd, submitOptions, clientType)
		{
			SetViewMap(viewMap);
		}

		internal P4Client(P4Server server, string name, string root, string stream, ClientOptions options = null, LineEndOption lineEnd = DefaultLineEnd, SubmitOptionSet submitOptions = DefaultSubmitOptions, ClientTypes clientType = DefaultClientType)
			: base(server, name, root, options ?? DefaultOptions, lineEnd, submitOptions, clientType, stream)
		{
		}

        internal P4Client(P4Server server, P4Record p4Record)
            : base(server, p4Record)
        {
        }

		public void SetViewMap(Dictionary<string, string> viewMap)
		{
			// null safety
			viewMap = viewMap ?? new Dictionary<string, string>();

			// sort roots - move parent folders before sub folders
			List<string> sortedKeysList = new List<string>();
			foreach (string root in viewMap.Keys)
			{
				bool insert = true;
				for (int i = 0; i < sortedKeysList.Count; ++i)
				{
					string sortedRoot = sortedKeysList[i];
					if (P4Utils.SubLocationInLocation(sortedRoot, root, Server.CaseSensitive))
					{
						// we have to insert this before because it is a parent directory of one our sorted roots
						sortedKeysList.Insert(i, root);
						insert = false;
						break;
					}
				}

				// if we didn't insert it, then just add it at the end
				if (insert)
				{
					sortedKeysList.Add(root);
				}
			}

			List<string> mappings = new List<string>();
			foreach (string key in sortedKeysList)
			{
				if (!viewMap[key].StartsWith("//"))
				{
					throw new P4Exception(String.Format("View maps values must start with '//'. Cannot map '{0}'!", viewMap[key]));
				}
				mappings.Add(String.Format("{0} {1}", key.Quoted(), P4Utils.RootedPath(Name, viewMap[key]).Quoted()));
			}
			ClientRecord.SetArrayField("view", mappings.ToArray());
		}

		public void UpdateViewMap(Dictionary<string, string> viewMap)
		{
			if (GetServer().CaseSensitive)
			{
				SetViewMap(GetViewMap().Update(viewMap));
			}
			else
			{
				SetViewMap(CaseInsensitiveUpdate(GetViewMap(), viewMap));
			}
		}

		public Dictionary<string, string> GetViewMap()
		{
			Dictionary<string, string> viewMap = new Dictionary<string, string>();
			if (ClientRecord.HasArrayField("view"))
			{
				foreach (string view in ClientRecord.GetArrayField("view"))
				{
					string[] split = view.Split(new char[] { ' ' }, 2);
					viewMap[split[0]] = split[1].Replace(Name + "/", "");
				}
			}
			return viewMap;
		}

		public void Save()
		{
			Server.EnsureLoggedIn();

			string[] fieldNameExclusions = new string[] {
				"extratag",
				"extratagtype",
				"writable"
			};

			P4Caller.Run(Server, "client", new string[] { "-i" }, input: ClientRecord.AsForm(fieldNameExclusions), timeoutMillisec: P4GlobalOptions.DefaultTimeoutMs);
			ClientRecord = P4Caller.Run(Server, "client", new string[] { "-o", Name }, timeoutMillisec: P4GlobalOptions.DefaultTimeoutMs).Records.First(); // read back client to get modified date, etc
			_Options = new ClientOptions(this); // options isn't automatically tied to record contents but reads from in during constructor, so update here
		}

		private Dictionary<string, string> CaseInsensitiveUpdate(Dictionary<string, string> oldViewMap, Dictionary<string, string> update)
		{
			Dictionary<string, string> updatedMap = new Dictionary<string, string>(oldViewMap);
			foreach (KeyValuePair<string, string> kvp in update)
			{
				foreach (string duplicateKey in updatedMap.Keys.Where(k => k.Equals(kvp.Key, StringComparison.InvariantCultureIgnoreCase)).ToArray())
				{
					updatedMap.Remove(duplicateKey);
				}
				updatedMap[kvp.Key] = kvp.Value;
			}
			return updatedMap;
		}
	}
}
