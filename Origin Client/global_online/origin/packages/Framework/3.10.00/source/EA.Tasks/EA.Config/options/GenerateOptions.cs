using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Logging;

using System;
using System.Collections;
using System.Collections.Generic;
using System.Text;
using System.IO;

namespace EA.Eaconfig
{
    public enum WarningSuppressionModes { On, Off, Strict, Custom };

    [TaskName("GenerateOptions")]
    public class GenerateOptions : EAConfigTask
    {
        [TaskAttribute("configsetname", Required = true)]
        public string ConfigSetName
        {
            get { return _configsetName; }
            set { _configsetName = value; }
        }

        public OptionSet ConfigOptionSet
        {
            get
            {
                if (_ConfigOptionSet == null)
                {
                    _ConfigOptionSet = Project.NamedOptionSets[ConfigSetName];
                    if (_ConfigOptionSet == null)
                    {
                        Error.Throw(Project, Location, "GenerateOptions", "OptionSet '{0}' does not exist.", ConfigSetName);
                    }
                }
                return _ConfigOptionSet;
            }

            set { _ConfigOptionSet = value; }
        }

        public static void Execute(Project project, OptionSet configOptionSet, string configsetName)
        {
            GenerateOptions task = null;

            string config_platform = configOptionSet.GetOptionOrDefault("config-platform", project.Properties["config-platform"]);

            if (!string.IsNullOrEmpty(config_platform))
            {
                task = LoadPlatform_GenerateOptionsTask(project, config_platform + "-generate-options", configOptionSet, configsetName);
            }

            // For backwards compatibility check system prefix:
            if (task == null)
            {
                string config_system = configOptionSet.GetOptionOrDefault("config-system", project.Properties["config-system"]);

                if (!string.IsNullOrEmpty(config_system))
                {
                    task = LoadPlatform_GenerateOptionsTask(project, "GenerateOptions_"+config_system, configOptionSet, configsetName);
                }
            }

            if (task == null)
            {
                task = new GenerateOptions(project, configOptionSet, configsetName);
            }
            task.Execute();
        }

        private static GenerateOptions LoadPlatform_GenerateOptionsTask(Project project, string taskname, OptionSet configOptionSet, string configsetName)
        {
            GenerateOptions task = null;

            if (!string.IsNullOrEmpty(taskname))
            {
                GeneratePlatformOptions platformTask = project.TaskFactory.CreateTask(taskname, project) as GeneratePlatformOptions;
                if (platformTask != null)
                {
                    platformTask.Init(project, configOptionSet, configsetName);
                }
                task = platformTask as GenerateOptions;
            }

            return task;
        }

        protected GenerateOptions() : base() { }

        public GenerateOptions(Project project, OptionSet configOptionSet, string configsetName)
            : base(project)
        {
            _ConfigOptionSet = configOptionSet;
            _configsetName = configsetName;
            InternalInit();
        }

        protected override void InitializeTask(System.Xml.XmlNode taskNode)
        {
            base.InitializeTask(taskNode);
            InternalInit();
        }

        protected override void ExecuteTask()
        {
            // Init Builders
            foreach (OptionType type in Enum.GetValues(typeof(OptionType)))
            {
                string option = ConfigOptionSet.Options[_optionNames[(int)type]];
                if (!String.IsNullOrEmpty(option))
                {
                    _optionBuilder[(int)type].AppendLine(option);
                }
            }

            // --- Common section ---

            AddOptionIf("debugflags", "on", OptionType.cc_Defines, "EA_DEBUG");
            AddOptionIf("debugflags", "on", OptionType.cc_Defines, "_DEBUG");  //First added for Wii.  Needed for VC++ too, as per Paul Pedriana.
            AddOptionIf("debugflags", "off", OptionType.cc_Defines, "NDEBUG");
            AddOptionIf("debugflags", "custom", OptionType.cc_Defines, ConfigOptionSet.Options["debugflags.custom.cc"]);

            AddOptionIf("debugsymbols", "custom", OptionType.cc_Options, ConfigOptionSet.Options["debugsymbols.custom.cc"]);
            AddOptionIf("debugsymbols", "custom", OptionType.link_Options, ConfigOptionSet.Options["debugsymbols.custom.link"]);
            AddOptionIf("debugsymbols", "custom", OptionType.lib_Options, ConfigOptionSet.Options["debugsymbols.custom.lib"]);

            AddOptionIf("optimization", "custom", OptionType.cc_Options, ConfigOptionSet.Options["optimization.custom.cc"]);
            AddOptionIf("optimization", "custom", OptionType.link_Options, ConfigOptionSet.Options["optimization.custom.link"]);
            AddOptionIf("optimization", "custom", OptionType.lib_Options, ConfigOptionSet.Options["optimization.custom.lib"]);

            // ----- GENERAL OPTIONS -----
            AddOptionIf("misc", "custom", OptionType.cc_Options, ConfigOptionSet.Options["misc.custom.cc"]);

            // ----- LINK OPTIONS -----
            AddOptionIf("usedebuglibs", "custom", OptionType.cc_Options, ConfigOptionSet.Options["usedebuglibs.custom.cc"]);

            // ----- WARNING OPTIONS -----
            AddOptionIf("warnings", "custom", OptionType.cc_Options, ConfigOptionSet.Options["warnings.custom.cc"]);
            AddOptionIf("warnings", "custom", OptionType.link_Options, ConfigOptionSet.Options["warnings.custom.link"]);

            if (FlagEquals("standardsdklibs", "on"))
            {
                AddOptionIf("usedebuglibs", "off", OptionType.link_Libraries, Properties["platform.sdklibs.regular"]);
                AddOptionIf("usedebuglibs", "on", OptionType.link_Libraries, Properties["platform.sdklibs.debug"]);
            }
            else if (FlagEquals("standardsdklibs", "custom"))
            {
                AddOption(OptionType.link_Libraries, ConfigOptionSet.Options["standardsdklibs.custom.link"]);
            }

            GeneratePlatformSpecificOptions();


            // Commit new values
            foreach (OptionType type in Enum.GetValues(typeof(OptionType)))
            {
                if (_optionBuilder[(int)type].Length > 0)
                {
                    ConfigOptionSet.Options[_optionNames[(int)type]] = _optionBuilder[(int)type].ToString();
                }
            }

            // Wireframe uses internal property build-compiler:
            Project.Properties["build-compiler"] = ConfigCompiler;
        }


