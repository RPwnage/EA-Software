// (c) Electronic Arts. All rights reserved.

using NAnt.Core;
using NAnt.Core.Attributes;

using EA.Eaconfig.Core;
using EA.Eaconfig.Modules;


namespace EA.Eaconfig
{
	[TaskName("osx-postprocess")]
	class OSX_Postprocess : AbstractModuleProcessorTask
	{
		public override void Process(Module_DotNet module)
		{
			base.Process(module);
			AddDefaultAssemblies(module);
		}

		protected virtual void AddDefaultAssemblies(Module_DotNet module)
		{
			if (module.Compiler != null && module.Package.Project.Properties.GetBooleanPropertyOrDefault(PropGroupName("usedefaultassemblies"), true))
			{
				AddDefaultPlatformIndependentDotNetCoreAssemblies(module);
			}
		}
	}
}
