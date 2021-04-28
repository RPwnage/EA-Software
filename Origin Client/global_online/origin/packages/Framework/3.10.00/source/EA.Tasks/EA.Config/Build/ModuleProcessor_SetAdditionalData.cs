using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Logging;
using NAnt.Core.Threading;
using NAnt.Core.Tasks;
using EA.FrameworkTasks.Model;
using EA.Eaconfig.Core;
using EA.Eaconfig.Modules;


namespace EA.Eaconfig.Build
{
    class ModuleAdditionalDataProcessor : ModuleProcessorBase, IModuleProcessor, IDisposable
    {
        internal ModuleAdditionalDataProcessor(ProcessableModule module)
            : base(module)
        {
        }

        public void Process(ProcessableModule module)
        {
            module.Apply(this);
        }

        public void Process(Module_Native module)
        {
            AddAdditionalDataToModule(module);
        }
        public void Process(Module_DotNet module)
        {
            AddAdditionalDataToModule(module);
        }
        public void Process(Module_Utility module)
        {
            AddAdditionalDataToModule(module);
        }
        
        public void Process(Module_MakeStyle module)
        {
            AddAdditionalDataToModule(module);
        }

        public void Process(Module_Python module)
        {
            AddAdditionalDataToModule(module);
        }

        public void Process(Module_ExternalVisualStudio module)
        {
        }

        public void Process(Module_UseDependency module)
        {
        }

        private void AddAdditionalDataToModule(ProcessableModule module)
        {
            AddDebugData(module);
            module.AdditionalData.GeneratorTemplatesData = GeneratorTemplatesData.CreateGeneratorTemplateData(Project, module.BuildGroup);
        }

        private void AddDebugData(ProcessableModule module)
        {

            module.AdditionalData.DebugData = DebugData.CreateDebugData
                (
                    executable      : PropGroupValue("commandprogram", String.Empty),
                    options         : PropGroupValue("commandargs", String.Empty),
                    workingDir      : PropGroupValue("workingdir", String.Empty),
                    env             : GetEnv(),
                    remoteMachine   : PropGroupValue("remotemachine", String.Empty),
                    enableUnmanagedDebugging: module.IsKindOf(Module.DotNet) ? 
                               ConvertUtil.ToNullableBoolean(PropGroupValue("enableunmanageddebugging", PropGroupValue("csproj.enableunmanageddebugging", PropGroupValue("fsproj.enableunmanageddebugging")))) 
                               : null
                );
        }

        private IDictionary<string, string> GetEnv()
        {
            var env = new Dictionary<string,string>();
            foreach (Property property in Project.Properties) 
            {
                    if (property.Prefix == ExecTask.EnvironmentVariablePropertyNamePrefix) 
                    {
                        env[property.Suffix] = property.Value;
                    }
            }
            return env.Count > 0 ? env : null;
        }



        #region Dispose interface implementation

        public void Dispose()
        {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        void Dispose(bool disposing)
        {
            if (!this._disposed)
            {
                if (disposing)
                {
                    // Dispose managed resources here
                }
            }
            _disposed = true;
        }
        private bool _disposed = false;

        #endregion

    }
}
