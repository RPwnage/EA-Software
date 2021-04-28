// (c) Electronic Arts. All rights reserved.
using System;

using NAnt.Core;
using NAnt.Core.Tasks;
using NAnt.Core.Attributes;

namespace EA.Eaconfig
{
	[TaskName("generateDefaultBuildLayout")]
	public class LegacyGenerateBuildLayout : NAnt.Core.Task
	{
		[TaskAttribute("OutputFile", Required = true)]
		public string OutputFile { get; set; }

		[TaskAttribute("LicenseeName", Required = false)]
		public string LicenseeName { get; set; }

		[TaskAttribute("BuildType", Required = true)]
		public string BuildType { get; set; }

		[TaskAttribute("EntrypointModule", Required = true)]
		public string EntrypointModule { get; set; }

		[TaskAttribute("Group", Required = false)]
		public string Group
		{
			get
			{
				if (String.IsNullOrEmpty(_group))
					return "runtime";
				return _group;
			}
			set { _group = value; }
		}
		string _group;

		[TaskAttribute("EntrypointSuffix", Required = false)]
		public string EntrypointSuffix { get; set; }
	
		protected override void ExecuteTask()
		{
			string fwBuildType = "StdProgram";
			if (!String.IsNullOrEmpty(EntrypointSuffix) && EntrypointSuffix.ToLowerInvariant().Contains("dll"))
			{
				fwBuildType = "DynamicLibrary";
			}

			string groupName = Group + "." + EntrypointModule;

			DoOnce.Execute(String.Format("generateBuildLayout.legacy.warning.{0}", Location), () =>
			{
				if (Project.Properties.GetBooleanPropertyOrDefault(groupName + "structured-xml-used", false))
				{
					if (Project.Properties.GetBooleanPropertyOrDefault("framework.create-build-layout-for-default-types", false))
					{
						Project.Log.ThrowWarning(NAnt.Core.Logging.Log.WarningId.LegacyGenerateBuildLayoutTask, NAnt.Core.Logging.Log.WarnLevel.Normal,
							"{0} Using legacy 'generateBuildLayout' task. " +
							"The masterconfig has set the property 'framework.create-build-layout-for-default-types=true' so a buildlayout will be generated automatically " +
							"with the default type set by the property 'eaconfig.default-buildlayout-type'. If you were generating a buildlayout file with a different type " +
							"you will need to explicitly add a <buildlayout> element to your modules structured xml definition. " +
							"Otherwise you can simply remove the legacy 'generateBuildLayout' task or wrap it in a condition so it is not interpreted when using newer versions of framework such as this.", Location);
					}
					else
					{
						Project.Log.ThrowWarning(NAnt.Core.Logging.Log.WarningId.LegacyGenerateBuildLayoutTask, NAnt.Core.Logging.Log.WarnLevel.Normal,
							"{0} Using legacy 'generateBuildLayout' task. " +
							"Instead use the <buildlayout> element in your structured module definition. " +
							"(For example <buildlayout build-type=\"EATest\" enable=\"true\" />)", Location);
					}
				}
			});

			Project.Properties[groupName + ".build-layout-type"] = BuildType;
			Project.Properties[groupName + ".generate-buildlayout"] = "true";

			OptionSet buildOptionSet = OptionSetUtil.GetOptionSetOrFail(Project, fwBuildType);
			OptionSet defaultLayoutOptions = Project.NamedOptionSets["config-build-layout-tags-common"];
			OptionSet buildLayoutTags = defaultLayoutOptions != null ? new OptionSet(defaultLayoutOptions) : new OptionSet();
			buildLayoutTags.Options["Configuration"] = Project.Properties.GetPropertyOrFail("config-name");
			buildLayoutTags.Options["BuildType"] = BuildType;
			buildLayoutTags.Options["Dll"] = Project.Properties.GetPropertyOrDefault("Dll", "false"); // whether configuration is a dll configuration, not whether this specific module is a dll
			buildLayoutTags.Options["Fastlink"] = buildOptionSet.GetOptionOrDefault("debugfastlink", Project.GetPropertyOrDefault("eaconfig.debugfastlink", "off"));
			buildLayoutTags.Options["CodeCoverage"] = buildOptionSet.GetOptionOrDefault("codecoverage", Project.GetPropertyOrDefault("eaconfig.codecoverage", "false"));
			buildLayoutTags.Options["IsFBConfiguration"] = Project.Properties.GetPropertyOrDefault("config-use-FBConfig", "false");
			buildLayoutTags.Options["ModuleBaseDirectory"] = Project.BaseDirectory;
			buildLayoutTags.Options["ModulePackageDirectory"] = Project.GetPropertyOrFail("package.dir");
			buildLayoutTags.Options["UseTAPEventsForTests"] = Project.Properties.GetPropertyOrDefault("use-tap-events-for-tests", "true");
			if (LicenseeName != null)
			{
				buildLayoutTags.Options["LicenseeName"] = LicenseeName;
			}

			Project.NamedOptionSets[groupName + ".build-layout-tags"] = buildLayoutTags;

			OptionSet entryPointOptions = Project.NamedOptionSets["config-build-layout-entrypoint-common"];
			OptionSet buildLayoutEntry = new OptionSet(entryPointOptions);
			Project.NamedOptionSets[groupName + ".build-layout-entrypoint"] = buildLayoutEntry;
		}
	}
}
