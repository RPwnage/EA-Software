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
using EA.FrameworkTasks.Model;

using EA.Eaconfig;
using EA.Eaconfig.Core;
using EA.Eaconfig.Build;
using EA.Eaconfig.Modules;
using EA.Eaconfig.Modules.Tools;

namespace EA.Eaconfig
{
	[TaskName("stadia-common-postprocess", XmlSchema = false)]
	public class Stadia_Common_PostprocessOptions : AbstractModuleProcessorTask
	{

		public override void Process(Module_Native module)
		{
			base.Process(module);

			AddAssetDeployment(module);
		}

		public override void Process(Module_DotNet module)
		{
			base.Process(module);
			AddAssetDeployment(module);
		}

		public virtual void AddAssetDeployment(Module module)
		{
			if (module.DeployAssets && module.Assets != null && module.Assets.Includes.Count > 0)
			{
				var step = new BuildStep("postbuild", BuildStep.PostBuild);
				var targetInput = new TargetInput();
				step.TargetCommands.Add(new TargetCommand("copy-asset-files.stadia", targetInput, nativeNantOnly:false));
				module.BuildSteps.Add(step);
			}
		}

	}
}
