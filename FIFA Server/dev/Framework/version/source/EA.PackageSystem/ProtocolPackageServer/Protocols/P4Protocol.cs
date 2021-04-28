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
using System.Runtime.ExceptionServices;
using System.Xml.Serialization;

using NAnt.Core;
using NAnt.Core.Logging;
using NAnt.Core.PackageCore;
using NAnt.Core.Tasks;
using NAnt.Core.Util;
using NAnt.Perforce;
using NAnt.Perforce.ConnectionManagment;

namespace EA.PackageSystem
{
	public class P4Protocol : ProtocolBase
	{
		public const string LogPrefix = "Protocol package server (p4): ";
		public const string SyncMarkerFilename = "p4protocol.sync";
		public const string SyncManifestFilename = "p4protocol.manifest";
		public const string SyncResumeInfoFilename = "p4protocol.resumesync";

		public const uint UseHeadCL = 0;

		private P4QueryData ParseQueryString(Uri uri, ProtocolPackageServer protocolServer)
		{
			P4QueryData queryData = P4QueryData.Parse(uri);
			if (queryData.Changelist == P4QueryData.UseHeadCL)
			{
				DoOnce.Execute(uri.ToString(), () =>
				{
					protocolServer.Log.ThrowWarning(Log.WarningId.MasterconfigURIHead, Log.WarnLevel.Normal, "The p4 uri string, '{0}', defaulted or lists the revision as #head. This will require an up-to-date check everytime Framework runs. Please specify a specific cl.", uri.ToString());
				});
			}
			return queryData;
		}

		// will throw if package is invalid
		private void ValidateP4Package(string name, string version, Uri uri, ProtocolPackageServer protocolServer)
		{
			P4QueryData queryData = ParseQueryString(uri, protocolServer);
			P4Server server = ConnectionManager.AttemptP4Connect(uri.Authority, protocolServer.Project, out P4Exception connectionError, defaultCharSet: queryData.CharSet);
			if (server == null)
			{
				ExceptionDispatchInfo.Capture(connectionError).Throw(); // rethrow without modifying stack trace
			}

			// check package has manifest
			string absolutePath = queryData.BuildP4AbsolutePath(uri, name, version);
			string reportingLocation = String.Format("{0}, {1}", server.Port, absolutePath);
			uint cl = queryData.Changelist;
			string p4ManifestLocation = P4Utils.Combine(absolutePath, "/Manifest.xml") + (cl != 0 ? String.Format("@{0}", cl) : String.Empty);
			P4FileSpec manifestFileSpec = P4FileSpec.Parse(p4ManifestLocation);
			if (!P4Utils.LocationExists(server, fileSpecs: manifestFileSpec))
			{
				throw new BuildException(String.Format("The package {0}-{1} was not found at {2}. Make sure you have read permissions to this location. Package must have a Manifest.xml file at this location to be valid. If this is a Framework 1 package it will need to be updated if you wish to use uri specification.", name, version, reportingLocation));
			}

			string manifestContents = server.Print(fileSpecs: manifestFileSpec).First().Contents;

			// check manifest versionName and packageName matches request version in masterconfig
			// we want to verify that the p4 path being synced is actually the package being specified
			Manifest manifest;
			try
			{
				// We expects the file to be an XML file.  In case there are any encoding issues, we
				// skip any characters that doesn't start with XML tag start '<' character.  
				// The XmlDocument.LoadXml would throw an error if input is not a valid XML.
				if (manifestContents[0] != '<')
				{
					int trimNumChars = 0;
					while (trimNumChars < manifestContents.Length)
					{
						if (manifestContents[trimNumChars] == '<')
						{
							break;
						}
						++trimNumChars;
					}
					if (trimNumChars > 0 &&
						trimNumChars != manifestContents.Length)
					{
						manifestContents = manifestContents.Substring(trimNumChars);
					}
				}
				manifest = Manifest.Parse(manifestContents, reportingLocation);
			}
			catch (Exception e)
			{
				string errMsg = String.Format("Error parsing manifest.xml at '{0}'.{1}The file has the following content:{1}{2}{1}{1}Exception details: {3}{1}",
									p4ManifestLocation ?? "(null)",
									Environment.NewLine,
									(manifestContents ?? "(null)").Replace('\r', '\n'), // The "Contents" property replaced all the \n with \r.  So un-doing that.
									e);
				throw new BuildException(errMsg);
			}

			// If the package version field in the manifest is provided ensure
			// that it matches what is specified in the masterconfig. Missing,
			// or empty, versionName fields skip this check because it assumes
			// the package author does not want to enforce version checking.
			if (!manifest.Version.IsNullOrEmpty() && manifest.Version != version)
			{
				throw new BuildException(String.Format("Package manifest.xml found at {0} has a mismatched version. Version requested in masterconfig was '{1}', version found was '{2}'. Check that uri in masterconfig is referencing correct location.", p4ManifestLocation, version, manifest.Version));
			}

			if (!manifest.PackageName.IsNullOrEmpty() && manifest.PackageName != name)
			{
				throw new BuildException(String.Format("Package manifest.xml found at {0} has a mismatched package name. Name requested in masterconfig was '{1}', packageName field in the manifest was '{2}'. Check that uri in masterconfig is referencing correct location.", p4ManifestLocation, name, manifest.PackageName));
			}

			// Check existence of the SyncMarkerFilename.  We don't want to see it in perforce.
			string p4protocolSyncFile = P4Utils.Combine(absolutePath, SyncMarkerFilename) + (cl != 0 ? String.Format("@{0}", cl) : String.Empty);
			P4FileSpec p4protocolSyncFileSpec = P4FileSpec.Parse(p4protocolSyncFile);
			if (P4Utils.LocationExists(server, fileSpecs: p4protocolSyncFileSpec))
			{
				throw new BuildException(String.Format("The package {0}-{1} must not contain {2} file in the package folder in Perforce.  This file must not be checked into perforce as it will cause automated Perforce syncs of this package to fail.", name, version, SyncMarkerFilename));
			}
		}

