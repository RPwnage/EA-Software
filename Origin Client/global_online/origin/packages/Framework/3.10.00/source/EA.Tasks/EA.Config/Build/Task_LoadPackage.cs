using System;
using System.Xml;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Attributes;
using NAnt.Core.Functions;
using NAnt.Core.Logging;
using NAnt.Core.Reflection;
using EA.FrameworkTasks.Model;

using EA.Eaconfig;
using EA.Eaconfig.Core;
using EA.Eaconfig.Build;
using EA.Eaconfig.Modules;

namespace EA.Eaconfig.Build
{

    [TaskName("load-package")]
    public class LoadPackageTask : TaskContainer
    {
        [TaskAttribute("build-group-names", Required = true)]
        public string BuildGroupNames
        {
            get { return _buildGroupNames; }
            set { _buildGroupNames = value; }
        }
        private string _buildGroupNames;

        [TaskAttribute("autobuild-target", Required = false)]
        public string AutobuildTarget
        {
            get { return _autobuildtarget; }
            set { _autobuildtarget = value; }
        }
        private string _autobuildtarget = null;

        [TaskAttribute("process-generation-data", Required = false)]
        public bool ProcessGenerationData
        {
            get { return _processGenerationData; }
            set { _processGenerationData = value; }
        }
        private bool _processGenerationData = false;

        private List<string> processedModuleDependetsList = new List<string>();

        protected override void ExecuteTask()
        {

            IPackage package;
            if (Project.TryGetPackage(out package))
            {
                SetPackageLevelData(package);

                if (Project.TargetStyle != NAnt.Core.Project.TargetStyleType.Clean)
                {
                    ExecutePackagePreProcessSteps(package);
                }

                var restoreBuildGroupTo = Properties["eaconfig.build.group"];

                using (new OnExit(() => { if (restoreBuildGroupTo != null) Properties["eaconfig.build.group"] = restoreBuildGroupTo; ((Package)package).SetType(Package.FinishedLoading); }))
                {

                    foreach (var buildgroup in GetBuildGroups())
                    {
                        Properties["eaconfig.build.group"] = buildgroup.ToString();

                        string modules = Properties[buildgroup + ".buildmodules"];

                        //IMTODO: Since I moved to processing in several passes and Project instances are stored in the Package
                        // I can move module creation code to the Dependencies process. This will reduce duplication and 
                        // make the code simpler and more flexible 

                        if (String.IsNullOrEmpty(modules))
                        {
                            AddPackageWithoutModules(package, buildgroup);
                        }
                        else
                        {
                            AddPackageWithModules(package, modules, buildgroup);
                        }
                    }
                }

                LoadUnprocessedModuledependents(package);
            }
            else
            {
                throw new BuildException(String.Format("Can't find package definition. Make sure package build file '{0}' has <package> task defined.", Project.BuildFileLocalName), this.Location);
            }
        }

        private void AddPackageWithModules(IPackage package, string modules, BuildGroups buildgroup)
        {

            var buildModules = GetmodulesWithDependencies(LogPrefix, Project, buildgroup, modules.ToArray(), package.Name, hasmodules:true);

            var constraints = package.Project.GetConstraints();

            IEnumerable<BuildModuleDef> constrainedModules = constraints == null ? null : LoadPackageTask.GetmodulesWithDependencies(LogPrefix, package.Project, buildgroup, constraints.Select(c => c.ModuleName).ToList(), package.Name, hasmodules: true);

            foreach (var module in buildModules)
            {
                string groupsourcedir = Path.Combine(Properties["eaconfig." + module.BuildGroup + ".sourcedir"], module.ModuleName);

                if (buildgroup != module.BuildGroup && module.Groupname == module.BuildGroup.ToString())
                {
                    groupsourcedir = Properties["eaconfig." + module.BuildGroup + ".sourcedir"];
                }

                var moduleInstance = AddModule(Project, package, module.BuildGroup, module.ModuleName, module.Groupname, groupsourcedir);

                if (constrainedModules == null || constrainedModules.Any(c => c.BuildGroup == moduleInstance.BuildGroup && c.ModuleName == moduleInstance.Name))
                {
                    ProcessModule(moduleInstance);

                    ExecuteLoadSubtasks(moduleInstance);

                    processedModuleDependetsList.Add(module.BuildGroup + ":" + module.ModuleName);
                }
            }

        }

