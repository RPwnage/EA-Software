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
using System.Linq;
using System.Net;
using System.Text;
using System.Timers;

using NAnt.Perforce.Extensions.IEnumerable;
using NAnt.Perforce.Internal;

namespace NAnt.Perforce
{
	[Flags]
	public enum FStatFields
	{
		All = 0,
		depotFile = 1 << 0,
		clientFile = 1 << 1,
		shelved = 1 << 2,
		isMapped = 1 << 3,
		headAction = 1 << 4,
		headType = 1 << 5,
		headTime = 1 << 6,
		headModTime = 1 << 7,
		digest = 1 << 8,
		fileSize = 1 << 9,
	}

	public class P4ClientStub : P4Context
	{
		private sealed class SyncTaskOutputMonitor : P4Caller.ITaskSpecificOutputMonitor, IDisposable
		{
			private int m_fileProcessCounter = 0;
			private string[] m_syncPaths = null;

			private Timer m_timer = new Timer(30 * 1000) // do an output every 30 sec
			{
				Enabled = false,
				AutoReset = true
			};

			private Action<string> m_updateCallback;

			public SyncTaskOutputMonitor(Action<string> updateCallback, string[] syncPaths)
			{
				m_updateCallback = updateCallback;
				m_timer.Elapsed += TimerEventTimedOut;
				m_syncPaths = syncPaths;
			}

			private void TimerEventTimedOut(object sender, ElapsedEventArgs e)
			{
				if (m_syncPaths != null && m_syncPaths.Length == 1)
				{
					// currently assuming the first argument is the package sync path
					string packagePath = m_syncPaths[0];
					m_updateCallback(String.Format("[{0}] Please Wait. P4 Sync of '{1}' in progress... ({2} files)", DateTime.Now.ToString(), packagePath, m_fileProcessCounter));
				}
				else
				{
					m_updateCallback(String.Format("[{0}] Please Wait.  P4 Sync in progress... ({1} files)", DateTime.Now.ToString(), m_fileProcessCounter));
				}
			}

			public void TaskOutputProc(string output)
			{
				if (!string.IsNullOrEmpty(output))
				{
					int idx = output.IndexOf("clientFile ");
					if (idx >= 0)
					{
						m_fileProcessCounter++;
						if (!m_timer.Enabled)
						{
							m_timer.Start();
						}
					}
				}
			}

			public void Dispose()
			{
				m_timer.Stop();
			}
		}

		public P4Server Server { get; private set; }

		internal P4Record ClientRecord = null;

		protected P4Client.ClientOptions _Options = null;

		public virtual string Name
		{
			get { return ClientRecord.GetField("client"); }
			set { ThrowReadOnlyException(); }
		}

		public virtual string Owner
		{
			get { return ClientRecord.GetField("owner"); }
			set { ThrowReadOnlyException(); }
		}

		public virtual P4Client.ClientOptions Options
		{
			get { return _Options; }
			set { ThrowReadOnlyException(); }
		}

		public virtual P4Client.LineEndOption LineEnd
		{
			get { return ClientRecord.GetEnumField<P4Client.LineEndOption>("lineend"); }
			set { ThrowReadOnlyException(); }
		}

		public virtual P4Client.SubmitOptionSet SubmitOptions
		{
			get { return (P4Client.SubmitOptionSet)Enum.Parse(typeof(P4Client.SubmitOptionSet), ClientRecord.GetField("submitoptions").Replace("+", "_")); }
			set { ThrowReadOnlyException(); }
		}

		public virtual string Root
		{
			get { return ClientRecord.GetField("root"); }
			set { ThrowReadOnlyException(); }
		}

		public virtual string Host
		{
			get { return ClientRecord.GetField("host"); }
			set { ThrowReadOnlyException(); }
		}

		public virtual string Stream
		{
			get { return ClientRecord.GetField("stream", null); }
			set { ThrowReadOnlyException(); }
		}

		public virtual P4Client.ClientTypes ClientType
		{
			get { return ClientRecord.GetEnumField<P4Client.ClientTypes>("type"); }
			set { ThrowReadOnlyException(); }
		}

		public string RootPath { get { return String.Format("//{0}/...", Name); } }

		protected override P4Location DefaultLocation { get { return new P4Location(RootPath); } }