		public override Release Install(MasterConfig.IPackage package, Uri uri, string rootDirectory, ProtocolPackageServer protocolServer)
		{
			P4QueryData requestedQueryData = ParseQueryString(uri, protocolServer);
			P4Server server = ConnectionManager.AttemptP4Connect(uri.Authority, protocolServer.Project, out P4Exception connectionError, defaultCharSet: requestedQueryData.CharSet);
			if (server == null)
			{
				ExceptionDispatchInfo.Capture(connectionError).Throw(); // if we could connect (i.e server = null) then this be the most relevant exception so we should throw it
																		// without modifying stack trace
			}

			ValidateP4Package(package.Name, package.Version, uri, protocolServer);
			string releasePath = SyncReleaseAtCl(package.Name, package.Version, rootDirectory, uri, server, requestedQueryData, 0, protocolServer);

			return new Release(package, releasePath, isFlattened: false, manifest: Manifest.Load(releasePath, protocolServer.Log));
		}

		public override void Update(Release release, Uri uri, string installRoot, ProtocolPackageServer protocolServer)
		{
			Update(release, uri, installRoot, protocolServer, forceCrossServerSync: false);
		}

		// this exists as a separate overload is only for testing purposes
		internal void Update(Release release, Uri uri, string installRoot, ProtocolPackageServer protocolServer, bool forceCrossServerSync)
		{
			protocolServer.Log.Debug.WriteLine(ProtocolPackageServer.LogPrefix + LogPrefix + "Checking p4 protocol package {0} is up to date...", release.Name);
			P4Server server = null;

			// check we have marker file, if not we need to resync no matter what
			bool resync = false;
			bool canIncrementalSync = false;
			bool serverDifference = false;
			string markerFileLocation = Path.Combine(release.Path, SyncMarkerFilename);
			uint requestedCL = ParseQueryString(uri, protocolServer).Changelist;

			P4Exception connectionError;
			if (TryReadMarkerFile(markerFileLocation, out uint previouslyRequestedCL, out uint previouslySyncedCL, out string previouslyRequestedServer, out string previouslyCentralServer, out string previouslyRequestedPath))
			{
				string storePath = '/' +
				(
					ParseQueryString(uri, protocolServer).Rooted ?
						uri.AbsolutePath.TrimEnd(new char[] { '/' }) + '/' + release.Name + '/' + release.Version :
						uri.AbsolutePath.TrimEnd(new char[] { '/' })
				)
				.TrimStart(new char[] { '/' });

				if (previouslyRequestedServer != uri.Authority && previouslyCentralServer != uri.Authority)
				{
					serverDifference = true;
				}

				if (previouslyRequestedPath != storePath) // different path than previous sync
				{
					resync = true;
				}
				else if (serverDifference) // check if server matches previous server or if it has the same central server
				{
					// the requested server does not match the last server we sync'd from OR the cetnral 
					// server that that corresponds to - user has changed to point a different server 
					// that might still be a proxy for the same central server - so attempt to connect
					// (resolving optimal proxy) and check for equivalence
					server = ConnectionManager.AttemptP4Connect(uri.Authority, protocolServer.Project, out connectionError, defaultCharSet: ParseQueryString(uri, protocolServer).CharSet);
					if (server == null)
					{
						// we can't connect to best server, and we don't know if this is same sever because we can't
						// connect, we have to fail here (even though user might actually not need to do anything)
						ExceptionDispatchInfo.Capture(connectionError).Throw(); // rethrow without modifying stack trace
					}

					string newCentralAddress = server.ResolveToCentralAddress(resolveThroughEdgeServers: true);
					if (newCentralAddress != previouslyCentralServer)
					{
						resync = true;
					}
				}

				if (!resync)
				{
					// if requested was head cl we need to find out what head cl is
					if (requestedCL == UseHeadCL)
					{
						if (server == null) // if we had to resolve head cl we already have a valid server
						{
							server = ConnectionManager.AttemptP4Connect(uri.Authority, protocolServer.Project, out connectionError, defaultCharSet: ParseQueryString(uri, protocolServer).CharSet);
							if (server == null)
							{
								if (connectionError is P4Exception && (connectionError as P4Exception).Flatten().OfType<CredentialException>().Any())
								{
									// if this was a credential error then it cannot be a connection error, we don't want users to accidentally continue
									// thinking they're packages are up to date due to bad credentials so throw this error
									ExceptionDispatchInfo.Capture(connectionError).Throw(); // rethrow without modifying stack trace
								}

								protocolServer.Log.Warning.WriteLine("Unable to login using request p4 protocol package server '{0}'", uri.Authority);
								protocolServer.Log.Warning.WriteLine("Package '{0}' was set to head revision in the masterconfig uri field but it can't be verified to be up-to-date because the server is unavailable. Framework will attempting to use the last synced version if it is available locally.", release.Name);
								return;
							}
						}

						// get most recent cl number
						string absolutePath = ((P4QueryData)ParseQueryString((Uri)uri, (ProtocolPackageServer)protocolServer)).BuildP4AbsolutePath(uri, release.Name, release.Version);
						string p4Location = P4Utils.Combine(absolutePath, "...");
						P4Changelist[] retrievedP4changes = server.Changes(1, P4Changelist.CLStatus.submitted, fileSpecs: new P4Location(p4Location));
						if (!retrievedP4changes.Any())
						{
							resync = true;
							protocolServer.Log.Warning.WriteLine("Unable to retrieve p4changes information for p4 file spec '{0}'.  Will attempt to resync this path.", p4Location);
						}
						else
						{
							requestedCL = retrievedP4changes.First().Changelist;
							if (
								requestedCL < previouslySyncedCL || // request cl was older than what we synced, we have to resync to an older cl
								requestedCL > previouslyRequestedCL // request cl is newer than what was previously *requested* which means there might have been a new checkin we need to sync
							)
							{
								canIncrementalSync = true; // this is the same package we sync before, we just have the wrong cl
								resync = true;
								protocolServer.Log.Debug.WriteLine("Updating package {0} from CL{1} to CL{2} because head revision was specified in uri: {3}.", release.Name, previouslySyncedCL, requestedCL, uri.ToString());
							}
						}
					}
					else
					{
						if (
							requestedCL < previouslySyncedCL || // request cl was older than what we synced, we have to resync to an older cl
							requestedCL > previouslyRequestedCL // request cl is newer than what was previously *requested* which means there might have been a new checkin we need to sync
						)
						{
							canIncrementalSync = true; // this is the same package we sync before, we just have the wrong cl
							resync = true;
						}
					}
				}
			}
			else
			{
				// no marker file, resync
				resync = true;
			}

			// if resync is true then we need to sync the package this might incremental, full or resume depending on package state
			if (resync)
			{
				if (server == null) // if we had to resolve head cl we already have a valid server
				{
					server = ConnectionManager.AttemptP4Connect(uri.Authority, protocolServer.Project, out connectionError, defaultCharSet: ParseQueryString(uri, protocolServer).CharSet);
					if (server == null)
					{
						ExceptionDispatchInfo.Capture(connectionError).Throw(); // rethrow without modifying stack trace
						return;
					}
				}

				ValidateP4Package(release.Name, release.Version, uri, protocolServer);
				SyncReleaseAtCl(release.Name, release.Version, installRoot, uri, server, ParseQueryString(uri, protocolServer), canIncrementalSync ? previouslySyncedCL : 0, protocolServer, forceCrossServerSync, serverDifference);

			}
			else
			{
				protocolServer.Log.Debug.WriteLine(ProtocolPackageServer.LogPrefix + LogPrefix + "{0} - up to date!", release.Name);
			}
		}

