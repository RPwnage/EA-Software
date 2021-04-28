using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.IO;

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Logging;
using NAnt.Core.Threading;
using PackageMap = NAnt.Core.PackageCore.PackageMap;
using EA.FrameworkTasks.Model;
using EA.Eaconfig.Core;
using EA.Eaconfig.Modules;
using NAnt.Shared.Properties;

namespace EA.Eaconfig.Build
{
    abstract public class ModuleProcessorBase
    {
        private readonly ProcessableModule _module;

        public readonly Project Project;

        protected readonly string LogPrefix;

        protected ModuleProcessorBase(ProcessableModule module, string logPrefix="")
        {
            Project = (module.Package as Package).Project;
            _module = module;
            LogPrefix = logPrefix;

            BuildOptionSet = GetModuleBuildOptionSet(module.BuildType.Name);
        }

        private BuildGroups BuildGroup
        {
            get { return _module.BuildGroup; }
        }

        public BuildType BuildType
        {
            get { return _module.BuildType; }
        }


        public readonly OptionSet BuildOptionSet;

        protected Log Log
        {
            get { return Project.Log; }
        }

        protected string PropGroupName(string name)
        {
            if (name.StartsWith("."))
            {
                return _module.GroupName + name;
            }
            return _module.GroupName + "." + name;
        }

        protected string PropGroupValue(string name, string defVal = "")
        {
            string val = null;
            if(!String.IsNullOrEmpty(name))
            {
                val = Project.Properties[PropGroupName(name)];
            }
            
            return val??defVal;
        }

        protected FileSet PropGroupFileSet(string name)
        {

            if (String.IsNullOrEmpty(name))
            {
                return null;
            }
            return Project.NamedFileSets[PropGroupName(name)];
        }

        protected string GetModulePropertyValue(string propertyname, params string[] defaultVals)
        {
            if (propertyname == null && (defaultVals == null || defaultVals.Length == 0))
            {
                return null;
            }

            string value = String.Concat(PropGroupValue(propertyname, null), Environment.NewLine, PropGroupValue(propertyname + "." + _module.Configuration.System, null)).TrimWhiteSpace();

            if (String.IsNullOrEmpty(value) && (defaultVals != null && defaultVals.Length > 0))
            {
                value = StringUtil.ArrayToString(defaultVals, Environment.NewLine);
            }

            return value.TrimWhiteSpace();
        }

        protected string GetModuleSinglePropertyValue(string propertyname, string defaultVal = "")
        {
            if (propertyname == null)
            {
                return defaultVal;
            }

            string value = PropGroupValue(propertyname + "." + _module.Configuration.System, null) ?? PropGroupValue(propertyname, null);

            return value ?? defaultVal;
        }


        protected bool GetModuleBooleanPropertyValue(string propertyname, bool defaultVal = false)
        {
            if (propertyname == null)
            {
                return defaultVal;
            }

            string value = PropGroupValue(propertyname + "." + _module.Configuration.System, null) ?? PropGroupValue(propertyname, null);

            return value.ToBooleanOrDefault(defaultVal);
        }


        protected FileSet AddModuleFiles(Module module, string propertyname, params string[] defaultfilters)
        {
            FileSet dest = new FileSet();
            if (0 < AddModuleFiles(dest, module, propertyname, defaultfilters))
            {
                return dest;
            }
            return null;
        }

        internal int AddModuleFiles(FileSet dest, Module module, string propertyname, params string[] defaultfilters)
        {
            int appended = 0;
            if (dest != null)
            {
#if FRAMEWORK_PARALLEL_TRANSITION
                // $$$ NOTE:  We need the behaviour to match what is done in SetupBuildFilesets() in ComputeBaseBuildData.cs in the old eaconfig
                if (propertyname == ".sourcefiles")
                {
                    appended += dest.AppendWithBaseDir(PropGroupFileSet(propertyname));
                    appended += dest.AppendWithBaseDir(PropGroupFileSet(propertyname + "." + module.Configuration.System));
                }
                else
                {
                    var srcfs = PropGroupFileSet(propertyname);
                    dest.Append(srcfs);
                    appended += srcfs != null ? 1 : 0;
                    srcfs = PropGroupFileSet(propertyname + "." + module.Configuration.System);
                    dest.Append(srcfs);
                    appended += srcfs != null ? 1 : 0;
                }
#else
                appended += dest.AppendWithBaseDir(PropGroupFileSet(propertyname), copyNoFailOnMissing:true);
                appended += dest.AppendWithBaseDir(PropGroupFileSet(propertyname + "." + module.Configuration.System), copyNoFailOnMissing: true);
#endif
                if (appended == 0 && defaultfilters != null)
                {
                    foreach (string filter in defaultfilters)
                    {
                        dest.Include(Project.ExpandProperties(filter));
                        appended++;
                    }
                }
            }
            return appended;
        }


