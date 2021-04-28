using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;

using NAnt.Core;
using NAnt.Core.Util;
using EA.FrameworkTasks.Model;
using EA.Eaconfig.Core;

namespace EA.Eaconfig.Build
{
    public static class NantInvocationProperties
    {
        public static string TargetToNantCommand(IModule module, TargetCommand targetCommand, bool addGlobal = false, PathString outputDir = null, Func<string, string> normalizePathString = null)
        {

            var  userproperties = new Dictionary<string, string>();

            foreach(var item in targetCommand.TargetInput.Data)
            {
                if(item.Type == TargetInput.InputItemType.Property)
                {
                    userproperties.Add(item.Name, module.Package.Project.Properties[item.SourceName]);
                }
            }

            var propertiesfilename = String.Format("target_{0}_properties_file.xml", targetCommand.Target);

            propertiesfilename = Path.Combine(outputDir.IsNullOrEmpty() ? module.IntermediateDir.Path : outputDir.Path, propertiesfilename);

            var properties = GetInvocationProperties(module, userproperties, addGlobal);

            if (normalizePathString != null)
            {
                foreach (var entry in properties.ToList())
                {
                    properties[entry.Key] = normalizePathString(entry.Value);
                }
            }

            PropertiesFileLoader.SavePropertiesToFile(propertiesfilename, properties);

            var normalize = normalizePathString == null ? noop : normalizePathString;

            return String.Format("{0}/nant.exe -propertiesfile:{1} {2} -buildfile:{3} -masterconfigfile:{4} -buildroot:{5}",
                normalize(Project.NantLocation),
                normalize(propertiesfilename).Quote(), 
                targetCommand.Target,
                normalize(module.Package.Project.BuildFileURI.LocalPath).Quote(),
                normalize(module.Package.Project.Properties[Project.NANT_PROPERTY_PROJECT_MASTERCONFIGFILE]).Quote(),
                normalize(module.Package.Project.Properties[Project.NANT_PROPERTY_PROJECT_BUILDROOT]).Quote()
                );
        }


        public static IDictionary<string, string> GetInvocationProperties(IModule module, IDictionary<string, string> userproperties, bool addGlobal = false)
        {
            var properties = new SortedDictionary<string, string>();

            if (module != null && module.Package.Project != null)
            {
                PropagateProperties(properties, userproperties);

                PropagateProperties(properties, GetStandardNantInvokeProperties(module as ProcessableModule));

                OptionSet commandlineproperties;
                if (module.Package.Project.NamedOptionSets.TryGetValue(Project.NANT_OPTIONSET_PROJECT_CMDPROPERTIES, out commandlineproperties))
                {
                    PropagateProperties(properties, commandlineproperties.Options);
                }

                if (addGlobal)
                {
                    // Set global properties from Masterconfig:
                    foreach (Project.GlobalPropertyDef propdef in Project.GlobalProperties.EvaluateExceptions(module.Package.Project))
                    {
                        if (propdef.Name == Project.NANT_PROPERTY_PROJECT_CMDTARGETS)
                        {
                            // This property is set at project initialization from the nant command line
                            continue;
                        }
                        if (!properties.ContainsKey(propdef.Name))
                        {
                            var val = module.Package.Project.Properties[propdef.Name];
                            if (val != null)
                            {
                                properties.Add(propdef.Name, val);
                            }

                        }
                    }
                }
            }
            return properties;
        }
        private static void PropagateProperties(IDictionary<string, string> target, IDictionary<string, string> source)
        {
            foreach (var prop in source)
            {
                if (prop.Value != null && !target.ContainsKey(prop.Key))
                {
                    target.Add(prop.Key, prop.Value);
                }
            }

        }

        private static IDictionary<string, string> GetStandardNantInvokeProperties(ProcessableModule module)
        {
            return new Dictionary<string, string>()
            {
                { "config",                 module.Package.Project.Properties["config"] },
                { "package.configs",        module.Package.Project.Properties["package.configs"] },

                { "build.module",           module.Name },
                { "build.buildtype",        module.BuildType.Name },
                { "build.buildtype.base",   module.BuildType.BaseName },
                { "groupname",              module.GroupName },
                { "eaconfig.build.group",   Enum.GetName(typeof(BuildGroups), module.BuildGroup) },
                { "build.outputname",       module.Package.Project.Properties[module.GroupName + ".outputname"] ?? module.Name },
            };
        }

        private static string noop(string input)
        {
            return input;
        }


    }

}
