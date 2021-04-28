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
    public class VC_Common_Options : GeneratePlatformOptions
    {
        protected override void SetPlatformOptions()
        {
            // Incredilink support:
            if (PropertyUtil.GetBooleanProperty(Project, "incredibuild.xge.useincredilink"))
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
                                string msg = Error.Format(Project, "GenerateBuildOptionset", "WARNING", "Optionset '{0}': -O2 optimization option is incompatible with edit-and-continue -ZI option. Edit-and-continue will be ignored", ConfigSetName);
                                Log.Warning.WriteLine(msg);
                            }

                            AddOption(OptionType.cc_Options, "-Zi");
                        }
                        else
                        {
                            if (FlagEquals("editandcontinue", "on"))
                            {
                                if (ContainsOption(OptionType.cc_Options, "-Zi"))
                                {
                                    string msg = Error.Format(Project, "GenerateBuildOptionset", "WARNING", "Optionset '{0}': -Zi option is already present, it is incompatible with edit-and-continue -ZI option. Edit-and-continue will be ignored", ConfigSetName);
                                    Log.Warning.WriteLine(msg);
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
                    string msg = Error.Format(Project, "GenerateBuildOptionset", "WARNING", "Optionset '{0}': c7debugsymbols='on', but debugsymbols is 'off', no -Z7 option will be set, no debug symbols generated.", ConfigSetName);
                    Log.Warning.WriteLine(msg);
                }
            }

            if (FlagEquals("clanguage-strict", "on"))
            {
                AddOptionIf("clanguage", "on", OptionType.cc_Options, "-TC");
            }

            AddOptionIf("rtti", "off", OptionType.cc_Options, "-GR-"); // Disable run time type information generation (overwrite vs2005 default behaviour, which is -GR)
            AddOptionIf("rtti", "on", OptionType.cc_Options, "-GR"); // Enable run time type information generation
            if (FlagEquals("exceptions", "on"))
            {
                //In VS 2005, using /clr (Common Language Runtime Compilation) implies /EHa
                //(/clr /EHa is redundant). The compiler will generate an error if /EHs[c]
                //is used after /clr.
                //See http://msdn2.microsoft.com/en-us/library/1deeycx5.aspx for more
                //details on the various Visual C++ exception handling modes.
                if (FlagEquals("toolconfig", "on") || ContainsOption(OptionType.cc_Options, "clr"))
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
            var useMpThreads = Properties["eaconfig.build-MP"];

            if (useMpThreads != null)
            {
                useMpThreads = useMpThreads.TrimWhiteSpace();

                if (String.IsNullOrEmpty(useMpThreads) || (useMpThreads.IsOptionBoolean() && useMpThreads.OptionToBoolean()))
                {
                    AddOption(OptionType.cc_Options, "-MP");
                }
                else
                {
                    int threads;
                    if (!Int32.TryParse(useMpThreads, out threads))
                    {
                        Log.Warning.WriteLine("eaconfig.build-MP='{0}' value is invalid, it can be empty or contain integer number of threads to use with /MP option : /MP${{eaconfig.build-MP}}. Adding '/MP' option with no value.", useMpThreads);

                        useMpThreads = String.Empty;
                    }
                    AddOption(OptionType.cc_Options, "-MP" + useMpThreads);
                }
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
                if (FlagEquals("managedcpp", "on"))
                {
                    AddOptionIf("multithreadeddynamiclib", "on", OptionType.link_Options, "-nodefaultlib:libcmt.lib");   // In case MCPP is linked with static lib that uses -MTd
                }
            }
            else
            {
                AddOptionIf("multithreadeddynamiclib", "off", OptionType.cc_Options, "-MTd");  // link with multithreaded library  (debug version)
                AddOptionIf("multithreadeddynamiclib", "on", OptionType.cc_Options, "-MDd");   // link with multithreaded dll library (debug version)
                if (FlagEquals("managedcpp", "on"))
                {
                    AddOptionIf("multithreadeddynamiclib", "on", OptionType.link_Options, "-nodefaultlib:libcmtd.lib");   // In case MCPP is linked with static lib that uses -MTd
                }
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
            AddOption(OptionType.lib_Options, "-out:\"%liboutputname%\"");

            // ----- WARNING OPTIONS ----- 
            AddOptionIf("warningsaserrors", "on", OptionType.cc_Options, "-WX"); // Treat warnings as errors 
            AddOptionIf("warningsaserrors", "off", OptionType.cc_Options, "-WX-"); // Disable Treat warnings as errors 

            if (FlagEquals("warnings", "on"))
            {
                
                PrependOption(OptionType.link_Options, "-ignore:4089");  //Disable warning: dll discarded by OPT:ref option

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
                    PrependOption(OptionType.cc_Options, "-wd4826");   //  Conversion from 'char *' to 'uint64_t' is sign-extended.
                    PrependOption(OptionType.cc_Options, "-wd4692");   //  'function': signature of non-private member contains assembly private native type 'native_type'
                    PrependOption(OptionType.cc_Options, "-wd4996");   //  Disable warning: 'strncpy' was declared deprecated.
                    PrependOption(OptionType.cc_Options, "-wd4746");   //  Disable warning:  volatile access order isn't guaranteed.

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
                
                PrependOption(OptionType.cc_Options, "-Wall");  //Set on all warnings (some default to off) 
                PrependOption(OptionType.cc_Options, "-W4");    // Set maximum warning level
            }
            if (FlagEquals("clanguage", "off"))
            {
                AddOptionIf("windowsruntime", "on", OptionType.cc_Options, "-ZW"); // Enable Windows Runtime extensions
            }
        }
    }
}
