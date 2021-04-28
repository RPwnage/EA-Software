using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Logging;
using NAnt.Shared.Properties;

using System;
using System.Collections;
using System.Collections.Generic;
using System.Text;

namespace EA.Eaconfig
{
    /// <summary>
    /// Creates <token>buildtype</token> optionset from a meta build optionset. 
    /// </summary>
    /// <example>
    ///  &lt;optionset name="config-options-library-custom" fromoptionset="config-options-library"&gt;
    ///    &lt;option name="buildset.name" value="Library-Custom" /&gt;
    ///    &lt;option name="optimization" value="off"/&gt;
    ///  &lt;/optionset&gt;
    ///  &lt;GenerateBuildOptionset configsetname="config-options-library-custom"/&gt;
    /// </example>
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

        /// <summary>
        /// List of the controlling properties. Intended to be used in the cofiguration packages to override default list of controlling properties like 
        /// 'eaconfig.debugflags", eaconfig.optimization, etc.
        /// </summary>
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

        public static OptionSet Execute(Project project, OptionSet configoptionset, string configsetname, bool saveFinalToproject = true)
        {
            GenerateBuildOptionset task = new GenerateBuildOptionset(project, configoptionset, saveFinalToproject);
            task.ConfigSetName = configsetname;
            task.ExecuteTask();
            return task._final;
        }

        public GenerateBuildOptionset(Project project, string configsetname)
            : base(project)
        {
            ConfigSetName = configsetname;
        }

        public GenerateBuildOptionset(Project project, OptionSet configoptionset, bool saveFinalToproject = true)
            : base(project)
        {
            _ConfigOptionSet = configoptionset;
            _saveFinalToproject = saveFinalToproject;
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
                Project.Properties.UpdateReadOnly(NAnt.Shared.Properties.FrameworkProperties.BulkBuildPropertyName, "false");
            }

            String buildOptionSetName = GetConfigOptionSet().Options["buildset.name"];

            if (String.IsNullOrEmpty(buildOptionSetName))
            {
                Error.Throw(Project, Location, Name, "Proto OptionSet '{0}' must have option 'buildset.name'.", ConfigSetName);
            }

            if (buildOptionSetName == ConfigSetName)
            {
                string msg = Error.Format(Project, Name, "WARNING", Location, "Final and proto BuilOptionSets have the same name '{0}'. Proto BuilOptionSet will be overwritten.", buildOptionSetName);
                Log.Warning.WriteLine(msg);
            }
            else if (Project.NamedOptionSets.Contains(buildOptionSetName))
            {
                // Something is wrong. 
                string msg = Error.Format(Project, Name, "WARNING", Location, "OptionSet '{0}' already exists. Attempt to create new OptionSet '{0}' from proto option set is ignored.", buildOptionSetName, ConfigSetName);
                Log.Warning.WriteLine(msg);
                return;
            }

            foreach (string propertyName in _controllingProperties)
            {
                ApplyProperty(propertyName);
            }

            OptionSet buildset = new OptionSet() { Project = _project };
            foreach (var entry in GetConfigOptionSet().Options)
            {
                string name = entry.Key;

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
                        string msg = Error.Format(Project, "GenerateBuildOptionset", "WARNING", Location,
                            "Proto optionset '{0}' contains conflicting options:\n  {1}={2}\n  {3}={4}\n\nOption {1} from '{0} will be used",
                            ConfigSetName, name, GetConfigOptionSet().Options[name], newName, GetConfigOptionSet().Options[newName]);
                        if (Log.Level > Log.LogLevel.Diagnostic)
                        {
                            Log.Warning.WriteLine(msg);
                        }

                        value = GetConfigOptionSet().Options[name] as string;
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

            //Remove options:
            var changedValues = new Dictionary<string, string>();
            foreach (var entry in buildset.Options)
            {
                if(!String.IsNullOrEmpty(entry.Key) && entry.Key.StartsWith("remove.") && !String.IsNullOrWhiteSpace(entry.Value))
                {
                    string newName = entry.Key.Replace("buildset.", "").Replace("remove.", "");

                    string valueToEdit;
                    if (buildset.Options.TryGetValue(newName, out valueToEdit))
                    {
                        foreach(var toRemove in entry.Value.LinesToArray())
                        {
                            valueToEdit = valueToEdit.Replace(toRemove + " ", " ").Replace(toRemove + "\n", "\n").Replace(toRemove + "\r", "\r");
                        }

                        changedValues[newName] = valueToEdit;
                    }
                }
            }
            foreach(var newEntry in changedValues)
            {
                buildset.Options[newEntry.Key] = newEntry.Value;
            }



            buildset.Options.Remove("buildset.protobuildtype");
            buildset.Options.Add("buildset.finalbuildtype", "true");
            buildset.Options.Add("buildset.protobuildtype.name", ConfigSetName);

            if (_saveFinalToproject)
            {
                Project.NamedOptionSets[buildOptionSetName] = buildset;
                // For backwards compatibility. Some build scripts wrongly use "buildsettings" option set.           
                Project.NamedOptionSets["buildsettings"] = buildset;
            }
            _final = buildset;

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

                if (val.Equals("true", StringComparison.OrdinalIgnoreCase) ||
                    val.Equals("on", StringComparison.OrdinalIgnoreCase))
                {
                    GetConfigOptionSet().Options[optionName] = "on";
                }
                else if (val.Equals("false", StringComparison.OrdinalIgnoreCase) ||
                         val.Equals("off", StringComparison.OrdinalIgnoreCase))
                {
                    GetConfigOptionSet().Options[optionName] = "off";
                }
                else
                {
                    Error.Throw(Project, Location, Name, "Property '{0}' has invalid value '{1}'. Valid values are: 'on', 'true', 'off', 'false'.", propertyName, val);
                }
            }
        }

        private OptionSet GetConfigOptionSet()
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

        private string _configsetName = String.Empty;
        private string[] _controllingProperties = DefaultControllingProperties;
        private OptionSet _ConfigOptionSet = null;
        private OptionSet _final = null;
        private bool _saveFinalToproject = true; 
    }
}
