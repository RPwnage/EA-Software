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

using NAnt.Core;
using NAnt.Core.Util;
using EA.FrameworkTasks.Model;

namespace EA.Eaconfig.Core
{
	// Data used for debugging, various generations, etc.
	public class ModuleAdditionalData
	{
		public DebugData DebugData;
		public GeneratorTemplatesData GeneratorTemplatesData;
		public SharedPchData SharedPchData;
	}

	public class DebugData
	{
		public readonly Command Command;
		public readonly string RemoteMachine;
		public readonly bool EnableUnmanagedDebugging; // For .Net projects

		public static DebugData CreateDebugData(string executable, string options, string workingDir, IDictionary<string, string> env, string remoteMachine, bool? enableUnmanagedDebugging = null, bool createAlways = false)
		{
			if (createAlways==false && String.IsNullOrEmpty(executable) && String.IsNullOrEmpty(options) && String.IsNullOrEmpty(workingDir) && String.IsNullOrEmpty(remoteMachine) && enableUnmanagedDebugging == null)
			{
				return null;                
			}
			return new DebugData(executable, options, workingDir, env, remoteMachine, enableUnmanagedDebugging);
		}

		private DebugData(string executable, string options, string workingDir, IDictionary<string, string> env, string remoteMachine, bool? enableUnmanagedDebugging)
		{
			Command = new Command(new PathString(executable), new string[] { options }, String.Empty, new PathString(workingDir), env);
			RemoteMachine = remoteMachine;
			EnableUnmanagedDebugging = enableUnmanagedDebugging??false;
		}
	}

	public class GeneratorTemplatesData
	{
		public readonly string SlnDirTemplate;
		public readonly string SlnNameTemplate;
		public readonly string ProjectDirTemplate;
		public readonly string ProjectFileNameTemplate;

		public GeneratorTemplatesData(string slnDirTemplate = null, string slnNameTemplate = null, string projectDirTemplate = null, string projectFileNameTemplate = null)
		{
			SlnDirTemplate = slnDirTemplate ?? String.Empty;
			SlnNameTemplate = slnNameTemplate ?? String.Empty;
			ProjectDirTemplate = projectDirTemplate ?? String.Empty;
			ProjectFileNameTemplate = projectFileNameTemplate ?? String.Empty;
		}

		public static GeneratorTemplatesData CreateGeneratorTemplateData(Project project, BuildGroups buildgroup)
		{
			return CreateGeneratorTemplateData(project, buildgroup.ToString());
		}
		public static GeneratorTemplatesData CreateGeneratorTemplateData(Project project, string buildgroup)
		{
		    return new GeneratorTemplatesData
			(
				slnDirTemplate: project.Properties[buildgroup + ".gen.slndir.template"].TrimWhiteSpace(),
				slnNameTemplate: project.Properties[buildgroup + ".gen.slnname.template"].TrimWhiteSpace(),
				projectDirTemplate: project.Properties[buildgroup + ".gen.projectdir.template"].TrimWhiteSpace(),
				projectFileNameTemplate: project.Properties[buildgroup + ".gen.projectname.template"].TrimWhiteSpace()
			);
		}
	}

	public class SharedPchData
	{
		public string SharedPchPackage { get; set; }
		public string SharedPchModule { get; set; }
	}
}
