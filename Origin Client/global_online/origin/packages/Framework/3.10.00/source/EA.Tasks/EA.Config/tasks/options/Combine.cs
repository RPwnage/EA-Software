using System;
using System.Collections;
using System.Collections.Generic;
using System.Text;

using NAnt.Core;
using NAnt.Core.Attributes;
using EA.Eaconfig.Build;

namespace EA.Eaconfig
{
    [TaskName("Combine")]
    public class Combine : EAConfigTask
    {
        public static readonly string[] CONFIG_OPTION_SETS = { 
                  "config-options-program", 
                  "config-options-library", 
                  "config-options-dynamiclibrary",
                  "config-options-easharplibrary",
                  "config-options-windowsprogram", 
                  "config-options-managedcppprogram", 
                  "config-options-managedcppwindowsprogram", 
                  "config-options-managedcppassembly",
                  "config-options-pythonprogram",
                  "config-options-csharplibrary",
                  "config-options-csharpprogram",
                  "config-options-csharpwindowsprogram",
                  "config-options-rsoprogram",
                  "config-options-fsharplibrary",
                  "config-options-fsharpprogram",
                  "config-options-fsharpwindowsprogram",
                  "config-options-winrtcppprogram",
                  "config-options-winrtcpplibrary",
                  "config-options-winrtcppdynamiclibrary"
        };


        public static void Execute(Project project)
        {
            Combine task = new Combine(project);
            task.Execute();
        }

        public Combine(Project project)
            : base(project)
        {
        }

        public Combine()
            : base()
        {
        }

        /// <summary>Execute the task.</summary>
        protected override void ExecuteTask()
        {
            List<string> buildSets = new List<string>();
            buildSets.AddRange(CONFIG_OPTION_SETS);

            SetRemappedLibBinDirectories();

            //NOTICE(RASHIN):
            //The below allows SDK packages to provide their own optionsets
            //but delay in actually generating them until other optionsets are defined (fromoptionset).
            string osName = ConfigSystem + ".buildoptionsets";
            OptionSet platformOS;
            if(Project.NamedOptionSets.TryGetValue(osName, out platformOS))           
            {
                foreach (KeyValuePair<string,string> entry in platformOS.Options)
                {
                    string name = (string)entry.Key;
                    buildSets.Add(name);
                }
            }

            foreach (string setName in buildSets)
            {
                if (Project.NamedOptionSets.Contains(setName))
                {
                    MergeOptionset.Execute(Project, setName, "config-options-common");
                }
            }

            SetDefaultOptionsIfMissing();

            // Make PPU and SPU specific settings
            if (ConfigSystem == "ps3")
            {
                // --- SPU --- : __private_create_spu_buildtypes
                //
                //Target which creates the SPU-specific buildtypes (SPUProgram and
                //SPULibrary".  This is called from within combine.xml, and uses the
                //GenerateBuildOptionsetSPU task which is defined further down this file.

                GenerateOptionsSPU.Execute(Project);

                // --- PPU --- : __private_merge_mixin_ppu_options
                //
                //Target to merge the values in the "mixin" PPU optionset into the Program
                //and Library "proto-buildtype" optionsets.  This is called from within
                //combine.xml prior to expansion of the StdProgram and StdLibrary optionsets
                //for PS3 builds.
                MergeOptionset.Execute(Project, "config-options-program", "config-options-ps3-ppu");
                MergeOptionset.Execute(Project, "config-options-library", "config-options-ps3-ppu");
                MergeOptionset.Execute(Project, "config-options-dynamiclibrary", "config-options-ps3-ppu");
            }

            

            foreach (string setName in buildSets)
            {
                if (Project.NamedOptionSets.Contains(setName))
                {
                    GenerateBuildOptionset.Execute(Project, setName);
                }
            }

            GenerateAdditionalBuildTypes();

        }