        protected virtual void GeneratePlatformSpecificOptions()
        {
            switch (ConfigCompiler)
            {
                case "sn":
                    Options_SN();
                    break;
                case "gcc":
                    Options_GCC();
                    break;
                case "mw":
                    Options_MW();
                    break;
                default:
                    Error.Throw(Project, Location, Name, "GenerateOptions task for '{0}' platform not found", ConfigPlatform);
                    break;
            }
        }

        protected virtual void Common_GCC_Options()
        {
            if (FlagEquals("clanguage", "off"))
            {
                AddOptionIf("rtti", "off", OptionType.cc_Options, "-fno-rtti"); //Do not generate run time type information
                AddOptionIf("exceptions", "off", OptionType.cc_Options, "-fno-exceptions"); //Do not generate exception handling code
            }

            // ----- WARNING OPTIONS -----
            AddOptionIf("warningsaserrors", "on", OptionType.cc_Options, "-Werror"); // Treat warnings as errors
            AddOptionIf("warningsaserrors", "off", OptionType.cc_Options, "-Wno-error"); // Disable warnings as errors

            PrependOptionIf("warnings", "off", OptionType.cc_Options, "-w"); // Disable warnings

            if (DefaultWarningSuppression == WarningSuppressionModes.On)
            {
                PrependOptionIf("warnings", "on", OptionType.cc_Options, "-Wno-deprecated-declarations"); // Disable warnings about deprecated declarations
                AddOptionIf("warnings", "on", OptionType.cc_Options, PropertyUtil.GetPropertyOrDefault(Project, "eaconfig." + ConfigPlatform + ".warningsuppression", ""));
            }
            else if (DefaultWarningSuppression == WarningSuppressionModes.Custom)
            {
                AddOptionIf("warnings", "on", OptionType.cc_Options, PropertyUtil.GetPropertyOrDefault(Project, "eaconfig." + ConfigPlatform + ".warningsuppression", ""));
            }
            else if (DefaultWarningSuppression == WarningSuppressionModes.Strict)
            {
                if (FlagEquals("optimization", "on"))
                {
                    PrependOptionIf("warnings", "on", OptionType.cc_Options, "-Winline");
                }
                PrependOptionIf("warnings", "on", OptionType.cc_Options, "-Wextra");
            }
            PrependOptionIf("warnings", "on", OptionType.cc_Options, "-Wall"); // Enable all warnings
        }

