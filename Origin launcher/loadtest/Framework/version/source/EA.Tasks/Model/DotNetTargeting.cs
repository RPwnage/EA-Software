using NAnt.Core;
using NAnt.Core.PackageCore;

using EA.Eaconfig;

namespace EA.Tasks.Model
{
	public enum DotNetFrameworkFamilies { Framework, Core, Standard }; 

	internal static class DotNetTargeting
	{
		internal static string GetNetVersion(Project project, DotNetFrameworkFamilies family)
		{
			if (family == DotNetFrameworkFamilies.Framework)
			{
				if (PackageMap.Instance.TryGetMasterPackage(project, "DotNet", out MasterConfig.IPackage dotNetMasterPackage))
				{
					TaskUtil.Dependent(project, "DotNet", Project.TargetStyleType.Use);
					return project.Properties.GetPropertyOrDefault("package.DotNet.defaulttargetframeworkversion", dotNetMasterPackage.Version);
				}
				throw new BuildException("Cannot determine target .NET version - this is required by the build system even when not building .NET code. Please add DotNet package version to your masterconfig.");
			}
			else
			{
				string propNameExt = "";
				if (family == DotNetFrameworkFamilies.Core)
				{
					propNameExt = ".core";
				}
				else if (family == DotNetFrameworkFamilies.Standard)
				{
					propNameExt = ".standard";
				}
				if (PackageMap.Instance.TryGetMasterPackage(project, "DotNetCoreSdk", out MasterConfig.IPackage dotNetMasterPackage))
				{
					TaskUtil.Dependent(project, "DotNetCoreSdk", Project.TargetStyleType.Use);
					return project.Properties.GetPropertyOrDefault("package.DotNetCoreSdk.defaulttargetframeworkversion" + propNameExt, dotNetMasterPackage.Version);
				}
				throw new BuildException("Cannot determine target .NET version - this is required by the build system even when not building .NET code. Please add DotNetCoreSdk package version to your masterconfig."); // NUGET-TODO is this true? this really only applies .vcxproj trying to resolve .net framework
			}
		}
	}
}