        private void GenerateAdditionalBuildTypes()
        {
            // --- Generate C language Program and Library BuildOptionSets ---:

            OptionSet libC = CreateBuildOptionSetFrom("CLibrary", "config-options-library-c", "config-options-library");
            libC.Options["clanguage"] = "on";
            SetOptionToPropertyValue(libC, "cc", "cc-clanguage");
            SetOptionToPropertyValue(libC, "as", "as-clanguage");
            SetOptionToPropertyValue(libC, "lib", "lib-clanguage");
            SetOptionToPropertyValue(libC, "link", "link-clanguage");

            GenerateBuildOptionset.Execute(Project, "config-options-library-c");

            OptionSet progC = CreateBuildOptionSetFrom("CProgram", "config-options-program-c", "config-options-program");
            progC.Options["clanguage"] = "on";
            SetOptionToPropertyValue(progC, "cc", "cc-clanguage");
            SetOptionToPropertyValue(progC, "as", "as-clanguage");
            SetOptionToPropertyValue(progC, "lib", "lib-clanguage");
            SetOptionToPropertyValue(progC, "link", "link-clanguage");

            GenerateBuildOptionset.Execute(Project, "config-options-program-c");

            // --- Generate DynamicLibrary-Static-Link BuildOptionSet ---:
            if (Project.NamedOptionSets.Contains("config-options-dynamiclibrary"))
            {
                // Provide a DLL optionset which doesn't define EA_DLL, so has static linkage against EAThread or
                // other packages which provide DLL configurations.  This should probably be the default behaviour
                // for DynamicLibrary actually, but that would break existing clients. -->
                OptionSet dynLibStat = new OptionSet();
                dynLibStat.FromOptionSetName = "config-options-dynamiclibrary";
                dynLibStat.Project = Project;
                dynLibStat.Append(OptionSetUtil.GetOptionSet(Project, dynLibStat.FromOptionSetName));
                dynLibStat.Options["buildset.name"] = "DynamicLibraryStaticLinkage";
                if (!Properties.Contains("Dll"))
                {
                    OptionSetUtil.ReplaceOption(dynLibStat, "buildset.cc.defines", "EA_DLL", String.Empty);
                }
                Project.NamedOptionSets["options-dynamiclibrary-static-linkage"] = dynLibStat;

                GenerateBuildOptionset.Execute(Project, "options-dynamiclibrary-static-linkage");

                //CDynamicLibrary
                OptionSet dynlibC = CreateBuildOptionSetFrom("CDynamicLibrary", "config-options-dynamiclibrary-c", "config-options-dynamiclibrary");
                dynlibC.Options["clanguage"] = "on";
                SetOptionToPropertyValue(dynlibC, "cc", "cc-clanguage");
                SetOptionToPropertyValue(dynlibC, "as", "as-clanguage");
                SetOptionToPropertyValue(dynlibC, "lib", "lib-clanguage");
                SetOptionToPropertyValue(dynlibC, "link", "link-clanguage");

                GenerateBuildOptionset.Execute(Project, "config-options-dynamiclibrary-c");
            }
        }

        protected bool SetOptionIfMissing(OptionSet optionset, string optionName, string optionValue)
        {
            bool ret = false;
            if (!optionset.Options.Contains(optionName))
            {
                optionset.Options[optionName] = optionValue;
                ret = true;
            }
            return ret;
        }


        protected void SetRemappedLibBinDirectories()
        {
            var mapping = Project.GetOutputMapping(Properties["package.name"]);

            if (mapping != null)
            {
                string configlibdir;
                if (mapping.Options.TryGetValue("configlibdir", out configlibdir))
                {
                    Project.Properties["package.configlibdir"] = configlibdir;
                }

                string configbindir;
                if (mapping.Options.TryGetValue("configbindir", out configbindir))
                {
                    Project.Properties["package.configbindir"] = configbindir;
                }
            }
        }

