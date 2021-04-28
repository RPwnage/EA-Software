using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Xml;
using System.Xml.Schema;

using System.IO;

using NAnt.Core.Logging;


namespace EA.Eaconfig
{
    public class NantSchemaGenerator
    {
        private readonly Log Log;
        private readonly string LogPrefix;

        private readonly SchemaMetadata SchemaData;
        private readonly XmlSchema NantXmlSchema;
        private static readonly List<string> ProjectLevelTasks = new List<string>();

        public NantSchemaGenerator(Log log, string logprefix, SchemaMetadata schemaData)
        {
            Log = log;
            LogPrefix = logprefix;

            SchemaData = schemaData;

            NantXmlSchema = InitXmlSchema();

            ProjectLevelTasks.Add("package");
        }

        private XmlSchema InitXmlSchema()
        {
            XmlSchema schema = new XmlSchema();

            schema.Namespaces.Add("nant", "schemas/ea/framework3.xsd");
            schema.Namespaces.Add("xs", XmlSchema.Namespace);
            schema.ElementFormDefault = XmlSchemaForm.Qualified;

            XmlSchemaAnnotation schemaAnnotation = new XmlSchemaAnnotation();
            XmlSchemaDocumentation schemaDocumentation = new XmlSchemaDocumentation();

            schemaAnnotation.Items.Add(schemaDocumentation);
            schema.Items.Add(schemaAnnotation);

            return schema;
        }

        private void CreateProjectElement(out XmlSchemaElement projectElement, out XmlSchemaChoice choice)
        {
            // create project attribute
            XmlSchemaComplexType projectCT = new XmlSchemaComplexType();

            projectElement = new XmlSchemaElement();
            projectElement.Name = "project";
            projectElement.SchemaType = projectCT;

            choice = new XmlSchemaChoice();
            choice.MinOccurs = 0;
            choice.MaxOccurs = Decimal.MaxValue;

            XmlSchemaSequence sequence = new XmlSchemaSequence();
            sequence.Items.Add(choice);

            projectCT.Particle = sequence;

            //name attribute
            System.Xml.Schema.XmlSchemaAttribute nameAttr = new System.Xml.Schema.XmlSchemaAttribute();
            nameAttr.Name = "name";
            nameAttr.Use = XmlSchemaUse.Optional;
            nameAttr.Annotation = CreateXmlDocumentationNode("The name of the project.");
            projectCT.Attributes.Add(nameAttr);

            //default attribute
            System.Xml.Schema.XmlSchemaAttribute defaultAttr = new System.Xml.Schema.XmlSchemaAttribute();
            defaultAttr.Name = "default";
            defaultAttr.Use = XmlSchemaUse.Optional;
            defaultAttr.Annotation = CreateXmlDocumentationNode("The Default build target. If not specified default target is 'build' (or 'clean')");
            projectCT.Attributes.Add(defaultAttr);

            //basedir attribute
            System.Xml.Schema.XmlSchemaAttribute baseDirAttr = new System.Xml.Schema.XmlSchemaAttribute();
            baseDirAttr.Name = "basedir";
            baseDirAttr.Use = XmlSchemaUse.Optional;
            baseDirAttr.Annotation = CreateXmlDocumentationNode("The Base Directory used for relative file references.");
            projectCT.Attributes.Add(baseDirAttr);
        }