		internal P4ClientStub(P4Server server, string name, string root, P4Client.ClientOptions options, P4Client.LineEndOption lineEnd, P4Client.SubmitOptionSet submitOptions, P4Client.ClientTypes clientType, string stream = null)
		{
			ClientRecord = new P4Record();

			Server = server;
			Name = name;
			Root = root;
			Host = Dns.GetHostName();
			Owner = server.User;
			Options = options;
			LineEnd = lineEnd;
			SubmitOptions = submitOptions;
			Stream = stream;
			ClientType = clientType;
		}

		internal P4ClientStub(P4Server server, P4Record p4Record)
		{
			Server = server;
			ClientRecord = p4Record;
			_Options = new P4Client.ClientOptions(this); // options isn't automatically tied to record contents but reads from in during constructor, so update here
		}

		public P4Changelist CreateNewChangelist(string description, params P4Location[] files)
		{
			Server.EnsureLoggedIn();
			P4Changelist changelist = new P4Changelist(this, description, files);
			changelist.Save();
			return changelist;
		}

		public void DeleteClient()
		{
			Server.EnsureLoggedIn();
			P4Caller.Run(Server, "client", new string[] { "-d", Name }, timeoutMillisec: P4GlobalOptions.DefaultTimeoutMs);
		}

		// ClientChanges overloads, returns the changelists associated with this client
		#region ClientChanges
		public P4Changelist[] ClientChanges(bool includeDescriptions = false, bool includeIntegrations = false, string userName = null, params P4FileSpec[] fileSpecs)
		{
			return DoChanges(includeDescriptions, includeIntegrations, Name, userName, fileSpecs);
		}

		public P4Changelist[] ClientChanges(uint max, bool includeDescriptions = false, bool includeIntegrations = false, string userName = null, params P4FileSpec[] fileSpecs)
		{
			return DoChanges(includeDescriptions, includeIntegrations, Name, userName, fileSpecs, CommonMaxArg(max));
		}

		public P4Changelist[] ClientChanges(P4Changelist.CLStatus status, bool includeDescriptions = false, bool includeIntegrations = false, string userName = null, params P4FileSpec[] fileSpecs)
		{
			return DoChanges(includeDescriptions, includeIntegrations, Name, userName, fileSpecs, CommonStatusArg(status));
		}

		public P4Changelist[] ClientChanges(uint max, P4Changelist.CLStatus status, bool includeDescriptions = false, bool includeIntegrations = false, string userName = null, params P4FileSpec[] fileSpecs)
		{
			return DoChanges(includeDescriptions, includeIntegrations, Name, userName, fileSpecs, CommonMaxArg(max).Concat(CommonStatusArg(status)).ToArray());
		}
		#endregion

		// AffectingChanges overloads, returns changelists that affect the locations mapped by this clientspec
		#region AffectingChanges
		public P4Changelist[] AffectingChanges(bool includeDescriptions = false, bool includeIntegrations = false, string clientName = null, string userName = null)
		{
			return DoChanges(includeDescriptions, includeIntegrations, clientName, userName, new P4FileSpec[] { DefaultLocation }, CommonStatusArg(P4Changelist.CLStatus.submitted));
		}

		public P4Changelist[] AffectingChanges(uint max, bool includeDescriptions = false, bool includeIntegrations = false, string clientName = null, string userName = null)
		{
			return DoChanges(includeDescriptions, includeIntegrations, clientName, userName, new P4FileSpec[] { DefaultLocation }, CommonMaxArg(max).Concat(CommonStatusArg(P4Changelist.CLStatus.submitted)).ToArray());
		}

		public P4Changelist[] AffectingChanges(uint fromChangelist, uint toChangelist, bool includeDescriptions = false, bool includeIntegrations = false, string clientName = null, string userName = null)
		{
			return DoChanges(includeDescriptions, includeIntegrations, clientName, userName, new P4FileSpec[] { new P4LocationAtChangelistRange(RootPath, fromChangelist, toChangelist) }, CommonStatusArg(P4Changelist.CLStatus.submitted));
		}

		public P4Changelist[] AffectingChanges(uint max, uint fromChangelist, uint toChangelist, bool includeDescriptions = false, bool includeIntegrations = false, string clientName = null, string userName = null)
		{
			return DoChanges(includeDescriptions, includeIntegrations, clientName, userName, new P4FileSpec[] { new P4LocationAtChangelistRange(RootPath, fromChangelist, toChangelist) }, CommonMaxArg(max).Concat(CommonStatusArg(P4Changelist.CLStatus.submitted)).ToArray());
		}
		#endregion

