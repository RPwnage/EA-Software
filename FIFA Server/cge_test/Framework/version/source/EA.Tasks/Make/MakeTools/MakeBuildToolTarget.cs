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
using System.IO;
using System.Collections.Generic;
using System.Linq;

using NAnt.Core;
using NAnt.Core.Util;

using EA.Eaconfig.Modules.Tools;
using EA.FrameworkTasks.Model;


using EA.Make.MakeItems;


namespace EA.Make.MakeTools
{
	class MakeBuildToolTarget : MakeBuildStepBase<BuildTool>
	{

		public MakeBuildToolTarget(MakeModuleBase makeModule, IModule module, BuildTool tool)
			: base(makeModule, module, tool)
		{
		}

		public void GenerateTargets(string targetName, IDictionary<string, int> targetnames)
		{
			IEnumerable<string> outputDependenciesCollection = GetToolOutputDependencies();
            IEnumerable<string> inputDependenciesCollection = GetToolInputDependencies();

			// create primary target
			bool isPhony = false;
			string primaryTargetName = GetPrimaryTargetName(outputDependenciesCollection, GetTargetName(targetName, targetnames), out isPhony);
			if (isPhony)
			{
				MakeModule.MakeFile.PHONY += primaryTargetName;
			}
			MakeTarget primaryTarget = MakeModule.MakeFile.Target(primaryTargetName);
			StepTargets.Add(primaryTarget);
			MakeCommand(BuildStepTool); // modifies the target just added to StepTargets
            primaryTarget.Prerequisites += inputDependenciesCollection.Select(path => MakeModule.ToMakeFilePath(path));

            // add secondary targets
            AddSerializationTargets(outputDependenciesCollection);
			
			// update clean
			MakeModule.AddFilesToClean(outputDependenciesCollection);
		}

		private string GetTargetName(string defaultName, IDictionary<string, int> targetnames)
		{
			string targetName = Tool.ToolName;

			if(String.IsNullOrEmpty(targetName) && !Tool.Executable.IsNullOrEmpty())
			{
				targetName = Path.GetFileName(Tool.Executable.Path);
			}
			if(String.IsNullOrEmpty(targetName))
			{
				targetName = defaultName;
			}

			int ind;
			if(targetnames.TryGetValue(targetName, out ind))
			{
				ind++;
				targetnames[targetName] = ind;

				targetName = string.Format("{0}_{1}", targetName, ind);
			}
			else
			{
				targetnames.Add(targetName, 0);
			}

			return targetName;
		}

		protected override IEnumerable<string> GetToolInputDependencies()
		{
			IEnumerable<string> inputDependencies = base.GetToolInputDependencies();
			if (BuildStepTool.Files != null && !BuildStepTool.Files.FileItems.IsNullOrEmpty())
			{
                inputDependencies = inputDependencies.Concat(BuildStepTool.Files.FileItems.Select(item => item.Path.Path));
			}
			return inputDependencies;
		}
	}
}
