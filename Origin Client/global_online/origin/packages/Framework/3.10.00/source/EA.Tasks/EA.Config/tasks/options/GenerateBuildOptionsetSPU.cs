using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Attributes;
using NAnt.Core.Logging;

using System;
using System.Collections;
using System.Collections.Generic;
using System.Text;

namespace EA.Eaconfig
{
    /// <summary>
    /// Creates <token>buildtype</token> optionset from a meta build optionset for PS3 SPU. 
    /// The name of the target optionset is provided by 'buildset.name' option.
    /// </summary>
    /// <example>
    ///  &lt;optionset name="config-options-spu-library-custom" fromoptionset="config-options-library"&gt;
    ///    &lt;option name="buildset.name" value="Library-CustomSPU" /&gt;
    ///    &lt;option name="optimization" value="off"/&gt;
    ///  &lt;/optionset&gt;
    ///  &lt;GenerateBuildOptionsetSPU configsetname="config-options-library-custom"/&gt;
    /// </example>
    [TaskName("GenerateBuildOptionsetSPU")]
    public class GenerateBuildOptionsetSPU : EAConfigTask
    {
        /// <summary>
        /// The name of the meta optionset. 
        /// The name of the target <token>buildtype</token> optionset is provided by 'buildset.name' option
        /// </summary>
        [TaskAttribute("configsetname", Required = true)]
        public string ConfigSetName
        {
            get { return _configsetName; }
            set { _configsetName = value; }
        }

        public static void Execute(Project project, string configsetname)
        {
            GenerateBuildOptionsetSPU task = new GenerateBuildOptionsetSPU(project, configsetname);
            task.Execute();
        }

        public static void Execute(Project project, OptionSet configoptionset, string configsetname)
        {
            GenerateBuildOptionsetSPU task = new GenerateBuildOptionsetSPU(project, configoptionset);
            task.ConfigSetName = configsetname;
            task.ExecuteTask();
        }

        public GenerateBuildOptionsetSPU(Project project, string configsetname)
            : base(project)
        {
            ConfigSetName = configsetname;
        }

        public GenerateBuildOptionsetSPU(Project project, OptionSet configoptionset)
            : base(project)
        {
            _ConfigOptionSet = configoptionset;
        }


        public GenerateBuildOptionsetSPU()
            : base()
        {
        }

        /// <summary>Execute the task.</summary>
        protected override void ExecuteTask()
        {
            GenerateSPUOptionset(ConfigOptionSet, ConfigSetName);
        }

        private void GenerateSPUOptionset(OptionSet optionset, string optionsetName)
        {
            string sdklibs_regular = Properties["platform.sdklibs.regular"];
            string sdklibs_debug = Properties["platform.sdklibs.debug"];

            Properties["platform.sdklibs.regular"] = Properties["platform.sdklibs.regular-spu"];
            Properties["platform.sdklibs.debug"] = Properties["platform.sdklibs.debug-spu"];

            GenerateBuildOptionset.Execute(Project, optionset, optionsetName);

            Properties["platform.sdklibs.regular"] = sdklibs_regular;
            Properties["platform.sdklibs.debug"] = sdklibs_debug;

            // Set Extra SPU flags:
            OptionSet buildOptionSet = OptionSetUtil.GetOptionSetOrFail(Project, optionset.Options["buildset.name"]);

            OptionSetUtil.AppendOption(buildOptionSet, "cc.defines", "SN_TARGET_PS3_SPU");
            OptionSetUtil.AppendOption(buildOptionSet, "cc.defines", "EA_CELLSPUDMA=1");
            OptionSetUtil.AppendOption(buildOptionSet, "cc.gccdefines", "__SPU__");

            OptionSetUtil.AppendOption(buildOptionSet, "as.options", "-Werror");
            OptionSetUtil.AppendOption(buildOptionSet, "as.options", "-c");
            OptionSetUtil.AppendOption(buildOptionSet, "cc.options", "-ffunction-sections");
            OptionSetUtil.AppendOption(buildOptionSet, "cc.options", "-fdata-sections");
            OptionSetUtil.AppendOption(buildOptionSet, "link.options", "-Wl,--gc-sections");

            OptionSetUtil.ReplaceOption(buildOptionSet, "cc.options", "-maltivec", String.Empty);
            OptionSetUtil.ReplaceOption(buildOptionSet, "cc.options", "-mvrsave=no", String.Empty);
            OptionSetUtil.ReplaceOption(buildOptionSet, "link.options", "-Wl,--oformat=fself", String.Empty);
            OptionSetUtil.ReplaceOption(buildOptionSet, "link.options", "-Wl,--write-fself-digest", String.Empty);

            if (!String.IsNullOrEmpty(OptionSetUtil.GetOption(buildOptionSet, "linkoutputname")))
            {
                if (!OptionSetUtil.ReplaceOnlyOption(buildOptionSet, "linkoutputname", Properties["secured-exe-suffix"], Properties["exe-suffix"]))
                {
                    //IMTODO: This late substitution does not always work. Why do I need it here?
                   // Log.Warning.WriteLine("Failed to replace suffix in 'linkoutputname' option: Replace('{0}', '{1}', '{2}')", OptionSetUtil.GetOption(buildOptionSet, "linkoutputname"), Properties["secured-exe-suffix"], Properties["exe-suffix"]);
                }
            }
        }

        private OptionSet ConfigOptionSet
        {
            get
            {
                if (_ConfigOptionSet == null)
                {
                    _ConfigOptionSet = Project.NamedOptionSets[ConfigSetName];
                    if (_ConfigOptionSet == null)
                    {
                        Error.Throw(Project, Location, "GenerateBuildOptionsetSPU", "OptionSet '{0}' does not exist.", ConfigSetName);
                    }
                    else if(_ConfigOptionSet.Options.Contains("buildset.finalbuildtype"))
                    {
                        String format =
                         "'{0}' is not a proto-buildtype optionset, and can not be used to generate build types.\n" +
                         "\n" +
                         "Possible reasons: '{0}' might be derived from a non-proto-buildtype optionset.\n" +
                         "\n" +
                         "Examples of proto-buildtypes are 'config-options-program', 'config-options-library' etc.\n" +
                         "and any other optionsets that are derived from proto-buildtypes.\n";

                        Error.Throw(Project, Location, Name, format, ConfigSetName);
                    }
                }
                return _ConfigOptionSet;
            }
        }

        private string _configsetName = String.Empty;
        private OptionSet _ConfigOptionSet = null;
    }
}