		public P4SyncFileCollection Sync(bool force = false, bool throwOnUpToDate = false, bool throwOnNoSuchFiles = false, uint batchSize = P4Defaults.DefaultBatchSize, uint retries = P4Defaults.DefaultRetries, int timeoutMs = P4GlobalOptions.DefaultLongTimeoutMs, Action<string> updateCallback = null, params P4FileSpec[] locations)
		{
			EnsureLoggedIn();
			locations = NoLocationIfNoneSpecified(locations);

			// set up batching
			IEnumerable<IEnumerable<P4FileSpec>> batches = null;
			if (batchSize <= P4Defaults.NoBatching)
			{
				batches = new P4FileSpec[][] { locations };
			}
			else
			{
				if (batchSize == P4Defaults.DefaultBatchSize)
				{
					batchSize = P4GlobalOptions.P4BatchSize;
				}
				batches = locations.Batch(batchSize);
			}

			List<P4SyncFileCollection.SingleBatch> batchOutputs = new List<P4SyncFileCollection.SingleBatch>();
			if (batches.Count() > 0)
			{
				foreach (IEnumerable<P4FileSpec> batch in batches)
				{
					batchOutputs.Add(SyncOneBatch(force, throwOnUpToDate, throwOnNoSuchFiles, retries, batch, timeoutMs, updateCallback));
				}
			}
			else
			{
				batchOutputs.Add(SyncOneBatch(force, throwOnUpToDate, throwOnNoSuchFiles, retries, new P4FileSpec[] { }, timeoutMs, updateCallback));
			}

			return new P4SyncFileCollection(batchOutputs.ToArray());
		}

		// TODO: implement additional command line arguments such as -L, -n, -q
		public P4SyncFileCollection Flush(bool force = false, bool throwOnUpdateToDate = false, bool throwOnNoSuchFiles = false, params P4FileSpec[] locations)
		{
			try
			{
				EnsureLoggedIn();
				locations = DefaultLocationIfNoneSpecified(locations);

				List<string> args = new List<string>() { };
				if (force)
				{
					args.Add("-f");
				}
				args.AddRange(from loc in locations select loc.GetSpecString());

				P4Output output = P4Caller.Run(this, "flush", args.ToArray(), timeoutMillisec: P4GlobalOptions.DefaultLongTimeoutMs);

				P4SyncFileCollection.SingleBatch batchOutput = new P4SyncFileCollection.SingleBatch(output);
				P4SyncFileCollection.SingleBatch[] batchOutputList = new P4SyncFileCollection.SingleBatch[] { batchOutput };
				return new P4SyncFileCollection(batchOutputList);
			}
			catch (P4Exception p4Ex)
			{
				List<Type> acceptableExceptions = new List<Type>();
				if (!throwOnUpdateToDate)
				{
					acceptableExceptions.Add(typeof(FilesUpToDateException));
				}
				if (!throwOnNoSuchFiles)
				{
					acceptableExceptions.Add(typeof(NoSuchFilesException));
				}
				p4Ex.ThrowStrippedIfNotOnly(acceptableExceptions);

				P4SyncFileCollection.SingleBatch batchOutput = new P4SyncFileCollection.SingleBatch(p4Ex.LinkedOutput);
				P4SyncFileCollection.SingleBatch[] batchOutputList = new P4SyncFileCollection.SingleBatch[] { batchOutput };
				return new P4SyncFileCollection(batchOutputList);
			}
		}

		public P4EditFileCollection Edit(string changelist = null, P4File.FileType fileType = null, bool throwOnNoSuchFiles = false, params P4Location[] locations)
		{
			try
			{
				EnsureLoggedIn();
				locations = DefaultLocationIfNoneSpecified(locations);
				List<string> args = new List<string>() { };
				if (changelist != null)
				{
					args.AddRange(CommonChangelistArg(changelist));
				}
				if (fileType != null)
				{
					args.AddRange(new string[] { "-t", P4Utils.MakeFileTypeString(fileType) });
				}
				args.AddRange(from loc in locations select loc.GetSpecString());
				P4Output output = P4Caller.Run(this, "edit", args.ToArray(), timeoutMillisec: P4GlobalOptions.DefaultTimeoutMs);
				return new P4EditFileCollection(output);
			}
			catch (P4Exception p4Ex)
			{
				p4Ex.ThrowStrippedIfNotOnly(throwOnNoSuchFiles, typeof(NoSuchFilesException));
				return new P4EditFileCollection(p4Ex.LinkedOutput);
			}
		}

