// (c) Electronic Arts. All rights reserved.

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
	[TaskName("pc-vc-generate-options", XmlSchema = false)]
	public class PC_VC_GenerateOptions : VC_Common_Options
	{
		protected override void SetPlatformOptions()
		{
			base.SetPlatformOptions();

			if (FlagEquals("retailflags", "on") && ConfigOptionSet.Options["optimization.ltcg"] == null && !PropertyUtil.GetBooleanProperty(Project, "frostbite.nomaster_in_solution"))
			{
				ConfigOptionSet.Options["optimization.ltcg"] = "on";
			}

			// ----- OPTIMIZATION OPTIONS ----- 
			if (FlagEquals("optimization", "on"))
			{
				AddOption(OptionType.cc_Options, (Project.Properties["disable-security-check-option"] ?? "-sdl-").ToArray().ToString(Environment.NewLine));// Disable SDL checking
			}

			// ----- GENERAL OPTIONS ----- 
			AddOptionIf("analyze", "build", OptionType.cc_Options, "-analyze");
			AddOptionIf("analyzewarnings", "on", OptionType.cc_Options, "-analyze:WX");
			AddOptionIf("analyzewarnings", "off", OptionType.cc_Options, "-analyze:WX-");
			AddOptionIf("general-member-fn-ptr-representation", "on", OptionType.cc_Options, "-vmg");

			// ----- WARNING OPTIONS ----- 
			AddOptionIf("disable-crt-secure-warnings", "on", OptionType.cc_Defines, "_CRT_SECURE_NO_WARNINGS");
		}
	}
}