        protected virtual void Common_SN_Options()
        {
            AddOptionIf("optimization", "off", OptionType.cc_Options, "-O0");

            // ----- OUTPUT OPTIONS -----
            AddOption(OptionType.link_Options, "-o \"%linkoutputname%\"");
            AddOption(OptionType.lib_Options, "-rs \"%liboutputname%\"");

            // ----- WARNING OPTIONS -----
            if (ConfigSystem != "ps3")
            {
                AddOptionIf("warningsaserrors", "on", OptionType.cc_Options, "-Werror"); // Treat warnings as errors
            }
        }
        private void Options_SN()
        {
            Common_SN_Options();

            switch (ConfigSystem)
            {

                case "ps2":
                    AddOptionIf("debugsymbols", "on", OptionType.cc_Options, "-g2"); // Generate debug information
                    AddOptionIf("debugsymbols", "on", OptionType.link_Options, "-g");
                    if(FlagEquals("clanguage", "off"))
                    {
                        AddOptionIf("rtti", "off", OptionType.cc_Options, "-fno-rtti");
                    }

                    AddOptionIf("optimization", "on", OptionType.cc_Options, "-O2");                    // Set optimization level 2
                    AddOptionIf("optimization", "on", OptionType.cc_Options, "-ffast-math");            // Improve FP speed by violating ANSI & IEEE rules
                    AddOptionIf("optimization", "on", OptionType.cc_Options, "-fomit-frame-pointer");   // Omit frame pointer when possible

                    // ----- GENERAL OPTIONS -----
                    if (FlagEquals("misc", "on"))
                    {
                        AddOption(OptionType.cc_Options, "-fno-common");                // In C, allocate even uninitialized global variables in data section
                                                                                        //   rather than as common blocks.  If a variable is declared in two different
                                                                                        //   compilations, it will cause a link error
                        AddOption(OptionType.cc_Options, "-fshort-wchar");              // Override the underlying type for wchar_t to `unsigned short
                        AddOption(OptionType.cc_Options, "-fno-keep-static-consts");    // Dont' emit static const variables even if they are not used
                        AddOption(OptionType.cc_Options, "-G0");                        // Put global and static data smaller than
                        AddOption(OptionType.cc_Options, "-mvu0-use-vf0-vf31");         // Specify the range of vu0 registers to use
                        AddOption(OptionType.cc_Options, "-ftemplate-depth-40");        // Allow more heavily templated code such as STL / EASTL
                        AddOption(OptionType.cc_Options, "-fshort-wchar");              // Override the underlying type for wchar_t to `unsigned short
                    }

                    // ----- OUTPUT OPTIONS -----
                    AddOption(OptionType.cc_Options, "-fdevstudio");            // Issue errors and warnings in MS dev studio format.

                    if (Properties["cc"].EndsWith("ps2cc.exe", StringComparison.Ordinal))
                    {
                        AddOptionIf("generatemapfile", "on", OptionType.link_Options, "-Map \"%linkoutputmapname%\"");
                    }
                    else
                    {
                        AddOptionIf("generatemapfile", "on", OptionType.link_Options, "-Wl,-Map,\"%linkoutputmapname%\"");
                    }

                    // ----- WARNING OPTIONS -----

                    if (DefaultWarningSuppression == WarningSuppressionModes.On)
                    {
                        PrependOptionIf("warnings", "on", OptionType.cc_Options, "-Wno-missing-braces");        // Don't warn about possibly missing braces around initialisers
                        PrependOptionIf("warnings", "on", OptionType.cc_Options, "-Wno-non-template-friend");
                        PrependOptionIf("warnings", "on", OptionType.cc_Options, "-Wno-ctor-dtor-privacy");     //Don't warn when all ctors/dtors are private
                        AddOptionIf("warnings", "on", OptionType.cc_Options, PropertyUtil.GetPropertyOrDefault(Project, "eaconfig.ps2-sn.warningsuppression", ""));
                    }
                    else if (DefaultWarningSuppression == WarningSuppressionModes.Custom)
                    {
                        AddOptionIf("warnings", "on", OptionType.cc_Options, PropertyUtil.GetPropertyOrDefault(Project, "eaconfig.ps2-sn.warningsuppression", ""));
                    }
                    else if (DefaultWarningSuppression == WarningSuppressionModes.Strict)
                    {
                    }
                    PrependOptionIf("warnings", "on", OptionType.cc_Options, "-Wall");
                    break;

                case "psp":
                    AddOptionIf("debugsymbols", "on", OptionType.cc_Options, "-g"); // Generate debug information - sn compiler doesn't take -g2
                    AddOptionIf("debugsymbols", "on", OptionType.link_Options, "-g");
                    if(FlagEquals("clanguage", "off"))
                    {
                        AddOptionIf("rtti", "off", OptionType.cc_Options, "-fno-rtti");
                        AddOptionIf("exceptions", "off", OptionType.cc_Options, "-fno-exceptions"); //Do not generate exception handling code
                    }

                    AddOptionIf("optimization", "on", OptionType.cc_Options, "-O3");  // Set optimization level 3

                    if (DefaultWarningSuppression == WarningSuppressionModes.On)
                    {
                        AddOptionIf("warnings", "on", OptionType.cc_Options, PropertyUtil.GetPropertyOrDefault(Project, "eaconfig.psp-sn.warningsuppression", ""));
                    }
                    else if (DefaultWarningSuppression == WarningSuppressionModes.Custom)
                    {
                        AddOptionIf("warnings", "on", OptionType.cc_Options, PropertyUtil.GetPropertyOrDefault(Project, "eaconfig.psp-sn.warningsuppression", ""));
                    }

                    // ----- GENERAL OPTIONS -----
                    AddOptionIf("misc", "on", OptionType.cc_Options, "-G0"); // Put global and static data smaller than

                    // ----- OUTPUT OPTIONS -----
                    AddOption(OptionType.cc_Options, "-fdevstudio");            // Issue errors and warnings in MS dev studio format.
                    AddOptionIf("generatemapfile", "on", OptionType.link_Options, "-Wl,-Map \"%linkoutputmapname%\"");
                    break;

                case "iop":

                    AddOptionIf("debugsymbols", "on", OptionType.link_Options, "-g");

                    // ----- GENERAL OPTIONS -----
                    if (FlagEquals("misc", "on"))
                    {
                        AddOption(OptionType.cc_Options, "-fno-common");        // In C, allocate even uninitialized global variables in data section
                                                                                //   rather than as common blocks.  If a variable is declared in two different
                                                                                //   compilations, it will cause a link error
                        AddOption(OptionType.cc_Options, "-ffast-math");        // Disable math asserts like divide-by-zero
                    }

                    // ----- OUTPUT OPTIONS -----
                    AddOption(OptionType.cc_Options, "-fdevstudio");            // Issue errors and warnings in MS dev studio format.

                    if (Properties["cc"].EndsWith("iop-elf-gcc.exe"))
                    {
                        AddOptionIf("generatemapfile", "on", OptionType.link_Options, "-Map \"%linkoutputmapname%\"");
                    }
                    else
                    {
                        AddOptionIf("generatemapfile", "on", OptionType.link_Options, "-Wl,-Map \"%linkoutputmapname%\"");
                    }

                    // ----- WARNING OPTIONS -----
                    if (DefaultWarningSuppression == WarningSuppressionModes.On )
                    {
                        PrependOptionIf("warnings", "on", OptionType.cc_Options, "-Wformat");                   // Check calls to printf and scanf, etc., to make sure  that the arguments supplied have types appropriate to the format string, etc.
                        PrependOptionIf("warnings", "on", OptionType.cc_Options, "-Wimplicit-function-dec");    // This warns if there is a non-static function that doesn't have a prototype when it hits the function instantiation
                        PrependOptionIf("warnings", "on", OptionType.cc_Options, "-Wimplicit-int");             //This warns if a variable is declared without a type, in which case it defaults to type of int.
                        AddOptionIf("warnings", "on", OptionType.cc_Options, PropertyUtil.GetPropertyOrDefault(Project, "eaconfig.iop-sn.warningsuppression", ""));
                    }
                    else if (DefaultWarningSuppression == WarningSuppressionModes.Custom)
                    {
                        AddOptionIf("warnings", "on", OptionType.cc_Options, PropertyUtil.GetPropertyOrDefault(Project, "eaconfig.iop-sn.warningsuppression", ""));
                    }

                    PrependOptionIf("warnings", "on", OptionType.cc_Options, "-Wall");

                    break;
            }
        }

