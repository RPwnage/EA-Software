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
using EA.FrameworkTasks.Model;
using NAnt.Shared.Properties;

using EA.Eaconfig;
using EA.Eaconfig.Core;
using EA.Eaconfig.Modules;
using EA.Eaconfig.Modules.Tools;

namespace EA.Eaconfig.Build
{
    internal class ModuleProcessor_AddModuleDependentsPublicData : ModuleProcessorBase, IModuleProcessor, IDisposable
    {
        internal ModuleProcessor_AddModuleDependentsPublicData(ProcessableModule module, ConcurrentDictionary<string, DependentCollection> transitivecollection, string logPrefix)
            : base(module, logPrefix)
        {
            TransitiveCollection = transitivecollection;
            IsTransitiveDependencies = transitivecollection != null;
        }

#if FRAMEWORK_PARALLEL_TRANSITION
        public bool ProcessGenerationData = false;
#endif

        private readonly ConcurrentDictionary<string, DependentCollection> TransitiveCollection;

       

        private readonly bool IsTransitiveDependencies;

        public static void VerifyPackagePublicData(Package package, string logPrefix)
        {
            var verification_supression_property = "package." + package.Name + ".supress-public-data-verification";

            if (package != null && (package.FrameworkVersion != FrameworkVersions.Framework1)
                && !package.Project.Properties.GetBooleanPropertyOrDefault(verification_supression_property, false))
            {
                var packageBuildLibs = package.Modules.Where(m => !(m is Module_UseDependency)).Select(m => m.LibraryOutput()).Where(lib => lib != null);
                var packageBuildDlls = package.Modules.Where(m => !(m is Module_UseDependency)).Select(m => m.DynamicLibOrAssemblyOutput()).Where(dll => dll != null);

                foreach (var module in package.Modules.Where(m => !(m is Module_UseDependency)))
                {
                    var outputlib = module.LibraryOutput();
                    var outputdll = module.DynamicLibOrAssemblyOutput();

                    var removedlibraries = new HashSet<PathString>(); // Collect all removed libs to print warning.
                    var removeddlls = new HashSet<PathString>(); // Collect all removed assemblies to print warning.

                    foreach (var data in package.GetModuleAllPublicData(module))
                    {
                        // ------------ Test libraries ------------------------------------------------------------
                        if (data.Libraries != null && data.Libraries.Includes.Count > 0)
                        {
                            var suspectlibs = (outputlib == null) ? packageBuildLibs : packageBuildLibs.Where(lib => outputlib != lib);
                            if (suspectlibs.Count() > 0)
                            {
                                var cleanedPublicLib = new FileSet();
                                foreach (var fileItem in data.Libraries.FileItems)
                                {
                                    if (!suspectlibs.Contains(fileItem.Path))
                                    {
                                        cleanedPublicLib.Include(fileItem);
                                    }
                                    else
                                    {
                                        if (Log.WarningLevel >= Log.WarnLevel.Advise)
                                        {
                                            removedlibraries.Add(fileItem.Path);
                                        }
                                    }
                                }

                                cleanedPublicLib.FailOnMissingFile = false;
                                ((PublicData)data).Libraries = cleanedPublicLib;
                            }
                        }
                        // ------------ Test assemblies------------------------------------------------------------
                        if (data.Assemblies != null && data.Assemblies.Includes.Count > 0)
                        {
                            var suspectdlls = (outputdll == null) ? packageBuildDlls : packageBuildDlls.Where(dll => outputdll != dll);
                            if (suspectdlls.Count() > 0)
                            {
                                var cleanedPublicDlls = new FileSet();
                                foreach (var fileItem in data.Assemblies.FileItems)
                                {
                                    if (!suspectdlls.Contains(fileItem.Path))
                                    {
                                        cleanedPublicDlls.Include(fileItem);
                                    }
                                    else
                                    {
                                        removeddlls.Add(fileItem.Path);
                                    }
                                }

                                cleanedPublicDlls.FailOnMissingFile = false;
                                ((PublicData)data).Assemblies = cleanedPublicDlls;
                            }
                        }
                    }
                    if (Log.WarningLevel >= Log.WarnLevel.Advise)
                    {
                        if (removedlibraries.Count > 0)
                        {
                            package.Project.Log.Warning.WriteLine(logPrefix + "package {0}-{1} ({2}) {3}:{4}\t\t\tFollowing public libraries defined in Initialize.xml file {4}{4}{5}{4}{4}\t\t\tare removed from the module '{3}' public data because these libraries are built by other modules in the same package.{4}\t\t\tFix Initialize.xml file by definig libs per module, or, if this behavior is intended - suppress autoremoval functionality for the package - define property '{6}=true' in the package build script.", package.Name, package.Version, package.ConfigurationName, module.BuildGroup + "." + module.Name, Environment.NewLine, removedlibraries.ToString(Environment.NewLine, ps => "\t\t\t\t\t" + ps.Path), verification_supression_property);
                        }
                        if (removeddlls.Count > 0)
                        {
                            package.Project.Log.Warning.WriteLine(logPrefix + "package {0}-{1} ({2}) {3}:{4}\t\t\tFollowing public assemblies defined in Initialize.xml file {4}{4}{5}{4}{4}\t\t\tare removed from the module '{3}' public data because these assembliesare built by other modules in the same package.{4}\t\t\tFix Initialize.xml file by definig assemblies per module, or, if this behavior is intended - suppress autoremoval functionality for the package - define property '{6}=true' in the package build script.", package.Name, package.Version, package.ConfigurationName, module.BuildGroup + "." + module.Name, Environment.NewLine, removeddlls.ToString(Environment.NewLine, ps => "\t\t\t\t\t" + ps.Path), verification_supression_property);
                        }
                    }
                }
            }
        }