        private void AddPackageWithoutModules(IPackage package, BuildGroups buildgroup)
        {

            // If <group>.buildtype isn't defined then we assume there are
            // no modules in that build group.  Before this check was added
            // there was no way to specify that there were 0 modules in a
            // particular group - it was always assumed that there was at
            // least one module in each group, with default source filesets
            // and properties.  This would lead to build failures when calling
            // the build targets for groups containing 0 modules.  This check
            // provides an explicit way to check for (and ignore) 0 module groups.

            // We also check for <group>.builddependencies, for backwards
            // compatibility.  There are several instances of end-users setting
            // up build files which are really programs in disguise, so we
            // look for <group>.builddependencies to catch that case.  When
            // this change was made, we also re-added the code which defaults
            // the buildtype to Program if it wasn't previously set.  Again,
            // this is for backwards-compatibility.

            if (Properties.Contains(buildgroup + ".buildtype") ||
                Properties.Contains(buildgroup + ".builddependencies"))
            {
                string packagename = Project.Properties["package.name"];

                var buildModules = GetmodulesWithDependencies(LogPrefix, Project, buildgroup, String.Empty.ToArray(), packagename, hasmodules: false);

                foreach (var module in buildModules)
                {
                    string groupsourcedir = Path.Combine(Properties["eaconfig." + module.BuildGroup + ".sourcedir"], module.ModuleName);

                    if ( module.Groupname == module.BuildGroup.ToString())
                    {
                        groupsourcedir = Properties["eaconfig." + module.BuildGroup + ".sourcedir"];
                    }

                    var moduleInstance = AddModule(Project, package, module.BuildGroup, module.ModuleName, module.Groupname, groupsourcedir);

                    ProcessModule(moduleInstance);

                    ExecuteLoadSubtasks(moduleInstance);

                    processedModuleDependetsList.Add(module.BuildGroup + ":" + module.ModuleName);
                }
            }
        }

        private IEnumerable<BuildGroups> GetBuildGroups()
        {
            var buildgroups = new List<BuildGroups>();
            
            foreach (var groupstr in BuildGroupNames.ToArray())
            {
                buildgroups.Add(ConvertUtil.ToEnum<BuildGroups>(groupstr));
            }
            // Add additional groups that can be specified by module constraints.
            var constraints = Project.GetConstraints();
            if (constraints != null)
            {
                buildgroups.AddRange(constraints.Select(c => c.Group));
            }
            return buildgroups.OrderedDistinct();
        }

        private void SetPackageLevelData(IPackage package)
        {
            if (Project.Properties.GetBooleanProperty("package.perforceintegration"))
            {
                package.SourceControl = new SourceControl(usingSourceControl: true, type: SourceControl.Types.Perforce);
            }
        }

        private static string GetDefaultBuildType(Project project, string groupname)
        {
            if (project.Properties.Contains(groupname + ".builddependencies"))
            {
                return "Program";
            }
            return "Library";
        }

        private static ProcessableModule AddModule(Project project,IPackage package, BuildGroups buildgroup, String modulename, string groupname, string groupsourcedir)
        {
            PropertyUtil.SetPropertyIfMissing(package.Project, groupname + ".buildtype", GetDefaultBuildType(project, groupname));

            ProcessableModule module = CreateModule(project, package, modulename, buildgroup, groupname, groupsourcedir);

            module.SetModuleBuildProperties();

            return module;
        }

        private void ProcessModule(ProcessableModule module)
        {
            using (var moduleProcessor = new ModuleProcessor_LoadPackage(module, AutobuildTarget, Project.TargetStyle, LogPrefix))
            {
                moduleProcessor.Process(module);
            }
        }

