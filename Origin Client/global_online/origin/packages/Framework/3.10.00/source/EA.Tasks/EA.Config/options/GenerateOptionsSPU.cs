using NAnt.Core;
using NAnt.Core.Attributes;

using System;
using System.Collections;
using System.Collections.Generic;
using System.Text;

namespace EA.Eaconfig
{
    /// <summary>
    /// Internal task used by eaconfig.
    /// </summary>
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

            OptionSet  lib = CreateBuildOptionSetFrom("SPULibrary", "config-options-library-spu", "config-options-library");

            OptionSet libC = CreateBuildOptionSetFrom("SPUCLibrary", "config-options-library-spu-c", "config-options-library");
            
            OptionSet prog = CreateBuildOptionSetFrom("SPUProgram", "config-options-program-spu", "config-options-program");

            // Merge settings from 'config-options-ps3-spu' optionset into the SPU program and library optionsets.
            // If there any conflicts then the settings in the mixin will override the ones in the original
            // optionsets, but that's what we want anyway.

            OptionSet configOptionsSPU = OptionSetUtil.GetOptionSet(Project, "config-options-ps3-spu");

            lib.Append(configOptionsSPU);
            libC.Append(configOptionsSPU);
            prog.Append(configOptionsSPU);

            libC.Options["clanguage"] = "on";

            GenerateBuildOptionsetSPU.Execute(Project, lib, "config-options-library-spu");
            GenerateBuildOptionsetSPU.Execute(Project, libC, "config-options-library-spu-c");
            GenerateBuildOptionsetSPU.Execute(Project, prog, "config-options-program-spu");
        }
    }
}
