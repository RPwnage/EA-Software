using System;
using System.IO;
using System.Text;
using System.Linq;
using System.Collections.Generic;
using System.Collections.Concurrent;
using System.Threading.Tasks;

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Logging;
using NAnt.Core.Threading;
using NAnt.Core.Reflection;
using NAnt.Core.PackageCore;
using EA.FrameworkTasks.Model;
using NAnt.Shared.Properties;

using EA.Eaconfig;
using EA.Eaconfig.Core;
using EA.Eaconfig.Modules;
using EA.Eaconfig.Modules.Tools;

namespace EA.Eaconfig.Build
{
    abstract public class ModuleDependencyProcessorBase : ModuleProcessorBase
    {
        protected class PackageDependencyDefinition
        {
            public readonly string Key;
            public readonly string PackageName;
            public readonly string ConfigurationName;

            public PackageDependencyDefinition(string packagename, string configuration)
            {
                PackageName = packagename;
                ConfigurationName = configuration;
                Key = CreateKey(packagename, configuration);
            }
            private static string CreateKey(string packagename, string configuration)
            {
                return packagename + ":" + configuration;
            }
        }

         protected ModuleDependencyProcessorBase(ProcessableModule module, string logPrefix= "")
            : base(module, logPrefix)
        {
#if FRAMEWORK_PARALLEL_TRANSITION
            HasLink = false;
            AutoBuildUse = false;
#else
            HasLink = BuildOptionSet.Options["build.tasks"].ToArray().Contains("link") || module.IsKindOf(Module.DotNet);
            AutoBuildUse = Project.Properties.GetBooleanPropertyOrDefault(FrameworkProperties.AutoBuildUseDependencyPropertyName, true);
#endif
        }

         protected readonly bool HasLink;
         protected readonly bool AutoBuildUse;


         protected IEnumerable<Dependency<PackageDependencyDefinition>> GetModuleDependentDefinitions(IModule module)
        {
             // We need ordered collection here so that dependencies are defined in the same order they are listed in the build file:
            var depList = new List<Dependency<PackageDependencyDefinition>>();

            var dependencies = new Dictionary<string, Dependency<PackageDependencyDefinition>>();

            foreach (var v in Mappings.DependencyPropertyMapping)
            {
                foreach (string dependent in GetModulePropertyValue(v.Key).ToArray())
                {
                    Dependency<PackageDependencyDefinition> depDef;

                    if (GetOrAddPackageDependency(dependencies, dependent, module.Configuration.Name, out depDef))
                    {
/*
#if DEBUG
                        Log.Warning.WriteLine(String.Format("Package [{0}], module [{1}] has multiple dependencies on Package [{2}]", module.Package.Name, module.Name, dependent));
                        Log.Warning.WriteLine(String.Format("Appending previous dependency type [{0}] with new type [{1}].", Mappings.DependencyPropertyMapping.Where(dpm => dpm.Value == depDef.Type).FirstOrDefault().Key, v.Key));
                        Log.Warning.WriteLine("The build script(s) for Package [{0}] should be cleaned up as this may result in undesired behavior.", module.Package.Name);
#endif
*/
                    }
                    else
                    {
                        depList.Add(depDef);
                    }

                    depDef.SetType(v.Value);

                    if (depDef.IsKindOf(DependencyTypes.AutoBuildUse))
                    {
                        if (AutoBuildUse)
                        {
                            if (HasLink)
                            {
                                depDef.SetType(DependencyTypes.Build);
                            }
                        }
                        else
                        {
                            depDef.SetType(DependencyTypes.Build);
                            depDef.ClearType(DependencyTypes.AutoBuildUse);
                        }
                    }
                }
            }
            return depList;
        }

         private bool GetOrAddPackageDependency(Dictionary<string, Dependency<PackageDependencyDefinition>> dependencies, string packageName, string configuration, out Dependency<ModuleDependencyProcessorBase.PackageDependencyDefinition> depDef)
         {
             bool dependencyAlreadyExists = false;

             string key = DependencyDefinition.CreateKey(packageName, configuration);
             if (!dependencies.TryGetValue(key, out depDef))
             {
                 depDef = new Dependency<PackageDependencyDefinition>(new PackageDependencyDefinition(packageName, configuration));
                 dependencies.Add(key, depDef);
             }
             else
             {
                 dependencyAlreadyExists = true;

             }
             return dependencyAlreadyExists;
         }

         internal static void RefineDependencyTypeFromConstraint(BitMask packagedep, BitMask constraint)
         {
             uint newtype = packagedep.Type & constraint.Type;
             packagedep.ClearType(packagedep.Type);
             packagedep.SetType(newtype);
         }

         protected List<ModuleDependencyConstraints> GetModuleDependenciesConstraints(Project project, IModule module, string dependentPackage)
         {
             List<ModuleDependencyConstraints> constraints = null;
             // Set moduledependencies for dependent package(constraints). Dependencies between modules in the same package are set elswhere
             foreach (BuildGroups dependentGroup in Enum.GetValues(typeof(BuildGroups)))
             {
                 foreach (var v in Mappings.ModuleDependencyConstraintsPropertyMapping)
                 {
                     string constraintsName = String.Format("{0}.{1}.{2}{3}", module.GroupName, dependentPackage, dependentGroup, v.Key);

                     foreach (string dependentModule in project.Properties[constraintsName].ToArray())
                     {
                         if (constraints == null)
                         {
                             constraints = new List<ModuleDependencyConstraints>();
                         }

                         var depDef = constraints.FirstOrDefault(c => c.Group == dependentGroup && c.ModuleName == dependentModule);

                         if (depDef == null)
                         {
                             depDef = new ModuleDependencyConstraints(dependentGroup, dependentModule, v.Value);
                             constraints.Add(depDef);
                         }
                         else
                         {
                             depDef.SetType(v.Value);
                         }

                         if (depDef.IsKindOf(DependencyTypes.AutoBuildUse))
                         {
                             if (AutoBuildUse)
                             {
                                 if (HasLink)
                                 {
                                     depDef.SetType(DependencyTypes.Build);
                                 }
                             }
                             else
                             {
                                 depDef.SetType(DependencyTypes.Build);
                                 depDef.ClearType(DependencyTypes.AutoBuildUse);
                             }
                         }
                     }
                 }
             }
             return constraints;
         }

    }
}
