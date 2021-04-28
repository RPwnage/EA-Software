// (c) Electronic Arts. All rights reserved.

using NAnt.Core.Util;
using NAnt.Core.Attributes;

using EA.Eaconfig.Core;
using EA.Eaconfig.Modules.Tools;
using System.Collections.Generic;
using NAnt.Core;
using NAnt.Core.Logging;

namespace EA.Eaconfig
{
	[TaskName("gcc-lib-postprocess", XmlSchema = false)]
	class GCC_LIB_PreprocessOptions : AbstractModuleProcessorTask
	{
		public override void ProcessTool(Linker link)
		{
			List<FileSetItem> to_remove = new List<FileSetItem>();

			foreach (var lib in link.Libraries.Includes)
			{
				if (lib.AsIs && (lib.Pattern.Value.StartsWith("-l") || lib.Pattern.Value.StartsWith("-Wl,")))
				{
					if (!link.Options.Contains(lib.Pattern.Value))
					{
						link.Options.Add(lib.Pattern.Value);
					}
					to_remove.Add(lib);
				}
			}

			if (to_remove.Count > 0)
			{
				foreach (var lib in to_remove)
				{
					link.Libraries.Includes.Remove(lib);
				}
				if (Log.WarningLevel >= Log.WarnLevel.Advise)
				{
					Log.Status.WriteLine(
						"[warning] moved " + to_remove.Count + " options from link.Libraries fileset to link.options optionset "
						+ "to prevent framework from treating these as files when checking dep file for changed dependents.");
				}
			}
		}
	}
}
