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

using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Logging;

using System;
using System.Collections.Generic;
using System.Xml;

namespace EA.Eaconfig.Structured
{
	/// <summary>
	/// Declares partial module definition. NOTE: schema and intellisense are generated for 'native' PartialModule only, for DotNet partial modules use 'CsharpLibrary' as a template.
	/// </summary>
	[TaskName("PartialModule", StrictAttributeCheck=false)]
	public class PartialModuleTask : Task
	{
		private static readonly Guid PartialModulesListId = new Guid("28E0C1DB-81A8-46c5-9257-7026916C9174");

		private string _name;

		/// <summary></summary>
		[TaskAttribute("name", Required = true)]
		public virtual string ModuleName
		{
			get { return _name;  }
			set { _name = value; }
		}

		/// <summary></summary>       
		[TaskAttribute("comment", Required = false)]
		public string Comment { get; set; }
		internal XmlNode Code { get; private set; } = null;

		protected override void InitializeTask(XmlNode taskNode)
		{
			Code = taskNode;

			Project.Partials.Add(this);
		}

		protected override void ExecuteTask()
		{
			Log.Info.WriteLine(LogPrefix + "Registering Partial Module definition'{0}'", ModuleName);
		}

		public static List<PartialModuleTask> GetPartialModules(Project project)
		{
			List<PartialModuleTask> list = new List<PartialModuleTask>();
			foreach (var p in project.Partials)
			{
				list.Add((PartialModuleTask)p);
			}
			return list;
		}

		public void ApplyPartialModule(ModuleBaseTask module)
		{
			if (typeof(DotNetModuleTask).IsAssignableFrom(module.GetType()))
			{
				var partialModule = new DotNetPartialModule(module as DotNetModuleTask);

				partialModule.Parent = module.Parent;

				partialModule.Initialize(Code);

				partialModule.SetupModule(module);

				partialModule.NantScript.Execute(NantScript.Order.after);
			}
			else
			{
				var partialModule = new PartialModule(module);

				partialModule.Parent = module.Parent;

				partialModule.Initialize(Code);

				partialModule.SetupModule(module);

				partialModule.NantScript.Execute(NantScript.Order.after);
			}
			
		}

		internal class PartialModule : ModuleTask
		{
			internal PartialModule(ModuleBaseTask module)
				: base(String.Empty)
			{
				Project = module.Project;
				if (module is ModuleTask)
				{
					mCombinedState = ((ModuleTask)module).mCombinedState;
				}
				else
				{
					mCombinedState = new CombinedStateData();
				}
			}

			internal void SetupModule(ModuleBaseTask module)
			{
				ModuleName = module.ModuleName;
				Group = module.Group;
				debugData = module.debugData;
				Debug = module.Debug;

				base.SetupModule();
			}
		}

		internal class DotNetPartialModule : DotNetModuleTask
		{
			internal DotNetPartialModule(DotNetModuleTask module)
				: base(String.Empty, module.CompilerName)
			{
				Project = module.Project;
				mCombinedState = ((DotNetModuleTask)module).mCombinedState;
			}

			internal void SetupModule(ModuleBaseTask module)
			{
				ModuleName = module.ModuleName;
				Group = module.Group;
				debugData = module.debugData;
				Debug = module.Debug;

				base.SetupModule();
			}
		}

	}
}
