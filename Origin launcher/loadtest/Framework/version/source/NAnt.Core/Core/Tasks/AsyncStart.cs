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
using System.Linq;
using System.Threading;
using Concurrent = System.Threading.Tasks;
using System.Collections.Concurrent;

using NAnt.Core.Util;
using NAnt.Core.Attributes;

namespace NAnt.Core.Tasks
{
	/// <summary>Starts asynchronous execution of nested block.
	/// <para>
	///   <b>NOTE.</b> Make sure there are no race conditions, properties with same names, etc each group.
	/// </para>
	/// <para>
	///   <b>NOTE.</b> local properties are local to the thread. Normal NAnt properties are shared between threads.
	/// </para>
	/// </summary>
	[TaskName("async.start")]
	public class AsyncStart : TaskContainer
	{

		/// <summary>Unique key string. When using global context make sure that key string does 
		/// not collide with keys that may be defined in other packages. 
		/// Using "package.[package name]." prefix is a good way to ensure unique values.
		/// </summary>
		[TaskAttribute("key", Required = true)]
		public string Key { get; set; } = String.Empty;

		[TaskAttribute("waitat", Required = false)]
		public AsyncStatus.WaitAt WaitAt { get; set; } = AsyncStatus.WaitAt.None;

		public static bool Execute(string key, AsyncStatus.WaitAt waitAt, Action action)
		{
			var status = AsyncStatus.TryAddNewTask(key, action, waitAt);
			if (status != null)
			{
				status.Task.Start();
				return true;
			}
			return false;
		}


		protected override void ExecuteTask()
		{
			AsyncStart.Execute(Key, WaitAt, ()=>base.ExecuteChildTasks());
		}
	}


	[TaskName("async.wait")]
	public class AsyncWait : TaskContainer
	{
		/// <summary>Unique key string. When using global context make sure that key string does 
		/// not collide with keys that may be defined in other packages. 
		/// Using "package.[package name]." prefix is a good way to ensure unique values.
		/// </summary>
		[TaskAttribute("key", Required = true)]
		public string Key { get; set; } = String.Empty;

		protected override void ExecuteTask()
		{
			AsyncStatus.WaitForAsyncTask(Key);
		}
	}

	public class AsyncStatus : BitMask
	{
		public enum WaitAt { None, BeforeGlobalPreprocess=1, AfterGlobalPreprocess = 2, BeforeGlobalPostprocess = 4, AfterGlobalPostprocess = 8};

		public readonly string Key;
		public readonly Concurrent.Task Task;
		public readonly CancellationTokenSource CancellationTokenSource;

		private AsyncStatus(string key, Action action, WaitAt waitAt = WaitAt.None) : base(WaitAtToMask(waitAt))
		{
			Key = key;
			CancellationTokenSource = new CancellationTokenSource();
			Task = new Concurrent.Task(action, CancellationTokenSource.Token, Concurrent.TaskCreationOptions.PreferFairness);
		}

		public static AsyncStatus TryAddNewTask(string key, Action action, WaitAt waitAt = WaitAt.None)
		{
			var status = new AsyncStatus(key, action, waitAt);

			if(_AsyncTaskStatus.TryAdd(key, status))
			{
				return status;
			}
			return null;
		}

		public static AsyncStatus GetAyncTaskStatus(string key)
		{
			AsyncStatus status = null;
			_AsyncTaskStatus.TryGetValue(key, out status);
			return status;
		}

		public static bool WaitForAsyncTask(string key)
		{
			bool res = false;
			AsyncStatus status = null;
			if(_AsyncTaskStatus.TryGetValue(key, out status))
			{
				status.Task.Wait(status.CancellationTokenSource.Token);
				res = true;
			}
			return res;
		}

		public static bool WaitForAsyncTasks(WaitAt waitAt)
		{
			bool res = false;
			uint mask = WaitAtToMask(waitAt);

			var statusentries = _AsyncTaskStatus.Where(e => e.Value.IsKindOf(mask));

			foreach (var se in statusentries) 
			{
				se.Value.Task.Wait(se.Value.CancellationTokenSource.Token);
				res = true;
			}
			return res;
		}

		public static bool CancelAsyncTask(string key)
		{
			bool res = false;
			AsyncStatus status = null;
			if (_AsyncTaskStatus.TryGetValue(key, out status))
			{
				status.CancellationTokenSource.Cancel();
				res = true;
			}
			return res;
		}


		public static bool CancelAsyncTasks()
		{
			bool res = false;
			foreach (var se in _AsyncTaskStatus)
			{
				se.Value.CancellationTokenSource.Cancel();
				res = true;
			}
			return res;
		}

		private static uint WaitAtToMask(WaitAt waitAt)
		{
			return (uint)waitAt;

		}

		private static readonly ConcurrentDictionary<string, AsyncStatus> _AsyncTaskStatus = new ConcurrentDictionary<string, AsyncStatus>();
	}


}
