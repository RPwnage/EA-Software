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
using System.Collections.Concurrent;

using NAnt.Core.Attributes;

namespace NAnt.Core.Tasks
{

	/// <summary>Allows wrapping of a group of tasks to be executed in a mutually exclusive way based on 'name' attribute.</summary>
	/// <remarks>
	///     <para>This task acts as a named mutex for a group of nested tasks.</para>
	///     <para>Make sure that name name value does not collide with namedlock names that can be defined in other packages.</para>
	/// </remarks>
	[TaskName("namedlock")]
	public class NamedLock : TaskContainer
	{
		private static ConcurrentDictionary<string, object> _SyncRoots = new ConcurrentDictionary<string, object>();

		/// <summary>Unique name for the lock. All &lt;namedlock&gt; sections with same name are mutually exclusive
		/// Make sure that key string does not collide with names of &lt;namedlock&gt; that may be defined in other packages. 
		/// Using "package.[package name]." prefix is a good way to ensure unique values.
		/// </summary>
		[TaskAttribute("name", Required = true)]
		public string LockName { get; set; } = String.Empty;

		protected override void ExecuteTask()
		{
			lock (_SyncRoots.GetOrAdd(LockName, new object()))
			{
				base.ExecuteTask();
			}
		}

		public static void Execute(Project project, string name, Action action)
		{
			lock (_SyncRoots.GetOrAdd(name, new object()))
			{
				action();
			}
		}
	}
}
