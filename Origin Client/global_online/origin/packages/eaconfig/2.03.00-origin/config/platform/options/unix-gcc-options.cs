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

    [TaskName("unix-gcc-generate-options")]
    public class Unix_GCC_GenerateOptions : Unix_Common_Options
    {
    }

    [TaskName("unix64-gcc-generate-options")]
    public class Unix64_GCC_GenerateOptions : Unix_Common_Options
    {
    }

    public class Unix_Common_Options : GeneratePlatformOptions
    {
        protected override void SetPlatformOptions()
        {
            Common_GCC_Options();

            AddOptionIf("optimization", "off", OptionType.cc_Options, "-O0");
            AddOptionIf("enable.strict.aliasing", "on", OptionType.cc_Options, "-fstrict-aliasing");     // Enable strict aliasing
            AddOptionIf("enable.strict.aliasing", "off", OptionType.cc_Options, "-fno-strict-aliasing");

            AddOptionIf("debugsymbols", "on", OptionType.cc_Options, "-g3"); //We may want to use -Wshorten-64-to-32 and -Wconversion when using Mac osx in 64 bit mode.

            // Some versions of Runtime do not have MacOSX definition. Use integer value 6
            if (OsUtil.PlatformID == (int)OsUtil.OsUtilPlatformID.MacOSX)
            {
                AddOption(OptionType.cc_Defines, "EA_PLATFORM_OSX=1");
            }
            else if (OsUtil.PlatformID == (int)PlatformID.Unix)
            {
                AddOption(OptionType.cc_Defines, "EA_PLATFORM_LINUX=1");

                if (!FlagEquals("generatedll", "on"))
                {
                    AddOptionIf("optimization", "on", OptionType.link_Options, "-Wl,--gc-sections"); //strip out unused functions in the output binary and make it smaller
                }
            }

            if (Properties.GetBooleanPropertyOrDefault("cc.cpp11", true) || FlagEquals("cc.cpp11", "on"))
            {
                AddOptionIf("clanguage", "off", OptionType.cc_Options, "-std=c++0x");
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
            AddOption(OptionType.cc_Options, Project.Properties["package.UnixGCC.cc.options"]);
            AddOption(OptionType.cc_Options, "-MMD"); //Dump a .d file
            // ----- LINK OPTIONS -----

            AddOptionIf("exceptions", "on", OptionType.link_Librarydirs, Properties["link.exceptionlibrarydirs"]);
            AddOptionIf("exceptions", "off", OptionType.link_Options, "-fno-exceptions");
            AddOption(OptionType.link_Options, Project.Properties["package.UnixGCC.link.options"]);
            // ----- OUTPUT OPTIONS -----
            AddOptionIf("generatedll", "on", OptionType.link_Options, "-shared"); //Make a DLL, not an exe.
            AddOption(OptionType.link_Options, "-o \"%linkoutputname%\"");

            if (OsUtil.PlatformID == (int)OsUtil.OsUtilPlatformID.MacOSX) // we are on OSX
            {
                AddOptionIf("generatemapfile", "on", OptionType.link_Options, "-Wl,-map,\"%linkoutputmapname%\"");
            }
            else
            {
                AddOptionIf("generatemapfile", "on", OptionType.link_Options, "-Wl,-Map,\"%linkoutputmapname%\"");
            }
        }
    }
}
