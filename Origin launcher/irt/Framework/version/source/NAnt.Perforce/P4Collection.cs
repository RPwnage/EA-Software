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
using System.Collections;
using System.Collections.Generic;
using System.Linq;

using NAnt.Perforce.Internal;

namespace NAnt.Perforce
{
	public abstract class P4Collection<RecordWrapperType> : IReadOnlyCollection<RecordWrapperType>
	{
		private RecordWrapperType[] _LazyInternalArray;
		private P4Output _SourceOutput;

		public int Count
		{
			get { return GetInternalArray().Length; }
		}
		
		internal P4Collection(P4Output output)
		{
			_LazyInternalArray = null;
			_SourceOutput = output;
		}
	
		public IEnumerator<RecordWrapperType> GetEnumerator()
		{
			return ((IEnumerable<RecordWrapperType>)GetInternalArray()).GetEnumerator();
		}

		IEnumerator IEnumerable.GetEnumerator()
		{
			return GetInternalArray().GetEnumerator();
		}

		internal abstract RecordWrapperType CreateWrapperType(P4Record record);

		internal virtual RecordWrapperType[] BuildInternalArray(P4Output output)
		{
			return output.Records.Select(record => CreateWrapperType(record)).ToArray();
		}

		private RecordWrapperType[] GetInternalArray()
		{
			if (_LazyInternalArray == null)
			{
				_LazyInternalArray = BuildInternalArray(_SourceOutput);
				_SourceOutput = null;
			}
			return _LazyInternalArray;
		}
	}

	public class P4DepotFileCollection : P4Collection<P4DepotFile>
	{
		internal P4DepotFileCollection(P4Output output) : base(output) { }
		internal override P4DepotFile CreateWrapperType(P4Record record) { return new P4DepotFile(record); }
	}

	// TODO: would be useful to have convenience properties for Added, Edited and Deleted files
	public class P4EditFileCollection : P4Collection<P4EditFile>
	{
		internal P4EditFileCollection(P4Output output) : base(output) { }
		internal override P4EditFile CreateWrapperType(P4Record record) { return new P4EditFile(record); }
	}
  
	public class P4FStatFileCollection : P4Collection<P4FStatFile>
	{
		internal P4FStatFileCollection(P4Output output) : base(output) { }
		internal override P4FStatFile CreateWrapperType(P4Record record) { return new P4FStatFile(record); }
	}

	public class P4RevertFileCollection : P4Collection<P4RevertFile>
	{
		internal P4RevertFileCollection(P4Output output) : base(output) { }
		internal override P4RevertFile CreateWrapperType(P4Record record) { return new P4RevertFile(record); }
	}

	public class P4IntegrateFileCollection : P4Collection<P4IntegrateFile>
	{
		internal P4IntegrateFileCollection(P4Output output) : base(output) { }
		internal override P4IntegrateFile CreateWrapperType(P4Record record) { return new P4IntegrateFile(record); }
	}

	public class P4ResolveFileCollection : P4Collection<P4ResolveFile>
	{
		internal P4ResolveFileCollection(P4Output output) : base(output) { }
		internal override P4ResolveFile CreateWrapperType(P4Record record) { return new P4ResolveFile(record); }

		// p4 resolve returns multiple records for content files that are resolved, we need to parse these and map them to resolve records
		internal override P4ResolveFile[] BuildInternalArray(P4Output output)
		{
			// Note that a file can have multiple changes (change filetype and change content).  So the following
			// resolveFiles need to be a List object and we should not be using Dictionary object for it.
			List<P4ResolveFile> resolveFiles = new List<P4ResolveFile>();
			foreach (P4Record record in output.Records)
			{
				if (record.HasField("how"))
				{
					P4ResolveHow newResolveHow = new P4ResolveHow(record);
					foreach (P4ResolveFile file in resolveFiles.Where(f => f.FromFile == newResolveHow.FromFile))
					{
						file.How = newResolveHow;
					}
				}
				else
				{
					P4ResolveFile newResolveFile = CreateWrapperType(record);
					resolveFiles.Add(newResolveFile);
				}
			}
			return resolveFiles.ToArray();
		}
	}

	public class P4UnshelveFileCollection : P4Collection<P4UnshelveFile>
	{
		internal P4UnshelveFileCollection(P4Output output) : base(output) { }
		internal override P4UnshelveFile CreateWrapperType(P4Record record) { return new P4UnshelveFile(record); }
	}  

	public class P4SubmitFileCollection : P4Collection<P4SubmitFile>
	{
		private P4Record _LazyLockRecord = null;
		private P4Output _LockOutput = null;
		private P4Record _LazySubmitRecord = null;
		private P4Output _SubmitOutput = null;