		public class SyncManifestEntry
		{
			public string m_fileName;
			public string m_digest;

			public SyncManifestEntry()
			{
				// For serialization purposes only.
			}

			public SyncManifestEntry(string filename, string digest)
			{
				m_fileName = filename;
				m_digest = digest;
			}

			public override string ToString()
			{
				return "{" + m_fileName + ", " + m_digest + "}";
			}

			public override bool Equals(object obj)
			{
				SyncManifestEntry other = obj as SyncManifestEntry;

				if (other == null)
				{
					return false;
				}

				return other.m_fileName == m_fileName && other.m_digest == m_digest;
			}

			public override int GetHashCode()
			{
				return Convert.ToInt32(m_digest.Substring(0, 8), 16);
			}
		}

		private static string MakeRelativePathRootFromPackageSpec(P4LocationAtChangeList packageSpec)
		{
			// MakeRelativePathRootFromPackageSpec leaves a trailing slash at the
			// end of the filename which simplifies path combination operations
			// later.
			string spec = packageSpec.Path;
			return spec.Substring(0, spec.IndexOf("..."));
		}

		private static string MakeRelativePath(string relativePathRoot, string depotFile)
		{
			return depotFile.Substring(relativePathRoot.Length);
		}

		private static SyncManifestEntry[] GenerateSyncManifest(P4Client client, P4LocationAtChangeList packageSpec)
		{
			FStatFields fields = FStatFields.headAction | FStatFields.depotFile | FStatFields.digest;
			P4FStatFileCollection fstatResults = client.FStat(fields, true, packageSpec);

			List<SyncManifestEntry> manifestEntries = new List<SyncManifestEntry>();

			string relativePathRoot = MakeRelativePathRootFromPackageSpec(packageSpec);

			foreach (P4FStatFile fstatResult in fstatResults)
			{
				// Deleted files do not have a digest and accessing that field
				// will result in an exception being thrown.
				if (fstatResult.HeadAction.Contains("delete"))
				{
					continue;
				}

				string relativePath = MakeRelativePath(relativePathRoot, fstatResult.DepotFile);
				SyncManifestEntry manifestEntry = new SyncManifestEntry(relativePath, fstatResult.Digest);
				manifestEntries.Add(manifestEntry);
			}

			manifestEntries.Sort((a, b) => string.Compare(a.m_fileName, b.m_fileName));

			return manifestEntries.ToArray();
		}

		private static void ClearReadOnlyFlag(string fileName)
		{
			if (!File.Exists(fileName))
				return;

			FileAttributes attributes = File.GetAttributes(fileName);
			FileAttributes newAttributes = (FileAttributes)((int)attributes & ~(int)FileAttributes.ReadOnly);
			File.SetAttributes(fileName, newAttributes);
		}

		private static void SerializeSyncManifestEntries(string file, SyncManifestEntry[] syncManifestEntries)
		{
			XmlSerializer serializer = new XmlSerializer(typeof(SyncManifestEntry[]));
			using (TextWriter writer = new StreamWriter(file))
			{
				serializer.Serialize(writer, syncManifestEntries);
			}
		}