        static ProcessableModule CreateModule(Project project, IPackage package, string modulename, BuildGroups buildgroup, string groupname, string groupsourcedir)
        {
            BuildType buildtype = project.CreateBuildType(groupname);

            // When targets are chained "build test-build . . " there may be residual UseDependency module. Instead of GetOrAdd use AddOrUpdate here, and overwrite UseDep module
            var module = project.BuildGraph().Modules.AddOrUpdate(Module.MakeKey(modulename, buildgroup, package), 
                key => ModuleFactory.CreateModule(modulename, groupname, groupsourcedir, CreateConfiguration(project,buildtype), buildgroup, buildtype, package),
                (key, mod)=> (mod is Module_UseDependency)? ModuleFactory.CreateModule(modulename, groupname, groupsourcedir, CreateConfiguration(project, buildtype), buildgroup, buildtype, package) : mod) as ProcessableModule;

            var scriptfile = project.Properties[groupname + ".scriptfile"].TrimWhiteSpace();
            if (!String.IsNullOrEmpty(scriptfile))
            {
                module.ScriptFile = PathString.MakeNormalized(scriptfile);
            }

            return module;
        }

        private void ExecuteLoadSubtasks(ProcessableModule module)
        {
            _current_build_module = module;

            ExecuteChildTasks();

            _current_build_module = null;
        }

        [XmlTaskContainer("-")]
        protected override Task CreateChildTask(XmlNode node)
        {
            Task task = Project.CreateTask(node);

            IBuildModuleTask bmt = task as IBuildModuleTask;
            if (bmt != null)
            {
                bmt.Module = _current_build_module;
            }

            return task;
        }

        private void ExecutePackagePreProcessSteps(IPackage package)
        {
            BufferedLog stepsLog = (Log.Level >= Log.LogLevel.Detailed) ? new BufferedLog(Log) : null;
            if (stepsLog != null)
            {
                stepsLog.Info.WriteLine(LogPrefix + "Executing '{0}' steps", "package.preprocess");
            }

            package.Project.ExecuteGlobalProjectSteps(package.Project.Properties["package.preprocess"].ToArray(), stepsLog);

            if (ProcessGenerationData)
            {
                package.Project.ExecuteTargetIfExists(package.Project.Properties["vcproj.prebuildtarget"], force: false);
                package.Project.ExecuteTargetIfExists(package.Project.Properties["csproj.prebuildtarget"], force: false);
                package.Project.ExecuteTargetIfExists(package.Project.Properties["fsproj.prebuildtarget"], force: false);
            }
        }

        public static Configuration CreateConfiguration(Project project, BuildType buildType)
        {
            return new Configuration(project.Properties["config"],
                                     project.Properties["config-system"],
                                     buildType.SubSystem,
                                     project.Properties["config-compiler"],
                                     project.Properties["config-platform"],
                                     project.Properties["config-name"]);
        }


        private void LoadUnprocessedModuledependents(IPackage package)
        {
            var PackageDependentProcessingStatus = Project.BuildGraph().GetPackageDependentProcessingStatus(package);

            foreach (var key in PackageDependentProcessingStatus.GetUnProcessedModules(processedModuleDependetsList, Project.TargetStyle, AutobuildTarget, LoadPackageTask.GetModulesWithDependencies))
            {
                var module = package.Modules.Where(m => m.BuildGroup + ":" + m.Name == key).FirstOrDefault() as ProcessableModule;
                if (module != null)
                {
                    ProcessModule(module);
                }
            }
        }