        private void Options_GCC()
        {
            Common_GCC_Options();

            switch (ConfigSystem)
            {
                case "cyg":
                    AddOptionIf("optimization", "off", OptionType.cc_Options, "-O0");
                    AddOptionIf("enable.strict.aliasing", "on", OptionType.cc_Options, "-fstrict-aliasing");     // Enable strict aliasing
                    AddOptionIf("enable.strict.aliasing", "off", OptionType.cc_Options, "-fno-strict-aliasing");

                    //EA_PLATFORM_OSX=1  It turns out that Mac OS X is a better match for what cygwin does than Linux. So using OSX might be useful instead of LINUX
                    AddOption(OptionType.cc_Defines, "EA_PLATFORM_LINUX=1"); //Cygwin emulates generic Unix and not Linux (which typically has extended functionality), though we often use Cygwin to emulate Linux.
                    AddOption(OptionType.cc_Defines, "NOMINMAX"); //If you are going to possibly see Microsoft headers, it's a good idea to define this.
                    // EA_PLATFORM_WINDOWS, EA_PLATFORM_WIN32, _WIN32 here? In practice doing so tends to cause problems. Perhaps there could be an EA_PLATFORM_CYGWIN.

                    // ----- GENERAL OPTIONS -----
                    if (FlagEquals("misc", "on"))
                    {
                        AddOption(OptionType.cc_Options, "-c");             //Compile only
                        AddOption(OptionType.cc_Options, "-fno-common");    // In C, allocate even uninitialized global variables in data section
                        // rather than as common blocks.  If a variable is declared in two different
                        // compilations, it will cause a link error
                        AddOption(OptionType.cc_Options, "-fshort-wchar");  // Override the underlying type for wchar_t to `unsigned short
                        AddOption(OptionType.cc_Options, "-nostdinc");      // Exclude standard include directories
                    }

                    // ----- LINK OPTIONS -----
                    AddOptionIf("exceptions", "on", OptionType.link_Librarydirs, Properties["link.exceptionlibrarydirs"]);
                    AddOptionIf("exceptions", "off", OptionType.link_Options, "-fno-exceptions");
                    // ----- OUTPUT OPTIONS -----
                    AddOptionIf("generatedll", "on", OptionType.link_Options, "-shared"); //Make a DLL, not an exe.
                    AddOption(OptionType.link_Options, "-o \"%linkoutputname%\"");
                    AddOption(OptionType.lib_Options, "-rs \"%liboutputname%\"");
                    AddOptionIf("generatemapfile", "on", OptionType.link_Options, "-Wl,-Map,\"%linkoutputmapname%\"");

                    break;
                case "bada":
                    AddOptionIf("optimization", "off", OptionType.cc_Options, "-O0");
                    AddOptionIf("enable.strict.aliasing", "on", OptionType.cc_Options, "-fstrict-aliasing");     // Enable strict aliasing
                    AddOptionIf("enable.strict.aliasing", "off", OptionType.cc_Options, "-fno-strict-aliasing");

                    AddOptionIf("debugsymbols", "on", OptionType.cc_Options, "-g3"); // Generate debug information

                    //AddOption(OptionType.cc_Options, Project.Properties["package.BadaSDK.cc.options"]);
                    //AddOption(OptionType.link_Options, Project.Properties["package.BadaSDK.link.options"]);
                    if (!FlagEquals("generatedll", "on"))
                    {
                        AddOptionIf("optimization", "on", OptionType.link_Options, "-Wl,--gc-sections"); //strip out unused functions in the output binary and make it smaller
                    }

                    AddOption(OptionType.cc_Options, "-MMD"); //Dump a .d file
                    if (FlagEquals("misc", "on"))
                    {
                        AddOption(OptionType.cc_Options, "-c");             //Compile only
                        AddOption(OptionType.cc_Options, "-fno-common");    // In C, allocate even uninitialized global variables in data section
                        // rather than as common blocks.  If a variable is declared in two different
                        // compilations, it will cause a link error
                        AddOption(OptionType.cc_Options, "-fshort-wchar");  // Override the underlying type for wchar_t to `unsigned short
                        AddOption(OptionType.cc_Options, "-fmessage-length=0"); // Put the output entirely on one line; don't add newlines.
                    }

                    // ----- OUTPUT OPTIONS -----
                    AddOptionIf("generatedll", "on", OptionType.link_Options, "-shared"); //Make a DLL, not an exe.
                    AddOption(OptionType.link_Options, "-o \"%linkoutputname%\"");
                    AddOptionIf("generatemapfile", "on", OptionType.link_Options, "-Wl,-Map,\"%linkoutputmapname%\"");

                    // ----- LINK OPTIONS -----
                    AddOptionIf("exceptions", "on", OptionType.link_Librarydirs, Properties["link.exceptionlibrarydirs"]);
                    AddOptionIf("exceptions", "off", OptionType.link_Options, "-fno-exceptions");

                    break;
                case "android":
                    AddOptionIf("optimization", "off", OptionType.cc_Options, "-O0");
                    AddOptionIf("enable.strict.aliasing", "on", OptionType.cc_Options, "-fstrict-aliasing");     // Enable strict aliasing
                    AddOptionIf("enable.strict.aliasing", "off", OptionType.cc_Options, "-fno-strict-aliasing");

                    AddOptionIf("debugsymbols", "on", OptionType.cc_Options, "-g3"); // Generate debug information
                    AddOption(OptionType.cc_Options, Project.Properties["package.AndroidSDK.cc.options"]);
                    AddOption(OptionType.link_Options, Project.Properties["package.AndroidSDK.link.options"]);
                    if (!FlagEquals("generatedll", "on"))
                    {
                        AddOptionIf("optimization", "on", OptionType.link_Options, "-Wl,--gc-sections"); //strip out unused functions in the output binary and make it smaller
                    }

                    AddOption(OptionType.cc_Options, "-MMD"); //Dump a .d file
                    if (FlagEquals("misc", "on"))
                    {
                        AddOption(OptionType.cc_Options, "-c");             //Compile only
                        AddOption(OptionType.cc_Options, "-fno-common");    // In C, allocate even uninitialized global variables in data section
                        // rather than as common blocks.  If a variable is declared in two different
                        // compilations, it will cause a link error
                        AddOption(OptionType.cc_Options, "-fshort-wchar");  // Override the underlying type for wchar_t to `unsigned short
                        AddOption(OptionType.cc_Options, "-fmessage-length=0"); // Put the output entirely on one line; don't add newlines.
                    }

                    // ----- OUTPUT OPTIONS -----
                    AddOptionIf("generatedll", "on", OptionType.link_Options, "-shared"); //Make a DLL, not an exe.
                    AddOption(OptionType.link_Options, "-o \"%linkoutputname%\"");
                    AddOptionIf("generatemapfile", "on", OptionType.link_Options, "-Wl,-Map,\"%linkoutputmapname%\"");
                    // ----- LINK OPTIONS -----
                    AddOptionIf("exceptions", "on", OptionType.link_Librarydirs, Properties["link.exceptionlibrarydirs"]);
                    AddOptionIf("exceptions", "off", OptionType.link_Options, "-fno-exceptions");

                    break;
                case "mobilearm":
                    AddOptionIf("optimization", "off", OptionType.cc_Options, "-O0");
                    AddOptionIf("enable.strict.aliasing", "on", OptionType.cc_Options, "-fstrict-aliasing");     // Enable strict aliasing
                    AddOptionIf("enable.strict.aliasing", "off", OptionType.cc_Options, "-fno-strict-aliasing");

                    AddOptionIf("debugsymbols", "on", OptionType.cc_Options, "-g3"); // Generate debug information
                    if (FlagEquals("clanguage", "off"))
                    {
                        AddOption(OptionType.cc_Options, Project.Properties["package.mobilearm.cc.options"]);
                    }
                    else
                    {
                        AddOption(OptionType.cc_Options, Project.Properties["package.mobilearm.c.options"]);
                    }
                    AddOption(OptionType.link_Options, Project.Properties["package.mobilearm.link.options"]);
                    if (!FlagEquals("generatedll", "on"))
                    {
                        AddOptionIf("optimization", "on", OptionType.link_Options, "-Wl,--gc-sections"); //strip out unused functions in the output binary and make it smaller
                    }

                    AddOption(OptionType.cc_Options, "-MMD"); //Dump a .d file
                    if (FlagEquals("misc", "on"))
                    {
                        AddOption(OptionType.cc_Options, "-c");             //Compile only
                        AddOption(OptionType.cc_Options, "-fno-common");    // In C, allocate even uninitialized global variables in data section
                        // rather than as common blocks.  If a variable is declared in two different
                        // compilations, it will cause a link error
                        AddOption(OptionType.cc_Options, "-fshort-wchar");  // Override the underlying type for wchar_t to `unsigned short
                        AddOption(OptionType.cc_Options, "-fmessage-length=0"); // Put the output entirely on one line; don't add newlines.
                    }

                    // ----- OUTPUT OPTIONS -----
                    AddOptionIf("generatedll", "on", OptionType.link_Options, "-shared"); //Make a DLL, not an exe.
                    AddOption(OptionType.link_Options, "-o \"%linkoutputname%\"");
                    AddOptionIf("generatemapfile", "on", OptionType.link_Options, "-Wl,-Map,\"%linkoutputmapname%\"");
                    // ----- LINK OPTIONS -----
                    AddOptionIf("exceptions", "on", OptionType.link_Librarydirs, Properties["link.exceptionlibrarydirs"]);
                    AddOptionIf("exceptions", "off", OptionType.link_Options, "-fno-exceptions");

                    break;
                case "palm":
                    AddOptionIf("optimization", "off", OptionType.cc_Options, "-O0");
                    AddOptionIf("enable.strict.aliasing", "on", OptionType.cc_Options, "-fstrict-aliasing");     // Enable strict aliasing
                    AddOptionIf("enable.strict.aliasing", "off", OptionType.cc_Options, "-fno-strict-aliasing");

                    AddOptionIf("debugsymbols", "on", OptionType.cc_Options, "-g3"); // Generate debug information
                    if (FlagEquals("clanguage", "off"))
                    {
                        AddOption(OptionType.cc_Options, Project.Properties["package.palmpdk.cc.options"]);
                    }
                    else
                    {
                        AddOption(OptionType.cc_Options, Project.Properties["package.palmpdk.c.options"]);
                    }
                    AddOption(OptionType.link_Options, Project.Properties["package.palmpdk.link.options"]);
                    if (!FlagEquals("generatedll", "on"))
                    {
                        AddOptionIf("optimization", "on", OptionType.link_Options, "-Wl,--gc-sections"); //strip out unused functions in the output binary and make it smaller
                    }

                    AddOption(OptionType.cc_Options, "-MMD"); //Dump a .d file
                    if (FlagEquals("misc", "on"))
                    {
                        AddOption(OptionType.cc_Options, "-c");             //Compile only
                        AddOption(OptionType.cc_Options, "-fno-common");    // In C, allocate even uninitialized global variables in data section
                        // rather than as common blocks.  If a variable is declared in two different
                        // compilations, it will cause a link error
                        AddOption(OptionType.cc_Options, "-fshort-wchar");  // Override the underlying type for wchar_t to `unsigned short
                        AddOption(OptionType.cc_Options, "-fmessage-length=0"); // Put the output entirely on one line; don't add newlines.
                    }

                    // ----- OUTPUT OPTIONS -----
                    AddOptionIf("generatedll", "on", OptionType.link_Options, "-shared"); //Make a DLL, not an exe.
                    AddOption(OptionType.link_Options, "-o \"%linkoutputname%\"");
                    AddOptionIf("generatemapfile", "on", OptionType.link_Options, "-Wl,-Map,\"%linkoutputmapname%\"");
                    // ----- LINK OPTIONS -----
                    AddOptionIf("exceptions", "on", OptionType.link_Librarydirs, Properties["link.exceptionlibrarydirs"]);
                    AddOptionIf("exceptions", "off", OptionType.link_Options, "-fno-exceptions");

                    break;
                case "iphone":
                    //NOTICE (Rashin):
                    //iPhone compiles with -ftree-vectorize which implicitly enables strict aliasing
                    //Do NOT enable this. Otherwise, we can't disable aliasing warnings using -fno-tree-vectorize
                    //AddOption(OptionType.cc_Options, "-fstrict-aliasing");

                    AddOption(OptionType.cc_Options, Project.Properties["package.iphonesdk.cc.options"]);
                    AddOption(OptionType.link_Options, Project.Properties["package.iphonesdk.link.options"]);

                    // ----- GENERAL OPTIONS -----
                    AddOption(OptionType.cc_Options, "-MMD"); //Dump a .d file

                    if (FlagEquals("misc", "on"))
                    {
                        AddOption(OptionType.cc_Options, "-c");             //Compile only
                        AddOption(OptionType.cc_Options, "-fno-common");    // In C, allocate even uninitialized global variables in data section
                                                                            // rather than as common blocks.  If a variable is declared in two different
                                                                            // compilations, it will cause a link error
                        AddOption(OptionType.cc_Options, "-fshort-wchar");  // Override the underlying type for wchar_t to `unsigned short
                        AddOption(OptionType.cc_Options, "-fmessage-length=0"); // Put the output entirely on one line; don't add newlines.
                    }

                    //-- Optimization --
                    AddOptionIf("optimization", "off", OptionType.cc_Options, "-O0");

                    //(NOTE): -mdynamic-no-pic does not work with dll builds.
                    if (!PropertyUtil.IsPropertyEqual(Project, "package.iphone.nopic", "false"))
                    {
                        AddOptionIf("optimization", "on", OptionType.cc_Options, "-mdynamic-no-pic");  // Generates code that is not position-independent but has position-independent external references
                        //dll builds want position independent addresses for globals/statics
                        AddOptionIf("generatedll", "on", OptionType.link_Options, "-read_only_relocs suppress");
                    }

                    // ----- OUTPUT OPTIONS -----
                    AddOptionIf("generatedll", "on", OptionType.link_Options, "-shared"); //Make a DLL, not an exe.
                    AddOption(OptionType.link_Options, "-o \"%linkoutputname%\"");
                    AddOptionIf("generatemapfile", "on", OptionType.link_Options, "-Wl,-map,\"%linkoutputmapname%\"");

                    AddOption(OptionType.lib_Options, "-o \"%liboutputname%\"");

                    break;

                default:
                    AddOptionIf("optimization", "off", OptionType.cc_Options, "-O0");
                    AddOptionIf("enable.strict.aliasing", "on", OptionType.cc_Options, "-fstrict-aliasing");     // Enable strict aliasing
                    AddOptionIf("enable.strict.aliasing", "off", OptionType.cc_Options, "-fno-strict-aliasing");
                    AddOptionIf("debugsymbols", "on", OptionType.cc_Options, "-g");
                    break;
            }

            if (ConfigSystem != "ps3")
            {
                AddOptionIf("debugsymbols", "on", OptionType.link_Options, "-g");
                AddOptionIf("optimization", "on", OptionType.cc_Options, "-O3"); //Set optimization level 3
                if (ConfigSystem != "iphone")
                {
                    AddOptionIf("optimization", "on", OptionType.cc_Options, "-fomit-frame-pointer"); //Omit frame pointer when possible
                }
            }
        }


