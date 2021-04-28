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

using NAnt.Core;
using NAnt.Core.Logging;
using NAnt.Core.PackageCore;
using NAnt.Core.Util;

namespace EA.PackageSystem
{
	public sealed class ProtocolPackageServer : IPackageServer
	{
		public const string LogPrefix = "Protocol package server: ";

		public Project Project { get; private set; }
		public Log Log { get { return Project.Log; } }

		public event InstallProgressEventHandler UpdateProgress;

		private readonly Dictionary<string, ProtocolBase> m_protocolMap = new Dictionary<string, ProtocolBase>();

		public void Init(Project project)
		{
			Project = project;

			P4Protocol p4Protocol = new P4Protocol();
			NugetProtocol nugetProtocol = new NugetProtocol();
			GitProtocol gitProtocol = new GitProtocol();
			StubProtocol stubProtocol = new StubProtocol();

			RegisterProtocol("p4", p4Protocol);
			RegisterProtocol("nuget-http", nugetProtocol);
			RegisterProtocol("nuget-https", nugetProtocol);
			RegisterProtocol("git-file", gitProtocol);
			RegisterProtocol("git-https", gitProtocol);
			RegisterProtocol("git", gitProtocol);
			RegisterProtocol("stub", stubProtocol);
		}

		private void RegisterProtocol(string identifier, ProtocolBase protocol)
		{
			m_protocolMap.Add(identifier, protocol);
		}

		// outside code ensure we never enter this function unless we have a release
		// returned out of TryGetRelease, not great to be relying on outside behav
		public Release Install(MasterConfig.IPackage packageSpec, string rootDirectory)
		{
			Uri uri = new Uri(packageSpec.AdditionalAttributes["uri"]);
			if (m_protocolMap.TryGetValue(uri.Scheme, out ProtocolBase protocol))
			{
				return protocol.Install(packageSpec, uri, rootDirectory, this);
			}
			else
			{
				throw new BuildException(String.Format("The uri protocol '{0}' is not supported by this version of Framework.", uri.Scheme));
			}
		}

		public void Update(Release release, Uri ondemandUri, string rootDirectory)
		{
			if (m_protocolMap.TryGetValue(ondemandUri.Scheme, out ProtocolBase protocol))
			{
				protocol.Update(release, ondemandUri, rootDirectory, this);
			}
			else
			{
				throw new BuildException(String.Format("The uri protocol '{0}' is not supported by this version of Framework.", ondemandUri.Scheme));
			}
		}

		public bool TryGetPackageReleases(string name, out IList<MasterConfig.IPackage> releases)
		{
			releases = new List<MasterConfig.IPackage>();
			return false;
		}

		public bool IsPackageServerRelease(Release release, string rootDirectory)
		{
			return release.Path == Path.Combine(rootDirectory, release.Name, release.Version); // TODO this feels very suspect...
		}

		public void Dispose()
		{
		}

		internal void UpdateProgressMessage(int progress, string message = null)
		{
			UpdateProgress?.Invoke(this, new InstallProgressEventArgs(progress, progress, message, null));
		}
	}
}