        internal static IEnumerable<BuildModuleDef> GetmodulesWithDependencies(string LogPrefix, Project project, BuildGroups group, ICollection<string> modulenames, string packageName, bool hasmodules)
        {
            ISet<BuildModuleDef> dependencies = new HashSet<BuildModuleDef>();
            Stack<BuildModuleDef> stack = new Stack<BuildModuleDef>();

            if(hasmodules)
            {
                foreach (var modulename in modulenames)
                {
                    var entry = new BuildModuleDef(modulename, group, group + "." + modulename);
                    stack.Push(entry);
                    dependencies.Add(entry);
                }
            }
            else
            {
                var entry = new BuildModuleDef(packageName, group, Enum.GetName(typeof(BuildGroups), group));
                stack.Push(entry);
                dependencies.Add(entry);
            }

            while (stack.Count > 0)
            {
                var moduledef = stack.Pop();

                foreach (BuildGroups dependentGroup in Enum.GetValues(typeof(BuildGroups)))
                {
                    bool needRuntimeModuleDependents = true;

                    // Collect all dependencies that are build or autobuilduse type:
                    foreach (var v in Mappings.ModuleDependencyPropertyMapping)
                    {
                        if (((v.Value & DependencyTypes.Build) == DependencyTypes.Build) || ((v.Value & DependencyTypes.AutoBuildUse) == DependencyTypes.AutoBuildUse))
                        {
                            string depPropName = String.Format("{0}.{1}.{2}{3}", moduledef.BuildGroup, moduledef.ModuleName, dependentGroup, v.Key);
                            string depPropNameConfig = depPropName + "." + project.Properties["config-system"];

                            var depPropVal = project.Properties[depPropName];
                            var depPropValConfig = project.Properties[depPropNameConfig];

                            if (modulenames.Count <= 1 && depPropVal == null && depPropValConfig == null)
                            {
                                //There is alternative format when one module only is defined. Convert property to standard format, 
                                // so that we don't have to deal with these differences later.

                                var depPropNameLegacy = String.Format("{0}.{1}{2}", moduledef.BuildGroup, dependentGroup, v.Key);
                                var depPropNameConfigLegacy = depPropName + "." + project.Properties["config-system"];

                                var depPropValLegacy = project.Properties[depPropNameLegacy];
                                var depPropValConfigLegacy = project.Properties[depPropNameConfigLegacy];

                                if (depPropValLegacy != null)
                                {
                                    project.Properties[depPropName] = depPropVal = depPropValLegacy;
                                }

                                if (depPropValLegacy != null)
                                {
                                    project.Properties[depPropNameConfig] = depPropValConfig = depPropValConfigLegacy;
                                }
                            }

                            // Automatically add dependency of other groups on runtime (except of the tool group) in case dependencies were not defined explicitly

                            if (dependentGroup == BuildGroups.runtime && moduledef.BuildGroup != dependentGroup)
                            {
                                if (depPropVal != null || depPropValConfig != null)
                                {
                                    needRuntimeModuleDependents = false;
                                }
                            }

                            var moduledependents = String.Concat(depPropVal, Environment.NewLine, depPropValConfig).ToArray();

                            foreach (var depmodule in moduledependents)
                            {
                                var dep_buildmodules = project.Properties[dependentGroup + ".buildmodules"];

                                //Dependent group with modules
                                if (dep_buildmodules != null)
                                {
                                    if (dep_buildmodules.Contains(depmodule))
                                    {
                                        var entry = new BuildModuleDef(depmodule, dependentGroup, dependentGroup + "." + depmodule);
                                        if (!dependencies.Contains(entry))
                                        {
                                            stack.Push(entry);
                                            dependencies.Add(entry);
                                        }
                                    }
                                    else
                                    {
                                        project.Log.Warning.WriteLine(LogPrefix+"module '{0}.{1}' in package '{2}' declares dependency on module '{3}.{4}' that is not defined in this package", moduledef.BuildGroup, moduledef.ModuleName, packageName, dependentGroup, depmodule);
                                    }
                                }
                                else if (project.Properties[dependentGroup + ".buildtype"] != null)
                                {
                                    //Dependent group without modules
                                    var entry = new BuildModuleDef(packageName, dependentGroup, dependentGroup.ToString());

                                    if (!dependencies.Contains(entry))
                                    {
                                        stack.Push(entry);
                                        dependencies.Add(entry);
                                    }
                                }
                                else
                                {
                                    project.Log.Warning.WriteLine(LogPrefix+"module '{0}.{1}' in package '{2}' declares dependency on module '{3}.{4}' that is not defined in this package", moduledef.BuildGroup, moduledef.ModuleName, packageName, dependentGroup, depmodule);
                                }
                            }
                        }
                    }

                    // Now add runtime modules to all groups except tool if needed: 
                    if (dependentGroup == BuildGroups.runtime && moduledef.BuildGroup != dependentGroup && moduledef.BuildGroup!=BuildGroups.tool && needRuntimeModuleDependents)
                    {
                        var runtime_buildmodules = project.Properties["runtime.buildmodules"];

                        string groupname = null;

                        if (runtime_buildmodules == null && project.Properties[dependentGroup + ".buildtype"] != null)
                        {
                            runtime_buildmodules = packageName;
                            groupname = dependentGroup.ToString();
                        }

                        if (runtime_buildmodules != null)
                        {

                            foreach (var runtime_module in runtime_buildmodules.ToArray())
                            {
                                var entry = new BuildModuleDef(runtime_module, dependentGroup, groupname ?? dependentGroup + "." + runtime_module);
                                if (!dependencies.Contains(entry))
                                {
                                    stack.Push(entry);
                                    dependencies.Add(entry);
                                }
                            }
                            var propName = String.Format("{0}.{1}.runtime.buildonlymoduledependencies", moduledef.BuildGroup, moduledef.ModuleName);
                            project.Properties[propName] = runtime_buildmodules.ToArray().ToString(Environment.NewLine);
                        }
                    }

                }
            }
            return dependencies;
        }