        public void Process(ProcessableModule module)
        {
            //Log.Info.WriteLine(LogPrefix + "{0}-{1} ({2}.{3} {4}) [{5}]", module.Package.Name, module.Package.Version, module.BuildGroup.ToString(), module.Name, module.Configuration.Name, module.GetType().Name);

            module.Apply(this);
        }

        public void Process(Module_Native module)
        {
            ProcessDependents(module);

            AddSystemIncludes(module.Cc);
            AddSystemIncludes(module.Asm);
        }

        public void Process(Module_DotNet module)
        {
            if (module.Compiler != null)
            {
                foreach (var pair in GetDependencyPairs(module, IsTransitiveDependencies))
                {
                    if (TransitiveCollection != null && !(pair.Dependency.Dependent is Module_UseDependency) && pair.ParentModule.Key != module.Key)
                    {
                        var propagatedDependency = new Dependency<IModule>(pair.Dependency.Dependent, pair.Dependency.BuildGroup, pair.Dependency.Type);

                        if (pair.Dependency.IsKindOf(DependencyTypes.AutoBuildUse) && pair.Dependency.IsKindOf(DependencyTypes.Link | DependencyTypes.Interface))
                        {
                            propagatedDependency.SetType(DependencyTypes.Build);
                        }

                        if (!pair.Dependency.IsKindOf(DependencyTypes.Build) && pair.Dependency.IsKindOf(DependencyTypes.Link | DependencyTypes.Interface))
                        {
                            propagatedDependency.SetType(DependencyTypes.Build);
                        }

                        //We only need to propagate build dependencies here
                        if (propagatedDependency.IsKindOf(DependencyTypes.Build))
                        {
                            propagatedDependency.ClearType(DependencyTypes.Interface); // We don't propagate interface

                            TransitiveCollection.AddOrUpdate(module.Key,
                                (mkey) => { var collection = new DependentCollection(module); collection.Add(propagatedDependency); return collection; },
                                (mkey, mcollection) => { mcollection.Add(propagatedDependency); return mcollection; });
                        }
                    }

                    if (pair.Dependency.IsKindOf(DependencyTypes.Link | DependencyTypes.Interface))
                    {
                        module.Compiler.DependentAssemblies.AppendIfNotNull(pair.Dependency.Dependent.Public(pair.ParentModule).Assemblies, (pair.Dependency.IsKindOf(DependencyTypes.CopyLocal) ? "copylocal" : null));
                    }
                }

                // For modules in the same package public data may not be defined. Add dependent assemblies
                foreach (var d in module.Dependents.Where(dep => dep.Dependent.Package.Key == module.Package.Key))
                {
                    if (d.IsKindOf(DependencyTypes.Link))  // Add Libraries
                    {
                        if (d.Dependent.IsKindOf(Module.DotNet | Module.Managed))
                        {
                            var assembly = d.Dependent.DynamicLibOrAssemblyOutput();
                            if (assembly != null)
                            {
                                module.Compiler.DependentAssemblies.Include(assembly.Path, failonmissing: false, optionSetName: (d.IsKindOf(DependencyTypes.CopyLocal)?"copylocal":null));
                            }
                        }
                    }
                }

            }

        }

