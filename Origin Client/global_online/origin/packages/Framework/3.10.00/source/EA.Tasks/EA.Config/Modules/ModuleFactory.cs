using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

using NAnt.Core;
using EA.FrameworkTasks.Model;
using EA.Eaconfig.Core;

namespace EA.Eaconfig.Modules
{
    internal static class ModuleFactory
    {
        static internal ProcessableModule CreateModule(string name, string groupname, string groupsourcedir, Configuration configuration, BuildGroups buildgroup, BuildType buildType, IPackage package)
        {
            ProcessableModule module;

            if (buildType.IsCLR && !buildType.IsManaged)
            {
                module = new Module_DotNet(name, groupname, groupsourcedir, configuration, buildgroup, buildType, package);

                if (buildType.IsCSharp) module.SetType(Module.CSharp);
                if (buildType.IsFSharp) module.SetType(Module.FSharp);
            }
            else if (buildType.IsMakeStyle)
            {
                module = new Module_MakeStyle(name, groupname, groupsourcedir, configuration, buildgroup, buildType, package);
            }
            else if (buildType.BaseName == "Utility")
            {
                module = new Module_Utility(name, groupname, groupsourcedir, configuration, buildgroup, buildType, package);
            }
            else if (buildType.BaseName == "PythonProgram")
            {
                module = new Module_Python(name, groupname, groupsourcedir, configuration, buildgroup, buildType, package);
            }
            else if (buildType.BaseName == "VisualStudioProject")
            {
                module = new Module_ExternalVisualStudio(name, groupname, groupsourcedir, configuration, buildgroup, buildType, package);
            }
            else
            {
                module = new Module_Native(name, groupname, groupsourcedir, configuration, buildgroup, buildType, package);

                if (buildType.IsManaged) module.SetType(Module.Managed);

                var buildOptionSet = OptionSetUtil.GetOptionSetOrFail(module.Package.Project, buildType.Name);

                if (buildType.BaseName == "Library" || buildType.BaseName == "StdLibrary")
                {
                    module.SetType(Module.Library);
                }
                else if (buildType.BaseName == "DynamicLibrary" || buildType.BaseName == "ManagedCppAssembly")
                {
                    module.SetType(Module.DynamicLibrary);
                }
                else if (buildType.BaseName == "Program" || buildType.BaseName == "StdProgram" || buildType.BaseName == "WindowsProgram" || buildType.BaseName == "ManagedCppProgram" || buildType.BaseName == "ManagedCppWindowsProgram")
                {
                    module.SetType(Module.Program);
                }
                else
                {
                    var buildtasks = OptionSetUtil.GetOptionOrFail(module.Package.Project, buildOptionSet, "build.tasks").ToArray();

                    if (buildtasks.Contains("link"))
                    {
                        module.SetType(Module.Program);
                    }
                    else if (buildtasks.Contains("lib"))
                    {
                        module.SetType(Module.Library);
                    }
                }

                if (buildOptionSet.GetOptionOrDefault("windowsruntime", "off").ToLower() == "on")
                {
                    module.SetType(Module.WinRT);
                }
            }

            return module;
        }
    }
}
