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
using EA.FrameworkTasks.Model;

using EA.Eaconfig;
using EA.Eaconfig.Core;
using EA.Eaconfig.Build;
using EA.Eaconfig.Modules;
using EA.Eaconfig.Modules.Tools;

namespace EA.Eaconfig.Core
{
    public abstract class AbstractModuleProcessorTask : Task, IBuildModuleTask, IModuleProcessor
    {
        public ProcessableModule Module
        {
            get { return _module; }
            set { _module = value; }
        }

        public Tool Tool
        {
            get { return _tool; }
            set { _tool = value; }
        }

        public OptionSet BuildTypeOptionSet 
        {
            get { return _buildTypeOptionSet; }
            set { _buildTypeOptionSet = value; }
        }

        protected override void ExecuteTask()
        {
            if (Tool != null)
            {
                ProcessTool(Tool);
            }
            else
            {
                Module.Apply(this);
                foreach(var tool in Module.Tools)
                {
                    ProcessTool(tool);
                }
            }
        }

        protected string PropGroupName(string name)
        {
            if (name.StartsWith("."))
            {
                return Module.GroupName + name;
            }
            return Module.GroupName + "." + name;
        }

        protected string PropGroupValue(string name, string defVal = "")
        {
            string val = null;
            if (!String.IsNullOrEmpty(name))
            {
                val = Project.Properties[PropGroupName(name)];
            }
            return val??defVal;
        }

        protected FileSet PropGroupFileSet(string name)
        {

            if (String.IsNullOrEmpty(name))
            {
                return null;
            }
            return Project.NamedFileSets[PropGroupName(name)];
        }

        protected FileSet AddModuleFiles(Module module, string propertyname, params string[] defaultfilters)
        {
            FileSet dest = new FileSet(Project);
            if (0 < AddModuleFiles(dest, module, propertyname, defaultfilters))
            {
                return dest;
            }
            return null;
        }

        internal int AddModuleFiles(FileSet dest, Module module, string propertyname, params string[] defaultfilters)
        {
            int appended = 0;
            if (dest != null)
            {
#if FRAMEWORK_PARALLEL_TRANSITION
                // $$$ NOTE:  We need the behaviour to match what is done in SetupBuildFilesets() in ComputeBaseBuildData.cs in the old eaconfig
                if (propertyname == ".sourcefiles")
                {
                    appended += dest.AppendWithBaseDir(PropGroupFileSet(propertyname));
                    appended += dest.AppendWithBaseDir(PropGroupFileSet(propertyname + "." + module.Configuration.System));
                }
                else
                {
                    var srcfs = PropGroupFileSet(propertyname);
                    dest.Append(srcfs);
                    appended += srcfs != null ? 1 : 0;
                    srcfs = PropGroupFileSet(propertyname + "." + module.Configuration.System);
                    dest.Append(srcfs);
                    appended += srcfs != null ? 1 : 0;
                }
#else
                appended += dest.AppendWithBaseDir(PropGroupFileSet(propertyname));
                appended += dest.AppendWithBaseDir(PropGroupFileSet(propertyname + "." + module.Configuration.System));
#endif
                if (appended == 0 && defaultfilters != null)
                {
                    foreach (string filter in defaultfilters)
                    {
                        dest.Include(Project.ExpandProperties(filter));
                        appended++;
                    }
                }
            }
            return appended;
        }


        public virtual void Process(Module_Native module)
        {
        }

        public virtual void Process(Module_DotNet module)
        {
        }

        public virtual void Process(Module_Utility module)
        {
        }

        public virtual void Process(Module_MakeStyle module)
        {
        }

        public virtual void Process(Module_Python module)
        {
        }

        public virtual void Process(Module_ExternalVisualStudio module)
        {
        }

        public virtual void Process(Module_UseDependency module)
        {
        }

        public virtual void ProcessTool(CcCompiler cc)
        {
        }

        public virtual void ProcessTool(AsmCompiler asm)
        {
        }

        public virtual void ProcessTool(Linker link)
        {
        }

        public virtual void ProcessTool(Librarian lib)
        {
        }

        public virtual void ProcessTool(BuildTool buildtool)
        {
        }

        public virtual void ProcessTool(CscCompiler csc)
        {
        }

        public virtual void ProcessTool(FscCompiler fsc)
        {
        }

        public virtual void ProcessTool(PostLink postlink)
        {
        }

        public virtual void ProcessTool(AppPackageTool apppackage)
        {
        }

        private void ProcessTool(Tool tool)
        {
            if (tool is CcCompiler)
                ProcessTool(tool as CcCompiler);
            else if (tool is AsmCompiler)
                ProcessTool(tool as AsmCompiler);
            else if (tool is Linker)
                ProcessTool(tool as Linker);
            else if (tool is Librarian)
                ProcessTool(tool as Librarian);
            else if (tool is BuildTool)
                ProcessTool(tool as BuildTool);
            else if (tool is CscCompiler)
                ProcessTool(tool as CscCompiler);
            else if (tool is FscCompiler)
                ProcessTool(tool as FscCompiler);
            else if (tool is PostLink)
                ProcessTool(tool as PostLink);
            else if (tool is AppPackageTool)
                ProcessTool(tool as AppPackageTool);
            else if (tool == null)
            {
                Log.Error.WriteLine("INTERNAL ERROR: module '{0}', task '{1}' null tool", Module.Name, Name);
            }
            else
                Log.Warning.WriteLine("module '{0}', task '{1}' unknown tool '{2}'", Module.Name, Name, tool.GetType().Name);
        }

        private ProcessableModule _module;
        private Tool _tool;
        private OptionSet _buildTypeOptionSet;
    }
}
