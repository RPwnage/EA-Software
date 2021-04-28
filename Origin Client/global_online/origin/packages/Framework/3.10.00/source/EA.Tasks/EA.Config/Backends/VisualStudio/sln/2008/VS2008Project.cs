using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

using NAnt.Core.Writers;
using EA.FrameworkTasks.Model;

namespace EA.Eaconfig.Backends.VisualStudio
{
    internal class VS2008Project : VSCppProject
    {
        internal VS2008Project(VSSolutionBase solution, IEnumerable<IModule> modules)
            : base(solution, modules)
        {
        }

        public override string ProjectFileName
        {
            get
            {
                return ProjectFileNameWithoutExtension + ".vcproj";
            }
        }

        protected override string Version 
        {
            get
            {
                return "9.00";
            }
        }

        protected override IXmlWriterFormat ProjectFileWriterFormat 
        { 
            get { return _xmlWriterFormat; }
        }

        private static readonly IXmlWriterFormat _xmlWriterFormat = new XmlFormat(
#if FRAMEWORK_PARALLEL_TRANSITION
            identChar :' ', 
            identation : 2, 
            newLineOnAttributes : false, 
            encoding : Encoding.UTF8 
#else
            identChar: ' ',
            identation: 6,
            newLineOnAttributes: true,
            encoding: new UTF8Encoding(false) // no byte order mask!
#endif
            );
    }
}
