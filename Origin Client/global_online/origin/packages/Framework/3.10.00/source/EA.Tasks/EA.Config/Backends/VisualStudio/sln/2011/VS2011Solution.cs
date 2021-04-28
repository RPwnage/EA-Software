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
    internal class VS2011Solution : VSSolutionBase
    {

        public VS2011Solution(Log log, string name, PathString outputdir)
            : base(log, name, outputdir)
        {
            VSConfig.config_vs_version = "11.0";
        }

        protected override ModuleGenerator CreateModuleGenerator(IEnumerable<IModule> modules)
        {
            switch (GetVSProjectType(modules))
            {
                case VSProjectTypes.CSharp:
                    return new VS2011CSProject(this, modules);
                case VSProjectTypes.FSharp:
                    return new VS2011FSProject(this, modules);
                case VSProjectTypes.Python:
                    return new VS2011PythonProject(this, modules);
                case VSProjectTypes.ExternalProject:
                    return new ExternalVSProject(this, modules);
                default:
                    return new VS2011Project(this, modules);
            }
        }


        //The vsmdi and testrunconfig files we get generated are not needed by vs2012 
        //(the test explorer works by using reflection on the built assemblies). Having them in the 
        //solution bring up another unit test UI which does not work. I would suggest removing the generation
        //of these files for vs2012. Just ignoring the ui or deleting those files solves the issue for me.
        protected override bool WriteUnitTestsDefinitions(IMakeWriter writer, out string vsdmiName)
        {
            vsdmiName = null;
            return false;
        }

        #region Virtual Overrides for Writing Solution File

        protected override void WriteHeader(IMakeWriter writer)
        {
            writer.WriteLine("Microsoft Visual Studio Solution File, Format Version 12.00");
            writer.WriteLine("# Visual Studio 2011");
        }

        protected override string TeamTestSchemaVersion
        {
            get { return "2011"; }
        }

        #endregion Virtual Overrides for Writing Solution File

    }
}
