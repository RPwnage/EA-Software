using System;
using System.IO;
using System.Text;
using System.Linq;
using System.Collections.Generic;
using System.Collections.Concurrent;
using System.Threading;
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
    internal class ModuleProcessor_ProcessDependencies : ModuleDependencyProcessorBase, IDisposable
    {

        internal ModuleProcessor_ProcessDependencies(ProcessableModule module, string logPrefix)
            : base(module, logPrefix)
        {
        }

        public void Process(ProcessableModule module)
        {
            ProcessPackageLevelDependencies(module);

            ProcessModuleDependents(module);

            if (Log.Level >= Log.LogLevel.Detailed)
            {
                var log = new BufferedLog(Log);
                log.Info.WriteLine(LogPrefix + "dependents of {0} - {1}.", module.GetType().Name, module.ModuleIdentityString());
                log.IndentLevel += 6;
                foreach (var dependency in module.Dependents)
                {
                    log.Info.WriteLine("{0} : {1} - {2}", DependencyTypes.ToString(dependency.Type), dependency.Dependent.GetType().Name, dependency.Dependent.ModuleIdentityString());
                }
                log.Flash();
            }
        }

        private void ProcessPackageLevelDependencies(IModule module)
        {
            try
            {

                int errors = 0;

#if NANT_CONCURRENT_NEVER // DependentCollection is not thread safe
                Parallel.ForEach(GetModuleDependentDefinitions(module), (d) =>
#else
                foreach (var d in GetModuleDependentDefinitions(module))
#endif
                {
                    if (0 == Interlocked.CompareExchange(ref errors, 0, 0))
                    {
                        try
                        {

                            Log.Debug.WriteLine(LogPrefix+"{0}: processing dependent package '{1}' {2}", module.ModuleIdentityString(), d.Dependent.PackageName, DependencyTypes.ToString(d.Type));

                            //Set up Module dependencies:
                            IPackage dependentPackage = null;
                            var packageVersion = PackageMap.Instance.GetMasterVersion(d.Dependent.PackageName, Project);

                            if (!Project.BuildGraph().TryGetPackage(d.Dependent.PackageName, packageVersion, d.Dependent.ConfigurationName, out dependentPackage))
                            {
                                var PackageDependentProcessingStatus = Project.BuildGraph().GetPackageDependentProcessingStatus(module.Package);

                                if (PackageDependentProcessingStatus != null && PackageDependentProcessingStatus.ProcessedModules.Contains(module.BuildGroup + ":" + module.Name))
                                {
                                    Log.Error.WriteLine(LogPrefix + "Module {0}  declares '{1}' dependency on {2} ({3}) and can't find 'Package' for dependent.",
                                        module.ModuleIdentityString(), DependencyTypes.ToString(d.Type), d.Dependent.PackageName, d.Dependent.ConfigurationName);
                                }
                            }
                            else
                            {
                                if (d.IsKindOf(DependencyTypes.Build | DependencyTypes.AutoBuildUse))
                                {
                                    if (!dependentPackage.IsKindOf(EA.FrameworkTasks.Model.Package.AutoBuildClean))
                                    {
                                        if (d.IsKindOf(DependencyTypes.Build))
                                        {
                                            Log.Info.WriteLine("{0}  Module {1}/{2} declares build dependency on non buildable package {3}. Dependency reset to use dependency", module.Configuration.Name, module.Package.Name, module.Name, dependentPackage.Name);
                                        }
                                        d.ClearType(DependencyTypes.Build | DependencyTypes.AutoBuildUse);
                                    }
                                }

                                // Modules declared in Initialize.Xml
                                var declaredModules = (Project.Properties["package." + d.Dependent.PackageName + "." + d.BuildGroup + ".buildmodules" + BuildType.SubSystem] ??
                                                       Project.Properties[PropGroupName(d.Dependent.PackageName + "." + d.BuildGroup + ".buildmodules")]).ToArray();

                                // Explicit subset of modules to depend on
                                var moduleConstraints = GetModuleDependenciesConstraints(Project, module, d.Dependent.PackageName);


                                // Modules already created in the package
                                IEnumerable<IModule> definedModules = null;

                                if ((dependentPackage.IsKindOf(EA.FrameworkTasks.Model.Package.AutoBuildClean)))
                                {
#if FRAMEWORK_PARALLEL_TRANSITION
                                    uint includeCrossSubSystemTypes = Module.MakeStyle;
#else
                                    uint includeCrossSubSystemTypes = Module.Utility | Module.MakeStyle;
#endif
                                    if(module.IsKindOf(includeCrossSubSystemTypes))
                                    {
                                        definedModules = dependentPackage.Modules;
                                    }
                                    else
                                    {
                                        if ("ps3" == module.Configuration.System)
                                        {
                                            //IMTODO Legacy code: need deprecation path. Try to make a generic rule instead. Respect explicitly defined module dependencies here.
                                            if (".spu" != module.Configuration.SubSystem)
                                            {
                                                // Only add spu modules that are non-library types (see SlnMaker.cs in nantToVSTools)
                                                definedModules = dependentPackage.Modules.Where(m => (".spu" != m.Configuration.SubSystem || !m.IsKindOf(Module.Library) || (module.Package.Name == m.Package.Name && m.IsKindOf(includeCrossSubSystemTypes)) && m.BuildGroup == d.BuildGroup)
                                                    || (moduleConstraints != null && moduleConstraints.Any(c => c.Group == m.BuildGroup && c.ModuleName == m.Name)));
                                            }
                                            else
                                            {
                                                definedModules = dependentPackage.Modules.Where(m => ((m.Configuration.SubSystem == module.Configuration.SubSystem || (module.IsKindOf(Module.Library) && m.IsKindOf(includeCrossSubSystemTypes))) && m.BuildGroup == d.BuildGroup)
                                                    || (moduleConstraints != null && moduleConstraints.Any(c => c.Group == m.BuildGroup && c.ModuleName == m.Name)));
                                            }
                                        }
                                        else if ("vc" == module.Configuration.Compiler && module.IsKindOf(Module.Library) && module.IsKindOf(Module.Native))
                                        {
                                            // Exclude dependencies from native libraries to Managed code unless they were set through constraint
                                            definedModules = dependentPackage.Modules.Where(m => (m.Configuration.SubSystem == module.Configuration.SubSystem || m.IsKindOf(includeCrossSubSystemTypes))
                                                                       && (!m.IsKindOf(Module.Managed | Module.DotNet) || (moduleConstraints != null && moduleConstraints.Any(c => c.Group == m.BuildGroup && c.ModuleName == m.Name))));
                                        }
                                        else
                                        {
                                            // Only add modules from the same subsystem
                                            definedModules = dependentPackage.Modules.Where(m => m.Configuration.SubSystem == module.Configuration.SubSystem || m.IsKindOf(includeCrossSubSystemTypes)
                                                                                           || (moduleConstraints != null && moduleConstraints.Any(c => c.Group == m.BuildGroup && c.ModuleName == m.Name)));
                                        }
                                    }

                                    if (definedModules.Count() == 0 || (moduleConstraints.IsNullOrEmpty() && definedModules.Where(defmod=>defmod.BuildGroup == d.BuildGroup).Count()==0))
                                    {
                                        if (Log.WarningLevel >= Log.WarnLevel.Advise)
                                        {
                                            // We have a build dependency on the package, but there are no defined build modules.
                                            // Obviously this is not a correct dependency on this package, but we accept it anyway, change it to an interface dependency and warn the user...

                                            string subsystem = String.IsNullOrEmpty(module.Configuration.SubSystem) ? "<none>" : module.Configuration.SubSystem;
                                            Log.Warning.WriteLine("module [{0}] has a build dependency on Package [{1}], but that package has no buildable modules for subsystem [{2}]", module.Name, dependentPackage.Name, subsystem);
                                            Log.Warning.WriteLine("Adding [{0}] as if it was an interface dependency.  The build scripts in Package [{1}] should be changed to reflect this.", dependentPackage.Name, module.Package.Name);
                                        }

                                        // We want to clean build type mask
                                        d.ClearType(DependencyTypes.Build | DependencyTypes.AutoBuildUse);

                                        definedModules = null;
                                    }

                                    if (!dependentPackage.IsKindOf(EA.FrameworkTasks.Model.Package.AutoBuildClean))
                                    {
                                        //IM: WE should never get here as Dependent task would take care of this!

                                        // We have a build dependency on a package which is buildable.  But the autobuildclean attribute is set to false for this package.
                                        // Hence we treat is as a use dependency.
                                        definedModules = null;

                                        // Clear the previous type and set the dependency as a use dependency
                                        d.ClearType(DependencyTypes.Build | DependencyTypes.AutoBuildUse);
                                    }
                                }

                                var useDependencyModules = new List<IModule>();

                                if (declaredModules.Count() == 0)
                                {
                                    if (definedModules == null)
                                    {
                                        useDependencyModules.Add(GetOrAddModule_UseDependency(Module_UseDependency.PACKAGE_DEPENDENCY_NAME, module.Configuration, d.BuildGroup, module.Configuration.SubSystem, dependentPackage));
                                    }
                                }
                                else
                                {
                                    foreach (var depmodule in declaredModules)
                                    {
                                        IModule dependentModule = null;

                                        if (definedModules != null)
                                        {
                                            dependentModule = definedModules.FirstOrDefault(m => m.Name == depmodule);
                                        }
                                        else
                                        {

                                            dependentModule = GetOrAddModule_UseDependency(depmodule, module.Configuration, d.BuildGroup, module.Configuration.SubSystem, dependentPackage);
                                            useDependencyModules.Add(dependentModule);
                                        }

                                        if (dependentModule == null && Log.WarningLevel >= Log.WarnLevel.Advise)
                                        {
                                            Log.Warning.WriteLine("Module '{0}' declared in intialize.xml file of package '{1}' is not defined in the {1}.build file.", depmodule, dependentPackage.Name);
                                            Log.Warning.WriteLine("{1}\tDeclared modules (Initialize.xml):   " + "{2};{1}\tDefined modules ({0}.build): {3};{1}", dependentPackage.Name, Environment.NewLine, declaredModules.ToString(", "), definedModules.ToString(", ", m => m.Name));
                                            Log.Warning.WriteLine("This may be due to the [package.{0}.buildmodules] property including modules from the wrong subsystem (e.g., non-spu and spu modules).", dependentPackage.Name);
                                            Log.Warning.WriteLine("To correct this, separate out the spu modules into a separate property named [package.{0}.buildmodules.spu]", dependentPackage.Name);
                                        }
                                    }
                                }

                                if (moduleConstraints != null && Log.WarningLevel >= Log.WarnLevel.Advise)
                                {
                                    // Check definitions
                                    foreach (var constraint in moduleConstraints)
                                    {
                                        if (null == (definedModules ?? useDependencyModules).FirstOrDefault(m => m.BuildGroup == constraint.Group && m.Name == constraint.ModuleName))
                                        {
                                            Log.Warning.WriteLine("Module '{0}' declares dependency on a module '{1}.{2}' of package '{3}' which is not defined in the {3}.build file or Intialize.xml.", module.Name, constraint.Group, constraint.ModuleName, dependentPackage.Name);
                                        }
                                    }
                                }

                                foreach (var depnentModule in (definedModules ?? useDependencyModules))
                                {
                                    var dependencyType = new BitMask(d.Type);

                                    if (!moduleConstraints.IsNullOrEmpty())
                                    {
                                        var constrainDef = moduleConstraints.FirstOrDefault(c => c.Group == depnentModule.BuildGroup && c.ModuleName == depnentModule.Name);
                                        if (constrainDef == null)
                                        {
                                            //Skip this module because it is not in the constraint.
                                            continue;
                                        }
                                        else
                                        {
                                            //Refine dependency flags based on constraints 
                                            RefineDependencyTypeFromConstraint(dependencyType, constrainDef);

                                            if (dependencyType.Type != d.Type)
                                            {
                                                Log.Info.WriteLine("Dependency '{0}' on a '{1}' changed type from '{2}' to '{3}'", module.ModuleIdentityString(), depnentModule.ModuleIdentityString(), DependencyTypes.ToString(d.Type), DependencyTypes.ToString(dependencyType.Type));
                                            }
                                        }
                                    }
                                    else if (depnentModule.BuildGroup != d.BuildGroup)
                                    {
                                            continue;
                                    }

                                    //IMTODO: logic below looks kind of messy. Can we deal with linking/not linking different subsystems in a beter, more generic way.
                                    // Maybe follow old nant and include public libs if they are defined but not direct output?
                                    if (depnentModule.Configuration.SubSystem != module.Configuration.SubSystem &&
                                        dependentPackage.IsKindOf(EA.FrameworkTasks.Model.Package.AutoBuildClean) &&
                                        !depnentModule.IsKindOf(Module.MakeStyle | Module.Utility))
                                    {
                                        // Don't link different subsystems. 
                                        // Unless dependent is non buildable or Utility or MakeStyle, and in this case we don't know it's subsystem and rely on the libs.+".subsystem syntax.
                                        dependencyType.ClearType(DependencyTypes.Link);
                                        dependencyType.SetType(DependencyTypes.Interface);
                                    }

                                    if (d.IsKindOf(DependencyTypes.AutoBuildUse) && depnentModule.IsKindOf(Module.MakeStyle | Module.Utility))
                                    {
                                        dependencyType.SetType(DependencyTypes.Build);
                                    }

                                    module.Dependents.Add(depnentModule, dependencyType.Type);
                                }
                            }
                        }
                        catch (Exception)
                        {
                            if (1 == Interlocked.Increment(ref errors))
                            {
                                throw;
                            }
                        }
                    }
                }
#if NANT_CONCURRENT_NEVER
                );
#endif

            }
            catch (Exception e)
            {
                Log.Error.WriteLine(ThreadUtil.GetStackTrace(e));

                ThreadUtil.RethrowThreadingException(
                    String.Format("Error processing dependents (ModuleProcessor_ProcessDependencies.ProcessPackageLevelDependencies) of [{0}-{1}], module '{2}'", module.Package.Name, module.Package.Version, module.Name),
                    Location.UnknownLocation, e);
            }
        }

        // Dependencies between modules in the same package
        private void ProcessModuleDependents(IModule module)
        {
            foreach (BuildGroups dependentGroup in Enum.GetValues(typeof(BuildGroups)))
            {
                foreach (var v in Mappings.ModuleDependencyPropertyMapping)
                {
                    string depPropName = String.Format("{0}.{1}.{2}{3}", module.BuildGroup, module.Name, dependentGroup, v.Key);
                    string depPropNameConfig = depPropName + "." + module.Configuration.System;

                    var depModuleNames = String.Concat(Project.Properties[depPropNameConfig], Environment.NewLine, Project.Properties[depPropName]).ToArray();

                    foreach (string dependentModule in depModuleNames)
                    {
                        bool found = false;
                        foreach (IModule m in module.Package.Modules)
                        {
                            if (m.BuildGroup == dependentGroup && m.Name == dependentModule)
                            {
                                uint dependencyType = v.Value;

                                if (v.Key == ".moduledependencies")
                                {
                                    // ".moduledependencies" behave as auto. We can change this if needed. But is seems to make sense for efficiency
                                    if (module.Configuration.SubSystem == m.Configuration.SubSystem && !module.IsKindOf(Module.Utility | Module.MakeStyle) && !m.IsKindOf(Module.Utility | Module.MakeStyle))
                                    {
                                        dependencyType |= DependencyTypes.AutoBuildUse;
                                        dependencyType &= ~DependencyTypes.Build;
                                    }
                                }

                                if (AutoBuildUse) 
                                {
                                    if (HasLink && (dependencyType & DependencyTypes.AutoBuildUse) == DependencyTypes.AutoBuildUse)
                                    {
                                        dependencyType |= DependencyTypes.Build;
                                    }
                                }
                                else if ((dependencyType & DependencyTypes.AutoBuildUse) == DependencyTypes.AutoBuildUse)
                                {
                                    dependencyType |= DependencyTypes.Build;
                                }

                                else if (module.Configuration.SubSystem != m.Configuration.SubSystem)
                                {
                                    dependencyType &= ~DependencyTypes.Link;
                                }
#if FRAMEWORK_PARALLEL_TRANSITION
                                 dependencyType &= ~DependencyTypes.Interface;
#endif
                                module.Dependents.Add(m, dependencyType);
                                found = true;
                                break;
                            }
                        }
                        if (!found)
                        {
                            module.Package.Project.Log.Warning.WriteLine("package {0}-{1} ({2}) {3}: Module {4}.{5} specified in property '{6}' is not defined in this package.", module.Package.Name, module.Package.Version, module.Configuration.Name, module.BuildGroup + "." + module.Name, dependentGroup, dependentModule, Project.Properties[depPropNameConfig] != null ? depPropNameConfig : depPropName);
                        }
                    }
                }
            }
        }
        
        private IModule GetOrAddModule_UseDependency(string name, Configuration configuration, BuildGroups buildgroup, string subsystem, IPackage package)
        {
            return Project.BuildGraph().Modules.GetOrAdd(Module.MakeKey(name, buildgroup, package), new Module_UseDependency(name, configuration, buildgroup, subsystem, package));
        }


        #region Dispose interface implementation

        public void Dispose()
        {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        void Dispose(bool disposing)
        {
            if (!this._disposed)
            {
                if (disposing)
                {
                    // Dispose managed resources here
                }
            }
            _disposed = true;
        }
        private bool _disposed = false;

        #endregion
    }
}