		private static SyncManifestEntry[] DeserializeSyncManifestEntries(string file)
		{
			XmlSerializer serializer = new XmlSerializer(typeof(SyncManifestEntry[]));
			using (TextReader reader = new StreamReader(file))
			{
				return (SyncManifestEntry[])serializer.Deserialize(reader);
			}
		}

		private static P4FileSpec[] GenerateFileSpecs(P4LocationAtChangeList packageSpec, SyncManifestEntry[] filesToSync)
		{
			P4FileSpec[] fileSpecs = new P4FileSpec[filesToSync.Length];
			uint changelist = packageSpec.Changelist;
			string path = packageSpec.Path;
			string packageRoot = MakeRelativePathRootFromPackageSpec(packageSpec);

			for (int i = 0; i < filesToSync.Length; ++i)
			{
				string file = packageRoot + filesToSync[i].m_fileName;
				fileSpecs[i] = new P4LocationAtChangeList(file, changelist);
			}

			return fileSpecs;
		}

		private void AttemptIncrementalCrossServerSync
		(
			ProtocolPackageServer protocolServer,
			P4Server server,
			P4LocationAtChangeList packageSpec,
			P4Client syncClient,
			string packageName,
			string packageVersion,
			string tempSyncPath,
			string finalDir,
			int syncTimeout,
			SyncManifestEntry[] existingSyncManifest,
			SyncManifestEntry[] syncManifest,
			bool doingSyncResume = false,
			string syncResumeInfoFile = null)
		{
			if (existingSyncManifest != null && syncManifest != null)
			{
				// An incremental update can only be performed if we have sync
				// manifests for the old version.

				// If we have two sets of files, the set we have (A), and the
				// set we want to sync (B), the operations we need to end up
				// with B we need to delete the files that are in A but not B
				// and then force sync the files that are in B but not A.

				// Constituent members of these sets are keyed on both the
				// file name and the digest, so even if a file exists by name
				// it would end up in set B if its content differs.
				SyncManifestEntry[] filesToDelete = existingSyncManifest.Except(syncManifest).ToArray();
				SyncManifestEntry[] filesToSync = syncManifest.Except(existingSyncManifest).ToArray();

				// If we delete everything and re-sync everything, skip over
				// this code path and just go the force-sync route.
				if (filesToDelete.Length != existingSyncManifest.Length
					&& filesToSync.Length != syncManifest.Length)
				{
					try
					{
						if (!doingSyncResume)
						{
							// During sync resume, directory is already moved!
							PathUtil.MoveDirectory(finalDir, tempSyncPath);
							if (syncResumeInfoFile != null)
							{
								WriteSyncResumeFile(syncResumeInfoFile, syncClient.Name, syncClient.Server.ServerAddress);
							}
						}
					}
					catch (Exception)
					{
						protocolServer.Log.Error.WriteLine("Unable to move package directory {0} for incremental sync attempt.", finalDir);
						protocolServer.Log.Error.WriteLine("Check if any files or directories are open and re-attempt.");

						throw;
					}

					try
					{
						foreach (SyncManifestEntry entry in filesToDelete)
						{
							string fileName = Path.Combine(tempSyncPath, entry.m_fileName);

							ClearReadOnlyFlag(fileName);
							File.Delete(fileName);
						}

						P4FileSpec[] syncFileSpecs = GenerateFileSpecs(packageSpec, filesToSync);
						if (syncFileSpecs.Any())
						{
						protocolServer.Log.Minimal.WriteLine("Attempting incremental sync{5} of {0}-{1} from '{2}:{3}' to '{4}'.", packageName, packageVersion, server.Port, packageSpec.ToString(), finalDir, doingSyncResume ? " resume" : "");
						syncClient.Sync(force: !doingSyncResume, locations: syncFileSpecs, throwOnNoSuchFiles: true, timeoutMs: syncTimeout);
						}
						else
						{
							protocolServer.Log.Status.WriteLine("Cross server incremental sync detected no differences in file digests. Not syncing.");
						}

						return;
					}
					catch (Exception e)
					{
						// If an error during the above operation was encountered,
						// delete the temp sync directory and fall through to the
						// full sync code.
						protocolServer.Log.Error.WriteLine("Unable to perform incremental sync as an exception was thrown:");
						protocolServer.Log.Error.WriteLine(ProtocolPackageServer.LogPrefix + LogPrefix + e.Message);
						protocolServer.Log.Error.WriteLine("Attempting full sync.");

						PathUtil.DeleteDirectory(tempSyncPath);
					}
				}
			}

			// Fallback path is to execute a full sync.
			if (!Directory.Exists(tempSyncPath))
			{
				// We triggered a full resync
				Directory.CreateDirectory(tempSyncPath);
				if (syncResumeInfoFile != null)
				{
					WriteSyncResumeFile(syncResumeInfoFile, syncClient.Name, syncClient.Server.ServerAddress);
				}
				doingSyncResume = false;
			}

			protocolServer.Log.Minimal.WriteLine("{5}yncing {0}-{1} from '{2}:{3}' to '{4}'.", packageName, packageVersion, server.Port, packageSpec.ToString(), finalDir, doingSyncResume ? "Resume s" : "S");
			syncClient.Sync(force: !doingSyncResume, locations: packageSpec, throwOnNoSuchFiles: true, timeoutMs: syncTimeout);
		}

