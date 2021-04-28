using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using System.Xml;
using NAnt.Core.Attributes;
using NAnt.Core;

namespace EA.Eaconfig
{
    /// <summary>Copies xml schema file to the schema directory under the 
    /// visual studio install directory. Also, extends the visual studio 
    /// schema catalog to associate the new schema with the *.build extension.</summary>
    [TaskName("nantschemainstall")]
    public class NAntSchemaInstallerTask : Task
    {
        private string _vs9path;
        private string _vs10path;
        private string _vs11path;
        private string _inputfile;
        private const String _nantInstallFilename = "nant_intellisense.xml";

        public NAntSchemaInstallerTask()
        {
            String path = System.Environment.GetEnvironmentVariable("VS90COMNTOOLS");
            if (path != null)
            {
                DirectoryInfo vsInstallDir = new DirectoryInfo(path).Parent.Parent;
                _vs9path = vsInstallDir.FullName + "/xml/Schemas/";
            }

            path = System.Environment.GetEnvironmentVariable("VS100COMNTOOLS");
            if (path != null)
            {
                DirectoryInfo vsInstallDir = new DirectoryInfo(path).Parent.Parent;
                _vs10path = vsInstallDir.FullName + "/xml/Schemas/";
            }

            path = System.Environment.GetEnvironmentVariable("VS110COMNTOOLS");
            if (path != null)
            {
                DirectoryInfo vsInstallDir = new DirectoryInfo(path).Parent.Parent;
                _vs11path = vsInstallDir.FullName + "/xml/Schemas/";
            }
        }

        /// <summary>The install directory of visual studio 2008 (vs9) where the schema files will be copied.
        /// By default uses environment variable VS90COMNTOOLS.</summary>
        [TaskAttribute("vs9path", Required = false)]
        public String Vs9Path { get { return _vs9path; } }

        /// <summary>The install directory of visual studio 2010 (vs10) where the schema files will be copied.
        /// By default uses environment variable VS100COMNTOOLS.</summary>
        [TaskAttribute("vs10path", Required = false)]
        public String Vs10Path { get { return _vs10path; } }

        /// <summary>The install directory of visual studio 2012 (vs11) where the schema files will be copied.
        /// By default uses environment variable VS110COMNTOOLS.</summary>
        [TaskAttribute("vs11path", Required = false)]
        public String Vs11Path { get { return _vs11path; } }

        /// <summary>The name of the schema file. Should match the name of the output file of the nantschema task</summary>
        [TaskAttribute("inputfile", Required = true)]
        public String InputFile { get { return _inputfile; } set { _inputfile = value; } }

        /// <summary>Creates a schema catalog file which is used to associate files with the 
        /// extension *.build with the xml schema file framework3.xsd</summary>
        private void CreateXmlSchemaCatalogDocument(string filename)
        {
            System.Xml.XmlDocument document = new System.Xml.XmlDocument();

            XmlNode rootNode = document.CreateNode(XmlNodeType.Element,
                "SchemaCatalog", "http://schemas.microsoft.com/xsd/catalog");
            document.AppendChild(rootNode);

            XmlElement element = document.CreateElement("Association", rootNode.NamespaceURI);
            XmlAttribute extension = document.CreateAttribute("extension");
            extension.Value = "build";
            element.Attributes.Append(extension);

            XmlAttribute schema = document.CreateAttribute("schema");
            schema.Value = "%InstallRoot%/xml/schemas/framework3.xsd";
            element.Attributes.Append(schema);

            XmlAttribute xmlns = document.CreateAttribute("targetNamespace");
            xmlns.Value = "schemas/ea/framework3.xsd";
            element.Attributes.Append(xmlns);

            rootNode.AppendChild(element);

            document.Save(filename);
        }

        /// <summary>Copies the schema file to all visual studio schema directories</summary>
        private void CopySchemaFile()
        {
            FileInfo schemafile = new FileInfo(_inputfile);
            var filename = Path.GetFileName(schemafile.FullName);
            if (Vs9Path != null)
            {
                DirectoryInfo vs9dir = new DirectoryInfo(Vs9Path);
                schemafile.CopyTo(vs9dir.FullName + "/" + filename, true);
            }

            if (Vs10Path != null)
            {
                DirectoryInfo vs10dir = new DirectoryInfo(Vs10Path);
                schemafile.CopyTo(vs10dir.FullName + "/" + filename, true);
            }

            if (Vs11Path != null)
            {
                DirectoryInfo vs11dir = new DirectoryInfo(Vs11Path);
                schemafile.CopyTo(vs11dir.FullName + "/" + filename, true);
            }
        }

        protected override void ExecuteTask()
        {
            CreateXmlSchemaCatalogDocument(Vs9Path + _nantInstallFilename);
            CreateXmlSchemaCatalogDocument(Vs10Path + _nantInstallFilename);
            CreateXmlSchemaCatalogDocument(Vs11Path + _nantInstallFilename);

            CopySchemaFile();
        }
    }
}
