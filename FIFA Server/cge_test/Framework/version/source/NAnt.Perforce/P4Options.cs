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

namespace NAnt.Perforce
{
	[Flags]
	public enum AddOptions
	{
		Default = 0x0,
		DowngradeEditToAdd = 0x1,
		ForceWildCardNames = 0x2,
		IgnoreP4Ignore = 0x4
	}

	[Flags]
	public enum DeleteOptions
	{
		Default = 0x0,
		KeepFileLocally = 0x1,
		DeleteUnsyncedFiles = 0x2
	}

	[Flags]
	public enum DirsOptions
	{
		Default = 0x0,
		IncludeDeleted = 0x1,
		LimitToClient = 0x2,
		IncludeOnlyHave = 0x4
	}

	[Flags]
	public enum ReconcileFilterOptions
	{
		Default = 0x0,
		/// <summary>Include files that have been edited in the result of the reconcile</summary>
		EditedFiles = 0x1,
		/// <summary>Include files that have been added in the result of the reconcile</summary>
		AddedFiles = 0x2,
		/// <summary>Include files that have been deleted in the result of the reconcile</summary>
		DeletedFiles = 0x4
	}

	public enum AutoResolveOptions
	{
		Default,
		/// <summary>
		/// Only resolve files that don't require merging
		/// </summary>
		ResolveSafe,
		/// <summary>
		/// Only resolve files that don't have conflicts
		/// </summary>
		ResolveMerge,
		/// <summary>
		/// Force merging files with conflicts, conflicts are marked in files with conflict markers
		/// </summary>
		ResolveForce,
		/// <summary>
		/// Force acceptance of your changes and ignore theirs
		/// </summary>
		ResolveYours,
		/// <summary>
		/// Force acceptance of their changes and overwrite any changes in your files
		/// </summary>
		ResolveTheirs
	}

	public enum ShelveOptionSet
	{
		submitunchanged,
		leaveunchanged
	}

	public enum ResponseFileBehaviour
	{
		UseGlobalDefault,
		UseWhenLimitExceeded,
		AlwaysUse,
		NeverUse
	}

	public static class P4Defaults
	{
		// special default argument specifies that signify special behaviour.
		// batching
		public const uint DefaultBatchSize = Int32.MaxValue;
		public const uint NoBatching = 0;
		// retries
		public const uint DefaultRetries = UInt32.MaxValue;

		// options defaults
		public const ShelveOptionSet DefaultShelveOption = ShelveOptionSet.submitunchanged;
		public const ResponseFileBehaviour DefaultResponseFileBehaviour = ResponseFileBehaviour.UseGlobalDefault;
	}
}