		public P4EditFileCollection Move(P4BaseLocation fromLocation, P4BaseLocation toLocation, string changelist = null)
		{
			try
			{
				EnsureLoggedIn();

				List<string> args = new List<string>() { };
				if (changelist != null)
				{
					args.AddRange(CommonChangelistArg(changelist));
				}
				args.Add(fromLocation.GetSpecString());
				args.Add(toLocation.GetSpecString());
				P4Output output = P4Caller.Run(this, "move", args.ToArray(), timeoutMillisec: P4GlobalOptions.DefaultTimeoutMs);
				return new P4EditFileCollection(output);
			}
			catch (P4Exception p4Ex)
			{
				return new P4EditFileCollection(p4Ex.LinkedOutput);
			}
		}

		public P4EditFileCollection Reconcile(bool throwOnNoFiles = false, string changelist = null, bool preview = false, ReconcileFilterOptions filter = ReconcileFilterOptions.Default, params P4Location[] locations)
		{
			try
			{
				EnsureLoggedIn();
				locations = DefaultLocationIfNoneSpecified(locations);
				List<string> args = new List<string>() { };
				if (changelist != null)
				{
					args.AddRange(CommonChangelistArg(changelist));
				}
				if (preview)
				{
					args.Add("-n");
				}
				if (filter.HasFlag(ReconcileFilterOptions.EditedFiles))
				{
					args.Add("-e");
				}
				if (filter.HasFlag(ReconcileFilterOptions.AddedFiles))
				{
					args.Add("-a");
				}
				if (filter.HasFlag(ReconcileFilterOptions.DeletedFiles))
				{
					args.Add("-d");
				}
				args.AddRange(from loc in locations select loc.GetSpecString());
				P4Output output = P4Caller.Run(this, "reconcile", args.ToArray(), timeoutMillisec: P4GlobalOptions.DefaultLongTimeoutMs);
				return new P4EditFileCollection(output);
			}
			catch (P4Exception p4Ex)
			{
				p4Ex.ThrowStrippedIfNotOnly(throwOnNoFiles, typeof(NoFilesToReconcileException));
				return new P4EditFileCollection(p4Ex.LinkedOutput);
			}
		}

		public P4RevertFileCollection Revert(bool throwOnNoFilesOpened = false, bool onlyUnchanged = false, bool deleteAddedFiles = false, string changeList = null, params P4Location[] locations)
		{
			try
			{
				EnsureLoggedIn();
				locations = DefaultLocationIfNoneSpecified(locations);
				List<string> args = new List<string>() { };
				if (onlyUnchanged)
				{
					args.Add("-a");
				}
				if (changeList != null)
				{
					args.AddRange(CommonChangelistArg(changeList));
				}

				// for older versions of ETBFPerforce we need to manually delete added file if required
				P4OpenedFile[] addedFiles = new P4OpenedFile[] { };
				if (deleteAddedFiles)
				{
					args.Add("-w");
				}

				args.AddRange(from loc in locations select loc.GetSpecString());
				P4Output output = P4Caller.Run(this, "revert", args.ToArray(), timeoutMillisec: P4GlobalOptions.DefaultLongTimeoutMs);

				// after revert delete added files
				foreach (P4OpenedFile addedFile in addedFiles)
				{
					string path = ClientPathToLocalPath(addedFile.ClientFile);
					File.Delete(path);
				}

				return new P4RevertFileCollection(output);
			}
			catch (P4Exception p4Ex)
			{
				p4Ex.ThrowStrippedIfNotOnly(throwOnNoFilesOpened, typeof(NoFilesOpenException));
				return new P4RevertFileCollection(p4Ex.LinkedOutput);
			}
		}

