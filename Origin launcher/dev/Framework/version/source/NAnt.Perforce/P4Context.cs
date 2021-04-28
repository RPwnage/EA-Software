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

using NAnt.Perforce.Extensions.String;
using NAnt.Perforce.Internal;

namespace NAnt.Perforce
{
	public abstract class P4Context
	{
		// used for default location in Change() and Files() overloads
		protected abstract P4Location DefaultLocation { get; }

		// get a specific changelist
		public P4Changelist Change(uint changeListNo)
		{
			EnsureLoggedIn();
			string[] args = { "-o", changeListNo.ToString() };
			P4Output output = P4Caller.Run(this, "change", args, timeoutMillisec: P4GlobalOptions.DefaultTimeoutMs);
			return new P4Changelist(this, output.Records.First());
		}

		// delete a changelist
		public void DeleteChangelist(uint changelist)
		{
			EnsureLoggedIn();
			string[] args = { "-d", changelist.ToString() };
			P4Caller.Run(this, "change", args, timeoutMillisec: P4GlobalOptions.DefaultTimeoutMs);
		}

		// Files overloads
		#region Files
		public P4DepotFileCollection Files(bool printAllRevisions = false, bool throwOnNoSuchFiles = false, bool ignoreDeletedFiles = false, params P4FileSpec[] locations)
		{
			try
			{
				EnsureLoggedIn();
				locations = DefaultLocationIfNoneSpecified(locations);
				List<string> arguments = new List<string>();
				if (printAllRevisions)
				{
					arguments.Add("-a");
				}
				if (ignoreDeletedFiles)
				{
					arguments.Add("-e");
				}
				arguments.AddRange(from loc in locations select loc.GetSpecString());
				P4Output output = P4Caller.Run(this, "files", arguments.ToArray(), timeoutMillisec: P4GlobalOptions.DefaultLongTimeoutMs);
				return new P4DepotFileCollection(output);
			}
			catch (P4Exception p4Ex)
			{
				p4Ex.ThrowStrippedIfNotOnly(throwOnNoSuchFiles ? new Type[] { } : new Type[] { typeof(NoSuchFilesException), typeof(MustReferToClientException) });
				return new P4DepotFileCollection(p4Ex.LinkedOutput);
			}
		}

		public P4DepotFileCollection Files(uint max, bool printAllRevisions = false, bool throwOnNoSuchFiles = false, bool ignoreDeletedFiles = false, params P4FileSpec[] locations)
		{
			try
			{
				EnsureLoggedIn();
				locations = DefaultLocationIfNoneSpecified(locations);
				List<string> arguments = CommonMaxArg(max).ToList();
				if (printAllRevisions)
				{
					arguments.Add("-a");
				}
				if (ignoreDeletedFiles)
				{
					arguments.Add("-e");
				}
				arguments.AddRange(from loc in locations select loc.GetSpecString());
				P4Output output = P4Caller.Run(this, "files", arguments.ToArray(), timeoutMillisec: P4GlobalOptions.DefaultLongTimeoutMs);
				return new P4DepotFileCollection(output);
			}
			catch (P4Exception p4Ex)
			{
				p4Ex.ThrowStrippedIfNotOnly(throwOnNoSuchFiles ? new Type[] { } : new Type[] { typeof(NoSuchFilesException), typeof(MustReferToClientException) });
				return new P4DepotFileCollection(p4Ex.LinkedOutput);
			}
		}
		#endregion

		/* returns complete files from perforce as strings without needing to sync them to the hardrive
		NOTE: due to limitations with how the files are returned from p4, files which contain content that 
		matches p4's ztag output will result in unexpected behaviour */
		#region Print
		public P4PrintFile[] Print(bool printAllRevisions = false, params P4FileSpec[] fileSpecs)
		{
			EnsureLoggedIn();
			List<string> arguments = new List<string>();
			if (printAllRevisions)
			{
				arguments.Add("-a");
			}
			arguments.AddRange(from loc in fileSpecs select loc.GetSpecString());
			P4Output output = P4Caller.Run(this, "print", arguments.ToArray(), useSyncOuput: true, timeoutMillisec: P4GlobalOptions.DefaultTimeoutMs);
			return (from record in output.Records select new P4PrintFile(record)).ToArray(); //TODO lazy
		}

		public P4PrintFile[] Print(uint max, bool printAllRevisions = false, params P4FileSpec[] fileSpecs)
		{
			EnsureLoggedIn();
			List<string> arguments = CommonMaxArg(max).ToList();
			if (printAllRevisions)
			{
				arguments.Add("-a");
			}
			arguments.AddRange(from loc in fileSpecs select loc.GetSpecString());
			P4Output output = P4Caller.Run(this, "print", arguments.ToArray(), useSyncOuput: true, timeoutMillisec: P4GlobalOptions.DefaultTimeoutMs);
			return (from record in output.Records select new P4PrintFile(record)).ToArray(); //TODO lazy
		}

		public P4DepotFile PrintToFile(string filePath, P4BaseLocation location)
		{
			EnsureLoggedIn();
			P4Output output = P4Caller.Run(this, "print", (new string[] { "-o", filePath, location.GetSpecString() }));
			return new P4DepotFile(output);
		}
		#endregion