        private void Options_MW()
        {
            AddOptionIf("rtti", "off", OptionType.cc_Options, "-RTTI off");
            // ----- OUTPUT OPTIONS -----
            AddOption(OptionType.link_Options, "-o \"%linkoutputname%\"");
            AddOption(OptionType.lib_Options, "-o \"%liboutputname%\"");
            // ----- WARNING OPTIONS -----
            AddOptionIf("warningsaserrors", "on", OptionType.cc_Options, "-W iserror"); // Treat warnings as errors

            AddOptionIf("warnings", "off", OptionType.cc_Options, "-w off");            // Disable warnings
                        // Enable all warnings
            if (DefaultWarningSuppression == WarningSuppressionModes.On)
            {
                AddOptionIf("warnings", "on", OptionType.cc_Options, "-w all");
                AddOptionIf("warnings", "on", OptionType.cc_Options, "-w noextracomma");    // Comma after the last item in an enum
                AddOptionIf("warnings", "on", OptionType.cc_Options, "-w nonotinlined");
                AddOptionIf("warnings", "on", OptionType.cc_Options, "-w nofilecaps");      // Case insensitive includes
                AddOptionIf("warnings", "on", OptionType.cc_Options, "-w nosysfilecaps");      // Case insensitive includes

                if (ConfigSystem == "rev")
                {
                    // We kept these for backward compatibility.
                    AddOption(OptionType.cc_Options, PropertyUtil.GetPropertyOrDefault(Project, "package.RevolutionSDK.warningsuppression", ""));
                    AddOption(OptionType.cc_Options, PropertyUtil.GetPropertyOrDefault(Project, "package.RevolutionCodeWarrior.warningsuppression", ""));
                }
                AddOption(OptionType.cc_Options, PropertyUtil.GetPropertyOrDefault(Project, "eaconfig." + ConfigPlatform + ".warningsuppression", ""));
            }
            else if (DefaultWarningSuppression == WarningSuppressionModes.Strict)
            {
                if (FlagEquals("optimization", "on"))
                {
                    AddOptionIf("warnings", "on", OptionType.cc_Options, "-w all, notinlined");
                }
                else
                {
                    AddOptionIf("warnings", "on", OptionType.cc_Options, "-w all");
                }
                AddOptionIf("warnings", "on", OptionType.cc_Options, "-iso_templates on");
            }
            else if (DefaultWarningSuppression == WarningSuppressionModes.Custom)
            {
                if (FlagEquals("optimization", "on"))
                {
                    AddOptionIf("warnings", "on", OptionType.cc_Options, "-w all, notinlined");
                }
                else
                {
                    AddOptionIf("warnings", "on", OptionType.cc_Options, "-w all");
                }
                AddOption(OptionType.cc_Options, PropertyUtil.GetPropertyOrDefault(Project, "eaconfig." + ConfigPlatform + ".warningsuppression", ""));
            }
            else
            {
                AddOptionIf("warnings", "on", OptionType.cc_Options, "-w all");
            }
            AddOptionIf("warnings", "on", OptionType.link_Options, "-w on");

            switch (ConfigSystem)
            {
                case "ds":
                    AddOptionIf("debugsymbols", "on", OptionType.cc_Options, "-g");
                    AddOptionIf("debugsymbols", "on", OptionType.link_Options, "-g");
                    AddOptionIf("debugsymbols", "on", OptionType.lib_Options, "-g");
                    ReplaceOptionIf("debugsymbols", "off", OptionType.link_Options, "-g", "");

                    AddOptionIf("exceptions", "off", OptionType.cc_Options, "-Cpp_exceptions off");
                    AddOptionIf("clanguage", "off", OptionType.cc_Options, "-lang c++");

                    AddOptionIf("optimization", "off", OptionType.cc_Options, "-inline off");
                    AddOptionIf("optimization", "off", OptionType.cc_Options, "-opt off");
                    AddOptionIf("optimization", "on", OptionType.cc_Options, "-opt full");

                    // ----- GENERAL OPTIONS -----
                    if (FlagEquals("misc", "on"))
                    {
                        AddOption(OptionType.cc_Options, "-char signed");   // Controls the default sign of the char data type-->
                        AddOption(OptionType.cc_Options, "-enc ascii");     // Specify the default source encoding used by the compiler.-->
                        AddOption(OptionType.cc_Options, "-enum int");      // Specify the default size for enumeration types.-->
                        AddOption(OptionType.cc_Options, "-gccinc");        // Controls the compilers use of GCC #include semantics-->
                        AddOption(OptionType.cc_Options, "-interworking");  // Generate ARM/Thumb interworking sequences.-->
                        AddOption(OptionType.cc_Options, "-msgstyle gcc");  // Controls the style used to show error and warning messages. </P>-->
                        AddOption(OptionType.cc_Options, "-nowraplines");   // Controls the word wrapping of messages. -->
                        AddOption(OptionType.cc_Options, "-nopic");         // Not geneate position-independent code references.-->
                        AddOption(OptionType.cc_Options, "-nopid");         // Not generate position-independent data references-->
                        AddOption(OptionType.cc_Options, "-nothumb");       // Not generate Thumb instructions. -->
                        AddOption(OptionType.cc_Options, "-Wno-missing-braces"); // rwmath VectorIntrinsic usage. -->
                        AddOption(OptionType.cc_Options, "-proc arm946e");  // Generates and links object code for a specific processor.-->
                        AddOption(OptionType.cc_Options, "-stdinc");        // Use standard system include paths as specified by the environment variable %MWCIncludes%.-->
                        AddOption(OptionType.cc_Options, "-stdkeywords off");  // Controls the use of ISO/IEC 9899-1990 C89 keywords.-->
                    }
                    // ----- OUTPUT OPTIONS -----
                    AddOptionIf("generatemapfile", "on", OptionType.link_Options, "--Map=\"%linkoutputmapname%\"");
                    break;
            }
        }