		// TODO: implement additional options such as -b, -n, -v, -q
		public P4IntegrateFileCollection Integrate(bool throwOnAlreadyIntegrated = false, string changelist = null, bool force = false, params P4FileSpec[] locations)
		{
			try
			{
				EnsureLoggedIn();
				locations = DefaultLocationIfNoneSpecified(locations);
				List<string> args = new List<string>() { };

				if (changelist != null)
				{
					args.AddRange(CommonChangelistArg(changelist));
				}
				if (force)
				{
					args.Add("-f");
				}
				args.AddRange(from loc in locations select loc.GetSpecString());
				P4Output output = P4Caller.Run(this, "integrate", args.ToArray(), timeoutMillisec: P4GlobalOptions.DefaultLongTimeoutMs);
				return new P4IntegrateFileCollection(output);
			}
			catch (P4Exception p4Ex)
			{
				p4Ex.ThrowStrippedIfNotOnly(throwOnAlreadyIntegrated, typeof(RevisionsAlreadyIntegratedException));
				return new P4IntegrateFileCollection(p4Ex.LinkedOutput);
			}
		}

		// TODO: implement additional options such as -A, -d, -f, -n, -N, -o, -t, -v, -c
		public P4ResolveFileCollection Resolve(bool throwOnNoFilesToResolve = false, AutoResolveOptions autoResolveOptions = AutoResolveOptions.Default, params P4FileSpec[] locations)
		{
			try
			{
				EnsureLoggedIn();
				locations = DefaultLocationIfNoneSpecified(locations);
				List<string> args = new List<string>() { };
				if (autoResolveOptions == AutoResolveOptions.ResolveSafe)
				{
					args.Add("-as");
				}
				else if (autoResolveOptions == AutoResolveOptions.ResolveMerge)
				{
					args.Add("-am");
				}
				else if (autoResolveOptions == AutoResolveOptions.ResolveForce)
				{
					args.Add("-af");
				}
				else if (autoResolveOptions == AutoResolveOptions.ResolveYours)
				{
					args.Add("-ay");
				}
				else if (autoResolveOptions == AutoResolveOptions.ResolveTheirs)
				{
					args.Add("-at");
				}
				args.AddRange(from loc in locations select loc.GetSpecString());
				P4Output output = P4Caller.Run(this, "resolve", args.ToArray(), timeoutMillisec: P4GlobalOptions.DefaultLongTimeoutMs);
				return new P4ResolveFileCollection(output);
			}
			catch (P4Exception p4Ex)
			{
				p4Ex.ThrowStrippedIfNotOnly(throwOnNoFilesToResolve, typeof(NoFilesToResolveException));
				return new P4ResolveFileCollection(p4Ex.LinkedOutput);
			}
		}

		private string ClientPathToLocalPath(string clientPath)
		{
			string relativePath = clientPath.Remove(0, RootPath.Trim('.').Length);
			string localPath = System.IO.Path.Combine(Root, relativePath);
			return localPath;
		}

		public P4ReopenFile[] Reopen(string changelist = null, P4File.FileType fileType = null, params P4Location[] locations)
		{
			EnsureLoggedIn();
			locations = DefaultLocationIfNoneSpecified(locations);
			List<string> args = new List<string>() { };
			if (changelist != null)
			{
				args.AddRange(CommonChangelistArg(changelist));
			}
			if (fileType != null)
			{
				args.AddRange(CommonTypeArg(fileType));
			}
			args.AddRange(from loc in locations select loc.GetSpecString());
			P4Output output = P4Caller.Run(this, "reopen", args.ToArray(), timeoutMillisec: P4GlobalOptions.DefaultLongTimeoutMs);
			return (from record in output.Records select new P4ReopenFile(record)).ToArray();
		}

		public P4OpenedFile[] OpenedFiles(string changelist = null, string user = null, params P4Location[] locations)
		{
			return DoGetOpenedFiles(changelist, Name, user, locations);
		}

		public P4OpenedFile[] OpenedFiles(uint max, string changelist = null, string user = null, params P4Location[] locations)
		{
			return DoGetOpenedFiles(changelist, Name, user, locations, CommonMaxArg(max).ToList());
		}

		public P4SubmitFileCollection Submit(string description = null, string changelist = null, bool keepFilesOpen = false, bool throwOnNoFilesToSubmit = false, params P4Location[] locations)
		{
			return DoSubmit(description, changelist, keepFilesOpen, throwOnNoFilesToSubmit, locations);
		}

		// This second overload for the submit function allows providing the submit option, when the submit option is not provided p4 makes the call on what the behaviour should be
		public P4SubmitFileCollection Submit(P4Client.SubmitOptionSet option, string description = null, string changelist = null, bool keepFilesOpen = false, bool throwOnNoFilesToSubmit = false, params P4Location[] locations)
		{
			return DoSubmit(description, changelist, keepFilesOpen, throwOnNoFilesToSubmit, locations, new List<string>() { "-f", option.ToString("g").Replace("_", "+") });
		}

