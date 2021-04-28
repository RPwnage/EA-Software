// Originally based on NAnt - A .NET build tool
// Copyright (C) 2003-2018 Electronic Arts Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
// 
// As a special exception, the copyright holders of this software give you 
// permission to link the assemblies with independent modules to produce 
// new assemblies, regardless of the license terms of these independent 
// modules, and to copy and distribute the resulting assemblies under terms 
// of your choice, provided that you also meet, for each linked independent 
// module, the terms and conditions of the license of that module. An 
// independent module is a module which is not derived from or based 
// on these assemblies. If you modify this software, you may extend 
// this exception to your version of the software, but you are not 
// obligated to do so. If you do not wish to do so, delete this exception 
// statement from your version. 
// 
// Electronic Arts (Frostbite.Team.CM@ea.com)

using System;
using System.Collections.Generic;
using System.Linq;
using System.Xml;
using System.Xml.Schema;
using System.IO;

using NAnt.Core.Logging;

namespace NAnt.Intellisense
{
	internal class NantSchemaGenerator
	{
		private readonly Log Log;
		private readonly string LogPrefix;

		private readonly SchemaMetadata SchemaData;
		private readonly XmlSchema NantXmlSchema;
		private static readonly List<string> ProjectLevelTasks = new List<string>();

		public NantSchemaGenerator(SchemaMetadata schemaData, Log log = null, string logprefix = null)
		{
			Log = log;
			LogPrefix = logprefix ?? "[NantSchemaGenerator]";

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
			List<XmlSchemaComplexType> ComplexTypes = new List<XmlSchemaComplexType>();
			HashSet<string> ComplexTypesUsed = new HashSet<string>();
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
					ComplexTypesUsed.Add(child.Key);

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

				ComplexTypes.Add(ct);
			}
			foreach (var ct in ComplexTypes)
			{
				if (ComplexTypesUsed.Contains(ct.Name))
				{
					schema.Items.Add(ct);
				}
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

			// check for duplicate attributes, which can occur when an attribute is overridden
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

		/// <summary>Creates simple XML data type nodes for representing enumerated types</summary>
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
				if (Log != null)
				{
					Log.Debug.WriteLine(LogPrefix + "Overriding existing schema path '{0}'", path);
				}
				else
				{
					Console.WriteLine(LogPrefix + "Overriding existing schema path '{0}'", path);
				}

				File.SetAttributes(path, File.GetAttributes(path) & ~FileAttributes.ReadOnly & ~FileAttributes.Hidden);
			}

			XmlNamespaceManager nsmgr = new XmlNamespaceManager(new NameTable());
			nsmgr.AddNamespace(String.Empty, "schemas/ea/framework3.xsd");
			nsmgr.AddNamespace("xs", "http://www.w3.org/2001/XMLSchema");

			using (StreamWriter writer = new StreamWriter(path, false))
			{
				schema.Write(writer, nsmgr);
			}

			if (Log != null)
			{
				Log.Status.WriteLine(LogPrefix + "Saved schema to file '{0}'", path);
			}
			else
			{
				Console.WriteLine(LogPrefix + "Saved schema to file '{0}'", path);
			}
		}

	}
}