		private string SyncReleaseAtCl(string packageName, string packageVersion, string rootDirectory, Uri uri, P4Server server, P4QueryData requestedQueryData, uint previousCL, ProtocolPackageServer protocolServer, bool forceCrossServerSync = false, bool serverDifference = false)
		{
			uint requestedCL = requestedQueryData.Changelist;

			// create temp directory for syncing
			string p4tempRoot = Path.Combine(rootDirectory, ".p4");

			// Make this temp sync package folder deterministic across different run.
			string tempUniqueSubFolderName = BuildP4SyncFolderHash(server.ServerAddress + uri.AbsolutePath + packageName + packageVersion);
			string tempPackageRoot = Path.Combine(p4tempRoot, tempUniqueSubFolderName);
			string tempSyncPath = Path.Combine(tempPackageRoot, packageVersion);
			string finalDir = Path.Combine(rootDirectory, packageName, packageVersion);
			string syncManifestFile = Path.Combine(finalDir, SyncManifestFilename);
			SyncManifestEntry[] existingSyncManifest = null;
			SyncManifestEntry[] syncManifest = null;

			// Check if we have a previous sync abort and currently doing a sync resume.
			bool doingSyncResume = false;		// This boolean means we're currently resuming previous sync operation.
			string syncResumeClientName = null;
			string syncResumeServerName = null;
			string syncResumeInfoFile = null;	// Non-null status also means current process allows sync resume operation
			// (meaning create resume info file, and not do client name cleanup if sync isn't completed normally)
			bool allowSyncResume = protocolServer.Project.Properties.GetBooleanPropertyOrDefault("nant.p4protocol.allowsyncresume", true);
			if (allowSyncResume)
			{
				syncResumeInfoFile = Path.Combine(tempSyncPath, SyncResumeInfoFilename);
				doingSyncResume = TryReadSyncResumeFile(syncResumeInfoFile, out syncResumeClientName, out syncResumeServerName);
				if (doingSyncResume && syncResumeServerName != server.ServerAddress)
				{
					doingSyncResume = false;
					// User might have manually modified something (switching server in masterconfig) or we got collision with the hash code?
					File.Delete(syncResumeInfoFile);
				}
			}

			// Remove existing temp sync folder if we're not doing a sync resume.
			if (Directory.Exists(tempSyncPath) && !doingSyncResume)
			{
				PathUtil.DeleteDirectory(tempSyncPath);
			}

			// try and load manifest file if one exists, throw specific error message on failure
			try
			{
				if (File.Exists(syncManifestFile))
				{
					existingSyncManifest = DeserializeSyncManifestEntries(syncManifestFile);
				}
			}
			catch (Exception ex)
			{
				existingSyncManifest = null;
				throw new BuildException(String.Format("Error deserializing '{0}': {1} This file may be corrupt - try deleting it and running again.", syncManifestFile, ex.Message), ex);
			}

			try
			{
				string absolutePath = requestedQueryData.BuildP4AbsolutePath(uri, packageName, packageVersion);
				string p4Location = P4Utils.Combine(absolutePath, "...");

				// uses changes to get sync cl as getting it from sync won't include delete-only cls
				P4FileSpec changeSpec = (requestedCL == UseHeadCL) ? (P4FileSpec)(new P4Location(p4Location)) : new P4LocationAtChangeList(p4Location, requestedCL);
				uint syncedCL = server.Changes(1, P4Changelist.CLStatus.submitted, fileSpecs: changeSpec).First().Changelist;

				// if head cl was request treat it as if they requested last affecting cl
				requestedCL = requestedCL == UseHeadCL ? syncedCL : requestedCL;

				// If the requested CL is newer than the last submitted CL, throw an error (unless we already sync'd the right version, since for ease of
				// integration we don't want to throw an error if the user had already sync'd the right version before we added this error check)
				if (requestedCL > syncedCL && previousCL != syncedCL)
				{
					throw new BuildException(
						string.Format("Cannot sync package {0} to changelist {1}, as its highest submitted changelist is {2}. Please edit your masterconfig to sync this package at a changelist that exists for this package.",
						packageName, requestedCL, syncedCL));
				}

				P4LocationAtChangeList packageSpec = new P4LocationAtChangeList(p4Location, syncedCL);

				Dictionary<string, string> singlePackageViewMap = new Dictionary<string, string> 
				{
					{ p4Location, String.Format("//...") }
				};

				int syncTimeout = P4GlobalOptions.DefaultLongTimeoutMs;
				if (protocolServer.Project != null)
				{
					string newTimeout = protocolServer.Project.GetPropertyOrDefault("nant.p4protocol.timeout.ms", null);
					if (!string.IsNullOrEmpty(newTimeout))
					{
						syncTimeout = Int32.Parse(newTimeout);
					}
				}

				P4Client syncClient = null;
				string syncClientName = doingSyncResume ? syncResumeClientName : server.CreateTempClientName("Nant_P4");
				if (doingSyncResume)
				{
					try
					{
						syncClient = server.GetClient(syncClientName);
					}
					catch
					{
						// If we failed to retrieve expected client name, it is possible that user tried to do some
						// clean up an accidentally deleted the client name for sync resume.  Let's revert back to
						// sync full package.
						protocolServer.Log.Info.WriteLine("Sync resume using client '{0}' failed.  Attempting full sync workflow ..." ,syncClientName);
						syncClient = null;	// Probably don't need this line.  But just in case.
						doingSyncResume = false;
						File.Delete(syncResumeInfoFile);
					}
				}

				if (syncClient == null)
				{
					try
					{
						P4Client.LineEndOption clientLineEndOption = P4Client.DefaultLineEnd;
						if (protocolServer.Project != null)
						{
							string clientLineEndOptionStr = protocolServer.Project.GetPropertyOrDefault("nant.p4protocol.client.lineendoption", null);
							if (!string.IsNullOrEmpty(clientLineEndOptionStr))
							{
								if (Enum.TryParse(clientLineEndOptionStr, true, out clientLineEndOption))
								{
								}
								else
								{
									List<string> validP4ClientLineEndOptions = Enum.GetValues(typeof(P4Client.LineEndOption)).Cast<P4Client.LineEndOption>().ToList().ConvertAll(d => d.ToString().ToLower());
									throw new BuildException(string.Format("Invalid Perforce client line option specified via the 'nant.p4protocol.client.lineendoption' property! The specified lineendoption was '{0}' and the valid lineendoptions are '{1}'", clientLineEndOptionStr, string.Join(" ", validP4ClientLineEndOptions)));
								}
							}
						}
						bool allowReadOnlyClient = protocolServer.Project.Properties.GetBooleanPropertyOrDefault("nant.p4protocol.allowreadonlyclient", true);
						syncClient = server.CreateClient(syncClientName, tempSyncPath, singlePackageViewMap, null, clientLineEndOption, P4Client.DefaultSubmitOptions, allowReadOnlyClient ? P4Client.ClientTypes.clienttype_readonly : P4Client.DefaultClientType);
					}
					catch
					{
						// We went into a situation where CreateClient thrown an exception (due to time out) but
						// the client actually got created by p4.  So we're testing for it.
						if (server.ClientExists(syncClientName))
						{
							syncClient = server.GetClient(syncClientName);
						}
						if (syncClient == null)
						{
							// re-throw original exception message about failed to create client.  If we failed somewhere
							// down the road, we'll get client doesn't exist and that error message doesn't truely tell
							// us where this problem came from.  So after testing for about special use case, we re-throw
							// original exception.
							throw;
						}
					}
				}

				try
				{
					syncManifest = GenerateSyncManifest(syncClient, packageSpec);

					if (doingSyncResume)
					{
						if (!VerifyClientRootAndViewMap(syncClient, tempSyncPath, singlePackageViewMap, protocolServer.Log))
						{
							throw new BuildException(
								string.Format("Encountered error resuming previous sync.  Sync client indicated in '{0}' seems to have different sync root or view map.  To get around this problem, you can try doing a full resync by removing the temp directory '{1}'.", syncResumeInfoFile, tempSyncPath));
						}
					}

					// Note that if sync resume is happening, previousCL will always be 0 because the finalDir has already been moved to tmp dir.
					// So Framework will think package hasn't been installed and trigger an "Install" function call which set previousCL to 0.
					// However, since during sync resume, we're re-using previous client, we should still be OK.
					// Also we only want to enter this block if there is a server difference, to reduce the chance of hitting issues in this complicated section of code.
					// So when the path is different we ensure that it forces a resync.
					if ((previousCL == 0 && serverDifference) || forceCrossServerSync)
					{
						try
						{
							// Writing out of the temp sync resume info file will be done inside the following function.  It needs to be done AFTER the MoveDirectory
							// call or it will trip up the MoveDirectory.
							AttemptIncrementalCrossServerSync(protocolServer, server, packageSpec, syncClient, packageName, packageVersion, tempSyncPath, finalDir, syncTimeout, existingSyncManifest, syncManifest, doingSyncResume, syncResumeInfoFile);
						}
						catch
						{
							// if incremental fails for any reason, start over with full sync
							protocolServer.Log.Info.WriteLine
							(
								"Incremental sync failed. Retrying full sync: {0}-{1} from '{2}:{3}' to '{4}'.",
								packageName, packageVersion, server.Port, packageSpec.ToString(), finalDir + Path.DirectorySeparatorChar + "..."
							);
							ForceFullSync(protocolServer, server, packageSpec, syncClient, packageName, packageVersion, tempSyncPath, finalDir, syncTimeout, allowSyncResume, syncResumeInfoFile);
						}
					}
					else if (previousCL == 0 && !doingSyncResume)
					{
						// if previousCL is 0 then we will do a full sync rather than an incremental sync
						ForceFullSync(protocolServer, server, packageSpec, syncClient, packageName, packageVersion, tempSyncPath, finalDir, syncTimeout, allowSyncResume, syncResumeInfoFile);
					}
					else
					{
						try
						{
							if (!doingSyncResume)
							{
								PathUtil.MoveDirectory(finalDir, tempSyncPath);

								// Write out current client/server info so that we can do sync resume
								if (allowSyncResume)
								{
									WriteSyncResumeFile(syncResumeInfoFile, syncClient.Name, syncClient.Server.ServerAddress);
								}
							}

							// force flush the revision our marker file says we have synced for update the version
							// p4 server thinks we have to be accurate, then perform incremental sync
							P4LocationAtChangeList previousSpec = new P4LocationAtChangeList(p4Location, previousCL);
							protocolServer.Log.Minimal.WriteLine("Incremental syncing {0}-{1} from '{2}:{3}' to '{4}'. Previously synced from {5}.", packageName, packageVersion, server.Port, packageSpec.ToString(), finalDir + Path.DirectorySeparatorChar + "...", previousSpec.ToString());
							if (!doingSyncResume)
							{
								syncClient.Flush(force: true, locations: previousSpec, throwOnNoSuchFiles: true);
							}
							
							syncClient.Sync(locations: packageSpec, throwOnNoSuchFiles: true, timeoutMs: syncTimeout, updateCallback: GetSyncCallback(protocolServer));
						}
						catch
						{
							// if incremental fails for any reason, start over with full sync
							protocolServer.Log.Info.WriteLine
							(
								"Incremental sync failed. Retrying full sync: {0}-{1} from '{2}:{3}' to '{4}'.",
								packageName, packageVersion, server.Port, packageSpec.ToString(), finalDir + Path.DirectorySeparatorChar + "..."
							);
							ForceFullSync(protocolServer, server, packageSpec, syncClient, packageName, packageVersion, tempSyncPath, finalDir, syncTimeout, allowSyncResume, syncResumeInfoFile);
						}
					}

					// If we got here, we must have finished the sync.  Clean up the sync resume info.
					if (doingSyncResume)
					{
						doingSyncResume = false;
					}

					if (!String.IsNullOrEmpty(syncResumeInfoFile))
					{
						if (File.Exists(syncResumeInfoFile))
						{
							File.Delete(syncResumeInfoFile);
						}
					}
				}
				finally
				{
					if (String.IsNullOrEmpty(syncResumeInfoFile) ||		// test for sync resume feature not enabled
						!File.Exists(syncResumeInfoFile))				// test for successful sync if sync resume was enabled
					{
						syncClient.RevertAllAndDelete();
					}
				}

				UpdateManifestFileBaseVersion(packageName, packageVersion, requestedCL, protocolServer, tempSyncPath);

				// move synced package to destination, try every seconds for 30s
				// to give slow machines with aggressive AV checker time to fully
				// scan the newly download package before we move it
				if (Directory.Exists(finalDir))
				{
					PathUtil.DeleteDirectory(finalDir);
				}
				PathUtil.MoveDirectory(tempSyncPath, finalDir, iterations: 30, waitMs: 1000);

				// put a marker file in the package that lets us know what we last sync'd to
				string storePath = '/' +
				(
					requestedQueryData.Rooted ?
						uri.AbsolutePath.TrimEnd(new char[] { '/' }) + '/' + packageName + '/' + packageVersion:
						uri.AbsolutePath.TrimEnd(new char[] { '/' })
				)
				.TrimStart(new char[] { '/' });

				WriteMarkerFile(Path.Combine(finalDir, SyncMarkerFilename), requestedCL, syncedCL, uri.Authority, server.ResolveToCentralAddress(resolveThroughEdgeServers: true), storePath);

				if (syncManifest != null)
				{
					SerializeSyncManifestEntries(syncManifestFile, syncManifest);
				}

				return finalDir;
			}
			catch (Exception e)
			{
				// print message here otherwise the original exception will get wrapped in other exceptions and never be printed
				protocolServer.Log.Error.WriteLine(ProtocolPackageServer.LogPrefix + LogPrefix + e.Message);
				throw;
			}
			finally
			{
				// clean up temp dir
				if (String.IsNullOrEmpty(syncResumeInfoFile) ||		// test for sync resume feature not enabled
					!File.Exists(syncResumeInfoFile))				// test for successful sync if sync resume was enabled
				{
					if (Directory.Exists(tempPackageRoot))
					{
						PathUtil.DeleteDirectory(tempPackageRoot);
					}

					// If the perforce temp root exists, remove it. Do not force
					// removal in case there are unfinished syncs that we need to
					// preserve for the future.
					if (Directory.Exists(p4tempRoot))
					{
						try
						{
							Directory.Delete(p4tempRoot);
						}
						catch (Exception)
						{
						}
					}
				}
			}
		}