		public P4EditFileCollection Add(string changelist = null, AddOptions options = AddOptions.Default, P4File.FileType fileType = null, params P4Location[] locations)
		{
			EnsureLoggedIn();
			locations = DefaultLocationIfNoneSpecified(locations);
			List<string> args = new List<string>() { };
			if (changelist != null)
			{
				args.AddRange(CommonChangelistArg(changelist));
			}
			if (options.HasFlag(AddOptions.DowngradeEditToAdd))
			{
				args.Add("-d");
			}
			if (options.HasFlag(AddOptions.ForceWildCardNames))
			{
				args.Add("-f");
			}
			if (options.HasFlag(AddOptions.IgnoreP4Ignore))
			{
				args.Add("-I");
			}
			if (fileType != null)
			{
				args.AddRange(CommonTypeArg(fileType));
			}
			args.AddRange(from loc in locations select loc.GetSpecString());
			P4Output output = P4Caller.Run(this, "add", args.ToArray(), timeoutMillisec: P4GlobalOptions.DefaultTimeoutMs);
			return new P4EditFileCollection(output);
		}

		// taking uint for changelist rahter than string here because "default" isn't valid for shelving
		public P4ShelveFile[] Shelve(uint changelist, ShelveOptionSet option = P4Defaults.DefaultShelveOption, bool overrideExisting = false, params P4Location[] locations)
		{
			EnsureLoggedIn();
			locations = NoLocationIfNoneSpecified(locations);
			List<string> args = CommonChangelistArg(changelist.ToString()).ToList();
			if (option != P4Defaults.DefaultShelveOption)
			{
				args.Add(String.Format("-a {0}", option.ToString("g")));
			}

			if (overrideExisting)
			{
				args.Add("-f");
			}
			args.AddRange(from loc in locations select loc.GetSpecString());
			P4Output output = P4Caller.Run(this, "shelve", args.ToArray(), timeoutMillisec: P4GlobalOptions.DefaultLongTimeoutMs);
			return (from record in output.Records select new P4ShelveFile(record)).ToArray();
		}

		// replaces replaces any shelved files in the changelist with the currently open files in the workspace
		// TODO: test (especially output type!) and re-enable
		/*public P4ShelveFile[] Reshelve(uint changelist, ShelveOptionSet option = P4Defaults.DefaultShelveOption)
		{
			EnsureLoggedIn();
			List<string> args = new List<string>()
			{
				String.Format("-a {0}", option.ToString("g")),
				P4ClientStub.CommonChangelistArg(changelist.ToString()),
				"-r"
			};
			P4Output output = P4Caller.Run(this, "shelve", args.ToArray());
			return (from record in output.Records select new P4ShelveFile(record)).ToArray();
		}*/

		public P4UnshelveFileCollection Unshelve(uint fromChangelist, string toChangelist = null, bool overrideWriteableFiles = false, bool throwOnNoFilesToUnshelve = false, params P4Location[] locations)
		{
			try
			{
				EnsureLoggedIn();
				locations = NoLocationIfNoneSpecified(locations);
				List<string> args = new List<string>();
				args.AddRange(CommonChangelistArg(fromChangelist, 's'));
				if (overrideWriteableFiles)
				{
					args.Add("-f");
				}
				if (toChangelist != null)
				{
					args.AddRange(CommonChangelistArg(toChangelist));
				}
				args.AddRange(from loc in locations select loc.GetSpecString());
				P4Output output = P4Caller.Run(this, "unshelve", args.ToArray(), timeoutMillisec: P4GlobalOptions.DefaultLongTimeoutMs);
				return new P4UnshelveFileCollection(output);
			}
			catch (P4Exception p4Ex)
			{
				p4Ex.ThrowStrippedIfNotOnly(throwOnNoFilesToUnshelve, typeof(NoFilesToUnshelveException));
				return new P4UnshelveFileCollection(p4Ex.LinkedOutput);
			}
		}