        public void Process(Module_Utility module)
        {
            AddTransitiveDependencies(module);
        }

        public void Process(Module_MakeStyle module)
        {
            AddTransitiveDependencies(module);
        }

        public void Process(Module_Python module)
        {
            AddTransitiveDependencies(module);
        }

        public void Process(Module_ExternalVisualStudio module)
        {
            AddTransitiveDependencies(module);
        }

        private void AddTransitiveDependencies(ProcessableModule module)
        {
            if (IsTransitiveDependencies && TransitiveCollection != null)
            {
                foreach (var dep in module.Dependents.Flatten(DependencyTypes.Build))
                {
                    if (!(dep.Dependent is Module_UseDependency) && dep.Dependent.Key != module.Key)
                    {
                        TransitiveCollection.AddOrUpdate(module.Key,
                            (mkey) => { var collection = new DependentCollection(module); collection.Add(dep); return collection; },
                            (mkey, mcollection) => { mcollection.Add(dep); return mcollection; });
                    }
                }
            }
        }

        public void Process(Module_UseDependency module)
        {
            Error.Throw(Project, "ModuleProcessor", "Internal error: trying to process use dependency module [{0}]", module.Name);
        }

        #region NativeModule

        private void ProcessDependents(Module_Native module)
        {
            bool useTransitiveIncludes = false;

            var dependentIncludeDirs = new List<PathString>();
            var dependentLibraryDirs = new List<PathString>();
            var dependentUsingDirs = new List<PathString>();
            var dependentDefines = new List<string>();
            var dependentAssemblies = new FileSet();

            if (!useTransitiveIncludes)
            {
                // Dependents include directories are not transive
                foreach (var d in module.Dependents.Where(dep => dep.Dependent.Public(module) != null))
                {
                    if (d.IsKindOf(DependencyTypes.Interface))  // Add IncludeDirectories
                    {
                        if (module.Cc != null || module.Asm != null)
                        {
                            if (d.Dependent.Public(module).IncludeDirectories != null)
                            {
                                dependentIncludeDirs.AddRange(d.Dependent.Public(module).IncludeDirectories);
                            }
                        }
                    }
                }
            }

            // Order dependents so that include dirs and libs always come in same order. Transitive data:
            foreach (var pair in GetDependencyPairs(module, IsTransitiveDependencies))
            {
                if (pair.ParentModule.Key != module.Key)  // If we are not 
                {
                    if ((pair.Dependency.Dependent is Module_UseDependency) && pair.Dependency.Dependent.Package.IsKindOf(EA.FrameworkTasks.Model.Package.Buildable))
                    {
                        if (pair.Dependency.IsKindOf(DependencyTypes.Link))
                        {
                            if (Log.WarningLevel >= Log.WarnLevel.Advise)
                            {
                                module.Package.Project.Log.Warning.WriteLine(LogPrefix+ "Skipped transitive propagation to '{0}.{1}' via '{2}.{3}' of package '{4}' (or one of its modules). Looks like module '{2}.{3}' declares use dependency on buildable package '{4}'. Use dependency implies linking but package '{4} is not built because no build dependency was defined. Correct build scripts to use interfacedependency (or builddependency) instead of usedependency", module.BuildGroup,module.Name, pair.ParentModule.BuildGroup, pair.ParentModule.Name, pair.Dependency.Dependent.Package.Name);
                            }

                            continue;
                        }

                    }
                }


                if (TransitiveCollection != null && !(pair.Dependency.Dependent is Module_UseDependency) && pair.ParentModule.Key != module.Key)
                {
                    //I only need to propagate build dependencies here when I have link or managed module needs Assembly for compilation:
                    if (module.Link != null || (pair.Dependency.IsKindOf(DependencyTypes.Link | DependencyTypes.Interface) && module.IsKindOf(Module.Managed)))
                    {
                        var propagatedDependency = new Dependency<IModule>(pair.Dependency.Dependent, pair.Dependency.BuildGroup, pair.Dependency.Type);

                        if (module.Link != null && pair.Dependency.IsKindOf(DependencyTypes.AutoBuildUse) && pair.Dependency.IsKindOf(DependencyTypes.Link))
                        {
                            propagatedDependency.SetType(DependencyTypes.Build);
                        }

                        if (module.Link != null && !pair.Dependency.IsKindOf(DependencyTypes.Build) && pair.Dependency.IsKindOf(DependencyTypes.Link))
                        {
                            propagatedDependency.SetType(DependencyTypes.Build);
                        }

                        if((pair.Dependency.IsKindOf(DependencyTypes.Link | DependencyTypes.Interface) && module.IsKindOf(Module.Managed)) && pair.Dependency.IsKindOf(DependencyTypes.AutoBuildUse))
                        {
                            propagatedDependency.SetType(DependencyTypes.Build);
                        }

                        if (module.IsKindOf(Module.Managed | Module.DotNet) && pair.ParentModule.IsKindOf(Module.Library) && pair.Dependency.Dependent.IsKindOf(Module.Managed | Module.DotNet))
                        {
                            // Link dependency on Managed assembly should not propagate through native library
                            propagatedDependency.ClearType(DependencyTypes.Link);
                        }

                        if (module.Package.Project.Log.Level >= Log.LogLevel.Detailed)
                        {
                            Dependency<IModule> existingDep;

                            if (module.Dependents.TryGet(pair.Dependency.Dependent, out existingDep))
                            {
                                if (existingDep.Type != propagatedDependency.Type)
                                {
                                    module.Package.Project.Log.Info.WriteLine(LogPrefix+"Transitive propagation to '{0}' via '{1}' of '{2}' dependency type changed from '{3}' to '{4}'.", module.Name, pair.ParentModule.Name, pair.Dependency.Dependent.Name, DependencyTypes.ToString(existingDep.Type), DependencyTypes.ToString(propagatedDependency.Type));
                                }
                            }
                            else
                            {
                                module.Package.Project.Log.Info.WriteLine(LogPrefix+"Transitive propagation to '{0}' via '{1}' of '{2}' new dependency '{3}'.", module.Name, pair.ParentModule.Name, pair.Dependency.Dependent.Name, DependencyTypes.ToString(propagatedDependency.Type));
                            }
                        }
                        //We only need to propagate build dependencies here
                        if(propagatedDependency.IsKindOf(DependencyTypes.Build))
                        {
                            propagatedDependency.ClearType(DependencyTypes.Interface); // We don't propagate interface

                            TransitiveCollection.AddOrUpdate(module.Key,
                                (mkey) => { var collection = new DependentCollection(module); collection.Add(propagatedDependency); return collection; },
                                (mkey, mcollection) => { mcollection.Add(propagatedDependency); return mcollection; });
                        }
                    }
                }

                var publicData = pair.Dependency.Dependent.Public(pair.ParentModule);

                if (publicData != null)
                {
                    if (pair.Dependency.IsKindOf(DependencyTypes.Interface))
                    {
                        if (module.Cc != null)
                        {
                            if (useTransitiveIncludes && publicData.IncludeDirectories != null)
                            {
                                dependentIncludeDirs.AddRange(publicData.IncludeDirectories);
                            }
                            if (publicData.Defines != null)
                            {
                                dependentDefines.AddRange(publicData.Defines);

                            }
                            if (publicData.UsingDirectories != null)
                            {
                                dependentUsingDirs.AddRange(publicData.UsingDirectories);
                            }
                        }
                    }

                    if (pair.Dependency.IsKindOf(DependencyTypes.Link | DependencyTypes.Interface) && module.IsKindOf(Module.Managed))
                    {
                        if (module.Cc != null)
                        {
                            dependentAssemblies.AppendIfNotNull(publicData.Assemblies);
                        }
                    }

                    if (pair.Dependency.IsKindOf(DependencyTypes.Link))  // Add Libraries
                    {
                        if (module.Link != null)
                        {
                            module.Link.Libraries.AppendIfNotNull(publicData.Libraries);

                            // Check for incorrect usage of ScriptInit Task:
                            if (pair.Dependency.Dependent.Package.IsKindOf(Package.AutoBuildClean) && pair.Dependency.Dependent.Package.Modules.Any(m => !(m is Module_UseDependency)))
                            {
                                var defaultScriptInitLib = module.Package.Project.Properties["package." + pair.Dependency.Dependent.Package.Name + ".default.scriptinit.lib"];
                                if (defaultScriptInitLib != null && !pair.Dependency.Dependent.Package.Modules.Any(m => m.Name == pair.Dependency.Dependent.Package.Name))
                                {
                                    module.Link.Libraries.Exclude(defaultScriptInitLib);

                                    if (Log.WarningLevel >= Log.WarnLevel.Advise)
                                    {
                                        Log.Warning.WriteLine("Processing '{0}':  Initialize.xml from package '{1}' invokes task ScriptInit without 'Libs' parameter. ScriptInit in this case exports library '{2}', but package does not have any modules with this name. Fix your Initialize.xml file.",
                                            module.GroupName + "." + module.Name, pair.Dependency.Dependent.Package.Name, defaultScriptInitLib);
                                    }
                                }
                            }

                            if (publicData.LibraryDirectories != null)
                            {
                                dependentLibraryDirs.AddRange(publicData.LibraryDirectories);
                            }
                        }
                    }
                }
            }

            foreach (var d in module.Dependents.Where(dep => dep.Dependent.Package.Key == module.Package.Key))
            {
                if (d.IsKindOf(DependencyTypes.Link))  // Add Libraries
                {
                    if (module.Link != null)
                    {
                        var library = d.Dependent.LibraryOutput();
                        if (library != null)
                        {
                            module.Link.Libraries.Include(library.Path, failonmissing: false);
                        }
                    }

                    if (module.IsKindOf(Module.Managed) && d.Dependent.IsKindOf(Module.DotNet | Module.Managed))
                    {
                        var assembly = d.Dependent.DynamicLibOrAssemblyOutput();
                        if (assembly != null)
                        {
                            dependentAssemblies.Include(assembly.Path, failonmissing: false);
                        }
                    }
                }
            }

            // Add module Cc and Asm data:

            var includeDirs = dependentIncludeDirs.OrderedDistinct();
            var defines = dependentDefines.Distinct();
            var usingDirs = dependentUsingDirs.OrderedDistinct();
            if (module.Cc != null)
            {
                module.Cc.IncludeDirs.AddUnique(includeDirs);
                module.Cc.Defines.AddUnique(defines);
                module.Cc.UsingDirs.AddUnique(usingDirs);
                module.Cc.Assemblies.Append(dependentAssemblies);

                foreach (var cc in module.Cc.SourceFiles.FileItems.Select(fi => fi.GetTool() as CcCompiler).Where(tool => tool != null))
                {
                    cc.IncludeDirs.AddUnique(includeDirs);
                    cc.Defines.AddUnique(defines);
                    cc.UsingDirs.AddUnique(usingDirs);
                    cc.Assemblies.Append(dependentAssemblies);
                }
            }

            if (module.Asm != null)
            {
                module.Asm.IncludeDirs.AddUnique(includeDirs);
                module.Asm.Defines.AddUnique(defines);

                foreach (var asm in module.Asm.SourceFiles.FileItems.Select(fi => fi.GetTool() as AsmCompiler).Where(tool => tool != null))
                {
                    asm.IncludeDirs.AddUnique(includeDirs);
                    asm.Defines.AddUnique(defines);
                }
            }
            if (module.Link != null)
            {
                module.Link.LibraryDirs.AddUnique(dependentLibraryDirs.OrderedDistinct());
            }
        }

