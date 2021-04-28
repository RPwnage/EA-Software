using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Attributes;
using NAnt.Core.Tasks;
using NAnt.Core.Logging;
using EA.PackageSystem.PackageCore;
using NAnt.DotNetTasks;
using NAnt.Shared.Properties;

using System;
using System.Collections;
using System.Collections.Generic;
using System.Text;
using System.IO;

// Loader for platform specific configurations.
namespace EA.Eaconfig
{
    [TaskName("LoadPlatformConfig")]
    public class LoadPlatformConfigTask : Task
    {
        public LoadPlatformConfigTask(Project project)
            : base()
        {
            Project = project;
            InternalInit();
        }

        public LoadPlatformConfigTask()
            : base()
        {
        }

        protected override void InitializeTask(System.Xml.XmlNode taskNode)
        {
            base.InitializeTask(taskNode);
            InternalInit();
        }

        private void InternalInit()
        {
            _config = Properties["config"];
        }

        [Property("include", Required = false)]
        public PropertyElement Include
        {
            get { return _include; }
        }private PropertyElement _include = new PropertyElement();


        protected override void ExecuteTask()
        {
            PlatformConfigPackage = PlatformName + PackageProperties.PlatformConfigPackagePostfix;
            PlatformConfigPackageVersion = PackageMap.Instance.GetMasterVersion(PlatformConfigPackage, Project);

            bool hasPlatformConfigPackage = PlatformConfigPackageVersion != null && (false == PlatformConfigPackageVersion.Equals("ignore", StringComparison.InvariantCultureIgnoreCase));


            if (!hasPlatformConfigPackage)
            {
                // Load traditional config file.
                
                ConfigFile = PathNormalizer.Normalize(Path.Combine(PackageMap.Instance.ConfigDir, Config + ".xml"), false);
                if(!File.Exists(ConfigFile))
                {
                    Error.Throw(Project, Location, Name, "Can not load configuration '{0}': neither platform configuration package '{1}' was declared in masterconfig nor configuration file '{2}' exists.", Config, PlatformConfigPackage, ConfigFile);
                }
            }
            else // Try to load config package
            {
                TaskUtil.Dependent(Project, PlatformConfigPackage, Project.TargetStyleType.Use, false);

                PlatformConfigPackageDir = Project.Properties["package." + PlatformConfigPackage + ".dir"];

                Project.Properties["config-platform-load-name"] = PlatformName;

                ConfigFile = PathNormalizer.Normalize(PlatformConfigPackageDir + "/config/" + Config + ".xml", false);
            }

            IncludeFile(ConfigFile);

            if (hasPlatformConfigPackage)
            {
                string configSetFile = SetPlatformDefinitionProperties();

                //Load initialize.xml
                TaskUtil.Dependent(Project, PlatformConfigPackage, Project.TargetStyleType.Use, true);

                Project.Properties["config-configset-file"] = configSetFile;

                foreach (string line in StringUtil.ToArray(Include.Value, "\n\r"))
                {
                    if (!String.IsNullOrEmpty(line))
                    {
                        string file = StringUtil.TrimQuotes(StringUtil.Trim(Project.ExpandProperties(line)));
                        IncludeFile(file);
                    }

                }
            }
        }

        protected void IncludeFile(string file)
        {
            // use the <include> task to include the "config" script
            IncludeTask includeTask = new IncludeTask();
            includeTask.Project = Project;
            includeTask.Verbose = Verbose;
            includeTask.FileName = file;
            includeTask.Execute();
        }

        protected string SetPlatformDefinitionProperties()
        {
            OptionSet platformDef = OptionSetUtil.GetOptionSet(Project, "configuration-definition");
            if (platformDef == null)
            {
                Error.Throw(Project, Location, Name, "Configuration file '{0}' in package '{1}' does nor define required optionset '{2}'", Path.GetFileName(ConfigFile), PlatformConfigPackage, CONFIGURATION_DEFINITION_OPTIONSET_NAME);
            }

            int filenameind = 0;
            string prefix = String.Empty;

            foreach (DictionaryEntry op in platformDef.Options)
            {
                string name = op.Key as string;
                string val = op.Value as string;

                switch (name)
                {
                    case "config-name":
                    case "config-system":
                    case "config-compiler":
                    case "config-platform":
                    case "config-processor":
                        {
                            Project.Properties[name] = val;
                        }
                        break;
                    case "debug":
                        if (val == "on") filenameind += 1;
                        break;
                    case "optimization":
                        if (val == "on") filenameind += 2;
                        break;
                    case "profile":
                        if (val == "on") filenameind += 4;
                        break;
                    case "dll":
                        if (val == "on") prefix = "dll-";
                        break;

                    default:
                        {
                            string msg = Error.Format(Project, Name, "WARNING", "Configuration set '{0}' in file '{1}' contains unknown option '{2}={3}'", CONFIGURATION_DEFINITION_OPTIONSET_NAME, Path.GetFileName(ConfigFile), name, val);
                            Log.WriteLine(msg);
                        }
                        break;
                }
            }

            if (filenameind > 4)
            {
                Error.Throw(Project, Location, Name, "Invalid combination of config options: '{0}' in '{1}'", fileNames[filenameind], Path.GetFileName(ConfigFile));
            }
            else if (filenameind < 1)
            {
                Error.Throw(Project, Location, Name, "None of the configuration options 'debug, opt, profile' is defined in'{0}'.", Path.GetFileName(ConfigFile));
            }
            string definitionFile = prefix + "dev-" + fileNames[filenameind];

            return definitionFile;
        }

        protected string PlatformName
        {
            get
            {
                if (_platformName == null)
                {
                    _platformName = Config.Substring(0, Config.IndexOf("-"));

                    string mappedName;
                    if (platformNameMap.TryGetValue(_platformName, out mappedName))
                    {
                        _platformName = mappedName;
                    }
                }
                return _platformName;
            }
        } private string _platformName;

        protected string Config
        {
            get { return _config; }
        } private string _config;

        private string ConfigFile;
        private string PlatformConfigPackage;
        private string PlatformConfigPackageVersion;
        private string PlatformConfigPackageDir;

        private readonly IDictionary<string, string> platformNameMap = new Dictionary<string, string>()
        {
            {"iphone", "ios"},
            {"pc64", "pc"},
            {"unix64", "unix"},
            {"badasim", "bada"},
            {"iphonesim", "iphone"},
            {"palmpixi", "palm"},
            {"bb10", "rimqnx"}
        };

        public const string CONFIGURATION_DEFINITION_OPTIONSET_NAME = "configuration-definition";

        public const string CONFIG_PACKAGE_BUILD_CONFIG = "any-platform";

        public static readonly string[] fileNames = new string[] {"debug", "debug", "opt", "debug-opt", "profile", "debug-profile", "opt-profile", "debug-opt-profile" };


    }
}
