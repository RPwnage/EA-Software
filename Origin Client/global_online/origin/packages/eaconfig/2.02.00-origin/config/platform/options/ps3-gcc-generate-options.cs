using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Attributes;
using NAnt.Core.Logging;

using System;
using System.Collections;
using System.Collections.Generic;
using System.Text;
using System.IO;
using System.Linq;

namespace EA.Eaconfig
{
    [TaskName("ps3-gcc-generate-options")]
    public class PS3_gcc_GenerateOptions : GeneratePlatformOptions
    {
        protected override void SetPlatformOptions()
        {
            SetPlatformOptions_GCC();
            SetSecureLinkoutputname();
        }
        protected void SetPlatformOptions_GCC()
        {
            Common_GCC_Options();

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
            AddOptionIf("stripunusedsymbols", "on", OptionType.link_Options, "--strip-unused-data");
            AddOptionIf("stripunusedsymbols", "on", OptionType.link_Options, "--strip-duplicates");

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
            AddOption(OptionType.lib_Options, "-rs \"%liboutputname%\"");
            AddOptionIf("generatemapfile", "on", OptionType.link_Options, "-Wl,-Map,\"%linkoutputmapname%\"");

            // ----- WARNING OPTIONS ----- 
            AddOptionIf("exceptions", "on", OptionType.link_Librarydirs, Properties["link.exceptionlibrarydirs"]);


        }

        protected void SetSecureLinkoutputname()
        {
            if (ConfigOptionSet.Options["ps3-spu"] == null)
            {
                string options;
                if (ConfigOptionSet.Options.TryGetValue("link.options", out options))
                {
                    if (-1 != options.IndexOf("-oformat=fself") || -1 != options.IndexOf("-oformat=fsprx"))
                    {
                        // Do some checking for backwards compatibility:
                        if (!IsObsoleteLinkOutputNameSettings(options))
                        {
                            // Anoter ccheck for possible incorrect input: linkoutput name is ovewritten and looks like intent is to use it in place of securedlinkoutputname
                            if (!IsLinkoutputNameOverWritten(options))
                            {
                                ConfigOptionSet.Options["linkoutputname"] = ConfigOptionSet.Options["securelinkoutputname"];
                            }
                        }

                        ConfigOptionSet.Options["link.postlink.commandline"] = String.Empty;
                    }
                }
            }
        }

        private bool IsLinkoutputNameOverWritten(string options)
        {
            string[] standard_outputs = new string[] { @"%intermediatedir%\%outputname%", @"%intermediatedir%/%outputname%", @"%outputdir%\\%outputname%", @"%outputdir%/%outputname%" };

            var linkoutputname = ConfigOptionSet.Options["linkoutputname"].TrimWhiteSpace();

            if (!String.IsNullOrEmpty(linkoutputname))
            {
                // Do checking for data integrity:
                string suffix = String.Empty;
                string suffix_secure = String.Empty;
                if (-1 != options.IndexOf("-oformat=fself"))
                {
                    suffix = Project.Properties["exe-suffix"];
                    suffix_secure = Project.Properties["secured-exe-suffix"];
                }
                else if (-1 != options.IndexOf("-oformat=fsprx"))
                {
                    suffix = Project.Properties["dll-suffix"];
                    suffix_secure = Project.Properties["secured-dll-suffix"];
                }

                var securelinkoutputname = ConfigOptionSet.Options["securelinkoutputname"].TrimWhiteSpace();

                bool standardLinkoutputname = standard_outputs.Any(s => s != null && s + suffix == linkoutputname);
                bool standardSecuredLinkoutputname = standard_outputs.Any(s => s != null && s + suffix_secure == securelinkoutputname);

                if (!standardLinkoutputname && standardSecuredLinkoutputname)
                {
                    ConfigOptionSet.Options["linkoutputname"] = linkoutputname.Replace(suffix, suffix_secure);

                    if (Log.WarningLevel >= Log.WarnLevel.Advise)
                    {
                        Project.Log.Warning.WriteLine("BuldOptionSet '{0}' has custom 'linkoutputname'='{1}' option, but it looks like 'securedlinkoutputname'='{2}' was not customized. Framework will use custom 'linkoutputname' to construct secured output='{3}'.",
                            ConfigOptionSet.Options["name"], linkoutputname, securelinkoutputname, ConfigOptionSet.Options["linkoutputname"]);
                    }
                    return true;
                }
            }
            return false;
        }

        private bool IsObsoleteLinkOutputNameSettings(string options)
        {
            // Test for obsolete 'linkoutputnameself' and 'linkoutputnamesprx'
            string oformat_secure = String.Empty;
            string linkoutput_secure = String.Empty;
            string suffix = String.Empty;
            string suffix_secure = String.Empty;

            if (-1 != options.IndexOf("-oformat=fself"))
            {
                oformat_secure = "-oformat=fself";
                linkoutput_secure = "linkoutputnameself";
                suffix = Project.Properties["exe-suffix"];
                suffix_secure = Project.Properties["secured-exe-suffix"];
            }
            else if (-1 != options.IndexOf("-oformat=fsprx"))
            {
                oformat_secure = "-oformat=fsprx";
                linkoutput_secure = "linkoutputnamesprx";
                suffix = Project.Properties["dll-suffix"];
                suffix_secure = Project.Properties["secured-dll-suffix"];
            }

            if (!String.IsNullOrEmpty(oformat_secure))
            {
                if (ConfigOptionSet.Options.ContainsKey(linkoutput_secure))
                {
                    var linkoutputname = ConfigOptionSet.Options["linkoutputname"].TrimWhiteSpace();
                    var linkoutputnamesecure = ConfigOptionSet.Options[linkoutput_secure].TrimWhiteSpace();
                    var standard_linkoutputname = @"%outputdir%/%outputname%" + suffix;
                    var standard_linkoutputname1 = @"%outputdir%\%outputname%" + suffix;
                    var standard_linkoutputname2 = @"%intermediatedir%/%outputname%" + suffix;
                    var standard_linkoutputname3 = @"%intermediatedir%\%outputname%" + suffix;


                    if (String.IsNullOrEmpty(linkoutputname) || (linkoutputname.Contains(standard_linkoutputname) || linkoutputname.Contains(standard_linkoutputname1) || linkoutputname.Contains(standard_linkoutputname2) || linkoutputname.Contains(standard_linkoutputname3)))
                    {
                        ConfigOptionSet.Options["linkoutputname"] = ConfigOptionSet.Options[linkoutput_secure].TrimWhiteSpace();
                    }
                    else
                    {
                        ConfigOptionSet.Options["linkoutputname"] = linkoutputname.Replace(suffix, suffix_secure);
                    }

                    if (Log.WarningLevel >= Log.WarnLevel.Deprecation)
                    {
                        Project.Log.Warning.WriteLine("BuldOptionSet '{0}' uses deprecated option'{1}'='{2}'. This option is deprecated 'linkoutputnameself' and 'linkoutputnamesprx' were replaced by cross-platform option 'securelinkoutputname'.", ConfigOptionSet.Options["name"], linkoutput_secure, linkoutputnamesecure);
                    }
                    return true;
                }
            }

            return false;
        }
    }
}
