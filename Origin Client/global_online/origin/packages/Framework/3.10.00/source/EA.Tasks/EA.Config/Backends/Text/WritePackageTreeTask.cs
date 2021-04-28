using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Attributes;
using EA.FrameworkTasks.Model;
 
namespace EA.Eaconfig.Backends.Text
{
    [TaskName("WritePackageTree")]
    public class WritePackageTreeTask : Task
    {
        [TaskAttribute("packagename", Required = true)]
        public string PackageName
        {
            get { return _packageName; }
            set { _packageName = value; }
        }


        protected override void ExecuteTask()
        {
            Log.Status.WriteLine("Package dependencies");
            Log.Status.WriteLine();
            int indent = 0;
            // Print package dependency tree for the specified package.
            foreach (IPackage package in Project.BuildGraph().Packages.Values.Where(p => p.Name == PackageName))
            {
                Log.Status.WriteLine("    Configuration: {0}", package.ConfigurationName);
                Log.Status.WriteLine();

                PrintPackage(package, indent);
            }
            Log.Status.WriteLine();
        }

        private void PrintPackage(IPackage package, int indent)
        {
            Log.Status.WriteLine("{0}{1}-{2} [{3}]", String.Empty.PadLeft(indent), package.Name, package.Version, package.ConfigurationName);

            indent += 4;
            foreach (var dependent in package.DependentPackages)
            {
                PrintPackage(dependent.Dependent, indent);
            }
        }

        private string _packageName;

    }


}

