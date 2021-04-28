using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

using EA.Eaconfig.Modules;

namespace EA.Eaconfig.Core
{
    public interface IModuleProcessor
    { 
        void Process(Module_Native module);
        void Process(Module_DotNet module);
        void Process(Module_Utility module);
        void Process(Module_MakeStyle module);
        void Process(Module_ExternalVisualStudio module);
        void Process(Module_UseDependency module);
        void Process(Module_Python module);
    }
}