        //IMTODO: remove this. All should be set in the platform-specific sections:
        private void SetDefaultOptionsIfMissing()
        {
            //Defaults for Dll.
            OptionSet dynamiclibrary;
            if (Project.NamedOptionSets.TryGetValue("config-options-dynamiclibrary", out dynamiclibrary))
            {
                SetOptionIfMissing(dynamiclibrary, "linkoutputmapname", @"%outputdir%\%outputname%.map");
                if (ConfigCompiler == "vc")
                {
                    SetOptionIfMissing(dynamiclibrary, "linkoutputpdbname", @"%outputdir%\%outputname%.pdb");
                    if (ConfigSystem == "xenon")
                    {
                        SetOptionIfMissing(dynamiclibrary, "linkoutputname", @"%outputdir%\%outputname%" + Properties["temp-dll-suffix"]);
                    }
                    else
                    {
                        SetOptionIfMissing(dynamiclibrary, "linkoutputname", @"%outputdir%\%outputname%.dll");
                    }
                }
                if (ConfigSystem == "xenon")
                {
                    SetOptionIfMissing(dynamiclibrary, "imgbldoutputname", @"%outputdir%\%outputname%" + Properties["dll-suffix"]);
                }
                else if (ConfigPlatform == "pc-gcc")
                {
                    SetOptionIfMissing(dynamiclibrary, "linkoutputname", @"%outputdir%\%outputname%.dll");
                }
                else if (ConfigSystem == "ps3")
                {
                    SetOptionIfMissing(dynamiclibrary, "linkoutputname", @"%intermediatedir%\%outputname%" + Properties["dll-suffix"]);
                    SetOptionIfMissing(dynamiclibrary, "securelinkoutputname", @"%outputdir%\%outputname%" + Properties["secured-dll-suffix"]);
                }
                else if (Properties["dll-suffix"] != null)
                {
                    SetOptionIfMissing(dynamiclibrary, "linkoutputname", @"%outputdir%/%outputname%" + Properties["dll-suffix"]);
                }
            }

            //Defaults for Program.
            OptionSet program = Project.NamedOptionSets["config-options-program"];
            if (program != null)
            {
                SetOptionIfMissing(program, "linkoutputname", @"%outputdir%/%outputname%" + Properties["exe-suffix"]);
                SetOptionIfMissing(program, "linkoutputpdbname", @"%outputdir%\%outputname%.pdb");
                SetOptionIfMissing(program, "linkoutputmapname", @"%outputdir%\%outputname%.map");
                if (ConfigSystem == "xenon")
                {
                    SetOptionIfMissing(program, "imgbldoutputname", @"%outputdir%\%outputname%" + Properties["exe-suffix"]);
                }
                if (ConfigSystem == "ps3")
                {
                    SetOptionIfMissing(dynamiclibrary, "linkoutputname", @"%intermediatedir%\%outputname%" + Properties["exe-suffix"]);
                    SetOptionIfMissing(dynamiclibrary, "securelinkoutputname", @"%outputdir%\%outputname%" + Properties["secured-exe-suffix"]);
                }
            }

            //Defaults for the Managed buildtypes for PC.
            string[] managedNames = { "windowsprogram", "managedcppprogram", "managedcppwindowsprogram", "managedcppassembly", "csharplibrary", "csharpprogram", "csharpwindowsprogram, fsharplibrary, fsharpprogram, fsharpwindowsprogram", "winrtcppprogram", "winrtcpplibrary" };
            foreach (string name in managedNames)
            {
                OptionSet os;

                if (Project.NamedOptionSets.TryGetValue("config-options-" + name, out os))
                {
                    if (name == "csharplibrary" || name == "fsharplibrary")
                    {
                        SetOptionIfMissing(os, "linkoutputname", @"%outputdir%\%outputname%.dll");
                    }
                    else
                    {
                        SetOptionIfMissing(os, "linkoutputname", @"%outputdir%\%outputname%" + Properties["exe-suffix"]);
                    }
                    SetOptionIfMissing(os, "linkoutputpdbname", @"%outputdir%\%outputname%.pdb");
                    SetOptionIfMissing(os, "linkoutputmapname", @"%outputdir%\%outputname%.map");
                }
            }
        }

        private void SetOptionToPropertyValue(OptionSet set, string optionName, string propName)
        {
            string propVal = Project.Properties[propName];
            if (propVal != null)
            {
                set.Options[optionName] = propVal;
            }
        }
    }
}
