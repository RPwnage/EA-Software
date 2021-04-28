using System;
using System.Collections.Generic;

using EA.FrameworkTasks.Model;

namespace EA.Eaconfig.Backends.VisualStudio
{
	public struct SolutionEntry
	{
		internal readonly string Name;
		internal readonly string ProjectGuidString;
		internal readonly string ProjectFileName;
		internal readonly string RelativeDir;
		internal readonly Guid ProjectTypeGuid;
		internal readonly Configuration[] ValidConfigs;
		internal readonly Configuration[] DeployConfigs;

		internal SolutionEntry(string name, string projectGuidString, string projectFileName, string relativeDir, Guid projectTypeGuid, Configuration[] validConfigs = null, Configuration[] deployConfigs = null) // TODO mixing of guids and guid strings feels nasty
		{
			Name = name;
			ProjectGuidString = projectGuidString;
			ProjectFileName = projectFileName;
			RelativeDir = relativeDir;
			ProjectTypeGuid = projectTypeGuid;
			ValidConfigs = validConfigs;
			DeployConfigs = deployConfigs ?? new Configuration[] { };
		}
	}
}
