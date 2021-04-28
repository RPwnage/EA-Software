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
using System.Threading;
using System.Collections.Concurrent;

using NAnt.Core.Attributes;
using NAnt.Core.PackageCore;

namespace NAnt.Core.Tasks
{

	/// <summary>Container task. Executes nested elements once for a given unique key and a context.</summary>
	/// <remarks>
	/// do.once task is thread safe.
	/// This task acts within single NAnt process. It can not cross NAnt process boundary.
	/// </remarks>
	/// <example>
	///  <para>
	///  In the following example an expensive code generation step is preformed which the build script writer only wants to happen once.
	///  </para>
	///  <code><![CDATA[
	///  <do.once key="package.TestPackage.generate-code" context="global">
	///    <do if="${config-system}==pc"/>
	///      <Generate-PC-Code/>
	///    </do>
	///  </do.once>
	///  ]]></code>
	/// </example>
	[TaskName("do.once")]
	public class DoOnce : TaskContainer
	{
		public enum DoOnceContexts {global, project};

		private class ExecuteCount
		{
			internal int count = 0;
		}

		private static readonly Guid ProjectExecuteStatusId = new Guid("F0858FEC-AC85-458E-9222-361F5DD49DDB");
		private static readonly Guid ProjectBlockingExecuteStatusId = new Guid("3BB689EA-CA01-40E9-9A5B-FFF86D7D0E06");
		private static readonly ConcurrentDictionary<string, ExecuteCount> _GlobalExecuteStatus = new ConcurrentDictionary<string, ExecuteCount>();
		private static readonly PackageAutoBuildCleanMap _GlobalExecuteBlockingStatus = new PackageAutoBuildCleanMap();

		/// <summary>Unique key string. When using global context make sure that key string does 
		/// not collide with keys that may be defined in other packages. 
		/// Using "package.[package name]." prefix is a good way to ensure unique values.
		/// </summary>
		[TaskAttribute("key", Required = true)]
		public string Key { get; set; } = String.Empty;

		/// <summary>
		/// Context for the key. Context can be either <b>global</b> or <b>project</b>. Default value is <b>global</b>.
		/// <list type="bullet">
		/// <item><b>global</b> context means that for each unique key task is executed once in NAnt process.</item>
		/// <item><b>project</b> context means that for each unique key task is executed once for each package.</item>
		/// </list>
		/// </summary>
		[TaskAttribute("context", Required = false)]
		public DoOnceContexts Context { get; set; } = DoOnceContexts.global;

		/// <summary>
		/// Defines behavior when several instances of do.once task with the same key are invoked simultaneously.
		/// <list type="bullet">
		/// <item><b>blocking=false</b> (default) - One instance will execute nested elements, other instances will return immediately without waiting.</item>
		/// <item><b>blocking=true</b> - One instance will execute nested elements, other instances will wait for the first instance to complete and
		/// then return without executing nested elements.</item>
		/// </list>
		/// </summary>
		[TaskAttribute("blocking", Required = false)]
		public bool IsBlocking { get; set; } = false;

		protected override void ExecuteTask()
		{
			Execute(Project, Key, () => base.ExecuteTask(), Context, IsBlocking);
		}

		public static void Execute(string key, Action action)
		{
			Execute(null, key, action, DoOnceContexts.global);
		}

		public static void Execute(Project project, string key, Action action, DoOnceContexts context = DoOnceContexts.global, bool isblocking = false)
		{
			if (isblocking)
			{
				PackageAutoBuildCleanMap executeStatus = null;

				switch (context)
				{
					case DoOnceContexts.global:
						executeStatus = _GlobalExecuteBlockingStatus;
						break;
					case DoOnceContexts.project:
						executeStatus = project.NamedObjects.GetOrAdd(ProjectBlockingExecuteStatusId, g => new PackageAutoBuildCleanMap()) as PackageAutoBuildCleanMap;
						break;
				}

				using (PackageAutoBuildCleanMap.PackageAutoBuildCleanState actionstatus = executeStatus.StartBuild(key, "Do.Once"))
				{
					if (!actionstatus.IsDone())
					{
						action();
					}
				}
			}
			else
			{

				ConcurrentDictionary<string, ExecuteCount> executeStatus = null;
				switch (context)
				{
					case DoOnceContexts.global:
						executeStatus = _GlobalExecuteStatus;
						break;
					case DoOnceContexts.project:
						executeStatus = project.NamedObjects.GetOrAdd(ProjectExecuteStatusId, g => new ConcurrentDictionary<string, ExecuteCount>()) as ConcurrentDictionary<string, ExecuteCount>;
						break;
				}

				var execcount = executeStatus.GetOrAdd(key, new ExecuteCount());
				if (1 == Interlocked.Increment(ref execcount.count))
				{
					action();
				}
			}
		}

		internal static void Reset()
		{
			_GlobalExecuteBlockingStatus.ClearAll();
			_GlobalExecuteStatus.Clear();
		}
	}
}