		public void DeleteShelvedFiles(uint changelist, bool throwOnNoShelvedFiles = false, bool throwOnNoSuchFiles = false, params P4Location[] locations)
		{
			try
			{
				EnsureLoggedIn();
				locations = NoLocationIfNoneSpecified(locations);
				List<string> args = new List<string>() { "-d" };
				args.AddRange(P4ClientStub.CommonChangelistArg(changelist.ToString()));
				args.AddRange(from loc in locations select loc.GetSpecString());
				P4Caller.Run(this, "shelve", args.ToArray(), timeoutMillisec: P4GlobalOptions.DefaultLongTimeoutMs);
			}
			catch (P4Exception p4Ex)
			{
				List<Type> acceptableExceptions = new List<Type>();
				if (!throwOnNoShelvedFiles)
				{
					acceptableExceptions.Add(typeof(NoShelvedFilesToDeleteException));
				}
				if (!throwOnNoSuchFiles)
				{
					acceptableExceptions.Add(typeof(NoSuchFilesException));
				}
				p4Ex.ThrowStrippedIfNotOnly(acceptableExceptions);
			}
		}

		public P4EditFileCollection DeleteFiles(string changelist = null, DeleteOptions options = DeleteOptions.Default, params P4Location[] locations)
        {
            EnsureLoggedIn();
            locations = DefaultLocationIfNoneSpecified(locations);
            List<string> args = new List<string>() { };
            if (changelist != null)
            {
                args.AddRange(CommonChangelistArg(changelist));
            }
            if (options.HasFlag(DeleteOptions.KeepFileLocally))
            {
                args.Add("-k");
            }
            if (options.HasFlag(DeleteOptions.DeleteUnsyncedFiles))
            {
                args.Add("v");
            }
            args.AddRange(from loc in locations select loc.GetSpecString());
			P4Output output = P4Caller.Run(this, "delete", args.ToArray(), timeoutMillisec: P4GlobalOptions.DefaultLongTimeoutMs);
			return new P4EditFileCollection(output);
        }

		// find where a perforce file is mapped to
		public P4WhereFile Where(string p4File, bool throwOnNotInView = false)
		{
			try
			{
				EnsureLoggedIn();
				P4Output output = P4Caller.Run(this, "where", new string[] { p4File }, timeoutMillisec: P4GlobalOptions.DefaultTimeoutMs);
				return new P4WhereFile(output.Records); // no lazy eval complexity here, if people are calling where we can assume they want the output
			}
			catch (P4Exception p4Ex)
			{
				p4Ex.ThrowStrippedIfNotOnly(throwOnNotInView, typeof(FilesNotInClientException));
				return null;
			}
		}

		static string PrepFStatFieldsFromFlags(FStatFields flags)
		{
			StringBuilder builder = new StringBuilder();

			if (flags == FStatFields.All)
			{
				return string.Empty;
			}

			builder.Append("-T");

			foreach (FStatFields field in Enum.GetValues(typeof(FStatFields)))
			{
				if (field == FStatFields.All)
				{
					continue;
				}

				if (flags.HasFlag(field))
				{
					string flagsName = field.ToString();
					builder.Append(",").Append(flagsName);
				}
			}

			builder[2] = ' ';

			return builder.ToString();
		}

		public P4FStatFileCollection FStat(FStatFields fields = FStatFields.All, bool includeDigestAndSize = false, params P4FileSpec[] locations)
		{
			EnsureLoggedIn();
			string fieldsString = PrepFStatFieldsFromFlags(fields);

			List<string> args = new List<string>();
			if (includeDigestAndSize)
			{
				args.Add("-Ol");
			}
			args.Add(fieldsString);
			args.AddRange(from location in locations select location.GetSpecString());

			P4Output output = P4Caller.Run(this, "fstat", args.ToArray(), timeoutMillisec: P4GlobalOptions.DefaultTimeoutMs);
			return new P4FStatFileCollection(output);
		}

		public void RevertAllAndDelete()
		{
			EnsureLoggedIn();
			Revert();
			foreach (P4Changelist changelist in ClientChanges(status: P4Changelist.CLStatus.pending))
			{
				// calling changelist.RevertAllAndDelete() would be nice from maintenance point of view
				// but we don't want the cost of calling revert per changelist since we've already done
				// a client wide revert
				changelist.DeleteShelvedFiles();
				changelist.Delete();
			}
			DeleteClient();
		}

		public static P4Client GetFullClient(P4ClientStub stub)
		{
			return stub.GetServer().GetClient(stub.Name);
		}

		internal override void EnsureLoggedIn()
		{
			Server.EnsureLoggedIn();
		}

		internal override P4Server GetServer()
		{
			return Server;
		}

