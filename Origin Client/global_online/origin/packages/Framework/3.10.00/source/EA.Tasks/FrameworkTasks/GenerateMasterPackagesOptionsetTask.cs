using System;
using System.IO;
using System.Threading;
using System.Collections.Concurrent;

using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.PackageCore;

namespace EA.FrameworkTasks
{
    /// <summary>
    /// creates optionset that contains all packages listed in masterconfig with key='package name', value='version'.
    /// 
    /// </summary>
    /// <remarks>
    /// <para>Package version exceptions are evaluated agaings task Project instance.</para>
    /// <para>Has static C# interface: public static OptionSet Execute(Project project).</para>
    /// </remarks>
    /// <param name="optionsetname">Name of the optionset to generate.</param>
    [TaskName("generate.master-packages-optionset")]
    public class GenerateMasterPackagesOptionsetTask : Task
    {
        private string _optionsetname;

        /// <summary>Name of the optionset.</summary>
        [TaskAttribute("optionsetname", Required = true)]
        public string OptionsetName { get { return _optionsetname; } set { _optionsetname = value; } }


        protected override void ExecuteTask()
        {
            var masterPackages = Execute(Project);

            Project.NamedOptionSets[OptionsetName] = masterPackages;
        }

        public static OptionSet Execute(Project project)
        {
            OptionSet masterPackages = new OptionSet();
            foreach (var package in PackageMap.Instance.MasterPackages)
            {
                masterPackages.Options.Add(package, PackageMap.Instance.GetMasterVersion(package, project));
            }

            return masterPackages;
        }
    }
}