        protected override void InternalInit()
        {
            base.InternalInit();


            string property = PropertyUtil.GetPropertyOrDefault(Project, "eaconfig.default_warning_suppression", "on").ToLower();
            switch(property)
            {
                case "on":
                    _defaultWarningSuppression = WarningSuppressionModes.On;
                    break;
                case "off":
                    _defaultWarningSuppression = WarningSuppressionModes.Off;
                    break;
                case "strict":
                    _defaultWarningSuppression = WarningSuppressionModes.Strict;
                    break;
                case "custom":
                    _defaultWarningSuppression = WarningSuppressionModes.Custom;
                    break;
                default:
                    {
                        string msg = Error.Format(Project, "GenerateBuildOptionset", "WARNING", "Invalid value of property 'eaconfig.default_warning_suppression'='{0}' are 'on/off/strict/custom', will use default value 'on'.", property);
                        Log.Info.WriteLine(msg);
                        _defaultWarningSuppression = WarningSuppressionModes.On;
                    }
                    break;

            }
        }

        protected WarningSuppressionModes DefaultWarningSuppression
        {
            get { return _defaultWarningSuppression; }

        }

        private WarningSuppressionModes _defaultWarningSuppression = WarningSuppressionModes.On;

        protected bool IsVC10
        {
            get
            {
                return !((null == Properties["package.eaconfig.isusingvc10"]) || (Properties["package.eaconfig.isusingvc10"] == "false"));
            }
        }

