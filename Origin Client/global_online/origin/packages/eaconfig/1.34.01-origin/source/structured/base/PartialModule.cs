using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Tasks;
using NAnt.Core.Logging;

using System;
using System.Collections;
using System.Collections.Generic;
using System.Text;
using System.Xml;

using EA.Eaconfig.Structured.ObjectModel;

namespace EA.Eaconfig.Structured
{
    [NAnt.Core.Attributes.Description("")]
    [NAnt.Core.Attributes.XmlSchema]
    [TaskName("PartialModule")]
    public class PartialModuleTask : ModuleTask
    {
        private static readonly Guid PartialModulesListId = new Guid("28E0C1DB-81A8-46c5-9257-7026916C9174");

        public PartialModuleTask()
            : base(String.Empty)
        {
        }

        public override string PartialModuleName
        {
            get { return String.Empty; }
            set {  }
        }

        protected override void SetupModule()
        {
            GetPatialModules(Project).Add(this);
        }

        public static List<PartialModuleTask> GetPatialModules(Project project)
        {
            List<PartialModuleTask> modules = project.NamedObjects[PartialModulesListId] as List<PartialModuleTask>;
            if (modules == null)
            {
                modules = new List<PartialModuleTask>();
                project.NamedObjects[PartialModulesListId] = modules;
            }
            return modules;
        }

        public void ApplyPartialModule(ModuleBaseTask module)
        {
            ModuleName = module.ModuleName;
            Group = module.Group;
            debugData = module.debugData;

            base.SetupModule();
        }
    }

}