		public string[] Dirs(DirsOptions options = DirsOptions.Default, params P4FileSpec[] locations)
		{
			EnsureLoggedIn();
			locations = NoLocationIfNoneSpecified(locations);
			List<string> args = new List<string>();
			if (options.HasFlag(DirsOptions.LimitToClient))
			{
				args.Add("-C");
			}
			if (options.HasFlag(DirsOptions.IncludeDeleted))
			{
				args.Add("-D");
			}
			if (options.HasFlag(DirsOptions.IncludeOnlyHave))
			{
				args.Add("-H");
			}
			args.AddRange(from location in locations select location.GetSpecString());
			P4Output output = P4Caller.Run(this, "dirs", args.ToArray(), timeoutMillisec: P4GlobalOptions.DefaultTimeoutMs);
			return (from record in output.Records select record.GetField("dir")).ToArray();
		}

		internal abstract void EnsureLoggedIn();
		internal abstract P4Server GetServer();
		internal abstract List<string> BuildGlobalArgs(uint retries, string responseFilePath = null);

		internal string GlobalArgsString(uint retries, string responseFilePath = null)
		{
			return String.Join(" ", BuildGlobalArgs(retries, responseFilePath));
		}

		internal SpecType[] NoLocationIfNoneSpecified<SpecType>(SpecType[] locations) where SpecType : P4FileSpec
		{
			if (locations == null)
			{
				return new SpecType[] { };
			}
			return locations;
		}

		internal SpecType[] DefaultLocationIfNoneSpecified<SpecType>(SpecType[] locations) where SpecType : P4FileSpec
		{
			if (locations == null || locations.Length == 0)
			{
				SpecType defaultLoc = DefaultLocation as SpecType;
				Debug.Assert(defaultLoc != null); // wish we could compile time check this, we're relying on the assumption 
				// that SpecType is P4FileSpec / P4BaseLocation / P4BaseLocation (which is entirely reasonable just not
				// compile time enforced )
				return new SpecType[] { defaultLoc };
			}
			return locations;
		}

		internal static string[] CommonMaxArg(uint max)
		{
			return new string[] { "-m", max.ToString() };
		}

		internal static string[] CommonStatusArg(P4Changelist.CLStatus status)
		{
			return new string[] { "-s", status.ToString("g") };
		}

		internal static string[] CommonChangelistArg(string changeList, char prefix = 'c')
		{
			uint clNumber;
			if (changeList != "default" && !UInt32.TryParse(changeList, out clNumber))
			{
				throw new InvalidChangelistSpecifiedException(changeList);
			}
			return new string[] { String.Format("-{0}", prefix), changeList.ToString() };
		}

		protected List<string> CommonGlobalArgs(uint retries, string responseFilePath)
		{
			List<string> globalArgs = new List<string> { "-Ztag" };

			// 0 retries is default in p4 terms so omit if 0 specified
			if (retries > 0)
			{
				bool defaultRetries = retries == P4Defaults.DefaultRetries;
				if (defaultRetries)
				{
					retries = P4GlobalOptions.P4Retries;
				}

				globalArgs.Add(String.Format("-r {0}", retries));
			}

			// set response file
			if (responseFilePath != null)
			{
				globalArgs.Add(String.Format("-x {0}", responseFilePath.Quoted())); // TODO we could also use p4 internal batching when using -x, -b sets the batching size (128 default)
			}

			return globalArgs;
		}

		protected P4Changelist[] DoChanges(bool includeDescriptions, bool includeIntegrations, string clientName, string userName, P4FileSpec[] fileSpecs, string[] extraArgs = null)
		{
			EnsureLoggedIn();
			List<string> args = new List<string>();
			fileSpecs = NoLocationIfNoneSpecified(fileSpecs);
			if (includeDescriptions)
			{
				args.Add("-l");
			}
			if (includeIntegrations)
			{
				args.Add("-i");
			}
			if (clientName != null)
			{
				args.AddRange(new string[] { "-c", clientName });
			}
			if (userName != null)
			{
				args.AddRange(new string[] { "-u", userName });
			}
			if (extraArgs != null)
			{
				args.AddRange(extraArgs);
			}
			args.AddRange(from fileSpec in fileSpecs select fileSpec.GetSpecString());
			P4Output output = P4Caller.Run(this, "changes", args.ToArray(), timeoutMillisec: P4GlobalOptions.DefaultTimeoutMs);
			return (from record in output.Records select new P4Changelist(this, record)).ToArray();
		}

		protected P4OpenedFile[] DoGetOpenedFiles(string changelist, string client, string userName, P4Location[] locations, List<string> additionalOptions = null)
		{
			EnsureLoggedIn();
			locations = NoLocationIfNoneSpecified(locations);
			List<string> args = additionalOptions ?? new List<string>();
			if (changelist != null)
			{
				args.AddRange(CommonChangelistArg(changelist));
			}
			if (client != null)
			{
				args.Add("-C");
				args.Add(client);
			}
			if (userName != null)
			{
				args.Add("-u");
				args.Add(userName);
			}
			args.AddRange(from loc in locations select loc.GetSpecString());
			P4Output output = P4Caller.Run(this, "opened", args.ToArray(), timeoutMillisec: P4GlobalOptions.DefaultTimeoutMs);
			return (from record in output.Records select new P4OpenedFile(record)).ToArray();
		}
	}
}