        private static ProcessableModule DelayedCreateNewModule(IPackage package, BuildModuleDef module)
        {
            string modules = package.Project.Properties[module.BuildGroup + ".buildmodules"].TrimWhiteSpace();

            if (modules.ToArray().Any(name => name == module.ModuleName))
            {
                string groupsourcedir = Path.Combine(package.Project.Properties["eaconfig." + module.BuildGroup + ".sourcedir"], module.ModuleName);
                return AddModule(package.Project, package, module.BuildGroup, module.ModuleName, module.Groupname, groupsourcedir);
            }
            return null;
        }

        internal static List<string> GetModulesWithDependencies(IPackage package, IEnumerable<string> unprocessedKeys)
        {
            var logPrefix = String.Empty;
            var modulesWithDependencies = new List<string>();
            if (unprocessedKeys != null)
            {
                foreach (var group in unprocessedKeys.GroupBy(um => um.Substring(0, um.IndexOf(':'))))
                {
                    bool groupHasModules = !String.IsNullOrEmpty(package.Project.Properties[group.Key + ".buildmodules"].TrimWhiteSpace());

                    var buildmodules = LoadPackageTask.GetmodulesWithDependencies(logPrefix, ((Package)package).Project, ConvertUtil.ToEnum<BuildGroups>(group.Key), group.Select(g => g.Substring(g.IndexOf(':') + 1)).ToList(), package.Name, hasmodules: groupHasModules);

                    foreach (var bm in buildmodules)
                    {
                        if (!package.Modules.Any(m => m.Name == bm.ModuleName && m.BuildGroup == bm.BuildGroup))
                        {
                            if (null == DelayedCreateNewModule(package, bm))
                            {
                                continue;
                            }
                        }
                        modulesWithDependencies.Add(bm.BuildGroup + ":" + bm.ModuleName);
                    }
                }
            }
            return modulesWithDependencies;
        }

        private ProcessableModule _current_build_module;

        internal class BuildModuleDef : IEquatable<BuildModuleDef>
        {
            internal BuildModuleDef(string moduleName, BuildGroups buildGroup, String groupname)
            {
                ModuleName = moduleName;
                BuildGroup = buildGroup;
                Groupname = groupname;
            }
            internal readonly string ModuleName;
            internal readonly BuildGroups BuildGroup;
            internal readonly string Groupname;

            public override string ToString()
            {
                return BuildGroup + "." + ModuleName;
            }

            public bool Equals(BuildModuleDef other)
            {
                bool ret = false;

                if (Object.ReferenceEquals(this, other))
                {
                    ret = true;
                }
                else if (Object.ReferenceEquals(other, null))
                {
                    ret = (ModuleName == null);
                }
                else
                {
                    ret = (BuildGroup == other.BuildGroup && ModuleName == other.ModuleName);
                }

                return ret;
            }

            public override int GetHashCode()
            {
                return (BuildGroup + "." + ModuleName).GetHashCode();
            }

        }
    }

    

}
