using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Logging;

using System;
using System.Collections;
using System.Collections.Generic;
using System.Text;

namespace EA.Eaconfig
{
    [TaskName("GenerateBuildOptionset")]
    public class GenerateBuildOptionset : EAConfigTask
    {
        public static String[] DefaultControllingProperties = { "package.usemultithreadeddynamiclib:multithreadeddynamiclib", 
                                                                "eaconfig.usemultithreadeddynamiclib:multithreadeddynamiclib",
                                                                "eaconfig.debugflags",
                                                                "eaconfig.debugsymbols",
                                                                "eaconfig.optimization",
                                                                "eaconfig.usedebuglibs",
                                                                "eaconfig.warnings",
                                                                "eaconfig.misc",
                                                                "eaconfig.warningsaserrors",
                                                                "eaconfig.rtti",
                                                                "eaconfig.exceptions",
                                                                "eaconfig.toolconfig",
                                                                "eaconfig.incrementallinking",
                                                                "eaconfig.generatemapfile",
                                                                "eaconfig.generatedll",
                                                                "eaconfig.multithreadeddynamiclib",
                                                                "eaconfig.managedcpp",
                                                                "eaconfig.banner",                          
                                                                "eaconfig.editandcontinue",
                                                                "eaconfig.c7debugsymbols",
                                                                "eaconfig.standardsdklibs",
                                                                "eaconfig.runtimeerrorchecking",
                                                                "eaconfig.pcconsole",
                                                                "eaconfig.profile",
                                                                "eaconfig.uselibrarydependencyinputs",
                                                                "eaconfig.optimization.ltcg"
                                                             };

        [TaskAttribute("configsetname", Required = true)]
        public string ConfigSetName
        {
            get { return _configsetName; }
            set { _configsetName = value; }
        }

        [TaskAttribute("controllingproperties", Required = false)]
        public string ControllingProperties
        {
            set
            {
                _controllingProperties = value.Split(new char[] { ' ', '\n', '\t' });
            }
        }

        public static void Execute(Project project, string configsetname)
        {
            GenerateBuildOptionset task = new GenerateBuildOptionset(project, configsetname);
            task.Execute();
        }

        public static void Execute(Project project, OptionSet configoptionset, string configsetname)
        {
            GenerateBuildOptionset task = new GenerateBuildOptionset(project, configoptionset);
            task.ConfigSetName = configsetname;
            task.ExecuteTask();
        }

        public GenerateBuildOptionset(Project project, string configsetname)
            : base(project)
        {
            ConfigSetName = configsetname;
        }

        public GenerateBuildOptionset(Project project, OptionSet configoptionset)
            : base(project)
        {
            _ConfigOptionSet = configoptionset;
        }


        public GenerateBuildOptionset()
            : base()
        {
        }