        private OptionSet GetModuleBuildOptionSet(string buildtypename)
        {
            OptionSet optionset;
            if (Project.Properties.GetBooleanProperty(PropGroupName("use.pch")))
            {
                var usepchname = "__UsePch__" + buildtypename;
                optionset = Project.NamedOptionSets.GetOrAdd(usepchname, oname =>
                    {
                        OptionSet os = new OptionSet();
                        os.Options["use-pch"] = "on";
                        return EA.Eaconfig.Structured.BuildTypeTask.ExecuteTask(Project, oname, buildtypename, os, saveFinalToproject: false);
                    });
            }
            else
            {
                optionset = OptionSetUtil.GetOptionSetOrFail(Project, buildtypename);
            }
            return optionset;
        }

        protected string GetOptionString(string name, OptionDictionary options, string extra_propertyname = null, bool useOptionProperty = true, params string[] defaultVals)
        {
            string val;
            if (options.TryGetValue(name, out val))
            {
                if (Log.Level >= Log.LogLevel.Diagnostic && !String.IsNullOrEmpty(val))
                {
                    Log.Debug.WriteLine(LogPrefix + "{0}: set option '{1}' from optionset '{2}': {3}", _module.ModuleIdentityString(), name, BuildType.Name, StringUtil.EnsureSeparator(val, " "));
                }
            }
            else if (useOptionProperty)
            {
                val = Project.Properties[name];

                if (Log.Level >= Log.LogLevel.Diagnostic && !String.IsNullOrEmpty(val))
                {
                    Log.Debug.WriteLine(LogPrefix + "{0} set option '{1}' from property '{2}': {3}", _module.ModuleIdentityString(), name, name, StringUtil.EnsureSeparator(val, " "));
                }
            }

            // Append extra properties:
            string extra = GetModulePropertyValue(extra_propertyname, defaultVals);
            if (!string.IsNullOrEmpty(extra))
            {
                val += Environment.NewLine + extra;
            }

            if(!String.IsNullOrEmpty(val))
            {
                val = Project.ExpandProperties(val);
            }
            return val;
        }


        protected OptionSet GetFileBuildOptionSet(FileItem fileitem, out bool isDerivedFromBuildType)
        {
            isDerivedFromBuildType = false;
            OptionSet optionset = null;
            OptionSet fileOptions;

            var optionsetname = fileitem.OptionSetName;

            if (optionsetname == "Program")
            {
                optionsetname = "StdProgram";
            }
            else if (optionsetname == "Library")
            {
                optionsetname = "StdLibrary";
            }

            if (Project.NamedOptionSets.TryGetValue(optionsetname, out fileOptions))
            {
                if (fileOptions.Options.Count == 1)
                {
                    var prefix = String.Empty;
                    var pchoption = String.Empty;

                    fileitem.OptionSetName = null;

                    if (fileOptions.GetBooleanOption("create-pch"))
                    {
                        prefix = "__CreatePch__";
                        pchoption = "create-pch";
                    }
                    else if (fileOptions.GetBooleanOption("use-pch"))
                    {
                        prefix = "__UsePch__";
                        pchoption = "use-pch";
                    }

                    if (!String.IsNullOrEmpty(prefix))
                    {
                        var pchname = prefix + _module.BuildType.Name;

                        optionset = Project.NamedOptionSets.GetOrAdd(pchname, oname =>
                        {
                            OptionSet os = new OptionSet();
                            os.Options[pchoption] = "on";
                            return EA.Eaconfig.Structured.BuildTypeTask.ExecuteTask(Project, oname, _module.BuildType.Name, os, saveFinalToproject: false);

                        });

                        isDerivedFromBuildType = true;
                        fileitem.OptionSetName = pchname;
                    }
                    else
                    { 
                        var entry = fileOptions.Options.FirstOrDefault();
                        if (!(entry.Key == "use-pch" || entry.Key == "create-pch"))
                        {
                            Error.Throw(Project, "GetFileBuildOptionSet", "File '{0}' specific optionset '{1} with single option contains invalid option '{2}={3}'; valid options for a file set with one option are  'create-pch' or 'use-pch'", fileitem.Path.Path, fileitem.OptionSetName, entry.Key, entry.Value);
                        }
                    }
                    
                }
                else if (fileOptions.Options.Count > 1)
                {
                    optionset = fileOptions;
                }

                // If optionset is empty - ignore it;
            }
            else
            {
                Error.Throw(Project, "CreateCcTool", "File '{0} optionset '{1}' does not exist", fileitem.Path, fileitem.OptionSetName);
            }

            return optionset;
        }
    }

    internal class DependencyDefinition
    {
        internal DependencyDefinition(string packagename, string configuration)
        {
            PackageName = packagename;
            ConfigurationName = configuration;
            Key = CreateKey(packagename, configuration);
        }
        internal static string CreateKey(string packagename, string configuration)
        {
            return packagename + ":" + configuration;
        }
        internal readonly string Key;
        internal readonly string PackageName;
        internal readonly string ConfigurationName;

        internal ISet<Tuple<BuildGroups, string>> ModuleDependencies
        {
            get
            {
                return _moduledependencies;
            }
        }

        internal bool AddModuleDependent(BuildGroups group, string moduleNmae)
        {
            if (_moduledependencies == null)
            {
                _moduledependencies = new HashSet<Tuple<BuildGroups, string>>(); ;
            }
            return _moduledependencies.Add(Tuple.Create<BuildGroups, string>(group, moduleNmae));
        }
        internal HashSet<Tuple<BuildGroups, string>> _moduledependencies;
    }

}