		private void ForceFullSync(ProtocolPackageServer protocolServer, P4Server server, P4LocationAtChangeList packageSpec, P4Client syncClient,
			string packageName, string packageVersion, string tempSyncPath, string finalDir, int syncTimeout, bool allowSyncResume, string syncResumeInfoFile)
		{
			protocolServer.Log.Minimal.WriteLine
			(
				"Syncing {0}-{1} from '{2}:{3}' to '{4}'.",
				packageName, packageVersion, server.Port, packageSpec.ToString(), finalDir + Path.DirectorySeparatorChar + "..."
			);
			if (Directory.Exists(tempSyncPath))
			{
				PathUtil.DeleteDirectory(tempSyncPath);
			}
			Directory.CreateDirectory(tempSyncPath);
			if (allowSyncResume)
			{
				WriteSyncResumeFile(syncResumeInfoFile, syncClient.Name, syncClient.Server.ServerAddress);
			}
			syncClient.Sync(force: true, locations: packageSpec, throwOnNoSuchFiles: true, timeoutMs: syncTimeout, updateCallback: GetSyncCallback(protocolServer));
		}

		private static bool VerifyClientRootAndViewMap(P4Client client, string expectedClientRoot, Dictionary<string, string> expectedClientViewMap, Log log)
		{
			if (String.Compare(client.Root, expectedClientRoot, true) != 0)
			{
				log.Error.WriteLine(string.Format("INTERNAL ERROR: p4 client ({0})'s root appear to have changed!", client.Name));
				return false;
			}
			Dictionary<string, string> currViewMap = client.GetViewMap();
			foreach (string key in expectedClientViewMap.Keys)
			{
				if (!currViewMap.Keys.Contains(key))
				{
					log.Error.WriteLine(string.Format("INTERNAL ERROR: p4 client ({0})'s viewMap appear to have changed!  It is missing mapping for '{1}'", client.Name, key));
					return false;
				}
				if (string.Compare(currViewMap[key], expectedClientViewMap[key], true) != 0)
				{
					log.Error.WriteLine(string.Format("INTERNAL ERROR: p4 client ({0})'s viewMap appear to have changed!  The mapping for '{1}' is expected to be '{2}'.  But it is now changed to '{3}'.",
								client.Name, key, expectedClientViewMap[key], currViewMap[key]));
					return false;
				}
			}
			return true;
		}

