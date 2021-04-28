// (c) Electronic Arts. All rights reserved.

using NAnt.Core;
using NAnt.Core.Attributes;
using EA.FrameworkTasks.Model;

using EA.Eaconfig.Core;
using EA.Eaconfig.Modules;

namespace EA.Eaconfig
{
	[TaskName("unix-common-postprocess", XmlSchema = false)]
	public class Unix_Common_PostprocessOptions : AbstractModuleProcessorTask
	{

		public override void Process(Module_Native module)
		{
			base.Process(module);

			AddAssetDeployment(module);
		}

		public override void Process(Module_DotNet module)
		{
			base.Process(module);
			AddDefaultAssemblies(module);
			AddAssetDeployment(module);
		}

		protected virtual void AddDefaultAssemblies(Module_DotNet module)
		{
			if (module.Compiler != null && module.Package.Project.Properties.GetBooleanPropertyOrDefault(PropGroupName("usedefaultassemblies"), true))
			{
				AddDefaultPlatformIndependentDotNetCoreAssemblies(module);
			}
		}

		public virtual void AddAssetDeployment(Module module)
		{
			if (module.DeployAssets && module.Assets != null && module.Assets.Includes.Count > 0)
			{
				var step = new BuildStep("postbuild", BuildStep.PostBuild);
				var targetInput = new TargetInput();
				step.TargetCommands.Add(new TargetCommand("copy-asset-files.unix", targetInput, nativeNantOnly:false));
				module.BuildSteps.Add(step);
			}
		}

	}
}
