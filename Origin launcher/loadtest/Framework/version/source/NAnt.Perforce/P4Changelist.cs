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
using System.Diagnostics;
using System.Linq;
using System.Text.RegularExpressions;
using NAnt.Perforce.Internal;

namespace NAnt.Perforce
{
	public class P4Changelist
	{
		public enum CLStatus
		{
			submitted,
			pending,
			shelved
		}

		private P4Record _ChangelistRecord = null;
		private P4Context _Context = null;

		public uint Changelist { get { return _ChangelistRecord.GetUint32Field("change"); } }
		public CLStatus Status { get { return _ChangelistRecord.GetEnumField<CLStatus>("status"); } }
		public DateTime Time { get { return _ChangelistRecord.GetDateField("time"); } }
		public string ChangeType { get { return _ChangelistRecord.GetField("changetype", null); } } // null -> can be unset in certain unsubmitted cases
		public string Client { get { return _ChangelistRecord.GetField("client"); } }
		public string Path { get { return _ChangelistRecord.GetField("path"); } }
		public string User { get { return _ChangelistRecord.GetField("user"); } }        
		
		public string Description  // changelist descriptions are handled inconsistently, sometimes "desc" sometimes "description"
								   // we make it easier for the user by hiding the internal key and correcting on export (see P4Record.AsForm)
		{
			get
			{
				if (_ChangelistRecord.HasField("desc"))
				{
					Debug.Assert(!_ChangelistRecord.HasField("description"));
					return _ChangelistRecord.GetField("desc");
				}
				else if (_ChangelistRecord.HasField("description"))
				{
					return _ChangelistRecord.GetField("description");
				}
				return null;
			}

			set 
			{
				if (_ChangelistRecord.HasField("desc"))
				{
					Debug.Assert(!_ChangelistRecord.HasField("description"));
					_ChangelistRecord.SetField("desc", value);
				}
				else
				{
					_ChangelistRecord.SetField("description", value);
				}
			}
		} 

		public P4ChangelistFile[] Files
		{
			get
			{
				return Describe();
			}
		}

		public P4ChangelistFile[] ShelvedFiles
		{
			get
			{
				return DescribeShelved();
			}
		}
		
		internal P4Changelist(P4Context context, string description, params P4Location[] files)
		{
			_Context = context;
			_ChangelistRecord = new P4Record();
			_ChangelistRecord.SetField("change", "new");
			Description = description;
			_ChangelistRecord.SetArrayField("files", (from P4Location loc in files select loc.GetSpecString()).ToArray());

		}

		internal P4Changelist(P4Context context, P4Record p4Record)
		{
			_Context = context;
			_ChangelistRecord = p4Record;
		}

		public P4EditFileCollection Edit(P4File.FileType fileType = null, bool throwOnNoSuchFiles = false, params P4Location[] locations)
		{
			P4ClientStub editContext = GetEditableContext();
			return editContext.Edit(Changelist.ToString(), fileType, throwOnNoSuchFiles, locations);
		}

		public P4RevertFileCollection Revert(bool throwOnNoFilesOpened = false, bool onlyUnchanged = false, bool deleteAddedFiles = false, params P4Location[] locations)
		{
			P4ClientStub revertContext = GetEditableContext();
			return revertContext.Revert(throwOnNoFilesOpened, onlyUnchanged, deleteAddedFiles, Changelist.ToString(), locations);
		}

		public P4ShelveFile[] Shelve(ShelveOptionSet option = P4Defaults.DefaultShelveOption, bool overrideExisting = false, params P4Location[] locations)
		{
			P4ClientStub shelveContext = GetEditableContext();
			return shelveContext.Shelve(Changelist, option, overrideExisting, locations);
		}

		// derives target client spec from changelist
		public P4UnshelveFileCollection UnshelveToChangelist(uint toChangelist, bool overrideWriteableFiles = false, bool throwOnNoSuchFiles = false, params P4Location[] locations)
		{
			P4Changelist targetChangelist = _Context.Change(toChangelist);
			if (targetChangelist.Status == CLStatus.submitted)
			{
				throw new CannotEditSubmittedChangelistException(toChangelist);
			}
			P4Client unshelveContext = _Context.GetServer().GetClient(targetChangelist.Client); // TODO using GetClient here is a bit much for an object we are going to dicsard at function exit a fake / quick client context might be better
			return unshelveContext.Unshelve(Changelist, toChangelist.ToString(), overrideWriteableFiles, throwOnNoSuchFiles, locations);
		}