        private void PopulateXmlSchema(XmlSchema schema, XmlSchemaChoice choice)
        {
            foreach (var element in SchemaData.NantElements.Values)
            {
                XmlSchemaComplexType ct = CreateXmlComplexTypeData(choice, element);
                XmlSchemaChoice ctChoices = new XmlSchemaChoice();
                ctChoices.MinOccurs = 0;
                ctChoices.MaxOccurs = Decimal.MaxValue;
                ct.Particle = ctChoices;

                foreach (NantChildElement child in element.ChildElements)
                {
                    XmlSchemaElement schemaElement = new XmlSchemaElement();
                    schemaElement.Name = child.Name;
                    schemaElement.SchemaTypeName = new XmlQualifiedName(child.Key);

                    //IM: I assume that when annotation is not present in the child node it will be taken from SchemaTypeName element
                    var childDescription = GenerateElementDocumentation(child);
                    if (!String.IsNullOrEmpty(childDescription))
                    {
                        schemaElement.Annotation = CreateXmlDocumentationNode(GenerateElementDocumentation(child));
                    }

                    ctChoices.Items.Add(schemaElement);
                }

                XmlSchemaAny otherNamespaceAny = new XmlSchemaAny();
                otherNamespaceAny.MinOccurs = 0;
                otherNamespaceAny.MaxOccurs = Decimal.MaxValue;
                otherNamespaceAny.Namespace = "##other";
                otherNamespaceAny.ProcessContents = XmlSchemaContentProcessing.Skip;
                ctChoices.Items.Add(otherNamespaceAny);

                schema.Items.Add(ct);
            }

            foreach (var e in this.SchemaData.EnumTypes)
            {
                schema.Items.Add(CreateXmlEnumTypeData(e.Value));
            }
        }

        private XmlSchemaAnnotation CreateXmlDocumentationNode(string summary)
        {
            XmlSchemaAnnotation annot = new XmlSchemaAnnotation();
            XmlSchemaDocumentation doc = new XmlSchemaDocumentation();
            doc.Markup = TextToNodeArray(summary);
            annot.Items.Add(doc);
            return annot;
        }

        private void AddXmlAttribute(XmlSchemaObjectCollection xmlAttributes, NantAttribute nantAttr, NantElement element)
        {
            string typeInfo = GenerateAttributeDocumentation(nantAttr);

            System.Xml.Schema.XmlSchemaAttribute attribute = CreateXmlSchemaAttribute(nantAttr);
            attribute.Annotation = CreateXmlDocumentationNode(GenerateAttributeDocumentation(nantAttr));

            if (nantAttr.ValidValues != null)
            {
                attribute.SchemaTypeName = new XmlQualifiedName(nantAttr.ValidValues.Key);
            }


            // IMTODO. I don't think we can have duplicates. This should be resolved while creating DATA, not here

            // check for duplicate attributes, which can occur when an attribute is overriden
            bool duplicate = false;
            foreach (System.Xml.Schema.XmlSchemaAttribute prevattr in xmlAttributes)
            {
                if (prevattr.Name == attribute.Name)
                {
                    duplicate = true;
                    break;
                }
            }
            if (!duplicate)
            {
                xmlAttributes.Add(attribute);
            }
        }

        private XmlSchemaComplexType CreateXmlComplexTypeData(XmlSchemaChoice choice, NantElement element)
        {
            XmlSchemaComplexType ct = new XmlSchemaComplexType();
            ct.Name = element.Key;
            ct.IsMixed = element.IsMixed;

            XmlSchemaElement taskElement = CreateXmlSchemaElement(element);
            if (element.IsSchemaElement && element.IsTask)
            {
                choice.Items.Add(taskElement);
            }
            else if (ProjectLevelTasks.Contains(element.ElementName))
            {
                choice.Items.Add(taskElement);
            }

            if (!element.IsStrict)
            {
                ct.AnyAttribute = new XmlSchemaAnyAttribute();
                ct.AnyAttribute.ProcessContents = XmlSchemaContentProcessing.Skip;
            }

            if (element.Attributes != null)
            {
                foreach (var nantAttr in element.Attributes)
                {
                    AddXmlAttribute(ct.Attributes, nantAttr, element);
                }
            }

            ct.Annotation = CreateXmlDocumentationNode(GenerateElementDocumentation(element));

            return ct;
        }

        /// <summary>Creates simple xml data type nodes for representing enumerated types</summary>
        private XmlSchemaSimpleType CreateXmlEnumTypeData(NantEnumType enumtype)
        {
            XmlSchemaSimpleType st = new XmlSchemaSimpleType();
            st.Name = enumtype.Key;

            if (enumtype.Names != null && enumtype.Names.Count() > 0)
            {
                XmlSchemaSimpleTypeRestriction restrict = new XmlSchemaSimpleTypeRestriction();
                restrict.BaseTypeName = new XmlQualifiedName("normalizedString", "http://www.w3.org/2001/XMLSchema");
                foreach (string name in enumtype.Names)
                {
                    XmlSchemaEnumerationFacet facet = new XmlSchemaEnumerationFacet();
                    facet.Value = name;
                    restrict.Facets.Add(facet);
                }
                st.Content = restrict;
            }
            return st;
        }

