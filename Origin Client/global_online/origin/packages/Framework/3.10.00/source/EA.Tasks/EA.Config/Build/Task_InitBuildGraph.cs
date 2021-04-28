using System;
using System.Xml;
using System.Collections.Generic;
using System.Collections.Concurrent;
using System.Linq;
using System.Threading;
using System.Text;
using System.IO;
using Threading = System.Threading.Tasks;

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Threading;
using NAnt.Core.Attributes;
using NAnt.Core.Functions;
using NAnt.Core.Logging;
using NAnt.Core.Reflection;
using NAnt.Core.PackageCore;

using EA.FrameworkTasks.Model;

using EA.Eaconfig;
using EA.Eaconfig.Core;
using EA.Eaconfig.Build;
using EA.Eaconfig.Modules;

namespace EA.Eaconfig.Build
{
    /// <summary>
    /// Verify whether we can reuse build graph or need to reset it. This is only needed when we are chaining targets.
    /// </summary>
    [TaskName("init-build-graph")]
    public class Task_InitBuildGraph : Task
    {
        [TaskAttribute("build-group-names", Required = true)]
        public string BuildGroupNames
        {
            get { return _buildgroupNames; }
            set { _buildgroupNames = value; }
        }
        private string _buildgroupNames;

        [TaskAttribute("build-configurations", Required = true)]
        public string BuildConfigurations
        {
            get { return _buildConfigurations; }
            set { _buildConfigurations = value; }
        }
        private string _buildConfigurations;


        [TaskAttribute("process-generation-data", Required = false)]
        public bool ProcessGenerationData
        {
            get { return _processGenerationData; }
            set { _processGenerationData = value; }
        }
        private bool _processGenerationData = false;

        protected override void ExecuteTask()
        {

            if (Project.HasBuildGraph() && !Project.BuildGraph().IsEmpty)
            {
                var buildGraph = Project.BuildGraph();

                if (!buildGraph.CanReuse(BuildConfigurations.ToArray(), BuildGroupNames.ToArray().Select(gn => ConvertUtil.ToEnum<BuildGroups>(gn)), Log, LogPrefix))
                {
                    var name = Properties["package.name"];

                    var configname = Properties["config"];

                    IPackage p;

                    Project.TryGetPackage(out p);

                    // Packages that are loaded through config package will not be reloaded in this project.
                    // instead of foreach(var package in Project.BuildGraph().Packages) reset only packages that have modules in the build graph.
                    // these packages will be reloaded during build graph construction.

                    foreach (var package in Project.BuildGraph().Modules.Where(m => m.Value.Configuration.Name == configname).Select(m => m.Value.Package).Distinct(new IPackageEqualityComparer()))
                    {
                        if (package.Name == name)
                        {
                            continue;
                        }
                        Project.Properties.RemoveReadOnly(String.Format("package.{0}-{1}.dir", package.Name, package.Version));
                        Project.Properties.RemoveReadOnly(String.Format("package.{0}.dir", package.Name));
                    }

                    Log.Info.WriteLine("Reseting build graph.");
                    Project.ResetBuildGraph();

                    if (p != null)
                    {
                        IPackage package;

                        if (!Project.TrySetPackage(p.Name, p.Version, p.Dir, p.ConfigurationName, p.FrameworkVersion, out package))
                        {
                            Project.Log.Warning.WriteLine("<package> task is called twice for {0}-{1} [{2}] in the build file, or Framework internal error", p.Name, p.Version, p.ConfigurationName);
                        }
                        else
                        {
                            package.SetType(p.IsKindOf(EA.FrameworkTasks.Model.Package.Buildable) ? EA.FrameworkTasks.Model.Package.Buildable : BitMask.None);
                            package.Project = Project;
                            package.PackageConfigBuildDir = p.PackageConfigBuildDir;
                        }
                    }

                }
            }
            else
            {
                Log.Info.WriteLine(LogPrefix + "Build graph does not exist");
            }
        }
    }
}
