// (c) Electronic Arts. All rights reserved.

using NAnt.Core;
using NAnt.Core.Attributes;

using System;
using System.IO;
using System.Linq;
using System.Text;

namespace EA.EAConfig
{
	[TaskName("get-ps5-ltcglib-property")]
	public class GetPs5LtcgLibProperty : Task
	{
		static string gNonFrostbiteLtcgLibs = null;
		static string gFrostbiteLtcgLibs = null;

		[TaskAttribute("ForFrostbite", Required = true)]
		public bool ForFrostbite { get; set; }

		protected override void ExecuteTask()
		{
			if (!Project.Properties.Contains("config-system") || Project.Properties["config-system"] != "ps5")
			{
				throw new BuildException("The task 'get-ps5-ltcg-libset' is expected to be executed under ps5 config only!");
			}

			if (ForFrostbite && gFrostbiteLtcgLibs != null)
			{
				Project.Properties["platform.sdklibs.ltcglibs"] = gFrostbiteLtcgLibs;
				return;
			}
			else if (!ForFrostbite && gNonFrostbiteLtcgLibs != null)
			{
				Project.Properties["platform.sdklibs.ltcglibs"] = gNonFrostbiteLtcgLibs;
				return;
			}

			// The .ltcglibs setup is based from platform.sdklibs.regular.  The fileset in this property is different depending on
			// under Frostbite config or non-Frostbite config.  So we setup two cache properties for the different scenario.
			// Aside from frostbite vs non-frostbite, we don't expect that the platform.sdklibs.regular being defined differently 
			// with different configs or different projects (ie different build files)!
			if (!Project.Properties.Contains("platform.sdklibs.regular"))
			{
				throw new BuildException("INTERNAL ERROR: Setting up property 'platform.sdklibs.ltcglibs' requires 'platform.sdklibs.regular' to be defined first!");
			}
			StringBuilder finalLibsSet = new StringBuilder();
			string currentRegularLibs = Project.Properties["platform.sdklibs.regular"];
			foreach (string libfile in currentRegularLibs.Split(new char[] { '\n' }).Select(ln => ln.Trim()).Where(ln => !String.IsNullOrEmpty(ln)))
			{
				string possibleLtcgFile = libfile.Replace(".a", "_lto.a");
				if (File.Exists(possibleLtcgFile))
				{
					finalLibsSet.AppendLine(possibleLtcgFile);
				}
				else
				{
					finalLibsSet.AppendLine(libfile);
				}
			}

			if (ForFrostbite)
			{
				gFrostbiteLtcgLibs = finalLibsSet.ToString();
				Project.Properties["platform.sdklibs.ltcglibs"] = gFrostbiteLtcgLibs;
			}
			else
			{
				gNonFrostbiteLtcgLibs = finalLibsSet.ToString();
				Project.Properties["platform.sdklibs.ltcglibs"] = gNonFrostbiteLtcgLibs;
			}
		}
	}
}