		// unshelve to the default change list of a specifc client
		public P4UnshelveFileCollection UnshelveToClient(string clientName, bool overrideWriteableFiles = false, bool throwOnNoSuchFiles = false, params P4Location[] locations)
		{
			P4Client unshelveContext = _Context.GetServer().GetClient(clientName); // TODO using GetClient here is a bit much for an object we are going to dicsard at function exit a fake / quick client context might be better
			return unshelveContext.Unshelve(Changelist, null, overrideWriteableFiles, throwOnNoSuchFiles, locations);
		}

		// unshelve to the default change list of a specifc client
		public P4UnshelveFileCollection UnshelveToClient(P4ClientStub toClient, bool overrideWriteableFiles = false, bool throwOnNoSuchFiles = false, params P4Location[] locations)
		{
			return toClient.Unshelve(Changelist, null, overrideWriteableFiles, throwOnNoSuchFiles, locations);
		}

		public void DeleteShelvedFiles(bool throwOnNoShelvedFiles = false, bool throwOnNoSuchFiles = false, params P4Location[] locations)
		{
			P4ClientStub deleteContext = GetEditableContext();
			deleteContext.DeleteShelvedFiles(Changelist, throwOnNoShelvedFiles, throwOnNoSuchFiles, locations);
		}

		public void Delete()
		{
			_Context.DeleteChangelist(Changelist);
		}

		public void RevertAllAndDelete()
		{
			Revert();
			DeleteShelvedFiles();
			Delete();
		}

		public void Save()
		{
			_Context.EnsureLoggedIn();
			P4Output saveOutput = P4Caller.Run(_Context, "change", new string[] { "-i" }, input: _ChangelistRecord.AsForm(), timeoutMillisec: P4GlobalOptions.DefaultTimeoutMs);
			uint changelistNumber = UInt32.Parse(Regex.Match(saveOutput.Messages.Last(), @"Change (\d+) \S+").Groups[1].Value);
			_ChangelistRecord = P4Caller.Run(_Context, "change", new string[] { "-o", changelistNumber.ToString() }, timeoutMillisec: P4GlobalOptions.DefaultTimeoutMs).Records.First();
		}

		public P4ChangelistFile[] Describe()
		{
			_ChangelistRecord = DoDescribe();
			return RecordToChangedFiles(_ChangelistRecord);
		}

		public P4ChangelistFile[] DescribeShelved()
		{
			P4Record shelvedRecord = DoDescribe(new List<string>() { "-S" });
			return RecordToShelvedFiles(shelvedRecord);
		}

		private P4Record DoDescribe(List<string> addtionalArguments = null)
		{
			_Context.EnsureLoggedIn();
			List<string> args = addtionalArguments ?? new List<string>();
			args.Add(Changelist.ToString());
			P4Output output = P4Caller.Run(_Context, "describe", args.ToArray(), timeoutMillisec: P4GlobalOptions.DefaultTimeoutMs);
			return output.Records.First();
		}

		// get editable context returns the client we need when we want to 
		// make changes that necessitate changing the client that the changelist
		// belongs too
		private P4ClientStub GetEditableContext()
		{
			// catches all bad calls to submitt change lists
			if (Status == CLStatus.submitted)
			{
				throw new CannotEditSubmittedChangelistException(Changelist);
			}

			// code should go through here for 99% of use cases
			if (_Context is P4ClientStub)
			{
				return (P4ClientStub)_Context;
			}

			// rarer case, use the server get client via changelist's client name
			if (_Context is P4Server)
			{
				return ((P4Server)_Context).GetClient(Client);
			}

			throw new InvalidOperationException("Changelist has invalid context!");
		}

		private static P4ChangelistFile[] RecordToChangedFiles(P4Record describedChangelistRecord)
		{
			return (
				from i in Enumerable.Range(0, describedChangelistRecord.SafeGetArrayField("depotfile").Length)
				select new P4ChangelistFile(describedChangelistRecord, (uint)i)
			).ToArray();
		}

		private static P4ChangelistFile[] RecordToShelvedFiles(P4Record describedChangelistRecord)
		{
			return (
				from i in Enumerable.Range(0, describedChangelistRecord.SafeGetArrayField("depotfile").Length)
				select new P4ChangelistFile(describedChangelistRecord, (uint)i)
			).ToArray();
		}
	}
}
