using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Attributes;
using EA.FrameworkTasks.Model;

using System;
using System.Collections;
using System.Collections.Generic;
using System.Text;
using System.Linq;

namespace EA.Eaconfig
{
    [TaskName("SortBuildModules")]
    public class SortBuildModules : EAConfigTask
    {

        [TaskAttribute("build-group-name", Required = true)]
        public BuildGroups BuildGroup
        {
            get { return _buildGroup; }
            set { _buildGroup = value; }
        }

        [OptionSet("build-modules")]
        public OptionSet BuildModules
        {
            get { return _buildModules; }
            set { _buildModules = value; }
        }


        [TaskAttribute("sorted-build-modules", Required = true)]
        public string SortedBuildModules
        {
            get 
            {
                StringBuilder sorted = new StringBuilder();
                foreach (var module in _sortedModules)
                {
                    sorted.Append(module.ModuleName);
                    sorted.Append(' ');
                }
                string result = sorted.ToString().Trim();
                return result; 
            }
        }

        public List<BuildModuleDef> SortedBuildModulesList
        {
            get
            {
                return _sortedModules;
            }
        }



        public static List<BuildModuleDef> Execute(Project project, BuildGroups buildGroup, OptionSet buildModules)
        {
            SortBuildModules task = new SortBuildModules(project, buildGroup, buildModules);
            task.Execute();
            return task.SortedBuildModulesList;
        }

        public SortBuildModules(Project project, BuildGroups buildGroup, OptionSet buildModules)
            : base(project)
        {
            BuildGroup = buildGroup;
            BuildModules   = buildModules;

            if (BuildModules == null)
            {
                Error.Throw(Project, Location, Name, "'BuildModules' parameter is empty.");
            }

            if (String.IsNullOrEmpty(BuildModules.Options[BuildGroup.ToString()]))
            {
                Error.Throw(Project, Location, Name, "'BuildModules[{0}]' parameter is empty.", BuildGroup.ToString());
            }
        }

        public SortBuildModules()
            : base()
        {
        }

        /// <summary>Execute the task.</summary>
        protected override void ExecuteTask()
        {
            var modulesbygroup = new Dictionary<BuildGroups, IEnumerable<string>>();

            foreach (var entry in BuildModules.Options)
            {
                modulesbygroup[ConvertUtil.ToEnum<BuildGroups>(entry.Key)] = entry.Value.ToArray().OrderedDistinct(); 
            }

            IEnumerable<string> startmodulelist;

            if (modulesbygroup.TryGetValue(BuildGroup, out startmodulelist))
            {
                if (startmodulelist != null)
                {
                    // Create DAG of build modules:
                    var unsorted = new List<DAGNode<BuildModuleDef>>();

                    foreach (string module in modulesbygroup[BuildGroup])
                    {
                        unsorted.Add(new DAGNode<BuildModuleDef>(new BuildModuleDef(module, BuildGroup)));
                    }

                    // Populate dependencies
                    for (int i = 0; i < unsorted.Count; i++)
                    {
                        var moduleNode = unsorted[i];

                        foreach (var dependentModule in GetModuleDependencies(moduleNode.Value.BuildGroup, moduleNode.Value.ModuleName))
                        {
                            var dependentNode = unsorted.Find(delegate(DAGNode<BuildModuleDef> n) { return n.Value.ModuleName == dependentModule.ModuleName && n.Value.BuildGroup == dependentModule.BuildGroup; });
                            if (dependentNode != null)
                            {
                                moduleNode.Dependencies.Add(dependentNode);
                            }
                            else
                            {
                                // If module is from different group and module exists then add it to unsorted list:
                                IEnumerable<string> groupmodulelist;
                                if (dependentModule.BuildGroup != BuildGroup && modulesbygroup.TryGetValue(dependentModule.BuildGroup, out groupmodulelist))
                                {
                                    if (groupmodulelist != null && groupmodulelist.Contains(dependentModule.ModuleName))
                                    {
                                        dependentNode = new DAGNode<BuildModuleDef>(dependentModule);
                                        unsorted.Add(dependentNode);
                                        moduleNode.Dependencies.Add(dependentNode);
                                    }
                                }
                                if (dependentNode == null)
                                {
                                    Log.Warning.WriteLine("Package '{0}' Module '{1}.{2}' declares dependency on Module '{3}.{4}' that is not listed in the '{3}.buildmodules' property. Dependency is ignored.", Properties["package.name"] ?? "unknown", moduleNode.Value.BuildGroup, moduleNode.Value.ModuleName, dependentModule.BuildGroup, dependentModule.ModuleName);
                                }
                            }
                        }
                    }

                    var sorted = DAGNode <BuildModuleDef>.Sort(unsorted);

                    foreach (var moduleNode in sorted)
                    {
                        _sortedModules.Add(moduleNode.Value);
                    }
                }
            }
        }

        private IEnumerable<BuildModuleDef> GetModuleDependencies(BuildGroups maingroup, string module)
        {
            ISet<BuildModuleDef> dependencies = new HashSet<BuildModuleDef>();
            Stack<BuildModuleDef> stack = new Stack<BuildModuleDef>();
            stack.Push(new BuildModuleDef(module, maingroup));
            while (stack.Count > 0)
            {
                var subsetentry = stack.Pop();

                foreach (BuildGroups group in Enum.GetValues(typeof(BuildGroups)))
                {
                    string depPropName = String.Format("{0}.{1}.{2}.moduledependencies", subsetentry.BuildGroup, subsetentry.ModuleName, group);
                    string depPropNameConfig = depPropName + "." + Properties["config-system"];

                    string depPropNameAuto = String.Format("{0}.{1}.{2}.automoduledependencies", subsetentry.BuildGroup, subsetentry.ModuleName, group);
                    string depPropNameAutoConfig = depPropNameAuto + "." + Properties["config-system"];

                    var moduledependents = String.Concat(Project.Properties[depPropNameConfig], Environment.NewLine, Project.Properties[depPropName], 
                        Environment.NewLine, Project.Properties[depPropNameAutoConfig], Environment.NewLine, Project.Properties[depPropNameAuto]);

                    foreach (var depModule in moduledependents.ToArray())
                    {
                        var entry = new BuildModuleDef(depModule, group);

                        if (!dependencies.Contains(entry)) 
                        {
                            stack.Push(entry);
                            dependencies.Add(entry);
                        }
                    }
                }
            }
            return dependencies;
        }

        private BuildGroups _buildGroup;
        private OptionSet _buildModules;

        private List<BuildModuleDef> _sortedModules = new List<BuildModuleDef>();

        public class BuildModuleDef : IEquatable<BuildModuleDef>
        {
            public BuildModuleDef(string moduleName, BuildGroups buildGroup)
            {
                ModuleName = moduleName;
                BuildGroup = buildGroup;
            }
            public readonly string ModuleName;
            public readonly BuildGroups BuildGroup;

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

        }


    }
}
