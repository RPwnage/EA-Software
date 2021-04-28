using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

using NAnt.Core.Writers;
using EA.FrameworkTasks.Model;
using EA.Eaconfig.Modules;
using NAnt.Core;
using EA.Eaconfig.Modules.Tools;

namespace EA.Eaconfig.Backends.VisualStudio
{
    internal class VS2011Project : VSCppMsbuildProject
    {
        internal VS2011Project(VSSolutionBase solution, IEnumerable<IModule> modules)
            : base(solution, modules)
        {
        }

        public override string ProjectFileName
        {
            get
            {
                return ProjectFileNameWithoutExtension + ".vcxproj";
            }
        }

        protected override string Version 
        {
            get
            {
                return "11.00";
            }
        }

        protected override string ToolsVersion 
        {
            get 
            { 
                return "4.0"; 
            } 
        }

        protected override string DefaultToolset
        {
            get
            {
                return "v110";
            }
        }

        protected override string DefaultTargetFrameworkVersion
        {
            get { return "4.5"; }
        }


        internal override bool SupportsDeploy(Configuration config) 
        {
           var module = Modules.FirstOrDefault(m => m.Configuration == config);

           if (IsWinRTProgram(module as Module_Native))
           {
               return module.Package.Project.Properties.GetBooleanPropertyOrDefault("package.consoledeployment", true);
           }
           return base.SupportsDeploy(config);
        }

        protected override void WriteLinkerTool(IXmlWriter writer, string name, Tool tool, Configuration config, Module module, IDictionary<string, string> nameToDefaultXMLValue = null, FileItem file = null)
        {
            base.WriteLinkerTool(writer, name, tool, config, module, nameToDefaultXMLValue, file);

            var linker = tool as Linker;
            if (linker != null)
            {
                if (linker.UseLibraryDependencyInputs)
                {
                    // Need an additional setting in to get UseLibraryDependencyInputs to work
                    // <ItemDefinitionGroup Condition=" '$(Configuration)|$(Platform)' == 'pc64-vc-debug|x64' ">
                    // ...
                    //  <ProjectReference>
                    //      <UseLibraryDependencyInputs>true</UseLibraryDependencyInputs>
                    //  </ProjectReference>
                    // </ItemDefinitionGroup>
                    writer.WriteStartElement("ProjectReference");
                    writer.WriteElementString("UseLibraryDependencyInputs", "TRUE");
                    writer.WriteEndElement(); //Tool
                }
            }
        }
    }
}
