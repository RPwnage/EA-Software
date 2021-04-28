using EA.Eaconfig.Build;
using EA.Eaconfig.Core;
using EA.Eaconfig.Structured;
using EA.FrameworkTasks.Model;

namespace EA.Eaconfig.Modules
{
	public class Module_Java : ProcessableModule
	{
		public override bool CopyLocalUseHardLink => false;
		public override CopyLocalInfo.CopyLocalDelegate CopyLocalDelegate => CopyLocalInfo.CommonDelegate;
		public override CopyLocalType CopyLocal => CopyLocalType.Undefined;

		public string GradleDirectory;
		public string GradleProject;

		public Module_Java(string name, string groupname, string groupsourcedir, Configuration configuration, BuildGroups buildgroup, BuildType buildType, IPackage package, bool isreal = true) 
			: base(name, groupname, groupsourcedir, configuration, buildgroup, buildType, package, isreal)
		{
		}

		public override void Apply(IModuleProcessor processor)
		{
			processor.Process(this);
		}
	}
}
