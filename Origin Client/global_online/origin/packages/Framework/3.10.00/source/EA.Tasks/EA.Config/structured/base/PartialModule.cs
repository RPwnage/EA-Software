using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Tasks;
using NAnt.Core.Logging;
using NAnt.Core.Util;

using System;
using System.Collections;
using System.Collections.Generic;
using System.Text;
using System.Xml;

using EA.Eaconfig.Modules;

namespace EA.Eaconfig.Structured
{
    /// <summary></summary>
    [TaskName("PartialModule", StrictAttributeCheck=false)]
    public class PartialModuleTask : TaskContainer
    {
        private static readonly Guid PartialModulesListId = new Guid("28E0C1DB-81A8-46c5-9257-7026916C9174");

        private string _name;
        private XmlNode _code = null;

        /// <summary></summary>
        [TaskAttribute("name", Required = true)]
        public virtual string ModuleName
        {
            get { return _name;  }
            set { _name = value; }
        }

        /// <summary></summary>       
        [TaskAttribute("comment", Required = false)]
        public string Comment
        {
            get { return _comment; }
            set { _comment = value; }
        } private string _comment;

        internal XmlNode Code
        {
            get { return _code; }
        }

        protected override void InitializeTask(XmlNode taskNode)
        {
            _code = taskNode;

            GetPatialModules(Project).Add(this);
        }

        protected override void ExecuteTask()
        {
            Log.Info.WriteLine(LogPrefix + "Registering Partial Module definition'{0}'", ModuleName);
        }

        public static List<PartialModuleTask> GetPatialModules(Project project)
        {
            return project.NamedObjects.GetOrAdd(PartialModulesListId, new List<PartialModuleTask>()) as List<PartialModuleTask>;
        }

        public void ApplyPartialModule(ModuleBaseTask module)
        {
            if (typeof(DotNetModuleTask).IsAssignableFrom(module.GetType()))
            {
                var partialModule = new DotNetPartialModule(module as DotNetModuleTask);

                partialModule.Parent = module.Parent;

                partialModule.Initialize(_code);

                partialModule.SetupModule(module);

                partialModule.NantScript.Execute(NantScript.Order.after);
            }
            else
            {
                var partialModule = new PartialModule(module);

                partialModule.Parent = module.Parent;

                partialModule.Initialize(_code);

                partialModule.SetupModule(module);

                partialModule.NantScript.Execute(NantScript.Order.after);
            }
            
        }

        internal class PartialModule : ModuleTask
        {
            internal PartialModule(ModuleBaseTask module)
                : base(String.Empty)
            {
                Project = module.Project;
                if (module is ModuleTask)
                {
                    mCombinedState = ((ModuleTask)module).mCombinedState;
                }
                else
                {
                    mCombinedState = new CombinedStateData();
                }
            }

            internal void SetupModule(ModuleBaseTask module)
            {
                ModuleName = module.ModuleName;
                Group = module.Group;
                debugData = module.debugData;
                Debug = module.Debug;

                base.SetupModule();
            }
        }

        internal class DotNetPartialModule : DotNetModuleTask
        {
            internal DotNetPartialModule(DotNetModuleTask module)
                : base(String.Empty, module.CompilerName)
            {
                Project = module.Project;
                mCombinedState = ((DotNetModuleTask)module).mCombinedState;
            }

            internal void SetupModule(ModuleBaseTask module)
            {
                ModuleName = module.ModuleName;
                Group = module.Group;
                debugData = module.debugData;
                Debug = module.Debug;

                base.SetupModule();
            }
        }

    }
}