		internal override List<string> BuildGlobalArgs(uint retries, string responseFilePath = null)
		{
			List<string> args = CommonGlobalArgs(retries, responseFilePath);
			args.AddRange(Server.ServerGlobalArgs());
			args.AddRange(new string[] {"-c", Name} );
			return args;
		}

		internal static string[] CommonChangelistArg(uint changeList, char prefix = 'c')
		{
			return new string[] { String.Format("-{0}", prefix), changeList.ToString() };
		}

		// if we revert unchanged on a submit and there are no file to submit then we get a dead changelist, this code cleans it up
		private void CleanUpNoFileChangelist(P4Output p4Output)
		{
			P4Record record = p4Output.Records.FirstOrDefault();
			if (record != null)
			{
				string unsubmittedChange = p4Output.Records.First().GetField("change");
				DeleteChangelist(UInt32.Parse(unsubmittedChange));
			}
		}

		private string[] CommonTypeArg(P4File.FileType fileType)
		{
			return new string[] { "-t", P4Utils.MakeFileTypeString(fileType) };
		}

		private void ThrowReadOnlyException()
		{
			throw new NotSupportedException("Cannot modify P4ClientStub please use P4Client (see static function P4ClientStub.GetFullClient).");
		}

		private P4SubmitFileCollection DoSubmit(string description, string changelist, bool keepFilesOpen, bool throwOnNoFilesToSubmit, P4Location[] locations, List<string> additionalOptions = null)
		{
			try
			{
				EnsureLoggedIn();
				locations = NoLocationIfNoneSpecified(locations);
				List<string> args = additionalOptions ?? new List<string>();

				if (changelist == null && description == null)
				{
					throw new Exception("Either provide a changelist to submit or provide a description to use for submitting the default changelist.");
				}
				else if (changelist != null && description != null)
				{
					throw new Exception("Cannot provide both a description and a changelist to p4 submit, the description is only for when submitting the default changelist.");
				}

				if (changelist != null)
				{
					args.AddRange(CommonChangelistArg(changelist));
				}
				if (description != null)
				{
					args.Add("-d");
					args.Add(description);
				}

				if (keepFilesOpen)
				{
					args.Add("-r");
				}
				args.AddRange(from loc in locations select loc.GetSpecString());
				P4Output output = P4Caller.Run(this, "submit", args.ToArray(), timeoutMillisec: P4GlobalOptions.DefaultLongTimeoutMs);
				return new P4SubmitFileCollection(output);
			}
			catch (P4Exception p4Ex)
			{
				p4Ex.ThrowStrippedIfNotOnly(throwOnNoFilesToSubmit, typeof(NoFilesToSubmitException));
				CleanUpNoFileChangelist(p4Ex.LinkedOutput);
				return new P4SubmitFileCollection(p4Ex.LinkedOutput, didSubmit: false);
			}
		}

		private P4SyncFileCollection.SingleBatch SyncOneBatch(bool force, bool throwOnUpToDate, bool throwOnNoSuchFiles, uint retries, IEnumerable<P4FileSpec> batch, int timeoutMs = P4GlobalOptions.DefaultLongTimeoutMs, Action<string> updateCallback = null)
		{
			List<string> arguments = new List<string>();
			if (force)
			{
				arguments.Add("-f");
			}
			arguments.AddRange(from loc in batch select loc.GetSpecString());
			try
			{
				using (SyncTaskOutputMonitor taskOutputMonitor = updateCallback != null ? new SyncTaskOutputMonitor(updateCallback, arguments.ToArray()) : null)
				{
					P4Output output = P4Caller.Run
					(
						this,
						"sync",
						arguments.ToArray(),
						responseFileBehaviour: ResponseFileBehaviour.AlwaysUse,
						retries: retries,
						timeoutMillisec: timeoutMs,
						taskOutputMonitor: taskOutputMonitor
					);
					return new P4SyncFileCollection.SingleBatch(output);
				}
			}
			catch (P4Exception p4Ex)
			{
				List<Type> acceptableExceptions = new List<Type>();
				if (!throwOnUpToDate)
				{
					acceptableExceptions.Add(typeof(FilesUpToDateException));
				}
				if (!throwOnNoSuchFiles)
				{
					acceptableExceptions.Add(typeof(NoSuchFilesException));
				}
				p4Ex.ThrowStrippedIfNotOnly(acceptableExceptions);
				return new P4SyncFileCollection.SingleBatch(p4Ex.LinkedOutput);
			}
		}
	}
}
