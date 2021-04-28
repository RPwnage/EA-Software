// (c) Electronic Arts. All rights reserved.

using System;
using System.Xml;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using Microsoft.Win32;

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Tasks;
using NAnt.Core.Attributes;
using NAnt.Core.Events;
using NAnt.Core.Functions;
using NAnt.Core.Logging;
using NAnt.Core.Writers;
using EA.FrameworkTasks.Model;

using EA.Eaconfig;
using EA.Eaconfig.Core;
using EA.Eaconfig.Build;
using EA.Eaconfig.Modules;
using EA.Eaconfig.Modules.Tools;
using Model=EA.FrameworkTasks.Model;

namespace EA.EAConfig
{
	[TaskName("ps5-clang-visualstudio-extension")]
	public class Ps5VisualStudioExtension : IMCPPVisualStudioExtension
	{
		public override void UserData(IDictionary<string, string> userData)
		{
			if (Module.IsKindOf(Model.Module.Program))
			{
				userData["DebuggerFlavor"] = "ProsperoDebugger";
                Module.Package.Project.Properties[Module.GroupName + ".workingdir"] = "$(OutDir)";
			}
		}

		public override void ProjectGlobals(IDictionary<string, string> projectGlobals)
		{
			// The environment override needs to be in Global scope.  There seems to be tasks needing
			// that info outside the config/platform scope.
			if (!projectGlobals.ContainsKey("SCE_PROSPERO_SDK_DIR"))
			{
				bool isPortable = Module.Package.Project.Properties.GetBooleanPropertyOrDefault("eaconfig.generate-portable-solution", false);
				if (!isPortable)
				{
					var sdkdir = Module.Package.Project.Properties["package.ps5sdk.sdkdir"].TrimWhiteSpace();
					if (!String.IsNullOrEmpty(sdkdir))
					{
						projectGlobals.Add("SCE_PROSPERO_SDK_DIR", PathNormalizer.Normalize(sdkdir));
					}
				}
			}
		}

		// JL: After adding SCE_ORBIS_SDK_DIR in the Globals section above, we probably don't need the following block any more.
		// As the default Orbis MSBuild setup already has that in the ExecutablePath.  But for now, we'll keep the following code
		// to be on the safe side.
		public override void WriteExtensionItems(IXmlWriter writer)
		{
			writer.WriteStartElement("PropertyGroup");
			writer.WriteAttributeString("Condition", Generator.GetConfigCondition(Module.Configuration));
			{
				var sdkdir = Module.Package.Project.Properties["package.ps5sdk.sdkdir"].TrimWhiteSpace();
				if (!String.IsNullOrEmpty(sdkdir))
				{
					bool isPortable = Module.Package.Project.Properties.GetBooleanPropertyOrDefault("eaconfig.generate-portable-solution", false);
					if (!isPortable)
					{
						writer.WriteElementString("SCE_PROSPERO_SDK_DIR", PathNormalizer.Normalize(sdkdir));
					}
					writer.WriteElementString("ExecutablePath", @"$(SCE_PROSPERO_SDK_DIR)\host_tools\bin;$(ExecutablePath)");
				}

				// Temporary fix for an issue where header file changes may not trigger a compilation. Should be fixed in a future SDK.
				// https://p.siedev.net/technotes/view/20
				writer.WriteElementString("ContinueCompilingOnError", "true");
			}
			writer.WriteEndElement();
		}
	}
}
