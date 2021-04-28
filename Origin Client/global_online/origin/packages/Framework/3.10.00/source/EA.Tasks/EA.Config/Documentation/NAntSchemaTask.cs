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

    /// <summary>Generates an xml schema file (*.xsd) based on task source files. This file
    /// can be used by visual studio to provide intellisense for editing build files.</summary>
    [TaskName("nantschema")]
    public class NAntSchemaTask : Task
    {
        private string _outputfile;
        private FileSet _assemblyFileSet = new FileSet();

        public NAntSchemaTask() : base("nantschema")
        {
        }

        /// <summary>The name of the output schema file.</summary>
        [TaskAttribute("outputfile", Required = true)]
        public String OutputFile { get { return _outputfile; } set { _outputfile = value; } }


        [FileSet("fileSet")]
        public FileSet AssemblyFileSet { get { return _assemblyFileSet; } }


        protected override void ExecuteTask()
        {
            var schemaMetaData = CreateSchemaMetaData();

            var generator = new NantSchemaGenerator(this.Log, LogPrefix, schemaMetaData);

            generator.GenerateSchema("schemas/ea/framework3.xsd");

            generator.WriteSchema(OutputFile);
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