		public readonly bool DidSubmit;

		public uint Change { get { return GetLockRecord().GetUint32Field("change"); } }
		public uint SubmittedChange { get { return GetSubmitRecord().GetUint32Field("submittedchange"); } }

		public P4SubmitFile[] Submitted
		{
			get
			{
				return this.Where(edited => edited.Action == P4File.ActionType.edit && !this.Any(reverted => reverted.DepotFile == edited.DepotFile && reverted.Action == P4File.ActionType.reverted)).ToArray();
			}
		}

		public P4SubmitFile[] Reverted
		{
			get
			{
				return this.Where(reverted => reverted.Action == P4File.ActionType.reverted).ToArray();
			}
		}

		internal P4SubmitFileCollection(P4Output output, bool didSubmit = true) 
			: base(output) 
		{
			DidSubmit = didSubmit;

			_LockOutput = output;
			if (didSubmit) // on submit records will go: locked record, submitted file record(s), submitted change record
			{
				_SubmitOutput = output;
			}
			else // on no submit records will go: locked record, editted file record(s) if any, reverted file record(s) if any (requires revertunchanged and you will get both and edit and revert record)
			{
				_LazySubmitRecord = new P4Record();
				_LazySubmitRecord.SetField("submittedchange", 0); // return 0 for submitted changed if we didn't submit anything
			}
		}

		internal override P4SubmitFile CreateWrapperType(P4Record record) { return new P4SubmitFile(record); }

		internal override P4SubmitFile[] BuildInternalArray(P4Output output)
		{
			if (DidSubmit) // if we submitted then first and last record are not file recorsd
			{
				return output.Records.Skip(1).Take(output.Records.Count() - 2).Select(record => CreateWrapperType(record)).ToArray();
			}
			else // if we didn't submit then all but first record are file records
			{
				return output.Records.Skip(1).Select(record => CreateWrapperType(record)).ToArray();
			}
		}

		private P4Record GetLockRecord()
		{
			if (_LazyLockRecord == null)
			{
				_LazyLockRecord = _LockOutput.FirstRecord;
				_LockOutput = null;
			}
			return _LazyLockRecord;
		}

		private P4Record GetSubmitRecord()
		{
			if (_LazySubmitRecord == null)
			{
				_LazySubmitRecord = _SubmitOutput.Records.Last();
				_SubmitOutput = null;
			}
			return _LazySubmitRecord;
		}
	}  

	// sync file collections are more interesting because we batch syncs, so it actually needs to wrap
	// mutliple collections each of which wraps the output from a sync commnad - we might want to make
	// this generic in future to allow batching of other commands
	public class P4SyncFileCollection : P4Collection<P4SyncFile> 
	{
		// this class implements the standard collection pattern for a single batch sync
		internal class SingleBatch : P4Collection<P4SyncFile>
		{ 
			private P4Record _LazySyncRecord = null;
			private P4Output _SyncOutput = null;

			public uint TotalFileSize { get { return GetSyncRecord().GetUint32Field("totalfilesize"); } }
			public uint TotalFileCount { get { return GetSyncRecord().GetUint32Field("totalfilecount"); } }
			public uint Change { get { return GetSyncRecord().GetUint32Field("change"); } }

			internal SingleBatch(P4Output output) 
				: base(output) 
			{
				_SyncOutput = output;
			}

			internal override P4SyncFile CreateWrapperType(P4Record record) { return new P4SyncFile(record); }

			private P4Record GetSyncRecord()
			{
				if (_LazySyncRecord == null)
				{
					_LazySyncRecord = _SyncOutput.FirstRecord;
					_SyncOutput = null;
				}
				return _LazySyncRecord;
			}
		}

		private readonly SingleBatch[] _BatchOutputs;

		public uint TotalFileSize { get { return (uint)_BatchOutputs.Sum(sb => sb.TotalFileSize); } }
		public uint TotalFileCount { get { return (uint)_BatchOutputs.Sum(sb => sb.TotalFileCount); } }
		public uint Change { get { return _BatchOutputs.Max(sb => sb.Change); } }

		internal P4SyncFileCollection(SingleBatch[] batchOutputs)
			: base (null)
		{
			_BatchOutputs = batchOutputs;
		}

		internal override P4SyncFile[] BuildInternalArray(P4Output output)
		{
			return _BatchOutputs.SelectMany(sb => sb).ToArray();
		}

		// unused for this implementation
		internal override P4SyncFile CreateWrapperType(P4Record record) 
		{
			throw new NotImplementedException();
		}
	}
}