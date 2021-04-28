using System;
using System.IO;
using System.Collections.Generic;

using NAnt.Core;
using NAnt.Core.Attributes;
using EA.Eaconfig.Core;
using EA.Eaconfig.Modules.Tools;

namespace EA.Eaconfig
{
    [TaskName("fixupvcxproj")]
    public class FixupVcxproj : IMCPPVisualStudioExtension
    {
        public override void AddPropertySheetsImportList(List<Tuple<string, string, string>> propSheetList)
        {
            string props = Project.Properties["package.dir"] + @"\scripts\BlazeServerPropertySheet.props";
            propSheetList.Add(new Tuple<string, string, string>(props, @"exists('" + props + "')", null));
        }

        public override void WriteExtensionCompilerToolProperties(CcCompiler cc, FileItem file, IDictionary<string, string> toolProperties)
        {
            if (toolProperties.ContainsKey("SDLCheck"))
            {
                toolProperties["SDLCheck"] = "";
            }
        }
    }
}

