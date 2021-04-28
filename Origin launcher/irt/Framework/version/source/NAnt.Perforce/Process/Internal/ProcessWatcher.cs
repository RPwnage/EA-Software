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

using System.Collections.Generic;

namespace NAnt.Perforce.Processes
{
	internal class ProcessWatcher
	{
		//private static readonly ProcessWatcher WatcherInstance = CreatePlatformWatcher();
		private static readonly object ParentMapLock = new object(); // concurrent dictionary not supported under target versions of mono // TODO is this still true?
		private static readonly Dictionary<int, int> ProcessParentMap = new Dictionary<int, int>();

		// protected default constructor
		protected ProcessWatcher()
		{
		}

		internal static void EnsureInitialized()
		{
			/*if (WatcherInstance == null)
			{
				throw new ApplicationException("Watcher instance should never be null!");
			}*/
		}

		internal static bool IsChildOf(int PID, int PPID)
		{
			if (PID == ChildProcess.INVALID_PID)
			{
				return false;
			}
			int parentId;
			lock (ParentMapLock)
			{
				if (ProcessParentMap.TryGetValue(PID, out parentId))
				{
					return parentId == PPID;
				}
			}
			return false;
		}

		internal static bool IsDesendedFrom(int PID, int PPID)
		{
			if (PID == ChildProcess.INVALID_PID)
			{
				return false;
			}
			int ancestorId = PID;
			while (true)
			{
				lock (ParentMapLock)
				{
					if (!ProcessParentMap.TryGetValue(ancestorId, out ancestorId))
					{
						break;
					}
				}
				if (ancestorId == PPID)
				{
					return true;
				}
			}
			return false;
		}

		internal static ChildProcess GetParent(ChildProcess childProcess)
		{
			int parentId;
			lock (ParentMapLock)
			{
				if (ProcessParentMap.TryGetValue(childProcess.PID, out parentId))
				{
					return ChildProcess.GetProcessById(parentId);
				}
			}
			return null;
		}

		protected static void AddParent(int childPID, int parentPID)
		{
			lock (ParentMapLock)
			{
				ProcessParentMap[childPID] = parentPID;
			}
		}

		private static ProcessWatcher CreatePlatformWatcher()
		{
			return null;
			/*switch (ETBFPlatform.Detection.CurrentPlatform)
			{
				case ETBFPlatform.Platform.Windows:
					if (WindowsProcessWatcher.CanRun())
					{
						return new WindowsProcessWatcher();
					}
					return new CommonProcessWatcher();
				default:
					return new CommonProcessWatcher();
			}*/
		}
	}
}