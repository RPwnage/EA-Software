using NAnt.Core;
using NAnt.Core.Attributes;

using System;
using System.Collections;
using System.Collections.Generic;
using System.Text;

namespace EA.Eaconfig
{
    [TaskName("SortBuildModules")]
    public class SortBuildModules : EAConfigTask
    {

        [TaskAttribute("build-group-name", Required = true)]
        public string BuildGroupName
        {
            get { return _buildGroupName; }
            set { _buildGroupName = value; }
        }

        [TaskAttribute("build-modules", Required = true)]
        public string BuildModules
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
                foreach (string module in _sortedModules)
                {
                    sorted.Append(module);
                    sorted.Append(' ');
                }
                string result = sorted.ToString().Trim();
                return result; 
            }
        }

        public List<string> SortedBuildModulesList
        {
            get
            {
                return _sortedModules;
            }
        }



        public static List<string> Execute(Project project, string buildGroupName, string buildModules)
        {
            SortBuildModules task = new SortBuildModules(project, buildGroupName, buildModules);
            task.Execute();
            return task.SortedBuildModulesList;
        }

        public SortBuildModules(Project project, string buildGroupName, string buildModules)
            : base(project)
        {
            BuildGroupName = buildGroupName;
            BuildModules   = buildModules;

            if (String.IsNullOrEmpty(BuildGroupName))
            {
                Error.Throw(Project, Location, Name, "'BuildGroupName' parameter is empty.");                
            }
            if (String.IsNullOrEmpty(BuildModules))
            {
                Error.Throw(Project, Location, Name, "'BuildModules' parameter is empty.");
            }
        }

        public SortBuildModules()
            : base()
        {
        }

        /// <summary>Execute the task.</summary>
        protected override void ExecuteTask()
        {
            if (!String.IsNullOrEmpty(BuildModules))
            {
                // Create DAG of build modules:
                List<DAGNode> unsorted = new List<DAGNode>();

                foreach (string module in StringUtil.ToArray(BuildModules))
                {
                    unsorted.Add(new DAGNode(module));
                }

                // Populate dependencies
                foreach (DAGNode moduleNode in unsorted)
                {

                    foreach (string dependentModule in StringUtil.ToArray(GetModuleDependencies(BuildGroupName, moduleNode.Name)))
                    {
                        DAGNode dependentNode = unsorted.Find(delegate(DAGNode n) { return n.Name == dependentModule; });
                        if (dependentNode != null)
                        {
                            moduleNode.Dependencies.Add(dependentNode);
                        }
                    }
                }

                List<DAGNode> sorted = DAGNode.Sort(unsorted);

                foreach (DAGNode moduleNode in sorted)
                {
                    _sortedModules.Add(moduleNode.Name);
                }
            }
        }

        private string GetModuleDependencies(string groupName, string module)
        {
            string dependencies = String.Empty;

            string depPropName = String.Format("{0}.{1}.{0}.moduledependencies", groupName, module);
            string depPropNameConfig = depPropName + "." + Properties["config-system"];

            dependencies = Properties[depPropNameConfig];

            if (String.IsNullOrEmpty(dependencies))
            {
                dependencies = Properties[depPropName];
            }
            return dependencies;
        }

        private string _buildGroupName;
        private string _buildModules;

        private List<string> _sortedModules = new List<string>();


    }
}
