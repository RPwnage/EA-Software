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
using System.Collections.ObjectModel;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
using NAnt.Perforce.Internal;

namespace NAnt.Perforce
{
	public abstract class P4File
	{
		public enum ActionType //TODO: figure out if add and added are the same thing or represent different stages, submitted vs pending maybe?
		{
			abandoned,
			add,
			added,
			branch,
			delete,
			deleted,	// When I did a p4 move and then revert, the entry at new location will have a "deleted" action.
			edit,
			integrate,
			move_add,
			move_delete,
			refreshed,
			reverted
		}

		public enum FileBaseType
		{
			apple,
			binary,
			resource,
			symlink,
			text,
			utf8,
			unicode,
			utf16
		}

		[Flags]
		public enum FileTypeFlags
		{
			None = 0x0,
			AlwaysClientWriteable = 0x1, // +w suffix
			Executable = 0x2, // +x suffix, x prefix
			RCSKeywordExpansion = 0x4, // +k suffix, k prefix
			OldKeywordExpansion = 0x8, // +ko suffix
			ExclusiveOpen = 0x10, // +l suffix
			Compressed = 0x20, // +C suffix, c prefix
			RCSDeltas = 0x40, // +D suffix
			StoreFullFilePerRevision = 0x80, // +F suffix, u or l prefix
			StoreOnlyHeadRevisions = 0x100, // +S suffix
			PreserverOriginalModtime = 0x200, // +m suffix
			ArchiveTriggerReguired = 0x400 // +X suffix
		}

		public class FileType
		{
			private static uint[] ValidRevisionStoreSizes = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 16, 32, 64, 128, 256, 512 };

			public readonly FileBaseType ContentType;
			public readonly FileTypeFlags TypeFlags;
			public readonly uint NumStoreRevisions;

			public FileType(FileBaseType contentType, FileTypeFlags typeFlags = FileTypeFlags.None, uint numStoreRevisions = UInt32.MaxValue)
			{
				ContentType = contentType;
				TypeFlags = typeFlags;
				NumStoreRevisions = numStoreRevisions;

				if (typeFlags.HasFlag(FileTypeFlags.OldKeywordExpansion) && !typeFlags.HasFlag(FileTypeFlags.RCSKeywordExpansion))
				{
					throw new P4Exception(String.Format("Cannot specifiy '{0}' flag unless '{1}' is also specified!", FileTypeFlags.OldKeywordExpansion.ToString("g"), FileTypeFlags.RCSKeywordExpansion.ToString("g")));
				}

				if (typeFlags.HasFlag(FileTypeFlags.StoreOnlyHeadRevisions))
				{
					if (NumStoreRevisions == UInt32.MaxValue)
					{
						NumStoreRevisions = 1;
					}
					if (!ValidRevisionStoreSizes.Contains(NumStoreRevisions))
					{
						throw new P4Exception(String.Format("Cannot set store revision size '{0}'! Valid sizes are: {1}.", NumStoreRevisions, String.Join(", ", ValidRevisionStoreSizes)));
					}
				}
				else
				{
					if (NumStoreRevisions == UInt32.MaxValue)
					{
						NumStoreRevisions = 0;
					}
					if (NumStoreRevisions != 0)
					{
						throw new P4Exception(String.Format("Cannot set number of store revisions unless flag '{0}' is specified!", FileTypeFlags.StoreOnlyHeadRevisions.ToString("g")));
					}
				}
			}
		}

		protected abstract string GetField(string field);
		protected abstract bool HasField(string field); // certain skip array fields might never show up, so we might not have values at all, in which case use safe function

		protected string GetField(string field, string defaultVal)
		{
			return HasField(field) ? GetField(field) : defaultVal;
		}

		protected uint GetUint32Field(string field)
		{
			return UInt32.Parse(GetField(field));
		}

		protected ulong GetUint64Field(string field)
		{
			return UInt64.Parse(GetField(field));
		}

		protected uint GetUint32Field(string field, UInt32 defaultVal)
		{
			return HasField(field) ? GetUint32Field(field) : defaultVal;
		}

		protected DateTime GetDateField(string field)
		{
			return P4Utils.P4TimestampToDateTime(GetField(field));
		}

