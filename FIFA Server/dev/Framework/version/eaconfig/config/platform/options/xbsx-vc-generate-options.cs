// (c) Electronic Arts. All rights reserved.

using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Logging;
using EA.Eaconfig;

using System;
using System.Collections;
using System.Collections.Generic;
using System.Text;
using System.IO;

namespace EA.Eaconfig
{
	[TaskName("xbsx-vc-generate-options", XmlSchema = false)]
	public class Xbsx_VC_GenerateOptions : VC_Common_Options
	{
		protected override void SetPlatformOptions()
		{
			if (FlagEquals("retailflags", "on") && ConfigOptionSet.Options["optimization.ltcg"] == null)
			{
				ConfigOptionSet.Options["optimization.ltcg"] = "on";
			}
			SetCommonOptions();

			AddOptionIf("profile", "on", OptionType.cc_Defines, "PROFILE_BUILD");
			AddOptionIf("debugsymbols", "off", OptionType.cc_Options, "-Fd\" \"");

			if (FlagEquals("usedebuglibs", "on") || FlagEquals("profile", "on"))
			{
				if (Properties.Contains("package.GDK.profilelib"))
				{
					foreach (var lib in Properties["package.GDK.profilelib"].LinesToArray())
					{
						AddOption(OptionType.link_Libraries, lib);
					}
				}
			}

			// ----- GENERAL OPTIONS ----- 
			AddOption(OptionType.cc_Options, "-bigobj"); // Allows object files to hold more than 2^16 addressable sections.
			AddOption(OptionType.cc_Options, "-arch:AVX2"); // Defaults to SSE2 on x64, MS recommends AVX2 for Xbsx
			AddOption(OptionType.cc_Defines, "_UITHREADCTXT_SUPPORT=0"); // Defines a property added by Visual Studio upon boot which unless defined will prevent builds from succeeding if they depend on ppltasks.h

			int vsversion = 2019;
			if (Int32.TryParse(Properties.GetPropertyOrDefault("vsversion."+Properties["config-system"], Properties.GetPropertyOrDefault("vsversion", "2019")), out vsversion))
			{
				if (vsversion >= 2019)
				{
					AddOptionIf("optimization.ltcg", "on", OptionType.link_Options, "-d2:-vzeroupper"); // MS docs state that if you are using Whole Program Optimization (WPO) / Link Time Code Generation (LTCG) then you need to specify /d2:-vzeroupper to the linker
					AddOption(OptionType.cc_Options, "-d2vzeroupper"); // With VS 2019 Update 3 or later MS recommends d2vzeroupper
				}
			}
		}
	}
}
