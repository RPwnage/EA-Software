using NAnt.Core;
using NAnt.Core.Attributes;

using System;
using System.Collections;
using System.Collections.Generic;
using System.Text;

namespace EA.Eaconfig
{
    [TaskName("GenerateOptionsSPU")]
    public class GenerateOptionsSPU : EAConfigTask
    {

        public static void Execute(Project project)
        {
            GenerateOptionsSPU task = new GenerateOptionsSPU(project);
            task.Execute();
        }

        public GenerateOptionsSPU(Project project)
            : base(project)
        {
        }

        public GenerateOptionsSPU()
            : base()
        {
        }

        /// <summary>Execute the task.</summary>
        protected override void ExecuteTask()
        {
            // Create -- SPU library and SPU program option sets

            OptionSet ps3Spu = OptionSetUtil.GetOptionSetOrFail(Project, "ps3spu");

            OptionSet  lib = CreateBuildOptionSetFrom("SPULibrary", "config-options-library-spu", "config-options-library");

            lib.Options["buildset.cc.includedirs"] = ps3Spu.Options["cc.includedirs"];
            lib.Options["cc"] = ps3Spu.Options["cc"];
            lib.Options["as"] = ps3Spu.Options["cc"];
            lib.Options["lib"] = ps3Spu.Options["lib"];

            OptionSet libC = CreateBuildOptionSetFrom("SPUCLibrary", "config-options-library-spu-c", "config-options-library");

            libC.Options["clanguage"] = "on";
            libC.Options["buildset.cc.includedirs"] = ps3Spu.Options["cc.includedirs"];
            libC.Options["cc"] = ps3Spu.Options["cc"];
            libC.Options["as"] = ps3Spu.Options["cc"];
            libC.Options["lib"] = ps3Spu.Options["lib"];
            
            OptionSet prog = CreateBuildOptionSetFrom("SPUProgram", "config-options-program-spu", "config-options-program");

            prog.Options["buildset.cc.includedirs"] = ps3Spu.Options["cc.includedirs"];
            prog.Options["cc"] = ps3Spu.Options["cc"];
            prog.Options["as"] = ps3Spu.Options["cc"];
            prog.Options["link"] = ps3Spu.Options["link"];
            prog.Options["buildset.link.librarydirs"] = ps3Spu.Options["link.librarydirs"];

            // Merge settings from 'config-options-ps3-spu' optionset into the SPU program and library optionsets.
            // If there any conflicts then the settings in the mixin will override the ones in the original
            // optionsets, but that's what we want anyway.

            lib.Append(OptionSetUtil.GetOptionSet(Project, "config-options-ps3-spu"));
            libC.Append(OptionSetUtil.GetOptionSet(Project, "config-options-ps3-spu"));
            prog.Append(OptionSetUtil.GetOptionSet(Project, "config-options-ps3-spu"));

            GenerateBuildOptionsetSPU.Execute(Project, lib, "config-options-library-spu");
            GenerateBuildOptionsetSPU.Execute(Project, libC, "config-options-library-spu-c");
            GenerateBuildOptionsetSPU.Execute(Project, prog, "config-options-program-spu");

        }
    }
}