		protected DateTime GetDateField(string field, DateTime defaultVal)
		{
			return HasField(field) ? GetDateField(field) : defaultVal;
		}

		protected TEnum GetEnumField<TEnum>(string field)
		{
			return (TEnum)Enum.Parse(typeof(TEnum), GetField(field));
		}

		protected TEnum GetEnumField<TEnum>(string field, TEnum defaultVal)
		{
			return HasField(field) ? GetEnumField<TEnum>(field) : defaultVal;
		}

		protected ActionType GetActionField(string field)
		{
			return (ActionType)Enum.Parse(typeof(ActionType), GetField(field).Replace("/", "_")); // enums don't allow slashes in names
		}

		protected bool GetBooleanField(string field)
		{
			return GetField(field) == "1";
		}

		protected bool GetBooleanField(string field, bool defaultVal)
		{
			return HasField(field) ? GetBooleanField(field) : defaultVal;
		} 
	}

	// wraps a single record
	public abstract class P4RecordFile : P4File
	{
		internal P4Record FileRecord
		{
			get
			{
				if (_InternalFileRecord == null)
				{
					_InternalFileRecord = _SourceOutput.Records.First(); // when we instantiate P4RecordFile from an output we do so when we expect one record in the output
				}
				return _InternalFileRecord;
			}
		}

		private P4Record _InternalFileRecord = null;

		private readonly P4Output _SourceOutput;

		internal P4RecordFile(P4Output output)
		{
			if (output == null)
			{
				throw new ArgumentNullException("Cannot instantiate P4 file from null output!");
			}
			_SourceOutput = output;
		}

		internal P4RecordFile(P4Record record)
		{
			if (record == null)
			{
				throw new ArgumentNullException("Cannot instantiate P4 file from null record!");
			}
			_InternalFileRecord = record;
			_SourceOutput = null;
		}

		protected override string GetField(string field)
		{
			return FileRecord.GetField(field);
		}

		protected override bool HasField(string field)
		{
			return FileRecord.HasField(field);
		}

		protected uint GetHaveRevField() // special case field
		{
			if (HasField("haverev"))
			{
				string haveRev = GetField("haverev");
				if (haveRev != "none")
				{
					return UInt32.Parse(haveRev);
				}
			}
			return 0;
		}
	}

	// wraps a record and an index into its array fields
	public abstract class P4ArrayRecordFile : P4File
	{
		private readonly uint _Index;
		private readonly P4Record _ArrayRecord = null;

		internal P4ArrayRecordFile(P4Record record, uint index)
		{
			_ArrayRecord = record;
			_Index = index;
		}

		protected override string GetField(string field)
		{
			return _ArrayRecord.GetArrayFieldElement(field, _Index);
		}

		protected override bool HasField(string field)
		{
			return _ArrayRecord.HasArrayField(field);
		}
	}

	//TODO: various p4 commands will return various combinations of fields for the files, the variety is 
	// pretty wide so a flat heirarchy is used (everything non-abstract class inherits P4RecordFile or 
	// P4ArrayRecordFile), however it might be useful to users of the library to have common properties 
	// grouped under interfaces, with one interface per property

	// for example every P4RecordFile derivative with the DepotFile attribute would implement IDepotFile for example
	// this adds up to a lot of interfaces on the larger classes but might be useful for writing more generic code to
	// deal with file objects (some internal refactoring might also be possible if we use these objects)

	public class P4WhereMappedFile : P4RecordFile
	{
		public string DepotFile { get { return GetField("depotfile"); } }
		public string ClientFile { get { return GetField("clientfile"); } }
		public string Path { get { return GetField("path"); } }

		internal P4WhereMappedFile(P4Record record)
			: base(record)
		{
		}
	}

	public class P4WhereFile : P4WhereMappedFile
	{
		public readonly ReadOnlyCollection<P4WhereMappedFile> UnmappedLocations;

		internal P4WhereFile(P4Record[] records)
			: base(records.Last())
		{
			UnmappedLocations = new ReadOnlyCollection<P4WhereMappedFile>((from r in records.Take(records.Length - 1) select new P4WhereMappedFile(r)).ToList());
		}
	}

	// returned from files command
	public class P4DepotFile : P4RecordFile
	{
		private FileType _Type = null;
		private P4Output output;

		public uint Revision { get { return GetUint32Field("rev"); } }
		public FileType Type { get { return _Type ?? (_Type = P4Utils.ParseFileTypeString(GetField("type"))); } }
		public DateTime Time { get { return GetDateField("time"); } }
		public uint Changelist { get { return GetUint32Field("change"); } }
		public string DepotFile { get { return GetField("depotfile"); } }
		public ActionType Action { get { return GetActionField("action"); } }

		internal P4DepotFile(P4Record record)
			: base(record)
		{
		}

		internal P4DepotFile(P4Output output)
			: base(output)
		{
			this.output = output;
		}
	}

	// return from a print, same as depotfile but with file contents
	public class P4PrintFile : P4DepotFile
	{
		public string Contents { get { return HandleEncodingBOM(GetRawContents()); } } // needs more testing before we really say this is correct before for now it works to have printed and synced file match

		private static readonly Regex _ContentsRegex = new Regex(@"\.\.\. depotFile .*\r?\n\.\.\. rev .*\r?\n\.\.\. change .*\r?\n\.\.\. action .*\r?\n\.\.\. type .*\r?\n\.\.\. time.*\r?\n\.\.\. fileSize .*\r?\n\r?\n([\s\S]*)\n$");
		// The true UTF8 BOM is actually 0xEF 0xBB 0xBF.  The following 3 values are actually the UTF8 BOM being converted to code page 437 encoding (North America command prompt encoding code page).
		// After we switch the Process output to always use UTF8 encoding, we won't see the following values in the text string any more.
		private static readonly string _PerforceUTF8Bom = "\u2229\u2557\u2510";
		// After we switch the Process output to always use UTF8 encoding (or in OSX/Unix enviornment), the UTF8 BOM got translated to a UTF16 BOM in the output.
		// It got converted to UTF16 BOM because C# string class use UTF16 as default encoding.
		private static readonly string _LittleUTF16Bom = new UnicodeEncoding(true, false).GetString(new UnicodeEncoding(false, true).GetPreamble()); // utf is .NETs internal representation
		private static readonly string _BigUTF16Bom = new UnicodeEncoding(true, false).GetString(new UnicodeEncoding(true, true).GetPreamble());

		internal P4PrintFile(P4Record record)
			: base(record)
		{
		}
		
		private string GetRawContents()
		{
			return _ContentsRegex.Match(FileRecord.AllText).Groups[1].Value.Replace('\n', '\r');
		}

		private string HandleEncodingBOM(string unprocessedContents)
		{
			if (Type.ContentType == FileBaseType.text)
			{
				if (CharByCharStartsWithComparison(unprocessedContents, _PerforceUTF8Bom))
				{
					return unprocessedContents.Substring(_PerforceUTF8Bom.Length);
				}
				if (CharByCharStartsWithComparison(unprocessedContents, _LittleUTF16Bom))
				{
					return unprocessedContents.Substring(_LittleUTF16Bom.Length);
				}
				if (CharByCharStartsWithComparison(unprocessedContents, _BigUTF16Bom))
				{
					return unprocessedContents.Substring(_BigUTF16Bom.Length);
				}
				return unprocessedContents;
			}
			else if (Type.ContentType == FileBaseType.utf16)
			{
				return unprocessedContents;
			}
			else if (Type.ContentType == FileBaseType.utf8)
			{
				return unprocessedContents;
			}
			else
			{
				throw new P4Exception(String.Format("Cannot retrieve text contents for this file as it does not have the 'utf8', 'utf16' or 'text' type. Type is '{0}'.", Type.ContentType.ToString("g")));
				// TODO - we should run some tests and see if we can reconstruct the original bytes and expose a seprate property for returning raw bytes, this may
				// be dependent on perforce server default encoding and os we're running p4 executable under but we readily have access to that information so it shouldn't
				// be too bad, only trickiness comes from text where p4 might try and hide the bom from us so we'll never have an exact binary match
			}
		}

		// non-standard characters seem to confuse .StartsWith so using a manual comparison
		private bool CharByCharStartsWithComparison(string source, string startString)
		{
			if (source.Length < startString.Length)
			{
				return false;
			}

			for (int i = 0; i < startString.Length; ++i)
			{
				if (source[i] != startString[i])
				{
					return false;
				}
			}

			return true;
		}
	}

	// returned from a sync
	public class P4ClientFile : P4RecordFile
	{
		public uint Revision { get { return GetUint32Field("rev"); } }
		public uint FileSize { get { return GetUint32Field("filesize", 0); } }
		public uint TotalFileSize { get { return GetUint32Field("totalfilesize"); } }
		public uint TotalFileCount { get { return GetUint32Field("totalfilecount"); } }
		public string ClientFile { get { return GetField("clientfile"); } }
		public uint Changelist { get { return GetUint32Field("change"); } }
		public string DepotFile { get { return GetField("depotfile"); } }
		public ActionType Action { get { return GetActionField("action"); } }

		internal P4ClientFile(P4Record record)
			: base(record)
		{
		}
	}

	// returned from an edit
	public class P4EditFile : P4RecordFile
	{
		public uint WorkRevision { get { return GetUint32Field("workrev"); } }
		public FileType Type { get { return P4Utils.ParseFileTypeString(GetField("type")); } }
		public string ClientFile { get { return GetField("clientfile"); } }
		public string DepotFile { get { return GetField("depotfile"); } }
		public ActionType Action { get { return GetActionField("action"); } }

		internal P4EditFile(P4Record record)
			: base(record)
		{
		}
	}

	// returned from a change list descibe
	public class P4ChangelistFile : P4ArrayRecordFile
	{
		public string DepotFile { get { return GetField("depotfile"); } }
		public ActionType Action { get { return GetActionField("action"); } }
		public uint Revision { get { return GetUint32Field("rev"); } }
		public FileType Type { get { return P4Utils.ParseFileTypeString(GetField("type")); } }
		public uint FileSize { get { return GetUint32Field("filesize", 0); } }
		public string Digest { get { return GetField("digest", null); } } // digest only exists for shelved or submitted files

		internal P4ChangelistFile(P4Record record, uint index)
			: base(record, index)
		{
		}
	}

	// returned from a revert
	public class P4RevertFile : P4RecordFile
	{
		public string ClientFile { get { return GetField("clientfile"); } }
		public string DepotFile { get { return GetField("depotfile"); } }
		public ActionType Action { get { return GetActionField("action"); } }
		public ActionType OldAction { get { return GetActionField("oldaction"); } }
		public uint HaveRevision { get { return GetHaveRevField(); } }

		internal P4RevertFile(P4Record record)
			: base(record)
		{
		}
	}

	// returned from a merge
	public class P4IntegrateFile : P4RecordFile
	{
		public string ClientFile { get { return GetField("clientfile"); } }
		public string DepotFile { get { return GetField("depotfile"); } }
		public uint WorkRevision { get { return GetUint32Field("workrev"); } }
		public ActionType Action { get { return GetActionField("action"); } }
		public string OtherAction { get { return GetField("otheraction"); } }
		public string FromFile { get { return GetField("fromfile"); } }
		public string StartFromRevision { get { return GetField("startfromrev"); } }
		public string EndFromRevision { get { return GetField("endfromrev"); } }

		internal P4IntegrateFile(P4Record record)
			: base(record)
		{
		}
	}

	// returned from a resolve
	public class P4ResolveFile : P4RecordFile
	{
		public string ClientFile { get { return GetField("clientfile"); } }
		public string FromFile { get { return GetField("fromfile"); } }
		public string StartFromRevision { get { return GetField("startfromrev"); } }
		public string EndFromRevision { get { return GetField("endfromrev"); } }
		public string ResolveType { get { return GetField("resolvetype"); } }
		public string ResolveFlag { get { return GetField("resolveflag"); } }

		// TODO: Should consider what valid content resolve types are and possibly
		// return an enumeration rather than a string.
		public string ContentResolveType { get { return GetField("contentresolvetype", null); } }
		public P4ResolveHow How = null;

		internal P4ResolveFile(P4Record record)
			: base(record)
		{
		}
	}

	// a second type of record returned by a resolve that is associated with a single p4resolvefile record
	public class P4ResolveHow : P4RecordFile
	{
		public string How { get { return GetField("how"); } }
		public string ToFile { get { return GetField("tofile"); } }
		public string FromFile { get { return GetField("fromfile"); } }

		internal P4ResolveHow(P4Record record)
			: base(record)
		{
		}
	}

	// returned from a reopen
	public class P4ReopenFile : P4RecordFile
	{
		private FileType _Type = null;

		public string DepotFile { get { return GetField("depotfile"); } }
		public uint WorkRevision { get { return GetUint32Field("workrev"); } }
		public FileType Type { get { return _Type ?? (_Type = P4Utils.ParseFileTypeString(GetField("type"))); } }

		internal P4ReopenFile(P4Record record)
			: base(record)
		{
		}
	}

	// returned from a opened
	public class P4OpenedFile : P4RecordFile
	{
		public string ClientFile { get { return GetField("clientfile"); } }
		public string DepotFile { get { return GetField("depotfile"); } }
		public uint Revision { get { return GetUint32Field("rev"); } }
		public uint HaveRevision { get { return GetHaveRevField(); } }
		public ActionType Action { get { return GetActionField("action"); } }
		public string Changelist { get { return GetField("change"); } }
		public FileType Type { get { return P4Utils.ParseFileTypeString(GetField("type")); } }
		public string User { get { return GetField("user"); } }
		public string Client { get { return GetField("client"); } }

		internal P4OpenedFile(P4Record record)
			: base(record)
		{
		}
	}

    // return from a submit
    public class P4SubmitFile : P4RecordFile
    {
        public string DepotFile { get { return GetField("depotfile"); } }
        public uint Revision { get { return GetUint32Field("rev"); } }
        public ActionType Action { get { return GetActionField("action"); } }
        
        internal P4SubmitFile(P4Record record)
            : base(record)
        {
        }
    }

	// returned from a shelve
	public class P4ShelveFile : P4RecordFile
	{
		public uint Changelist { get { return GetUint32Field("change"); } }
		public string DepotFile { get { return GetField("depotfile"); } }
		public uint Revision { get { return GetUint32Field("rev"); } }
		public ActionType Action { get { return GetActionField("action"); } }
		
		internal P4ShelveFile(P4Record record)
			: base(record)
		{
		}
	}

	// returned from a unshelve
	public class P4UnshelveFile : P4RecordFile
	{
		public string DepotFile { get { return GetField("depotfile"); } }
		public uint Revision { get { return GetUint32Field("rev"); } }
		public ActionType Action { get { return GetActionField("action"); } }

		internal P4UnshelveFile(P4Record record)
			: base(record)
		{
		}
	}

	// returned from a sync
	public class P4SyncFile : P4RecordFile
	{
		public uint Changelist { get { return GetUint32Field("change"); } }
		public string ClientFile { get { return GetField("clientfile"); } }
		public string DepotFile { get { return GetField("depotfile"); } }
		public uint FileSize { get { return GetUint32Field("filesize", 0); } } // deleted files have no size
		public uint Revision { get { return GetUint32Field("rev"); } }
		public ActionType Action { get { return GetActionField("action"); } }

		internal P4SyncFile(P4Record record)
			: base(record)
		{
		}
	}

	// Returned from an fstat operation
	public class P4FStatFile : P4RecordFile
	{
		public string DepotFile { get { return GetField("depotfile"); } }
		public string ClientFile { get { return GetField("clientfile"); } }
		public bool Shelved { get { return HasField("shelved"); } }
		public bool IsMapped { get { return HasField("ismapped"); } }
		public string HeadAction { get { return GetField("headaction"); } }
		public FileType HeadType { get { return P4Utils.ParseFileTypeString(GetField("headtype")); } }
		public DateTime HeadTime { get { return GetDateField("headtime"); } }
		public DateTime HeadModTime { get { return GetDateField("headmodtime"); } }
		public string Digest { get { return GetField("digest"); } }
		public ulong FileSize { get { return GetUint64Field("filesize"); } }

		internal P4FStatFile(P4Record record)
			: base(record)
		{
		}
	}
}
