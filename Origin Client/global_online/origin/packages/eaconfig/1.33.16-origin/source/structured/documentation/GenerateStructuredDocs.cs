using System;
using System.Reflection;

using NAnt.Core;
using NAnt.Core.Logging;
using NAnt.Core.Attributes;

namespace EA.Eaconfig.Structured.Documentation
{
    [NAnt.Core.Attributes.Description("")]
    [NAnt.Core.Attributes.XmlSchema]
    [TaskName("GenerateStructuredDocs")]
    public class GenerateStructuredDocs : Task
    {
        [NAnt.Core.Attributes.Description("Sets the output directory for the structured documents")]
        [TaskAttribute("outputdir", Required = true)]
        public string OutputDir
        {
            get { return _outputdir; }
            set { _outputdir = value; }
        } string _outputdir;


        protected override void ExecuteTask()
        {
            if (String.IsNullOrEmpty(_outputdir))
                throw new InvalidOperationException("A directory must be provided GenerateStructuredDocs");

            StructuredDoc doc = new StructuredDoc();
            Type[] types = Assembly.GetExecutingAssembly().GetTypes();
            foreach (Type type in types)
            {
                if (type.IsClass && type.Namespace == "EA.Eaconfig.Structured" && type.IsSubclassOf(typeof(ModuleTask)))
                {
                    doc.ReadType(type);
                    StructuredDocWriter.Write(doc, _outputdir);
                }
            }
        }
    }
}
