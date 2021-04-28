using System;
using System.Collections;
using System.Collections.Specialized;
using System.Diagnostics;
using System.IO;
using System.Xml;
using System.Text.RegularExpressions;
using System.Reflection;
using System.Xml.Schema;
using System.Globalization;
using System.Collections.Generic;

using NAnt.Core.Attributes;
using NAnt.Core.Logging;
using NAnt.Core;
using NAnt.Core.Reflection;


namespace EA.Eaconfig
{

    /// <summary>Generates XML file from NantSchema. </summary>
    [TaskName("nantschematoxml")]
    public class NantSchemaToXmlTask : Task
    {
        private string _outputfile;
        private bool _excludecomments = false;
        private FileSet _assemblyFileSet = new FileSet();

        public NantSchemaToXmlTask()
            : base("nantschematoxml")
        {
        }

        /// <summary>The name of the output XML file.</summary>
        [TaskAttribute("outputfile", Required = true)]
        public String OutputFile { get { return _outputfile; } set { _outputfile = value; } }

        /// <summary>If true comments are not included in generated XML file.</summary>
        [TaskAttribute("excludecomments", Required = false)]
        public bool ExcludeComments { get { return _excludecomments; } set { _excludecomments = value; } }



        [FileSet("fileSet")]
        public FileSet AssemblyFileSet { get { return _assemblyFileSet; } }

        protected override void ExecuteTask()
        {
            var schemaMetaData = CreateSchemaMetaData();

            var generator = new NantSchemaToXml(this.Log, LogPrefix, schemaMetaData, ExcludeComments);

            generator.GenerateXMLFromNantSchema(OutputFile, bynamespaces:true);

        }

        private SchemaMetadata CreateSchemaMetaData()
        {
            var schemaMetaData = new SchemaMetadata(this.Log, LogPrefix);

            foreach (var assemblyInfo in AssemblyLoader.GetAssemblyCacheInfo())
            {
                schemaMetaData.AddAssembly(assemblyInfo.Assembly, assemblyInfo.Path);
            }

            if (AssemblyFileSet != null)
            {
                foreach (var fi in AssemblyFileSet.FileItems)
                {
                    schemaMetaData.AddAssembly(Assembly.LoadFrom(fi.Path.Path));
                }
            }

            schemaMetaData.GenerateSchemaMetaData();

            return schemaMetaData;

        }
    }
}