		private static void UpdateManifestFileBaseVersion(string packageName, string packageVersion, uint requestedCL, ProtocolPackageServer protocolServer, string tempSyncPath)
		{
			// Update / Add baseVersion field in Manifest.xml with info <versionNumber>.<CL>
			string manifestxmlfile = Path.Combine(tempSyncPath, "Manifest.xml");
			string baseVersionEntry = String.Format("{0}.{1}", packageVersion, requestedCL);

			if (!File.Exists(manifestxmlfile))
			{
				return;
			}

			// The file should already exists as we have checks to make sure file exists before we even do a sync.
			// We added this file exists check just in case that code got changed.
			try
			{
				System.Xml.XmlDocument manifestxml = new System.Xml.XmlDocument();
				manifestxml.Load(manifestxmlfile);
				System.Xml.XmlNode manifestRoot = manifestxml.DocumentElement;
				System.Xml.XmlNode baseVersionNode = manifestxml.SelectSingleNode("//package/baseVersion");
				if (baseVersionNode != null)
				{
					// Update existing value
					baseVersionNode.InnerText = baseVersionEntry;
				}
				else
				{
					// create a new node
					System.Xml.XmlElement baseVersionElem = manifestxml.CreateElement("baseVersion");
					baseVersionElem.InnerText = baseVersionEntry;
					manifestRoot.AppendChild(baseVersionElem);
				}

				FileAttributes currAttr = File.GetAttributes(manifestxmlfile);
				if (currAttr.HasFlag(FileAttributes.ReadOnly))
				{
					File.SetAttributes(manifestxmlfile, currAttr & ~FileAttributes.ReadOnly);
				}
				manifestxml.Save(manifestxmlfile);
				File.SetAttributes(manifestxmlfile, currAttr);
			}
			catch
			{
				// Adding try-catch here just in case the operation failed.  But this baseVersion update probably
				// not a fatal error. Just print a warning message.
				protocolServer.Log.ThrowWarning
				(
					Log.WarningId.PackageServerErrors,
					Log.WarnLevel.Normal,
					"Unable to add/update {0}-{1}'s manifest.xml with baseVersion = {2}", packageName, packageVersion, baseVersionEntry
				);
			}
		}