        protected void AddOptionIf(string flag, string flagValue, OptionType type, string value)
        {
            if (FlagEquals(flag, flagValue))
            {
                AddOption(type, value);
            }
        }

        protected void PrependOptionIf(string flag, string flagValue, OptionType type, string value)
        {
            if (FlagEquals(flag, flagValue))
            {
                PrependOption(type, value);
            }
        }

        protected void SetOptionsString(OptionType type, string value)
        {
            _optionBuilder[(int)type] = new StringBuilder(value);
        }

        protected void AddOption(OptionType type, string value)
        {
            _optionBuilder[(int)type].AppendLine(value);
        }

        protected void PrependOption(OptionType type, string value)
        {
            _optionBuilder[(int)type].Insert(0, value + "\n");
        }

        protected void ReplaceOptionIf(string flag, string flagValue, OptionType type, string oldValue, string newValue)
        {
            if (FlagEquals(flag, flagValue))
            {
                ReplaceOption(type, oldValue, newValue);
            }
        }

        protected void ReplaceOption(OptionType type, string oldValue, string newValue)
        {
            if (ContainsOption(type, oldValue))
            {
                _optionBuilder[(int)type].Replace(oldValue, newValue);
            }
            else
            {
                AddOption(type, newValue);
            }
        }

        protected bool ContainsOption(OptionType type, string value)
        {
            string option = _optionBuilder[(int)type].ToString();
            return (!String.IsNullOrEmpty(option) && option.Contains(value));
        }


