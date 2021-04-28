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
using System.Collections.Specialized;
using System.IO;
using System.Web;

using NAnt.Authentication;
using NAnt.Core;
using NAnt.Core.Logging;
using NAnt.Core.PackageCore;
using NAnt.Core.Tasks;
using NAnt.Core.Util;
using NAnt.Shared.Properties;

using LibGit2Sharp;

namespace EA.PackageSystem
{
	public class GitProtocol : ProtocolBase
	{
		public const string LogPrefix = "Protocol package server (git): ";

		class GitProtocolImpl
		{
			Project m_project;

			class GitQueryData
			{
				public readonly string Commit;

				GitQueryData(string commit, string tag)
				{
					if (commit == null && tag == null)
					{
						Commit = "HEAD";
					}

					if (commit == null && tag != null)
					{
						Commit = "tag/" + tag;
					}

					if (commit != null)
					{
						Commit = commit;
					}
				}

				public static GitQueryData Parse(Uri uri)
				{
					const string CommitKey = "commit";
					const string TagKey = "tag";

					NameValueCollection collection = HttpUtility.ParseQueryString(uri.Query);
					string commit = collection.Get(CommitKey);
					string tag = collection.Get(TagKey);

					if (commit != null && tag != null)
					{
						throw new FormatException(
							string.Format("The uri provided ('{0}') provides both a commit hash ('{1}') as well as a tag ('{2}'). Only one or the other may be set.",
							uri.OriginalString,
							commit,
							tag));
					}

					return new GitQueryData(commit, tag);
				}
			}

			class NantCredentialsHandler
			{
				Log m_log;
				string m_credentialStore;

				public NantCredentialsHandler(Project project)
				{
					m_log = project.Log;
					m_credentialStore = project.Properties[FrameworkProperties.CredStoreFilePropertyName];
				}

				public Credentials GitHandler(string url, string usernameFromUrl, SupportedCredentialTypes types)
				{
					if (!types.HasFlag(SupportedCredentialTypes.UsernamePassword))
					{
						return new DefaultCredentials();
					}

					Uri uri = new Uri(url);
					string server = uri.Authority;
					Credential storedCredential = CredentialStore.Retrieve(m_log, server, m_credentialStore);
					if (storedCredential != null)
					{
						UsernamePasswordCredentials credentials = new UsernamePasswordCredentials();
						credentials.Username = storedCredential.Name;
						credentials.Password = storedCredential.Password;

						return credentials;
					}
					else
					{
						Credential userInputCredential = CredentialStore.Prompt(m_log, server);
						if (userInputCredential == null)
						{
							throw new Exception($"Credentials are incorrect for {server}. To rectify this\ncall \"eapm credstore {server}\" and enter your correct credentials");
						}
						UsernamePasswordCredentials credentials = new UsernamePasswordCredentials();
						credentials.Username = userInputCredential.Name;
						credentials.Password = userInputCredential.Password;
						return credentials;
					}
				}
			}

			public GitProtocolImpl(Project project)
			{
				m_project = project;
			}

			static string MakeGitUri(Uri uri)
			{
				string uriString = uri.AbsoluteUri;

				return uriString.Replace("git-https://", "https://").Replace("git-file://", "file://");
			}

			void CheckoutProgressHandler(string path, int completedSteps, int totalSteps)
			{
				m_project.Log.Debug.WriteLine(m_project.LogPrefix + LogPrefix + "Path: {0} {1}/{2} steps", path, completedSteps, totalSteps);
			}

			bool ProgressHandler(string serverProgressOutput)
			{
				m_project.Log.Debug.WriteLine(m_project.LogPrefix + LogPrefix + serverProgressOutput);
				return true;
			}

			bool TransferProgressHandler(TransferProgress progress)
			{
				m_project.Log.Debug.WriteLine(m_project.LogPrefix + LogPrefix + "Received {0}/{1} objects ({2} bytes), {3} objects indexed", progress.ReceivedObjects, progress.TotalObjects, progress.ReceivedBytes, progress.IndexedObjects);
				return true;
			}

			CloneOptions MakeCloneOptions(GitQueryData queryData)
			{
				CloneOptions cloneOptions = new CloneOptions();
				cloneOptions.CredentialsProvider = new NantCredentialsHandler(m_project).GitHandler;

				if (queryData.Commit == null)
				{
					cloneOptions.Checkout = true;
				}

				cloneOptions.OnCheckoutProgress = CheckoutProgressHandler;
				cloneOptions.OnProgress = ProgressHandler;
				cloneOptions.OnTransferProgress = TransferProgressHandler;

				return cloneOptions;
			}

