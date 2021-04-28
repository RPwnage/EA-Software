using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

using NAnt.Core;
using EA.FrameworkTasks.Model;

namespace EA.Eaconfig.Backends.VisualStudio
{

    internal class VS2010FSProject : VS2008FSProject
    {
        internal VS2010FSProject(VSSolutionBase solution, IEnumerable<IModule> modules)
            : base(solution, modules)
        {
        }

        protected override string DefaultTargetFrameworkVersion
        {
            get { return "v4.0"; }
        }

        protected override string ToolsVersion
        {
            get { return "4.0"; }
        }

        protected override string ProductVersion
        {
            get { return "8.0.30703"; }
        }

        protected override string Version
        {
            get
            {
                return "10.00";
            }
        }

        protected override void WriteImportStandardTargets(IXmlWriter writer)
        {
            writer.WriteStartElement("Import");
            writer.WriteAttributeString("Project", @"$(MSBuildExtensionsPath32)\FSharp\1.0\Microsoft.FSharp.Targets");
            writer.WriteAttributeString("Condition", @"!Exists('$(MSBuildBinPath)\Microsoft.Build.Tasks.v4.0.dll')");
            writer.WriteEndElement(); //Import

            writer.WriteStartElement("Import");
            writer.WriteAttributeString("Project", @"$(MSBuildExtensionsPath32)\..\Microsoft F#\v4.0\Microsoft.FSharp.Targets");
            writer.WriteAttributeString("Condition", @" Exists('$(MSBuildBinPath)\Microsoft.Build.Tasks.v4.0.dll')");
            writer.WriteEndElement(); //Import
        }
    }
}