        private void AddSystemIncludes(CcCompiler tool)
        {
            if (tool != null)
            {
                bool prepend = Project.Properties.GetBooleanPropertyOrDefault(tool.ToolName + ".usepreincludedirs", false);

                var systemincludes = GetOptionString(tool.ToolName + ".includedirs", BuildOptionSet.Options).LinesToArray().Select(dir => PathString.MakeNormalized(dir)).OrderedDistinct();
                if (prepend)
                    tool.IncludeDirs.InsertRange(0, systemincludes);
                else
                    tool.IncludeDirs.AddUnique(systemincludes);

                foreach (var filetool in tool.SourceFiles.FileItems.Select(fi => fi.GetTool() as CcCompiler).Where(t => t != null))
                {
                    if (prepend)
                        filetool.IncludeDirs.InsertRange(0, systemincludes);
                    else
                        filetool.IncludeDirs.AddUnique(systemincludes);
                }
            }
        }

        private void AddSystemIncludes(AsmCompiler tool)
        {
            if (tool != null)
            {
                bool prepend = Project.Properties.GetBooleanPropertyOrDefault("as.usepreincludedirs", false);

                var systemincludes = GetOptionString("as.includedirs", BuildOptionSet.Options).LinesToArray().Select(dir => PathString.MakeNormalized(dir)).OrderedDistinct();
                if (prepend)
                    tool.IncludeDirs.InsertRange(0, systemincludes);
                else
                    tool.IncludeDirs.AddUnique(systemincludes);

                foreach (var filetool in tool.SourceFiles.FileItems.Select(fi => fi.GetTool() as AsmCompiler).Where(t => t != null))
                {
                    if (prepend)
                        filetool.IncludeDirs.InsertRange(0, systemincludes);
                    else
                        filetool.IncludeDirs.AddUnique(systemincludes);
                }
            }
        }


        #endregion

        #region Helper functions

        private IEnumerable<DependentCollection.ParentDependencyPair> GetDependencyPairs(IModule module, bool isTransitive = true)
        {
            // We do not want transitive dependencies propagation initiated by interface dependencies when parent is  exe or dll.
            uint transitiveFilter = (module.IsKindOf(Module.Native) && !module.IsKindOf(Module.Managed) && module.IsKindOf(Module.Program | Module.DynamicLibrary)) ? (DependencyTypes.Link | DependencyTypes.Build) : (DependencyTypes.Link | DependencyTypes.Build | DependencyTypes.Interface);
            return module.Dependents.FlattenParentChildPair(isTransitive, transitiveFilter).OrderBy(p => (p.Dependency.Dependent as ProcessableModule).GraphOrder).ThenBy(p => (p.ParentModule as ProcessableModule).GraphOrder);
        }


        #endregion

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
