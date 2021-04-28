using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Logging;

using System;
using System.Collections;
using System.Collections.Generic;
using System.Text;

namespace EA.Eaconfig
{
    [TaskName("TargetInit")]
    public class TargetInit : EAConfigTask
    {
        public readonly static OptionSet TargetDefaults;

        static TargetInit()
        {
            TargetDefaults = new OptionSet();

            TargetDefaults.Options["build"]             = "include" ;
            TargetDefaults.Options["buildall"]          = "include" ;
            TargetDefaults.Options["build-copy"]        = "include" ;
            TargetDefaults.Options["build-copy-library"] = "include";
            TargetDefaults.Options["build-copy-program"] = "include";
            TargetDefaults.Options["build-library"]     = "include" ;
            TargetDefaults.Options["build-program"]     = "include" ;
            TargetDefaults.Options["clean"]             = "include" ;
            TargetDefaults.Options["clean-default"]     = "include" ;
            TargetDefaults.Options["cleanall"]          = "include" ;
            TargetDefaults.Options["codewizard"]        = "include" ;
            TargetDefaults.Options["codewizardall"]     = "include" ;
            TargetDefaults.Options["run"]               = "exclude" ;
            TargetDefaults.Options["runall"]            = "exclude";
            TargetDefaults.Options["run-fast"]          = "include" ;
            // TODO: DEPRECATED, remove later 
            TargetDefaults.Options["csproj"]            = "include" ;
            TargetDefaults.Options["csprojexample"]     = "include" ;
            TargetDefaults.Options["csprojtest"]        = "include" ;
            TargetDefaults.Options["csprojtool"]        = "include" ;
            //  END OF TODO 
            TargetDefaults.Options["distributed-build"]         = "include" ;
            TargetDefaults.Options["distributed-build-example"] = "include" ;
            TargetDefaults.Options["distributed-build-test"]    = "include" ;
            TargetDefaults.Options["distributed-build-tool"]    = "include" ;
            TargetDefaults.Options["doc"]                       = "include" ;
            TargetDefaults.Options["lint"]                      = "include" ;
            TargetDefaults.Options["lintall"]                   = "include" ;
            TargetDefaults.Options["nantvcproj"]                = "include" ;
            TargetDefaults.Options["package"]                   = "include" ;
            TargetDefaults.Options["package-default"]           = "include" ;
            TargetDefaults.Options["example-build"]             = "include" ;
            TargetDefaults.Options["example-buildall"]           = "include" ;
            TargetDefaults.Options["example-codewizard"]        = "exclude" ;
            TargetDefaults.Options["example-lint"]              = "exclude" ;
            TargetDefaults.Options["example-run"]               = "exclude" ;
            TargetDefaults.Options["example-run-fast"]          = "include" ;
            TargetDefaults.Options["incredibuild"]              = "include" ;
            TargetDefaults.Options["incredibuild-example"]      = "include" ;
            TargetDefaults.Options["incredibuild-test"]         = "include" ;
            TargetDefaults.Options["incredibuild-tool"]         = "include" ;
            TargetDefaults.Options["incredibuildall"]           = "include" ;
            TargetDefaults.Options["incredibuild-example-all"]  = "include" ;
            TargetDefaults.Options["incredibuild-test-all"]     = "include" ;
            TargetDefaults.Options["incredibuild-tool-all"]     = "include" ;
            TargetDefaults.Options["package-external"]          = "include" ;
            TargetDefaults.Options["package-external-all"]      = "include";
            TargetDefaults.Options["post"]                      = "exclude" ;
            TargetDefaults.Options["post-external"]             = "exclude" ;
            TargetDefaults.Options["slnexample-slnmaker"]       = "include" ;
            TargetDefaults.Options["slnruntime-slnmaker"]       = "include" ;
            TargetDefaults.Options["slntest-slnmaker"]          = "include" ;
            TargetDefaults.Options["slntool-slnmaker"]          = "include" ;
            TargetDefaults.Options["sndbs"]                     = "include" ;
            TargetDefaults.Options["sndbs-example"]             = "include" ;
            TargetDefaults.Options["sndbs-test"]                = "include" ;
            TargetDefaults.Options["sndbs-tool"]                = "include" ;
            TargetDefaults.Options["test-build"]                = "include" ;
            TargetDefaults.Options["test-buildall"]             = "include" ;
            TargetDefaults.Options["test-codewizard"]           = "exclude" ;
            TargetDefaults.Options["test-lint"]                 = "exclude" ;
            TargetDefaults.Options["test-run"]                  = "exclude" ;
            TargetDefaults.Options["test-runall"]               = "exclude" ;
            TargetDefaults.Options["test-run-fast"]             = "include" ;
            TargetDefaults.Options["test-runall-fast"]          = "exclude" ;
            TargetDefaults.Options["tool-build"]                = "include" ;
            TargetDefaults.Options["tool-buildall"]             = "include" ;
            TargetDefaults.Options["tool-run"]                  = "exclude" ;
            TargetDefaults.Options["tool-run-fast"]             = "include" ;
            TargetDefaults.Options["tool-codewizard"]           = "exclude" ;
            TargetDefaults.Options["tool-lint"]                 = "exclude" ;
            // TODO: DEPRECATED, remove later 
            TargetDefaults.Options["vcproj"]                    = "include" ;
            TargetDefaults.Options["vcprojall"]                 = "include" ;
            TargetDefaults.Options["vcprojallexample"]          = "include" ;
            TargetDefaults.Options["vcprojalltest"]             = "include" ;
            TargetDefaults.Options["vcprojalltool"]             = "include" ;
            TargetDefaults.Options["vcprojexample"]             = "include" ;
            TargetDefaults.Options["vcprojtest"]                = "include" ;
            TargetDefaults.Options["vcprojtool"]                = "include" ;
            // END OF TODO 
            TargetDefaults.Options["visualstudio"]              = "include" ;
            TargetDefaults.Options["visualstudio-example"]      = "include" ;
            TargetDefaults.Options["visualstudio-test"]         = "include" ;
            TargetDefaults.Options["visualstudio-tool"]         = "include" ;
            TargetDefaults.Options["vsprojexample"]             = "include" ;
            TargetDefaults.Options["vsprojruntime"]             = "include" ;
            TargetDefaults.Options["vsprojtest"]                = "include" ;
            TargetDefaults.Options["vsprojtool"]                = "include" ;
        }


