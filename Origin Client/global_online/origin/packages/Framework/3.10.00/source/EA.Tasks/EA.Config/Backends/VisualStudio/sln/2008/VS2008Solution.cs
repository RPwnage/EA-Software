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
    internal class VS2008Solution : VSSolutionBase
    {

        public VS2008Solution(Log log, string name, PathString outputdir)
            : base(log, name, outputdir)
        {
            VSConfig.config_vs_version = "9.0";
        }

        protected override ModuleGenerator CreateModuleGenerator(IEnumerable<IModule> modules)
        {
            switch (GetVSProjectType(modules))
            {
                case VSProjectTypes.CSharp:
                    return new VS2008CSProject(this, modules);
                case VSProjectTypes.FSharp:
                    return new VS2008FSProject(this, modules);
                case VSProjectTypes.ExternalProject:
                    return new ExternalVSProject(this, modules);
                case VSProjectTypes.Python:
                    Log.Error.WriteLine("Python project generation is not supported in visual studio 2008");
                    throw new Exception();
                default:
                    return new VS2008Project(this, modules);
            }
        }

        #region Virtual Overrides for Writing Solution File

        protected override void WriteHeader(IMakeWriter writer)
        {
            writer.WriteLine("Microsoft Visual Studio Solution File, Format Version 10.00");
            writer.WriteLine("# Visual Studio 2008");
        }

        protected override string TeamTestSchemaVersion
        {
            get { return "2006"; }
        }

        #endregion Virtual Overrides for Writing Solution File

    }
}