        /// <summary>Execute the task.</summary>
        protected override void ExecuteTask()
        {
            //NOTICE (RASHIN):
            //This is a hackish attempt at specifying a group of options
            //that should be applied when performing a build for external
            //distribution. Requested by James Fairweather
            if (ExternalBuild)
            {
                Project.Properties.UpdateReadOnly("eaconfig.optimization.ltcg", "false");
                Project.Properties.UpdateReadOnly("eaconfig.debugsymbols", "false");
                Project.Properties.UpdateReadOnly("eaconfig.generatemapfile", "false");
                Project.Properties.UpdateReadOnly("bulkbuild", "false");
            }

            String buildOptionSetName = ConfigOptionSet.Options["buildset.name"];

            if (String.IsNullOrEmpty(buildOptionSetName))
            {
                Error.Throw(Project, Location, Name, "Proto OptionSet '{0}' must have option 'buildset.name'.", ConfigSetName);
            }

            if (buildOptionSetName == ConfigSetName)
            {
                string msg = Error.Format(Project, Name, "WARNING", "Final and proto BuilOptionSets have the same name '{0}'. Proto BuilOptionSet will be overwritten.", buildOptionSetName);
                Log.WriteLine(msg);
            }
            else if (Project.NamedOptionSets.Contains(buildOptionSetName))
            {
                // Something is wrong. 
                string msg = Error.Format(Project, Name, "WARNING", "OptionSet '{0}' already exists. Attempt to create new OptionSet '{0}' from proto option set is ignored.", buildOptionSetName, ConfigSetName);
                Log.WriteLine(msg);
                return;
            }

            foreach (string propertyName in _controllingProperties)
            {
                ApplyProperty(propertyName);
            }

            OptionSet buildset = new OptionSet();
            foreach (DictionaryEntry entry in ConfigOptionSet.Options)
            {
                string name = entry.Key as String;

                if (!String.IsNullOrEmpty(name))
                {
                    string newName = name.Replace("buildset.", "");
                    if (name == "buildset.tasks")
                    {
                        newName = "build.tasks";
                    }
                    string value = entry.Value as String;

                    // There should be no duplicates;
                    if (buildset.Options.Contains(newName))
                    {
                        string msg = Error.Format(Project, "GenerateBuildOptionset", "WARNING",
                            "Proto optionset '{0}' contains conflicting options:\n  {1}={2}\n  {3}={4}\n\nOption {1} from '{0} will be used",
                            ConfigSetName, name, ConfigOptionSet.Options[name], newName, ConfigOptionSet.Options[newName]);

                        value = ConfigOptionSet.Options[name] as string;
                        if (value == null) value = String.Empty;

                        buildset.Options[newName] = value;
                    }
                    else
                    {
                        buildset.Options.Add(newName, value);
                    }
                }
            }

            GenerateOptions.Execute(Project, buildset, buildOptionSetName);

            buildset.Options.Remove("buildset.protobuildtype");
            buildset.Options.Add("buildset.finalbuildtype", "true");
            buildset.Options.Add("buildset.protobuildtype.name", ConfigSetName);

            Project.NamedOptionSets[buildOptionSetName] = buildset;
            // For backwards compatibility. Some build scripts wrongly use "buildsettings" option set.           
            Project.NamedOptionSets["buildsettings"] = buildset;

            //Some of the packages hook into the following internal properties.
            // This should be revoved 
            Project.Properties["config.optionset.name"] = ConfigSetName;
            Project.Properties["build.optionset.name"] = buildOptionSetName;
        }

        private void ApplyProperty(string propertyName)
        {
            int optionNameIndex = propertyName.LastIndexOf(':');
            string optionName = null;
            if (optionNameIndex > 0)
            {
                optionName = propertyName.Substring(optionNameIndex + 1);
                propertyName = propertyName.Substring(0, optionNameIndex);
            }
            string val = Properties[propertyName];
            if (val != null)
            {
                if (String.IsNullOrEmpty(optionName))
                {
                    int dotIndex = propertyName.IndexOf('.');
                    optionName = propertyName.Substring(dotIndex < 0 ? 0 : dotIndex + 1);
                }

                if (val.Equals("true", StringComparison.InvariantCultureIgnoreCase) ||
                    val.Equals("on", StringComparison.InvariantCultureIgnoreCase))
                {
                    ConfigOptionSet.Options[optionName] = "on";
                }
                else if (val.Equals("false", StringComparison.InvariantCultureIgnoreCase) ||
                         val.Equals("off", StringComparison.InvariantCultureIgnoreCase))
                {
                    ConfigOptionSet.Options[optionName] = "off";
                }
                else
                {
                    Error.Throw(Project, Location, Name, "Property '{0}' has invalid value '{1}'. Valid values are: 'on', 'true', 'off', 'false'.", propertyName, val);
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
                        Error.Throw(Project, Location, Name, "OptionSet '{0}' does not exist.", ConfigSetName);
                    }
                    else if (_ConfigOptionSet.Options.Contains("buildset.finalbuildtype"))
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
        private string[] _controllingProperties = DefaultControllingProperties;
        private OptionSet _ConfigOptionSet = null;
    }
}
