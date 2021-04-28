// (c) Electronic Arts. All rights reserved.

using System;

using NAnt.Core;
using NAnt.Core.Tasks;
using NAnt.Core.Attributes;
using NAnt.Core.Logging;

namespace EA.Eaconfig
{

	[TaskName("unix-clang-generate-options", XmlSchema = false)]
	public class Unix_Clang_GenerateOptions : Unix_Clang_Common_Options
	{
	}

	[TaskName("unix64-clang-generate-options", XmlSchema = false)]
	public class Unix64_Clang_GenerateOptions : Unix_Clang_Common_Options
	{
	}

	[TaskName("unix-x64-clang-generate-options", XmlSchema = false)]
	public class Unix_x64_Clang_GenerateOptions : Unix_Clang_Common_Options
	{
	}

	public class Unix_Clang_Common_Options : GeneratePlatformOptions
	{
		protected override void SetPlatformOptions()
		{
			Common_Clang_Options();

			if (FlagEquals("clanguage", "off"))
			{
				SetupStandardLibraryOptions();
			}

			if (FlagEquals("debugsymbols", "on"))
			{
				switch (OsUtil.PlatformID)
				{
					case (int)OsUtil.OsUtilPlatformID.MacOSX:
						AddOption(OptionType.cc_Options, "-glldb");
						break;
					default:
						AddOption(OptionType.cc_Options, "-ggdb");
						break;
				}
			}

			AddOptionIf("enable.strict.aliasing", "on", OptionType.cc_Options, "-fstrict-aliasing");     // Enable strict aliasing
			AddOptionIf("enable.strict.aliasing", "off", OptionType.cc_Options, "-fno-strict-aliasing");

			// Some versions of Runtime do not have MacOSX definition. Use integer value 6
			if (OsUtil.PlatformID == (int)OsUtil.OsUtilPlatformID.MacOSX)
			{
				AddOption(OptionType.cc_Defines, "EA_PLATFORM_OSX=1");
			}
			else if (OsUtil.PlatformID == (int)PlatformID.Unix || 
					 OsUtil.PlatformID == (int)PlatformID.Win32NT)    // We're doing unix cross build on PC.
			{
				AddOption(OptionType.cc_Defines, "EA_PLATFORM_LINUX=1");

				if (!FlagEquals("generatedll", "on"))
				{
					AddOptionIf("optimization", "on", OptionType.link_Options, "-Wl,--gc-sections"); //strip out unused functions in the output binary and make it smaller
				}
			}

			// Enable the clang timing report to help diagnose compile-time performance issues
			AddOptionIf("buildtiming", "on", OptionType.cc_Options, "-ftime-report");

			if (FlagEquals("optimization", "on"))
			{
				AddOption(OptionType.cc_Options, "-O2");
			}
			else if (FlagEquals("optimization", "off"))
			{
				AddOption(OptionType.cc_Options, "-O0");
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
			AddOption(OptionType.cc_Options, Project.Properties["package.UnixClang.cc.options"]);
			AddOption(OptionType.cc_Options, "-MD"); //Dump a .d file

			// ----- LINK OPTIONS -----
			AddOptionIf("exceptions", "on", OptionType.link_Librarydirs, Properties["link.exceptionlibrarydirs"]);
			AddOptionIf("exceptions", "off", OptionType.link_Options, "-fno-exceptions");
			AddOption(OptionType.link_Options, Project.Properties["package.UnixClang.link.options"]);
			AddOptionIf("exportdynamic", "on", OptionType.link_Options, "-Wl,--export-dynamic");

			// ----- OUTPUT OPTIONS -----
			AddOptionIf("generatedll", "on", OptionType.link_Options, "-shared"); //Make a DLL, not an exe.
			// for shared lib, we want to add -soname linker (ld) argument to set the internal DT_SONAME field to the specified name 
			// (which is %outputname%.so in our case).  Any executable linked with a shared lib which has this DT_SONAME field will
			// attempt to load the shared with the name specified by DT_SONAME during runtime.  Otherwise, the executable will attempt 
			// to use the full path given to the linker (which is not desirable).
			AddOptionIf("generatedll", "on", OptionType.link_Options, "-Wl,-soname,%outputname%" + Project.Properties["dll-suffix"]);
			AddOption(OptionType.link_Options, "-o \"%linkoutputname%\"");
			AddOption(OptionType.link_Options, "-Wl,-rpath,'$ORIGIN/'");	// Allow binary to search for shared library from the binary's folder.

			if (OsUtil.PlatformID == (int)OsUtil.OsUtilPlatformID.MacOSX) // we are on OSX
			{
				AddOptionIf("generatemapfile", "on", OptionType.link_Options, "-Wl,-map,\"%linkoutputmapname%\"");
			}
			else
			{
				AddOptionIf("generatemapfile", "on", OptionType.link_Options, "-Wl,-Map,\"%linkoutputmapname%\"");
			}
		}

		private void SetupStandardLibraryOptions()
		{
			// Choose between clang's libc++ vs gcc's libstdc++
			bool unixClangPackageSupportLibcxx = false;
			if (PropertyUtil.GetPropertyOrDefault(Project, "package.UnixClang.version", "0.0.0").StrCompareVersions("3.8.1") >= 0)
			{
				if (NAnt.Core.Util.PlatformUtil.IsWindows)
				{
					if (PropertyUtil.GetPropertyOrDefault(Project, "package.UnixCrossTools.version", "0.0.0").StrCompareVersions("2.00.00") >= 0)
					{
						unixClangPackageSupportLibcxx = true;
					}
				}
				else
				{
					unixClangPackageSupportLibcxx = true;
				}
			}
			if (unixClangPackageSupportLibcxx)
			{
				// The same switch need to be used for both compiler and linker.  The compiler need to use the correct
				// include files (typically in include\c++\v1) and the linker need to link with the correct library
				// typically libc++*.*.  Note that if the -stdlib switch is not specified, clang will use GNU's libstdc++ by
				// default.
				string unix_stdlib_prop_value = PropertyUtil.GetPropertyOrDefault(Project, "unix.stdlib", string.Empty);
				if (!string.IsNullOrEmpty(unix_stdlib_prop_value))
				{
					AddOption(OptionType.cc_Options, "-stdlib=" + unix_stdlib_prop_value);
					AddOption(OptionType.link_Options, "-stdlib=" + unix_stdlib_prop_value);
				}
				else if (ConfigOptionSet.Options.Contains("unix.stdlib"))
				{
					AddOption(OptionType.cc_Options, "-stdlib=" + ConfigOptionSet.Options["unix.stdlib"]);
					AddOption(OptionType.link_Options, "-stdlib=" + ConfigOptionSet.Options["unix.stdlib"]);
				}
				else
				{
					AddOption(OptionType.cc_Options, "-stdlib=libc++");
					AddOption(OptionType.link_Options, "-stdlib=libc++");
				}
			}
		}
	}
}