		private string BuildP4SyncFolderHash(string value)
		{
			// Note that we're not using GetHashCode() for the string because it is not stable across different domain.
			// Our app.config has set useRandomizedStringHashAlgorithm to enabled.  That would make the result
			// not consistent across different run / domain.
			byte[] digest = Hash.GetSha2Hash(value);
			return Hash.BytesToHex(digest, 0, 6);
		}

		private static void WriteMarkerFile(string markerFilePath, uint requestedCL, uint syncedCL, string requestedServer, string centralServer, string requestedPath)
		{
			File.WriteAllLines
			(
				markerFilePath,
				new string[] 
				{
					syncedCL.ToString(), // 134538, cl we sync'd too
					requestedCL.ToString(), // 134800, cl that was request (0 if head was requested)
					requestedServer, // myp4serverProxy.domain.com, cache requested server since we want to compare when updating without doing any resolves or p4 connects
					centralServer, // myp4server.domain.com, cache central server since we don't want to resync just because user changed proxy
					requestedPath  // /depot/stuff/package/version, cache this in case user changes to different location on same server
				}
			);
		}

		private static bool TryReadMarkerFile(string markerFileLocation, out uint previouslyRequestedCL, out uint previouslySyncedCL, out string previouslyRequestedServer, out string previouslyCentralServer, out string previouslyRequestedPath)
		{
			previouslySyncedCL = 0;
			previouslyRequestedCL = 0;
			previouslyRequestedServer = null;
			previouslyCentralServer = null;
			previouslyRequestedPath = null;

			if (!File.Exists(markerFileLocation))
			{
				return false;
			}

			string[] markerFileLines = File.ReadAllLines(markerFileLocation);
			if (markerFileLines.Length != 5)
			{
				return false;
			}

			string previouslySyncedCLString = markerFileLines[0];
			string previouslyRequestedCLString = markerFileLines[1];
			previouslyRequestedServer = markerFileLines[2];
			previouslyCentralServer = markerFileLines[3];
			previouslyRequestedPath = markerFileLines[4];

			if (!UInt32.TryParse(previouslyRequestedCLString, out previouslyRequestedCL))
			{
				return false;
			}

			if (!UInt32.TryParse(previouslySyncedCLString, out previouslySyncedCL))
			{
				return false;
			}

			return true;
		}

		private static bool TryReadSyncResumeFile(string syncResumeFile, out string prevClientName, out string prevServerName)
		{
			prevClientName = null;
			prevServerName = null;

			if (!File.Exists(syncResumeFile))
			{
				return false;
			}

			string[] fileLines = File.ReadAllLines(syncResumeFile);
			foreach (string line in fileLines)
			{
				if (line.StartsWith("Client:"))
				{
					prevClientName = line.Substring("Client:".Length).Trim();
					continue;
				}
				else if (line.StartsWith("Server:"))
				{
					prevServerName = line.Substring("Server:".Length).Trim();
					continue;
				}
			}

			return (!String.IsNullOrEmpty(prevClientName) && !String.IsNullOrEmpty(prevServerName));
		}

		private static void WriteSyncResumeFile(string syncResumeFile, string clientName, string serverName)
		{
			if (String.IsNullOrEmpty(syncResumeFile))
			{
				return;
			}

			string dir = Path.GetDirectoryName(syncResumeFile);
			if (!Directory.Exists(dir))
			{
				Directory.CreateDirectory(dir);
			}

			File.WriteAllLines
			(
				syncResumeFile,
				new string[]
				{
					"Client: " + clientName,
					"Server: " + serverName
				}
			);
		}

		private static Action<string> GetSyncCallback(ProtocolPackageServer protocolServer)
		{
			if (protocolServer.Log.StatusEnabled)
			{
				Log log = protocolServer.Log;
				return (update) => log.Status.WriteLine(update);
			}
			return null;
		}
	}
}