        protected bool FlagEquals(string name, string val)
        {

            return Flag(name).Equals(val, StringComparison.OrdinalIgnoreCase);
        }

        private string Flag(string name)
        {
            string option = ConfigOptionSet.Options[name];

            string prop = Properties["eaconfig." + name];
            if (!String.IsNullOrEmpty(prop))
            {
                if (prop.Equals("true", StringComparison.OrdinalIgnoreCase) || prop.Equals("on", StringComparison.OrdinalIgnoreCase))
                {
                    option = "on";
                }
                else if (prop.Equals("false", StringComparison.OrdinalIgnoreCase) || prop.Equals("off", StringComparison.OrdinalIgnoreCase))
                {
                    option = "off";
                }
            }
            if (option == null)
                option = String.Empty;
            return option;
        }


        private string _configsetName = String.Empty;
        private OptionSet _ConfigOptionSet = null;

        protected enum OptionType : int
        {
            cc_Options = 0,
            cc_Defines = 1,
            link_Options = 2,
            link_Libraries= 3,
            link_Librarydirs = 4,
            lib_Options = 5
        }

        private string[] _optionNames = new string[] { "cc.options", "cc.defines", "link.options", "link.libraries", "link.librarydirs", "lib.options" };
        private StringBuilder[] _optionBuilder = new StringBuilder[] { new StringBuilder(), new StringBuilder(), new StringBuilder(), new StringBuilder(), new StringBuilder(), new StringBuilder() };

    }
}
