// (c) Electronic Arts. All rights reserved.

using System;
using System.Xml;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Attributes;
using NAnt.Core.Functions;
using NAnt.Core.Logging;
using NAnt.Core.PackageCore;
using Model = EA.FrameworkTasks.Model;

using EA.Eaconfig;
using EA.Eaconfig.Core;
using EA.Eaconfig.Build;
using EA.Eaconfig.Modules;
using EA.Eaconfig.Modules.Tools;

namespace EA.Eaconfig
{
	// This task only get executed during build graph creation.  If the expected output name contains
	// invalid characters, it will create or modify the [group].[module].outputname property even if 
	// user already provided that property.
	[TaskName("iphone-preprocess")]
	class Iphone_Preprocess : AbstractModuleProcessorTask
	{
		public override void Process(Module_Native module)
		{
			if (module != null && module.IsKindOf(Model.Module.Program))
			{
				IphoneOutputNameHelper.SanitizeOutputName(module.Package.Project, module.BuildGroup.ToString(), module.Name);
			}
		}
	}

	// This task performs the same operation as in iphone-preprocess.  It allow us to be consistent in doing the same outputname
	// modification outside the buildgraph processing if necessary!  This can happen during XcodeProjectizer's post build phase
	// execution.
	[TaskName("sanitize-iphone-program-outputname")]
	class SanitizeIphoneProgramOutputnameTask : Task
	{
		[TaskAttribute("buildgroup", Required = true)]
		public string BuildGroup { get; set; }

		[TaskAttribute("modulename", Required = true)]
		public string ModuleName { get; set; }

		protected override void ExecuteTask()
		{
			IphoneOutputNameHelper.SanitizeOutputName(Project, BuildGroup, ModuleName);
		}
	}

	internal static class IphoneOutputNameHelper
	{
		private static readonly char[] INVALID_EXECUTABLE_NAME_CHARACTERS = { '_', '.', '+' };

		static public void SanitizeOutputName(Project proj, string buildgroup, string modulename)
		{
			if (String.IsNullOrEmpty(modulename))
			{
				return;
			}

			string grpname = (string.IsNullOrEmpty(buildgroup) ? "runtime" : buildgroup) + "." + modulename;
			string outputnamePropertyName = grpname + ".outputname";

			string name = proj.Properties[outputnamePropertyName] ?? modulename;

			if (-1 != name.IndexOfAny(INVALID_EXECUTABLE_NAME_CHARACTERS))
			{
				var sb = new StringBuilder(name);
				foreach (var c in INVALID_EXECUTABLE_NAME_CHARACTERS)
				{
					sb.Replace(c.ToString(), String.Empty);
				}
				proj.Properties[outputnamePropertyName] = sb.ToString();
			}
		}
	}
}