        /// <summary>Generates some default documentation based on the type and
        /// adds this to the user provided documentation.</summary>
        private string GenerateAttributeDocumentation(NantAttribute nantAttr)
        {
            if (!String.IsNullOrEmpty(nantAttr.TypeDescription))
            {
                return String.Format("{0} : {1}", nantAttr.TypeDescription, nantAttr.Summary ?? String.Empty);
            }
            return nantAttr.Summary ?? String.Empty;
        }

        private string GenerateElementDocumentation(NantChildElement element)
        {
            if (!String.IsNullOrEmpty(element.TypeDescription))
            {
                return String.Format("{0} : {1}", element.TypeDescription, element.Summary ?? String.Empty);
            }
            return element.Summary ?? String.Empty;
        }

        private string GenerateElementDocumentation(NantElement element)
        {
            return element.Summary ?? String.Empty;
        }

        private XmlSchemaElement CreateXmlSchemaElement(NantElement task)
        {
            XmlSchemaElement taskElement = new XmlSchemaElement();
            taskElement.Name = task.ElementName;
            taskElement.SchemaTypeName = new XmlQualifiedName(task.Key);
            return taskElement;
        }

        private System.Xml.Schema.XmlSchemaAttribute CreateXmlSchemaAttribute(NantAttribute nantAttr)
        {
            System.Xml.Schema.XmlSchemaAttribute attribute = new System.Xml.Schema.XmlSchemaAttribute();
            attribute.Name = nantAttr.AttributeName;
            attribute.Use = nantAttr.IsRequired == true ? XmlSchemaUse.Required : XmlSchemaUse.Optional;
            return attribute;
        }

        private static XmlNode[] TextToNodeArray(string text)
        {
            XmlDocument doc = new XmlDocument();
            return new XmlNode[1] { doc.CreateTextNode(text) };
        }


        public XmlSchema GenerateSchema(string targetNamespace)
        {
            // create the project element
            XmlSchemaElement projectElement;
            XmlSchemaChoice choice;
            CreateProjectElement(out projectElement, out choice);

            // populate the complex types and element choices
            PopulateXmlSchema(NantXmlSchema, choice);

            NantXmlSchema.Items.Add(projectElement);

            XmlSchemaAny otherNamespaceAny = new XmlSchemaAny();
            otherNamespaceAny.MinOccurs = 0;
            otherNamespaceAny.MaxOccurs = Decimal.MaxValue;
            otherNamespaceAny.Namespace = "##other";
            otherNamespaceAny.ProcessContents = XmlSchemaContentProcessing.Lax;
            choice.Items.Add(otherNamespaceAny);

            // check to see if the schema compiles
            XmlSchemaSet xmlSchemaSet = new XmlSchemaSet();
            xmlSchemaSet.Add(NantXmlSchema);
            xmlSchemaSet.Compile();

            NantXmlSchema.TargetNamespace = targetNamespace;

            return NantXmlSchema;
        }

        public void WriteSchema(string path , XmlSchema schema=null)
        {
            schema = schema ?? NantXmlSchema;

            Directory.CreateDirectory(Path.GetDirectoryName(path));

            if (File.Exists(path))
            {
                Log.Debug.WriteLine(LogPrefix + "Overriding existing schema path '{0}'", path);

                File.SetAttributes(path, File.GetAttributes(path) & ~FileAttributes.ReadOnly & ~FileAttributes.Hidden);
            }

            XmlNamespaceManager nsmgr = new XmlNamespaceManager(new NameTable());
            nsmgr.AddNamespace(String.Empty, "schemas/ea/framework3.xsd");
            nsmgr.AddNamespace("xs", "http://www.w3.org/2001/XMLSchema");

            using (StreamWriter writer = new StreamWriter(path, false))
            {
                schema.Write(writer, nsmgr);
            }
            Log.Status.WriteLine(LogPrefix + "Saved schema to file '{0}'", path);
        }

    }
}