        public static void Execute(Project project)
        {
            TargetInit task = new TargetInit(project);
            task.Execute();
        }

        public TargetInit(Project project)
            : base(project)
        {
        }

        public TargetInit()
            : base()
        {
        }

        /// <summary>Execute the task.</summary>
        protected override void ExecuteTask()
        {
            Project.NamedOptionSets["eaconfig.targetdefaults"] = TargetDefaults;

            OptionSet targets = new OptionSet();
            targets.Append(TargetDefaults);

            OptionSet targetoverrides; 
            if (Project.NamedOptionSets.TryGetValue("config.targetoverrides", out targetoverrides))
            {
                foreach (KeyValuePair<string, string> entry in targetoverrides.Options)
                {
                    string name = entry.Key as String;
                    string value = entry.Value as string;
                    
                    if(!String.IsNullOrEmpty(name))
                    {
                        if(targets.Options.Contains(name))
                        {
                            if(targets.Options[name] == value)
                            {
                                if (Log.WarningLevel >= Log.WarnLevel.Advise)
                                {
                                    Log.Warning.WriteLine(
                                        "[{0}-{1}]: The value for '{2}' in 'config.targetoverrides' is the same as the eaconfig default and thus can be removed!",
                                        PackageName, PackageVersion, name);
                                }
                            }

                            targets.Options[name] = value;
                        }
                        else
                        {
                            Log.Warning.WriteLine("[{0}-{1}: Invalid target name '{2}' in 'config.targetoverrides'!", PackageName, PackageVersion, name);
                        }
                    }
                }
            }
            Project.NamedOptionSets["eaconfig.targets"] = targets;
        }
    }
}
