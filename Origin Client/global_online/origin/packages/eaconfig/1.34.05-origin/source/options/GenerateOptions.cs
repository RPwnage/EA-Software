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

            string platformLoadName = project.Properties["config-platform-load-name"];
            if (!string.IsNullOrEmpty(platformLoadName))
            {
                GeneratePlatformOptions platformTask = TaskFactory.Instance.CreateTask("GenerateOptions_" + platformLoadName, project) as GeneratePlatformOptions;
                if (platformTask != null)
                {
                    platformTask.Init(project, configOptionSet, configsetName);
                }
                task = platformTask as GenerateOptions;
            }
            if (task == null)
            {
                task = new GenerateOptions(project, configOptionSet, configsetName);
            }
            task.Execute();
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

            // Wireframe uses internal property build-compiler:
            Project.Properties["build-compiler"] = ConfigCompiler;

            if (ConfigCompiler == "sn")
            {
                AddOptionIf("optimization", "custom", OptionType.cc_Options, ConfigOptionSet.Options["optimization.custom.cc"]);
            }


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

            if (ConfigCompiler != "sn")
            {
                AddOptionIf("optimization", "custom", OptionType.cc_Options, ConfigOptionSet.Options["optimization.custom.cc"]);
            }

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
        }


        protected virtual void GeneratePlatformSpecificOptions()
        {
            switch (ConfigCompiler)
            {
                case "sn":
                    Options_SN();
                    break;
                case "clang":
                case "gcc":
                    Options_GCC();
                    break;
                case "vc":
                    Options_VC();
                    break;
                case "mw":
                    Options_MW();
                    break;
                default:
                    Error.Throw(Project, Location, Name, "Unknown 'config-compiler'='{0}'", ConfigCompiler);
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
            AddOption(OptionType.lib_Options, "-rs \"%outputdir%/" + Properties["lib-prefix"] + "%outputname%" + Properties["lib-suffix"] + "\"");

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
                case "ps3":

                    // ----- WARNING OPTIONS ----- 

                    if (DefaultWarningSuppression == WarningSuppressionModes.On)
                    {
                        AddOptionIf("warningsaserrors", "on", OptionType.cc_Options, "-Xquit=1"); // Treat warnings as errors 
                        AddOptionIf("warnings", "on", OptionType.cc_Options, "-Xdiag=1");
                        AddOption(OptionType.cc_Options, PropertyUtil.GetPropertyOrDefault(Project, "eaconfig.ps3-sn.warningsuppression", ""));
                    }
                    else if (DefaultWarningSuppression == WarningSuppressionModes.Strict)
                    {
                        AddOptionIf("warningsaserrors", "on", OptionType.cc_Options, "-Xquit=2");
                        AddOptionIf("warnings", "on", OptionType.cc_Options, "-Xdiag=2");
                    }
                    else if (DefaultWarningSuppression == WarningSuppressionModes.Custom)
                    {
                        AddOptionIf("warningsaserrors", "on", OptionType.cc_Options, "-Xquit=1"); // Treat warnings as errors 
                        AddOption(OptionType.cc_Options, PropertyUtil.GetPropertyOrDefault(Project, "eaconfig.ps3-sn.warningsuppression", ""));
                    }
                    else
                    {
                        AddOptionIf("warningsaserrors", "on", OptionType.cc_Options, "-Xquit=1"); // Treat warnings as errors 
                        AddOptionIf("warnings", "on", OptionType.cc_Options, "-Xdiag=1");
                    }

                    // ----- OPTIMIZATION OPTIONS ----- 
                    AddOptionIf("optimization", "on", OptionType.cc_Options, "-Os"); //optimization for space
                    //<!-- -Xfastmath=1 -->
                    //<!-- -Xassumecorrectsign=1 -->
                    //<!-- -Xassumecorrectalignment=1 -->

                    // ----- CODEGENERATION OPTIONS -----

                    AddOptionIf("embedguid", "on", OptionType.link_Options, "-ppuguid");  // Store the unique build guid field, which allows unique identification of each .elf/.self.
                    AddOptionIf("enable.strict.aliasing", "on", OptionType.cc_Options, "-Xrelaxalias=2");
                    AddOptionIf("enable.strict.aliasing", "off", OptionType.cc_Options, "-Xrelaxalias=0");
                    if (FlagEquals("clanguage", "off"))
                    {
                        AddOptionIf("rtti", "off", OptionType.cc_Options, "-Xc-=rtti");
                        AddOptionIf("exceptions", "off", OptionType.cc_Options, "-Xc-=exceptions"); //Do not generate exception handling code
                    }
                    ReplaceOptionIf("rtti", "on", OptionType.cc_Options, "-Xc-=rtti", "-Xc += rtti");

                    AddOptionIf("debugsymbols", "on", OptionType.cc_Options, "-g");

                    AddOptionIf("create-pch", "on", OptionType.cc_Options, "--create_pch=\"%pchfile%\"");
                    AddOptionIf("use-pch", "on", OptionType.cc_Options, "--use_pch=\"%pchfile%\"");
                    AddOptionIf("use-pch", "on", OptionType.cc_Options, "-Xpch_override=1");  //Warning 1643: the file being compiled needs to be in the same directory as the file used to create the PCH file
                    AddOptionIf("use-pch", "on", OptionType.cc_Options, "--diag_suppress=631"); //Warning 631: the command line options do not match those used when precompiled header file
                    AddOptionIf("create-pch", "on", OptionType.cc_Options, "--diag_suppress=631"); //Warning 631: the command line options do not match those used when precompiled header file

                    //In case precompiled header is defined directly through cc options
                    if (ContainsOption(OptionType.cc_Options, "--create_pch"))
                    {
                        ConfigOptionSet.Options["create-pch"] = "on";
                    }
                    if (ContainsOption(OptionType.cc_Options, "--use_pch"))
                    {
                        AddOption(OptionType.cc_Options, "-Xpch_override=1"); //Warning 1643: the file being compiled needs to be in the same directory as the file used to create the PCH file
                    }


                    // ----- GENERAL OPTIONS ----- 
                    AddOptionIf("misc", "on", OptionType.cc_Options, "-c"); // Compile only 

                    // ----- LINK OPTIONS ----- 
                    AddOptionIf("stripunusedsymbols", "on", OptionType.link_Options, "--strip-unused-data --strip-duplicates");

                    if (FlagEquals("exceptions", "on"))
                    {
                        AddOption(OptionType.link_Options, "--exceptions");
                        SetOptionsString(OptionType.link_Librarydirs, Properties["link.exceptionlibrarydirs"]);
                        ReplaceOption(OptionType.link_Libraries, "\\fno-exceptions\\fno-rtti", "");
                    }
                    else
                    {
                        AddOption(OptionType.link_Options, "--no-exceptions");
                    }

                    // ----- OUTPUT OPTIONS ----- 
                    string cmp = NAnt.Core.Functions.StringFunctions.StrCompareVersions(Project, Properties["package." + Properties["PlayStation3Package"] + ".version"], "300.003");
                    int cmpint = -1;
                    if (Int32.TryParse(cmp, out cmpint))
                    {
                        if (cmpint > 0)
                        {
                            string snTempPath = Properties["package.builddir"] + "\\" + Properties["config"] + "\\build\\SNTemp";
                            if (!Directory.Exists(snTempPath))
                            {
                                Directory.CreateDirectory(snTempPath);
                            }
                            AddOption(OptionType.cc_Options, "-td=\"" + snTempPath + "\"");
                        }
                    }
                    AddOptionIf("generatedll", "on", OptionType.link_Options, "--oformat=fsprx");

                    AddOptionIf("generatemapfile", "on", OptionType.link_Options, "--Map=\"%linkoutputmapname%\"");

                    break;

                case "ps2":
                    AddOptionIf("debugsymbols", "on", OptionType.cc_Options, "-g2"); // Generate debug information
                    AddOptionIf("debugsymbols", "on", OptionType.link_Options, "-g");
                    if (FlagEquals("clanguage", "off"))
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

                    if (Properties["cc"].EndsWith("ps2cc.exe"))
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
                    if (FlagEquals("clanguage", "off"))
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
                    if (DefaultWarningSuppression == WarningSuppressionModes.On)
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
                case "ps3":

                    AddOptionIf("debugsymbols", "on", OptionType.cc_Options, "-g");

                    AddOptionIf("optimization", "off", OptionType.cc_Options, "-O0");
                    AddOptionIf("optimization", "on", OptionType.cc_Options, "-O2"); //Set optimization level 2
                    AddOptionIf("optimization", "on", OptionType.cc_Options, "-ffast-math"); //Improve FP speed by violating ANSI & IEEE rules
                    AddOptionIf("optimization", "on", OptionType.cc_Options, "-fomit-frame-pointer"); //Omit frame pointer when possible
                    AddOptionIf("optimization", "on", OptionType.cc_Options, "-mvrsave=no"); //Disable VRSAVE register since it is not used, helps gain performance                          

                    AddOptionIf("enable.strict.aliasing", "on", OptionType.cc_Options, "-fstrict-aliasing");     // Enable strict aliasing
                    AddOptionIf("enable.strict.aliasing", "off", OptionType.cc_Options, "-fno-strict-aliasing");

                    AddOptionIf("embedguid", "on", OptionType.link_Options, "-mppuguid");  // Store the unique build guid field, which allows unique identification of each .elf/.self.

                    // ----- GENERAL OPTIONS ----- 
                    if (FlagEquals("misc", "on"))
                    {
                        AddOption(OptionType.cc_Options, "-c");             //Compile only
                        AddOption(OptionType.cc_Options, "-fno-common");    // In C, allocate even uninitialized global variables in data section
                        // rather than as common blocks.  If a variable is declared in two different                                
                        // compilations, it will cause a link error
                        AddOption(OptionType.cc_Options, "-fshort-wchar");  // Override the underlying type for wchar_t to `unsigned short
                        AddOption(OptionType.cc_Options, "-maltivec");      // Enabling altivec instructions/intrinsics 
                    }

                    // ----- LINK OPTIONS ----- 
                    if (FlagEquals("exceptions", "on"))
                    {
                        SetOptionsString(OptionType.link_Librarydirs, Properties["link.exceptionlibrarydirs"]);
                        ReplaceOption(OptionType.link_Libraries, "\\fno-exceptions\\fno-rtti", "");
                    }
                    else
                    {
                        AddOption(OptionType.link_Options, "-fno-exceptions");
                    }
                    AddOptionIf("rtti", "off", OptionType.link_Options, "-fno-rtti"); // Do not generate run time type information

                    // ----- OUTPUT OPTIONS ----- 
                    if (FlagEquals("generatedll", "on"))
                    {
                        AddOption(OptionType.cc_Options, "-mprx");
                        AddOption(OptionType.link_Options, "-mprx");
                        AddOption(OptionType.link_Options, "-zgenprx");  //Generate a PRX
                        AddOption(OptionType.link_Options, "-zgenstub"); //Also generate a stub library for that PRX.
                        AddOption(OptionType.link_Options, "--prx-fixup");
                    }
                    AddOption(OptionType.link_Options, "-o \"%linkoutputname%\"");
                    AddOption(OptionType.lib_Options, "-rs \"%outputdir%/" + Properties["lib-prefix"] + "%outputname%" + Properties["lib-suffix"] + "\"");
                    AddOptionIf("generatemapfile", "on", OptionType.link_Options, "-Wl,-Map,\"%linkoutputmapname%\"");

                    // ----- WARNING OPTIONS ----- 
                    AddOptionIf("exceptions", "on", OptionType.link_Librarydirs, Properties["link.exceptionlibrarydirs"]);
                    break;
                case "unix":
                case "unix64":
                    AddOptionIf("optimization", "off", OptionType.cc_Options, "-O0");
                    AddOptionIf("enable.strict.aliasing", "on", OptionType.cc_Options, "-fstrict-aliasing");     // Enable strict aliasing
                    AddOptionIf("enable.strict.aliasing", "off", OptionType.cc_Options, "-fno-strict-aliasing");

                    AddOptionIf("debugsymbols", "on", OptionType.cc_Options, "-g3"); //We may want to use -Wshorten-64-to-32 and -Wconversion when using Mac osx in 64 bit mode.

                    AddOption(OptionType.cc_Defines, "EA_PLATFORM_LINUX=1");

                    if (!FlagEquals("generatedll", "on"))
                    {
                        AddOptionIf("optimization", "on", OptionType.link_Options, "-Wl,--gc-sections"); //strip out unused functions in the output binary and make it smaller
                    }

                    // ----- GENERAL OPTIONS ----- 
                    if (FlagEquals("misc", "on"))
                    {
                        AddOption(OptionType.cc_Options, "-c");             //Compile only
                        AddOption(OptionType.cc_Options, "-fno-common");    // In C, allocate even uninitialized global variables in data section
                        // rather than as common blocks.  If a variable is declared in two different                                
                        // compilations, it will cause a link error
                        AddOption(OptionType.cc_Options, "-fmessage-length=0");  // Put the output entirely on one line; don't add newlines
                    }

                    if (ConfigCompiler == "gcc")
                    {
                        AddOptionIf("clanguage", "off", OptionType.cc_Options, "-std=c++0x");
                        AddOption(OptionType.cc_Options, Project.Properties["package.UnixGCC.cc.options"]);
                        AddOption(OptionType.link_Options, Project.Properties["package.UnixGCC.link.options"]);
                    }
                    else
                    {
                        // If not GCC, assuming clang.
                        AddOptionIf("clanguage", "off", OptionType.cc_Options, "-std=c++11");
                        AddOption(OptionType.cc_Options, Project.Properties["package.UnixClang.cc.options"]);
                        AddOption(OptionType.link_Options, Project.Properties["package.UnixClang.link.options"]);
                    }

                    AddOption(OptionType.cc_Options, "-MMD"); //Dump a .d file
                    // ----- LINK OPTIONS ----- 

                    AddOptionIf("exceptions", "on", OptionType.link_Librarydirs, Properties["link.exceptionlibrarydirs"]);
                    AddOptionIf("exceptions", "off", OptionType.link_Options, "-fno-exceptions");

                    // ----- OUTPUT OPTIONS ----- 
                    AddOptionIf("generatedll", "on", OptionType.link_Options, "-shared"); //Make a DLL, not an exe.
                    AddOption(OptionType.link_Options, "-o \"%linkoutputname%\"");

                    AddOptionIf("generatemapfile", "on", OptionType.link_Options, "-Wl,-Map,\"%linkoutputmapname%\"");

                    break;
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
                    AddOption(OptionType.lib_Options, "-rs \"%outputdir%/" + Properties["lib-prefix"] + "%outputname%" + Properties["lib-suffix"] + "\"");
                    AddOptionIf("generatemapfile", "on", OptionType.link_Options, "-Wl,-Map,\"%linkoutputmapname%\"");

                    break;
                case "pc":
                case "pc64":
                    AddOptionIf("optimization", "off", OptionType.cc_Options, "-O0");
                    AddOptionIf("enable.strict.aliasing", "on", OptionType.cc_Options, "-fstrict-aliasing");     // Enable strict aliasing
                    AddOptionIf("enable.strict.aliasing", "off", OptionType.cc_Options, "-fno-strict-aliasing");

                    AddOption(OptionType.cc_Defines, "EA_PLATFORM_WINDOWS=1"); //pc-gcc means MinGW 
                    AddOption(OptionType.cc_Defines, "NOMINMAX"); // If you are going to possibly see Microsoft headers, it's a good idea to define this.
                    AddOption(OptionType.cc_Defines, "_WIN32"); // VC++ defines _WIN32, so we do so too..

                    // ----- GENERAL OPTIONS ----- 
                    AddOptionIf("pcconsole", "on", OptionType.link_Options, "-mconsole");
                    AddOptionIf("pcconsole", "off", OptionType.link_Options, "-mwindows");
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
                    AddOption(OptionType.lib_Options, "-rs \"%outputdir%/" + Properties["lib-prefix"] + "%outputname%" + Properties["lib-suffix"] + "\"");
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

                    string androidSDK_linkoptions = Project.Properties["package.AndroidSDK.link.options"];
                    if (FlagEquals("generatedll", "on"))
                    {
                        StringBuilder sb = new StringBuilder();
                        foreach (string option in StringUtil.ToArray(androidSDK_linkoptions, new char[] { '\n', '\r' }))
                        {
                            if (String.IsNullOrEmpty(option) || option.StartsWith("-DEA_ANDROID_ENTRY_POINT_NAME"))
                            {
                                continue;
                            }
                            sb.AppendLine(option);
                        }
                        androidSDK_linkoptions = sb.ToString();
                    }

                    AddOption(OptionType.link_Options, androidSDK_linkoptions);

                    if (FlagEquals("clanguage", "off"))
                    {
                        if (FlagEquals("rtti", "off"))
                        {
                            AddOption(OptionType.cc_Options, "-fno-rtti"); //Do not generate run time type information
                        }
                        if (FlagEquals("rtti", "on"))
                        {
                            AddOption(OptionType.cc_Options, "-frtti"); //Generate run time type information
                            AddOption(OptionType.link_Options, "-frtti"); //Generate run time type information
                            AddOption(OptionType.link_Librarydirs, Project.ExpandProperties("${gcc_prefix}/${android_gcc_folder}/lib"));
                            AddOption(OptionType.link_Libraries, "-lsupc++");
                        }


                        AddOptionIf("exceptions", "off", OptionType.link_Options, "-fno-exceptions");
                        if (FlagEquals("exceptions", "on"))
                        {
                            AddOption(OptionType.cc_Options, "-fexceptions");
                            AddOption(OptionType.link_Options, "-fexceptions");
                            AddOptionIf("exceptions", "on", OptionType.link_Librarydirs, Properties["link.exceptionlibrarydirs"]);
                            if (!FlagEquals("rtti", "on"))
                            {
                                AddOption(OptionType.link_Options, "-frtti"); //Generate run time type information
                                AddOption(OptionType.link_Librarydirs, Project.ExpandProperties("${gcc_prefix}/${android_gcc_folder}/lib"));
                                AddOption(OptionType.link_Libraries, "-lsupc++");
                            }
                        }
                    }

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

                    AddOption(OptionType.lib_Options, "-o \"%outputdir%/" + Properties["lib-prefix"] + "%outputname%" + Properties["lib-suffix"] + "\"");

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

        private void Options_VC()
        {
            // Incredilink support:
            if (PropertyUtil.GetBooleanProperty(Project, "incredibuild.xge.useincredilink") && ConfigPlatform != "xbox")
            {
                ConfigOptionSet.Options["incrementallinking"] = "on";
                ConfigOptionSet.Options["uselibrarydependencyinputs"] = "on";
                if (ConfigOptionSet.Options["buildset.name"] != "ManagedCppAssembly")
                {
                    AddOption(OptionType.link_Options, "-opt:noref");
                    AddOption(OptionType.link_Options, "-opt:noicf");
                }
            }

            AddOptionIf("runtimeerrorchecking", "on", OptionType.cc_Options, "-RTC1"); // Generate run-time error checking code

            // Specify a .pdb file name to avoid generic vs name.
            // Regardless of whether we're building with debug symbols or not,
            // this is necessary for vsbuilds during generation of .idb dependency files as well.
            AddOption(OptionType.cc_Options, "-Fd\"%outputdir%\\%outputname%.pdb\"");

            if (FlagEquals("debugsymbols", "on"))
            {
                if (FlagEquals("c7debugsymbols", "on"))
                {
                    AddOption(OptionType.cc_Options, "-Z7"); // Generate C7-style embedded debug information
                }
                else
                {
                    if (FlagEquals("managedcpp", "off"))
                    {
                        if (FlagEquals("optimization", "on"))
                        {
                            if (FlagEquals("editandcontinue", "on"))
                            {
                                string msg = Error.Format(Project, "GenerateBuildOptionset", "WARNING", "Optionset '{0}': -O2 optimization option is incompatible with edit-and-continue -ZI option. Edit-and-continue will be ignored", _configsetName);
                                Log.WriteLine(msg);
                            }

                            AddOption(OptionType.cc_Options, "-Zi");
                        }
                        else
                        {
                            if (FlagEquals("editandcontinue", "on"))
                            {
                                if (ContainsOption(OptionType.cc_Options, "-Zi"))
                                {
                                    string msg = Error.Format(Project, "GenerateBuildOptionset", "WARNING", "Optionset '{0}': -Zi option is already present, it is incompatible with edit-and-continue -ZI option. Edit-and-continue will be ignored", _configsetName);
                                    Log.WriteLine(msg);
                                }
                                else
                                {
                                    AddOption(OptionType.cc_Options, "-ZI");
                                }
                            }
                            AddOptionIf("editandcontinue", "off", OptionType.cc_Options, "-Zi");
                        }
                    }
                    else
                    {
                        AddOption(OptionType.cc_Options, "-Zi");
                    }
                }
            }
            else
            {
                // Add warning to help detecting inconsistency
                if (FlagEquals("c7debugsymbols", "on"))
                {
                    string msg = Error.Format(Project, "GenerateBuildOptionset", "WARNING", "Optionset '{0}': c7debugsymbols='on' but debugsymbols is 'off', no -Z7 option will be set, no debug symbols generated.", ConfigSetName);
                    Log.WriteLine(msg);
                }
            }


            AddOptionIf("rtti", "off", OptionType.cc_Options, "-GR-"); // Disable run time type information generation (overwrite vs2005 default behaviour, which is -GR)
            AddOptionIf("rtti", "on", OptionType.cc_Options, "-GR"); // Enable run time type information generation
            if (FlagEquals("exceptions", "on"))
            {
                //In VS 2005, using /clr (Common Language Runtime Compilation) implies /EHa
                //(/clr /EHa is redundant). The compiler will generate an error if /EHs[c]
                //is used after /clr.
                //We choose to explicitly specify /EHa here as a workaround for an
                //IncrediBuild-2.61 bug for VS2005 builds.  IncrediBuild doesn't seem to
                //default the exception-handling setting correctly for all /clr* flags,
                //so we need to set this exception handling flag explicitly for the time
                //being for reliable IncrediBuild builds.                
                //See http://msdn2.microsoft.com/en-us/library/1deeycx5.aspx for more
                //details on the various Visual C++ exception handling modes.
                if (IsVC8 && (FlagEquals("toolconfig", "on") || ContainsOption(OptionType.cc_Options, "clr")))
                {
                    AddOption(OptionType.cc_Options, "-EHa");
                }
                else
                {
                    AddOption(OptionType.cc_Options, "-EHsc");
                }
            }
            else
            {
                if (IsVC10)
                {
                    //NOTICE(RASHIN):
                    //Seems this is required for vs2010 builds, otherwise, include headers
                    //like xlocale will spit out warnings as errors.
                    AddOption(OptionType.cc_Defines, "_HAS_EXCEPTIONS=0");
                }
            }
            AddOptionIf("iteratorboundschecking", "off", OptionType.cc_Defines, "_SECURE_SCL=0");
            AddOptionIf("iteratorboundschecking", "on", OptionType.cc_Defines, "_SECURE_SCL=1");

            AddOptionIf("create-pch", "on", OptionType.cc_Options, "/Yc\"%pchheaderfile%\"");
            AddOptionIf("create-pch", "on", OptionType.cc_Options, "/Fp\"%pchfile%\"");
            AddOptionIf("use-pch", "on", OptionType.cc_Options, "/Yu\"%pchheaderfile%\"");
            AddOptionIf("use-pch", "on", OptionType.cc_Options, "/Fp\"%pchfile%\"");
            //In case precompiled header is defined directly through cc options
            if (ContainsOption(OptionType.cc_Options, "/Yc") || ContainsOption(OptionType.cc_Options, "-Yc"))
            {
                ConfigOptionSet.Options["create-pch"] = "on";
            }


            AddOptionIf("optimization", "off", OptionType.cc_Options, "-Od");

            if (FlagEquals("debugsymbols", "on"))
            {
                AddOption(OptionType.link_Options, "-debug");                         //Generate debug information
                AddOption(OptionType.link_Options, "-PDB:\"%linkoutputpdbname%\"");   //Need to explicitly set output location - otherwise the default PDB location for VCPROJ builds is to the 'build' dir, not 'bin' dir.
                AddOptionIf("managedcpp", "on", OptionType.link_Options, "-ASSEMBLYDEBUG");  //Debuggable Assembly: Runtime tracking and disable optimizations 
            }

            // ----- GENERAL OPTIONS ----- 
            if (PropertyExists("eaconfig.build-MP"))
            {
                AddOption(OptionType.cc_Options, "-MP" + Properties["eaconfig.build-MP"]);
            }
            if (FlagEquals("misc", "on"))
            {
                AddOption(OptionType.cc_Options, "-c");             // Compile only
                AddOption(OptionType.cc_Options, "-Zc:forScope");   // Enforce Standard C++ for scoping
            }

            // ----- LINK OPTIONS ----- 
            if (FlagEquals("usedebuglibs", "off"))
            {
                AddOptionIf("multithreadeddynamiclib", "off", OptionType.cc_Options, "-MT");  // link with multithreaded library  
                AddOptionIf("multithreadeddynamiclib", "on", OptionType.cc_Options, "-MD");   // link with multithreaded dll library
            }
            else
            {
                AddOptionIf("multithreadeddynamiclib", "off", OptionType.cc_Options, "-MTd");  // link with multithreaded library  (debug version)
                AddOptionIf("multithreadeddynamiclib", "on", OptionType.cc_Options, "-MDd");   // link with multithreaded dll library (debug version)
            }

            AddOptionIf("incrementallinking", "off", OptionType.link_Options, "-incremental:no ");  // Disable incremental linking
            AddOptionIf("incrementallinking", "on", OptionType.link_Options, "-incremental");       // Enable incremental linking

            // ----- OUTPUT OPTIONS ----- 
            AddOptionIf("banner", "off", OptionType.cc_Options, "-nologo");    // Suppress copyright message
            AddOptionIf("banner", "off", OptionType.link_Options, "-nologo");  // Suppress copyright message
            AddOptionIf("banner", "off", OptionType.lib_Options, "-nologo");  // Suppress copyright message
            AddOptionIf("generatemapfile", "on", OptionType.link_Options, "-map:\"%linkoutputmapname%\"");
            AddOptionIf("generatedll", "on", OptionType.link_Options, "-dll");
            AddOption(OptionType.link_Options, "-out:\"%linkoutputname%\"");
            AddOption(OptionType.lib_Options, "-out:\"%outputdir%/" + Properties["lib-prefix"] + "%outputname%" + Properties["lib-suffix"] + "\"");

            // ----- WARNING OPTIONS ----- 
            AddOptionIf("warningsaserrors", "on", OptionType.cc_Options, "-WX"); // Treat warnings as errors 
            AddOptionIf("warningsaserrors", "off", OptionType.cc_Options, "-WX-"); // Disable Treat warnings as errors 

            PrependOptionIf("warnings", "on", OptionType.link_Options, "-ignore:4089");  //Disable warning: dll discarded by OPT:ref option
            if (FlagEquals("warnings", "on"))
            {
                if (DefaultWarningSuppression == WarningSuppressionModes.On)
                {
                    //  Wall turns some warnings on that we don't want 
                    PrependOption(OptionType.cc_Options, "-wd4061");   //  Disable warning: enumerator 'abc' in switch of enum 'xyz' is not explicitly handled by a case label-->
                    PrependOption(OptionType.cc_Options, "-wd4062");   //  Disable warning: enumerator 'abc' in switch of enum 'xyz' is not handled -->
                    PrependOption(OptionType.cc_Options, "-wd4100");   //  Disable warning: unreferenced formal parameter -->
                    PrependOption(OptionType.cc_Options, "-wd4324");   //  Disable warning: padding due to alignment statement -->
                    PrependOption(OptionType.cc_Options, "-wd4619");   //  Disable warning: #pragma warning : there is no warning number '####' -->
                    PrependOption(OptionType.cc_Options, "-wd4514");   //  Disable warning: unreferenced inline function removed -->
                    PrependOption(OptionType.cc_Options, "-wd4640");   //  Disable warning: construction of local static object is not thread safe -->
                    PrependOption(OptionType.cc_Options, "-wd4668");   //  Disable warning: use of undefined preprocessor macro (default value is zero) -->
                    PrependOption(OptionType.cc_Options, "-wd4710");   //  Disable warning: inline function not inlined (compiler's discretion) -->
                    PrependOption(OptionType.cc_Options, "-wd4711");   //  Disable warning: function selected for automatic inline expansion -->
                    PrependOption(OptionType.cc_Options, "-wd4738");   //  Disable warning: storing 32-bit float result in memory, possible loss of performance -->
                    PrependOption(OptionType.cc_Options, "-wd4820");   //  Disable warning: padding at end of structure by compiler -->
                    PrependOption(OptionType.cc_Options, "-wd4826");   //  Disable warning: Conversion from 'char *' to 'uint64_t' is sign-extended. -->

                    //Disable warning: an ANSI source file is compiled on a system with a codepage that
                    //cannot represent all characters in the file.    This is a workaround for a long-standing
                    //IncrediBuild bug.   Their virtual environment doesn't capture the machine's codebase,
                    //so if source files with special characters in their comments are distributed to
                    //machines with Chinese or Japanese code-pages, then the distributed build can fail.
                    //This warning is exceedingly unlikely to represent a real issue, so we're disabling
                    //it to avoid the risk of hitting the IncrediBuild bug. -->
                    PrependOption(OptionType.cc_Options, "-wd4819");   // 

                    // Extra VC8 warnings on that we don't want -->
                    PrependOption(OptionType.cc_Options, "-wd4826");   //  VS2K5: Conversion from 'char *' to 'uint64_t' is sign-extended. -->
                    PrependOption(OptionType.cc_Options, "-wd4692");   //  VS2K5: 'function': signature of non-private member contains assembly private native type 'native_type'-->
                    PrependOption(OptionType.cc_Options, "-wd4996");   //  Disable warning: 'strncpy' was declared deprecated. net2005 specific -->

                    if (OptionSetUtil.IsOptionContainValue(ConfigOptionSet, "cc.defines", "RWDEBUG"))
                    {
                        PrependOption(OptionType.cc_Options, "-wd4127"); // Caused by RW's rwASSERT and rwMESSAGE macros after including RWDEBUG define 
                    }
                    PrependOptionIf("warnings", "off", OptionType.cc_Options, "-W0"); // Disable warnings

                    AddOption(OptionType.cc_Options, PropertyUtil.GetPropertyOrDefault(Project, "eaconfig." + ConfigPlatform + ".warningsuppression", ""));
                }
                else if (DefaultWarningSuppression == WarningSuppressionModes.Strict)
                {
                    PrependOption(OptionType.cc_Options, "-wd4514");   //  Disable warning: unreferenced inline function removed -->
                    PrependOptionIf("optimization", "off", OptionType.cc_Options, "-wd4710");   //  This warning only makes sense when inlining is enabled 
                }
                else if (DefaultWarningSuppression == WarningSuppressionModes.Custom)
                {
                    AddOption(OptionType.cc_Options, PropertyUtil.GetPropertyOrDefault(Project, "eaconfig." + ConfigPlatform + ".warningsuppression", ""));
                }

                PrependOption(OptionType.cc_Options, "-Wall");  // Enable all warnings, including warning disabled by default, except those disabled by -wd<nnnn>
            }
            else
            {
                PrependOptionIf("warnings", "on", OptionType.cc_Options, "-W4");    // Level 4 enables most warnings
            }

            switch (ConfigSystem)
            {
                case "pc":
                case "pc64":
                case "winrt":

                    AddOptionIf("analyze", "build", OptionType.cc_Options, "-analyze");
                    AddOptionIf("analyzewarnings", "on", OptionType.cc_Options, "-analyze:WX");
                    AddOptionIf("analyzewarnings", "off", OptionType.cc_Options, "-analyze:WX-");



                    // ----- OPTIMIZATION OPTIONS ----- 
                    if (FlagEquals("optimization", "on"))
                    {

                        if (IsVC8)
                        {
                            AddOption(OptionType.cc_Options, "-O2");// Maximize speed (equivalent to /Og /Oi /Ot /Oy /Ob2 /Gs /GF /Gy) 
                            AddOption(OptionType.cc_Options, "-GS-");// Disable runtime buffer security checking
                            if (Properties["eaconfig.disable_framepointer_optimization"] != null)
                            {
                                AddOption(OptionType.cc_Options, "-Oy-");// Disable runtime buffer security checking
                            }
                        }
                        else
                        {
                            AddOption(OptionType.cc_Options, "-Og");// Enable global optimization -->
                            AddOption(OptionType.cc_Options, "-GB");// Optimize for blended model -->
                            AddOption(OptionType.cc_Options, "-Oi");// Enable intrinsic functions -->
                            AddOption(OptionType.cc_Options, "-Os");// Favor smaller code -->
                            AddOption(OptionType.cc_Options, "-Oy-");// Disable Frame-Pointer Omission -->
                            AddOption(OptionType.cc_Options, "-Ob2");// Inline Function Expansion -->
                            AddOption(OptionType.cc_Options, "-Gs");// Control Stack Checking Calls -->
                            AddOption(OptionType.cc_Options, "-GF");// Enable read only string pooling -->
                            AddOption(OptionType.cc_Options, "-Gy");// Enable separate functions for linker -->
                        }
                        // link time code generation:
                        if (FlagEquals("optimization.ltcg", "on"))
                        {
                            AddOption(OptionType.cc_Options, "-GL");     //enable link-time code generation
                            AddOption(OptionType.lib_Options, "-LTCG"); //enable link-time code generation
                            switch (ConfigOptionSet.Options["optimization.ltcg.mode"])
                            {
                                case "normal":
                                    AddOption(OptionType.link_Options, "-LTCG");
                                    break;
                                case "instrument":
                                    AddOption(OptionType.link_Options, "-LTCG:PGINSTRUMENT");
                                    break;
                                case "optimize":
                                    AddOption(OptionType.link_Options, "-LTCG:PGOPTIMIZE");
                                    break;
                                case "update":
                                    AddOption(OptionType.link_Options, "-LTCG:PGUPDATE");
                                    break;
                                default:
                                    AddOption(OptionType.link_Options, "-LTCG");
                                    break;
                            }
                        }
                        if (ConfigOptionSet.Options["buildset.name"] != "ManagedCppAssembly")
                        {
                            if (!PropertyUtil.GetBooleanProperty(Project, "incredibuild.xge.useincredilink"))
                            {
                                AddOption(OptionType.link_Options, "-opt:ref");// eliminates functions and/or data that are never referenced
                            }
                        }
                    }

                    // ----- GENERAL OPTIONS ----- 
                    AddOptionIf("pcconsole", "on", OptionType.link_Options, "-subsystem:console");
                    AddOptionIf("pcconsole", "off", OptionType.link_Options, "-subsystem:windows");

                    // ----- WARNING OPTIONS ----- 

                    break;
                case "xenon":

                    AddOptionIf("analyze", "static", OptionType.cc_Options, "/analyze:only");
                    AddOptionIf("analyze", "build", OptionType.cc_Options, "/analyze");
                    if (!IsVC10)
                    {
                        AddOptionIf("analyze", "static", OptionType.cc_Options, "/analyze:wdpath"); //Enable static analysis
                        AddOptionIf("analyze", "build", OptionType.cc_Options, "/analyze:wdpath");
                    }

                    AddOptionIf("analyzewarnings", "on", OptionType.cc_Options, "-analyze:WX");
                    AddOptionIf("analyzewarnings", "off", OptionType.cc_Options, "-analyze:WX-");

                    if (FlagEquals("optimization", "on"))
                    {
                        // (TODO)
                        // These 3 options really shouldn't be here (some are mutually exclusive with each other)
                        // But we've noticed problems with existing test data that's
                        // dependent on one or more of these optimization flags being present.
                        // We haven't figured out why really, but for backwards compatibility
                        // they're staying...
                        AddOption(OptionType.cc_Options, "-Oi");// Enable intrinsic functions 
                        AddOption(OptionType.cc_Options, "-Os");// Favor smaller code 
                        AddOption(OptionType.cc_Options, "-Ob2");// Inline Function Expansion 

                        AddOption(OptionType.cc_Options, "-Ox");// Maximize speed  (this implys -Oi, -Oy, -Ob2, -GF)
                        AddOption(OptionType.cc_Options, "-Oz");// Optimizations inline assembly
                        AddOption(OptionType.cc_Options, "-Ou");// Enable prescheduling 
                        AddOption(OptionType.cc_Options, "-Gy");// Enable separate functions for linker

                        if (!PropertyUtil.GetBooleanProperty(Project, "incredibuild.xge.useincredilink"))
                        {
                            AddOption(OptionType.link_Options, "-opt:ref");// eliminates functions and/or data that are never referenced
                        }

                        // link time code generation:
                        if (FlagEquals("optimization.ltcg", "on"))
                        {
                            AddOption(OptionType.cc_Options, "-GL");     //enable link-time code generation
                            AddOption(OptionType.lib_Options, "-LTCG"); //enable link-time code generation
                            switch (ConfigOptionSet.Options["optimization.ltcg.mode"])
                            {
                                case "normal":
                                    AddOption(OptionType.link_Options, "-LTCG");
                                    break;
                                case "instrument":
                                    AddOption(OptionType.link_Options, "-LTCG:PGINSTRUMENT");
                                    break;
                                case "optimize":
                                    AddOption(OptionType.link_Options, "-LTCG:PGOPTIMIZE");
                                    break;
                                case "update":
                                    AddOption(OptionType.link_Options, "-LTCG:PGUPDATE");
                                    break;
                                default:
                                    AddOption(OptionType.link_Options, "-LTCG");
                                    break;
                            }
                        }
                    }
                    // ----- GENERAL OPTIONS ----- 
                    AddOptionIf("misc", "on", OptionType.cc_Options, "-QVMX128");   // Generates VMX128 native code 

                    // ----- WARNING OPTIONS ----- 
                    if (FlagEquals("warnings", "on") && DefaultWarningSuppression == WarningSuppressionModes.On)
                    {
                        //Suppress extra warnings if provided.  This has been taken out in the earlier version
                        //but we find it necessary to put this back in because we don't want eaconfig to have
                        //a dependency of a particular sdk version.  By having sdk packages suggesting which 
                        //warning to suppress, config packages can choose whether to suppress them or not, and 
                        //they don't have to care about which warning should be disabled for a particular version
                        //of the sdk 

                        // We can now use eaconfig.xenon-vc.warningsuppression in the xenonsdk package.  We keep the following
                        // for backward compatibility.
                        AddOption(OptionType.cc_Options, PropertyUtil.GetPropertyOrDefault(Project, "package.xenonsdk.warningsuppression", ""));
                    }
                    // ----- PROFILE OPTIONS ----- 
                    ReplaceOptionIf("profile", "on", OptionType.cc_Options, "-GL", String.Empty);
                    AddOptionIf("profile", "on", OptionType.cc_Options, "-callcap"); //Enable callcap profiling
                    ReplaceOptionIf("profile", "on", OptionType.link_Options, "-LTCG", String.Empty);
                    break;
                case "xbox":
                    if (FlagEquals("optimization", "on"))
                    {
                        AddOption(OptionType.cc_Options, "-Og");// Enable global optimization 
                        AddOption(OptionType.cc_Options, "-Oi");// Enable intrinsic functions 
                        AddOption(OptionType.cc_Options, "-Os");// Favor smaller code 
                        AddOption(OptionType.cc_Options, "-Oy-");// Disable Frame-Pointer Omission 
                        AddOption(OptionType.cc_Options, "-Ob2");// Inline Function Expansion 
                        AddOption(OptionType.cc_Options, "-Gs");// Control Stack Checking Calls 
                        AddOption(OptionType.cc_Options, "-G6");// Optimize for PPro, P-II, P-III, P4 processors
                    }
                    // ----- GENERAL OPTIONS ----- 
                    AddOptionIf("misc", "on", OptionType.cc_Options, "-arch:SSE");        // Enable Streaming SIMD Extensions

                    break;
            }

            if (FlagEquals("clanguage", "off"))
            {
                AddOptionIf("windowsruntime", "on", OptionType.cc_Options, "-ZW"); // Enable Windows Runtime extensions
            }
        }

        private void Options_MW()
        {
            AddOptionIf("rtti", "off", OptionType.cc_Options, "-RTTI off");
            // ----- OUTPUT OPTIONS ----- 
            AddOption(OptionType.link_Options, "-o \"%linkoutputname%\"");
            AddOption(OptionType.lib_Options, "-o \"%outputdir%/" + Properties["lib-prefix"] + "%outputname%" + Properties["lib-suffix"] + "\"");
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

                case "rev":
                    //-- Debugging --
                    AddOptionIf("debugsymbols", "on", OptionType.cc_Options, "-g");
                    AddOptionIf("debugsymbols", "on", OptionType.cc_Options, "-gdwarf-2");
                    AddOptionIf("debugsymbols", "on", OptionType.link_Options, "-g");
                    AddOptionIf("debugsymbols", "on", OptionType.link_Options, "-gdwarf-2");
                    AddOptionIf("debugsymbols", "on", OptionType.lib_Options, "-g");
                    AddOptionIf("debugsymbols", "on", OptionType.lib_Options, "-gdwarf-2");

                    AddOptionIf("exceptions", "off", OptionType.cc_Options, "-Cpp_exceptions off");

                    ReplaceOptionIf("create-pch", "on", OptionType.cc_Options, "-o \"%objectfile%\"", "-precompile \"%pchfile%\"");
                    AddOptionIf("use-pch", "on", OptionType.cc_Options, "-include \"%pchfile%\"");

                    //In case precompiled header is defined directly through cc options
                    if (ContainsOption(OptionType.cc_Options, "-precompile"))
                    {
                        ConfigOptionSet.Options["create-pch"] = "on";
                    }

                    //-- Optimization --
                    AddOptionIf("optimization", "off", OptionType.cc_Options, "-O0");
                    AddOptionIf("optimization", "off", OptionType.cc_Options, "-inline off");

                    AddOptionIf("optimization", "on", OptionType.cc_Options, "-O4,s");     //full optimization for space
                    AddOptionIf("optimization", "on", OptionType.cc_Options, "-ipa file"); //per file level optimization

                    // ----- GENERAL OPTIONS ----- 
                    if (FlagEquals("misc", "on"))
                    {
                        AddOption(OptionType.cc_Options, "-msgstyle gcc");  // Controls the style used to show error and warning messages.
                        AddOption(OptionType.cc_Options, "-nowraplines");   // Controls the word wrapping of messages
                    }

                    // ----- OUTPUT OPTIONS ----- 
                    AddOptionIf("generatemapfile", "on", OptionType.link_Options, "-map \"%linkoutputmapname%\"");

                    if (FlagEquals("generatedll", "on"))
                    {
                        string sdkDir = WiiTaskUtil.GetSdkDir(Project, "RevolutionSDK");
                        AddOptionIf("usedebuglibs", "off", OptionType.link_Libraries, Path.Combine(sdkDir, @"RVL\lib\rso.a"));
                        AddOptionIf("usedebuglibs", "on", OptionType.link_Libraries, Path.Combine(sdkDir, @"RVL\lib\rsoD.a"));
                    }

                    break;
            }
        }


        protected void InternalInit()
        {
            if (ConfigSystem == "ps3" && ConfigOptionSet.Options["ps3-spu"] != null)
            {
                // SPU - use GCC
                ConfigCompiler = "gcc";
                ConfigPlatform = "ps3-gcc";
            }

            string property = PropertyUtil.GetPropertyOrDefault(Project, "eaconfig.default_warning_suppression", "on").ToLower();
            switch (property)
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
                        Log.WriteLineIf(Verbose, msg);
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

        protected bool IsVC8
        {
            get
            {
                return !((null == Properties["package.eaconfig.isusingvc8"]) || (Properties["package.eaconfig.isusingvc8"] == "false"));
            }
        }

        protected bool IsVC9
        {
            get
            {
                return !((null == Properties["package.eaconfig.isusingvc9"]) || (Properties["package.eaconfig.isusingvc9"] == "false"));
            }
        }

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



        private void SetOptionsString(OptionType type, string value)
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
            string option = _optionBuilder[(int)type].ToString();
            if (ContainsOption(type, oldValue))
            {
                _optionBuilder[(int)type].Replace(oldValue, newValue);
            }
            else
            {
                AddOption(type, newValue);
            }
        }

        private bool ContainsOption(OptionType type, string value)
        {
            string option = _optionBuilder[(int)type].ToString();
            return (!String.IsNullOrEmpty(option) && option.Contains(value));
        }


        protected bool FlagEquals(string name, string val)
        {

            return Flag(name).Equals(val, StringComparison.InvariantCultureIgnoreCase);
        }

        private string Flag(string name)
        {
            string option = ConfigOptionSet.Options[name];

            string prop = Properties["eaconfig." + name];
            if (!String.IsNullOrEmpty(prop))
            {
                if (prop.Equals("true", StringComparison.InvariantCultureIgnoreCase))
                {
                    option = "on";
                }
                else if (prop.Equals("false", StringComparison.InvariantCultureIgnoreCase))
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
            link_Libraries = 3,
            link_Librarydirs = 4,
            lib_Options = 5
        }

        private string[] _optionNames = new string[] { "cc.options", "cc.defines", "link.options", "link.libraries", "link.librarydirs", "lib.options" };
        private StringBuilder[] _optionBuilder = new StringBuilder[] { new StringBuilder(), new StringBuilder(), new StringBuilder(), new StringBuilder(), new StringBuilder(), new StringBuilder() };

    }
}
