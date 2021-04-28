// (c) Electronic Arts. All rights reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

using NAnt.Core;
using NAnt.Core.Attributes;
using EA.FrameworkTasks.Model;
using EA.Eaconfig.Core;
using EA.Eaconfig.Modules.Tools;

namespace config.config.platform.options
{
	[TaskName("pc-vc-global-postprocess", XmlSchema = false)]
	public class pc_vc_global_postprocess : Task
	{
		protected override void ExecuteTask()
		{
			if (Project.Properties.GetBooleanPropertyOrDefault("eaconfig.multithreadeddynamiclib.makeconsistent", false))
			{
				MakeDynamicLibSettingsConsistent(false); // first pass: make dynamic lib settings consistent
			}

			if (Project.Properties.GetBooleanPropertyOrDefault("eaconfig.multithreadeddynamiclib.verify", false))
			{
				MakeDynamicLibSettingsConsistent(true); // second pass: verify that dynamic lib settings are consistent, otherwise fail
			}
		}

		private void MakeDynamicLibSettingsConsistent(bool verify_consistency)
		{
			foreach (var module in Project.BuildGraph().TopModules)
			{
				string toplevel_buildtype = ((ProcessableModule)module).BuildType.Name;
				string toplevel_module_name = ((ProcessableModule)module).Name;
				string toplevel_dynamiclib_setting = module.Package.Project.NamedOptionSets[toplevel_buildtype].Options["multithreadeddynamiclib"];

				if (toplevel_dynamiclib_setting != null)
				{
					foreach (var dependency in module.Dependents)
					{
						if (dependency.IsKindOf(DependencyTypes.Link) == false)
						{
							continue;
						}

						var dep_module = ((ProcessableModule)dependency.Dependent);
						string dependent_buildtype = dep_module.BuildType.Name;

						// interface dependencies have no attached project and are ignored
						if (dependent_buildtype == "UseDependency")
						{
							continue;
						}

						OptionSet dependent_optionset = dep_module.Package.Project.NamedOptionSets[dependent_buildtype];
						string dependent_dynamiclib_setting = dependent_optionset.Options["multithreadeddynamiclib"];

						// for modules like Utility modules that don't have this setting, skip and don't traverse dependents
						if (dependent_dynamiclib_setting == null || toplevel_dynamiclib_setting == dependent_dynamiclib_setting)
						{
							continue;
						}

						if (verify_consistency)
						{
							if (Project.Properties.GetBooleanPropertyOrDefault("eaconfig.multithreadeddynamiclib.makeconsistent", true))
							{
								Log.Error.WriteLine("Inconsistent multithreadeddynamiclib setting. Module '{0}' is set to '{1}' but dependent module '{2}' is set to '{3}'. "
									+ "Values may differ from values in the build script because a first pass attempt was made to make all dependent modules consistent but this failed "
									+ "because dependent may have multiple parent modules with a different setting that could not be resolved. "
									+ "First pass consistency correction can be disabled to help debug this issue by possibly providing a more accurate error message, "
									+ "set property eaconfig.multithreadeddynamiclib.makeconsistent to false.",
									toplevel_module_name, toplevel_dynamiclib_setting, ((ProcessableModule)dependency.Dependent).Name, dependent_dynamiclib_setting);
							}
							else
							{
								Log.Error.WriteLine("Inconsistent multithreadeddynamiclib setting. Module '{0}' is set to '{1}' but dependent module '{2}' is set to '{3}'.",
									toplevel_module_name, toplevel_dynamiclib_setting, ((ProcessableModule)dependency.Dependent).Name, dependent_dynamiclib_setting);
							}
							throw new BuildException("Inconsistent multithreadeddynamiclib setting");
						}
						else
						{
							dependent_optionset.Options["multithreadeddynamiclib"] = toplevel_dynamiclib_setting;
							string cc_options = dependent_optionset.Options["cc.options"];
							

							if (toplevel_dynamiclib_setting == "on")
							{
								cc_options = cc_options.Replace("-MTd", "-MDd").Replace("-MT", "-MD");

								CcCompiler compiler = dep_module.Tools.FirstOrDefault(t => t.ToolName == "cc") as CcCompiler;
								if (compiler != null)
								{
									for (int i = 0; i < compiler.Options.Count(); ++i)
									{
										if (compiler.Options[i] == "-MTd") compiler.Options[i] = "-MDd";
										if (compiler.Options[i] == "-MT") compiler.Options[i] = "-MD";
									}
								}
							}
							else if (toplevel_dynamiclib_setting == "off")
							{
								cc_options = cc_options.Replace("-MDd", "-MTd").Replace("-MD", "-MT");

								CcCompiler compiler = dep_module.Tools.FirstOrDefault(t => t.ToolName == "cc") as CcCompiler;
								if (compiler != null)
								{
									for (int i = 0; i < compiler.Options.Count(); ++i)
									{
										if (compiler.Options[i] == "-MDd") compiler.Options[i] = "-MTd";
										if (compiler.Options[i] == "-MD") compiler.Options[i] = "-MT";
									}
								}
							}

							dep_module.Package.Project.NamedOptionSets[dependent_buildtype].Options["cc.options"] = cc_options;
							if (dep_module.Package.Project.NamedOptionSets.Contains(dependent_buildtype + "-temp"))
							{
								dep_module.Package.Project.NamedOptionSets[dependent_buildtype + "-temp"].Options["cc.options"] = cc_options;
							}
						}
						
					}
				}
			}
		}

	}
}
