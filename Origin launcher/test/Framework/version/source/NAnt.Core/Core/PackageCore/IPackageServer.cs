// Originally based on NAnt - A .NET build tool
// Copyright (C) 2003-2015 Electronic Arts Inc.
// Copyright (C) 2001-2003 Gerry Shaw
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
// 
// 
// Gerry Shaw (gerry_shaw@yahoo.com)
// Ian MacLean (ian_maclean@another.com)
// Electronic Arts (Frostbite.Team.CM@ea.com)

using System;
using System.Collections.Generic;

namespace NAnt.Core.PackageCore
{
	public class InstallProgressEventArgs : EventArgs
	{
		public InstallProgressEventArgs(int globalProgress, int localProgress, string message, Release release)
		{
			LocalProgress = localProgress;
			GlobalProgress = globalProgress;
			Message = message;
			Release = release;
		}

		public int LocalProgress { get; }

		public int GlobalProgress { get; }

		public string Message { get; }

		public Release Release { get; }
	}

	public delegate void InstallProgressEventHandler(object sender, InstallProgressEventArgs e);

	public class BreakException : ApplicationException
	{
	}

	/// <summary>
	/// Version 2 of the IPackageServer interface. Can be used to connect with package server V2 API.
	/// </summary>
	public interface IPackageServerV2 : IPackageServer
	{
		/// <summary>
		/// Initializes the connection with the package server.
		/// </summary>
		/// <param name="project">The project</param>
		/// <param name="usev2">Whether or not to use the V2 API</param>
		/// <param name="token">The auth token when using V2 API, otherwise may be empty</param>
		void Init(Project project, bool usev2, string token);
	}

	public interface IPackageServer : IDisposable
	{
		event InstallProgressEventHandler UpdateProgress;

		Project Project { get; }

		/// <summary>
		/// Initializes the connection with the package server.
		/// </summary>
		void Init(Project project);

		/// <summary>
		/// Returns a list of all of the release of a package found on the package server.
		/// This is used when the version request is not found but other versions are available, in order to provide a helpful message.
		/// </summary>
		/// <param name="name">The name of the package</param>
		/// <param name="releases">A list of all of the releases of the package found on the package server</param>
		/// <returns></returns>
		bool TryGetPackageReleases(string name, out IList<MasterConfig.IPackage> releases);

		/// <summary>
		/// Installs required package from a Package Server or throws an exception.
		/// </summary>
		/// <param name="packageSpec">Name of the package to install.</param>>
		/// <param name="rootDirectory">Local directory to install the release into.</param>
		Release Install(MasterConfig.IPackage packageSpec, string rootDirectory);

		/// <summary>
		/// For protocol package servers only, updates release to version specified by protocol uri if required.
		/// </summary>
		/// <param name="release">The release to update.</param>
		/// <param name="ondemandUri">Uri to update package from.</param>
		/// <param name="rootDirectory">This directory is default install root. Package server can use this to temporarily relocation the package during the update.</param>
		void Update(Release release, Uri ondemandUri, string rootDirectory);

		/// <summary>
		/// For protocol package servers only, returns true if passed in release is controlled by this package server.
		/// </summary>
		/// <param name="release">Release to check</param>
		/// <param name="rootDirectory"></param>
		bool IsPackageServerRelease(Release release, string rootDirectory);
	}
}