			void Fetch(Repository repository, string uri)
			{
				string remoteHash = Hash.GetSha2HashString(uri);
				string remoteName = "package_" + remoteHash;

				// First try to see if the remote is already added:
				bool foundRepo = false;
				foreach (var repo in repository.Network.Remotes)
				{
					if (repo.Name == remoteName && repo.Url == uri)
					{
						foundRepo = true;
						break;
					}
				}
				if (!foundRepo)
					repository.Network.Remotes.Add(remoteName, uri);

				Commands.Fetch(
					repository,
					remoteName,
					new string[0],
					new FetchOptions {
						CredentialsProvider = new NantCredentialsHandler(m_project).GitHandler,
						OnProgress = ProgressHandler,
						OnTransferProgress = TransferProgressHandler
					},
					null);
			}

			void Clone(string baseDir, string finalDir, string gitUri, GitQueryData queryData, ProtocolPackageServer protocolServer)
			{
				string finalGitDir = Path.Combine(finalDir, ".git");

				if (Directory.Exists(finalGitDir))
				{
					return;
				}

				string subdir = Path.GetRandomFileName();
				string cloneDir = Path.Combine(baseDir, ".gittemp", subdir);

				protocolServer.Log.Info.WriteLine(ProtocolPackageServer.LogPrefix + LogPrefix + "Cloning git project into {0}", finalDir);

				CloneOptions cloneOptions = MakeCloneOptions(queryData);
				Repository.Clone(gitUri, cloneDir, cloneOptions);

				try
				{
					PathUtil.MoveDirectory(cloneDir, finalDir);
				}
				catch
				{
					Console.WriteLine("Could not move {0} to {1}", cloneDir, finalDir);
					PathUtil.DeleteDirectory(cloneDir);
					throw;
				}
			}

			public string FetchAndSwitch(string packageName, string packageVersion, string rootDirectory, Uri uri, ProtocolPackageServer protocolServer)
			{
				string finalDir = Path.Combine(rootDirectory, packageName);
				// Need to put the version name on the folder. i.e. package/version/...
				// otherwise it puts into flattened package/...  which collides with other local versions.
				// If user requests "flattened", we will flatten.
				if (packageVersion != "flattened")
				{
					finalDir = Path.Combine(finalDir, packageVersion);
				}

				string gitUri = MakeGitUri(uri);

				GitQueryData queryData = GitQueryData.Parse(uri);
				if (queryData.Commit == "HEAD")
				{
					DoOnce.Execute(uri.ToString(), () => protocolServer.Log.ThrowWarning(Log.WarningId.MasterconfigURIHead, Log.WarnLevel.Normal, "The git uri string, '{0}', defaulted or lists the revision as #head. This will require an up-to-date check everytime Framework runs.", uri.ToString()));
				}

				// Need to trim off the query data (git logic won't know what that is)
				int index = gitUri.IndexOf("?");
				if (index > 0)
				{
					gitUri = gitUri.Substring(0, index);
				}

				protocolServer.Log.Status.WriteLine("Syncing {0}-{1} from '{2}' to {3}", packageName, packageVersion, gitUri, finalDir);
				Clone(rootDirectory, finalDir, gitUri, queryData, protocolServer);

				using (Repository repository = new Repository(finalDir))
				{
					protocolServer.Log.Info.WriteLine(ProtocolPackageServer.LogPrefix + LogPrefix + "Ensuring project {0} is up-to-date with a fetch from {0}", gitUri);
					// TODO: Possible optimization: check to see if we can checkout
					// a particular commit without performing the fetch.
					Fetch(repository, gitUri);

					protocolServer.Log.Info.WriteLine(ProtocolPackageServer.LogPrefix + LogPrefix + "Checking out specified commit {0}", queryData.Commit);
					Commands.Checkout(repository, queryData.Commit, new CheckoutOptions { OnCheckoutProgress = CheckoutProgressHandler });
				}

				return finalDir;
			}
		}

		public override Release Install(MasterConfig.IPackage package, Uri uri, string rootDirectory, ProtocolPackageServer protocolServer)
		{
			GitProtocolImpl protocolImpl = new GitProtocolImpl(protocolServer.Project);

			protocolServer.Log.Info.WriteLine(ProtocolPackageServer.LogPrefix + LogPrefix + "Installing git protocol package {0}...", package.Name);
			string releasePath = protocolImpl.FetchAndSwitch(package.Name, package.Version, rootDirectory, uri, protocolServer);

			return new Release(package, releasePath, isFlattened: package.Version == "flattened", manifest: Manifest.Load(releasePath, protocolServer.Log));
		}

		public override void Update(Release release, Uri uri, string defaultInstallDir, ProtocolPackageServer protocolServer)
		{
			GitProtocolImpl protocolImpl = new GitProtocolImpl(protocolServer.Project);

			protocolServer.Log.Info.WriteLine(ProtocolPackageServer.LogPrefix + LogPrefix + "Checking git protocol package {0} is up to date...", release.Name);
			protocolImpl.FetchAndSwitch(release.Name, release.Version, defaultInstallDir, uri, protocolServer);
		}
	}
}
