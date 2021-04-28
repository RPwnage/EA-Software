using System;
using System.Xml;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Attributes;
using NAnt.Core.Functions;
using NAnt.Core.Logging;
using NAnt.Core.Reflection;
using EA.FrameworkTasks.Model;

using EA.Eaconfig;
using EA.Eaconfig.Core;
using EA.Eaconfig.Build;
using EA.Eaconfig.Modules;
using EA.Eaconfig.Backends;

namespace EA.Eaconfig.Backends.VisualStudio
{
    internal class VS2010Solution : VSSolutionBase
    {

        public VS2010Solution(Log log, string name, PathString outputdir)
            : base(log, name, outputdir)
        {
            VSConfig.config_vs_version = "10.0";
        }

        protected override ModuleGenerator CreateModuleGenerator(IEnumerable<IModule> modules)
        {
            switch (GetVSProjectType(modules))
            {
                case VSProjectTypes.CSharp:
                    return new VS2010CSProject(this, modules);
                case VSProjectTypes.FSharp:
                    return new VS2010FSProject(this, modules);
                case VSProjectTypes.Python:
                    return new VS2010PythonProject(this, modules);
                case VSProjectTypes.ExternalProject:
                    return new ExternalVSProject(this, modules);
                default:
                    return new VS2010Project(this, modules);
            }
        }

        #region Virtual Overrides for Writing Solution File

        protected override void WriteHeader(IMakeWriter writer)
        {
            writer.WriteLine("Microsoft Visual Studio Solution File, Format Version 11.00");
            writer.WriteLine("# Visual Studio 2010");
        }

        protected override string TeamTestSchemaVersion
        {
            get { return "2010"; }
        }

        #endregion Virtual Overrides for Writing Solution File

    }
}
