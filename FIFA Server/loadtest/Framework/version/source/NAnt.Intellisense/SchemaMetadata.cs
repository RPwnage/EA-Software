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
using System.Reflection;
using System.IO;

using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Logging;
using System.Xml;

namespace NAnt.Intellisense
{
	internal class SchemaMetadata // DAVE-FUTURE-REFACTOR-TODO this overlaps a lot with the reflection code in ElementInitHelper - be nice to commonize some of it
	{
		public readonly IDictionary<string, NantElement> NantTasks = new Dictionary<string, NantElement>();
		public readonly IDictionary<string, NantElement> NantElements = new Dictionary<string, NantElement>();
		public readonly IDictionary<string, NantEnumType> EnumTypes = new Dictionary<string, NantEnumType>();
		public readonly IDictionary<string, NantFunctionClass> FunctionClasses = new Dictionary<string, NantFunctionClass>();
		public readonly List<NantElement> PlatformExtensionsList = new List<NantElement>();

		private NantElement PlatformExtensionsElement;

		private readonly Log Log;
		private readonly string LogPrefix;

		public readonly XmlDocDataParser XmlDocs;
		private readonly List<Tuple<string, Assembly>> Assemblies = new List<Tuple<string, Assembly>>();

		public SchemaMetadata(Log log = null, string logprefix = null)
		{
			Log = log;
			LogPrefix = logprefix ?? "[SchemaMetadata]";

			XmlDocs = new XmlDocDataParser(Log, LogPrefix);
		}

		public void AddAssembly(Assembly assembly, string path=null)
		{
			Assemblies.Add(Tuple.Create(path??assembly.Location, assembly));
		}

		public void GenerateSchemaMetaData()
		{
			foreach (var asmInfo in Assemblies)
			{
				if (!String.IsNullOrEmpty(asmInfo.Item1) && !asmInfo.Item1.Any(c => Path.GetInvalidPathChars().Contains(c)))
				{
					string docPath = Path.ChangeExtension(asmInfo.Item1, ".XML");
					XmlDocs.ParseXMLDocumentation(docPath);
				}
			}

			foreach (var asmInfo in Assemblies)
			{
				ProcessAssembly(asmInfo.Item2, asmInfo.Item1);
			}

			foreach (var e in NantElements)
			{
				// Process container as a second pass because we need all elements created to fill containers
				ProcessContainers(e.Value);
			}

			ProcessPlatformExtensions();
		}

		public IDictionary<string, IEnumerable<NantElement>> SortTasksByNamespaces()
		{
			 var nantTasksInNamespaces = new Dictionary<string, IEnumerable<NantElement>>();

			foreach (var t in NantTasks)
			{
				var namesp = "default";
				if (t.Value.ClassType != null && !String.IsNullOrEmpty(t.Value.ClassType.Namespace))
				{
					namesp = t.Value.ClassType.Namespace;
				}
				IEnumerable<NantElement> tasksinnamesp;
				List<NantElement> list;
				if (nantTasksInNamespaces.TryGetValue(namesp, out tasksinnamesp))
				{
					list = tasksinnamesp as List<NantElement>;
				}
				else
				{
					list = new List<NantElement>();
					nantTasksInNamespaces.Add(namesp, list);
				}
				list.Add(t.Value);
			}
			return nantTasksInNamespaces;
		}

		private void ProcessAssembly(Assembly assembly, string path=null)
		{
			if (Log != null)
			{
				Log.Debug.WriteLine(LogPrefix + "SchemaMetadata: processing assembly " + assembly.FullName);
			}
			else
			{
				Console.WriteLine(LogPrefix + "SchemaMetadata: processing assembly " + assembly.FullName);
			}

			Type[] classTypes = assembly.GetTypes();
			foreach (Type classType in classTypes)
			{
				ProcessClassMetaData(classType);
			}
		}

		private bool IsValidNantXmlElement(Type type)
		{
			bool ret =
				type.IsSubclassOf(typeof(Element)) &&
				!type.IsAbstract &&
				!IsSubclass("EA.Eaconfig.Core.IBuildModuleTask", type) && // Pre/postprocess tasks should never appear in build scripts. They are invoked by nant
				!IsSubclass("EA.Eaconfig.GenerateOptions", type) // Generate options tasks are invoked
				;
			return ret;
		}

		private bool IsSubclass(string parentName, Type child)
		{
			for (Type current = child; current != null && current.FullName != "System.Object"; current = current.BaseType) {
				if (child.FullName == parentName)
				{
					return true;
				}
			}
			return false;
		}

		// adding all of the platform extensions to their parent node after they and the parent node have all been found.
		// this step allows us to add the extensions as children of platform extensions dynamically rather than with a hardcoded
		// list of xmlelements above the initializeTask() method.
		private void ProcessPlatformExtensions()
		{
			foreach (NantElement ext in PlatformExtensionsList)
			{
				XmlElementAttribute attr = new XmlElementAttribute(ext.ElementName.Replace("structured-extension-", ""), ext.Key);
				var childElem = CreateChildElement(PlatformExtensionsElement, PlatformExtensionsElement.ClassType, attr as XmlElementAttribute);

				if (childElem != null)
				{
					PlatformExtensionsElement.ChildElements.Add(childElem);
				}
			}
		}

		private void ProcessClassMetaData(Type classType)
		{
			// If we are not retrieving necessary types recursively, we have to scan all types here. 
			// It will create schema with lots of complex type definitions that aren't used anywhere, 
			// but that the only way if we pre extract metadata. Class derived from Element does not even 
			// have to have any attribute in class definition. It still can be used as Task or another element property.
			if (IsValidNantXmlElement(classType))
			{
				var nantElement = CreateNantElement(classType);

				if (!NantElements.ContainsKey(nantElement.Key))
				{
					NantElements.Add(nantElement.Key, nantElement);

					if (nantElement.IsTask && nantElement.IsSchemaElement)
					{
						NantTasks.Add(nantElement.Key, nantElement);
					}

					ProcessMembers(nantElement);

					if (nantElement.ClassType == typeof(EA.Eaconfig.Structured.PartialModuleTask))
					{
						// Because partialModule is a special case: it can represent either DonNet partial module or native partial module
						// we need to use merged schema that is combination of both because XSD 1.0 does not support conditional selection of element types.
						// XSD 1.1 does support this, when Visual Studio
						ProcessPartialModuleMembers(nantElement);
					}
					else
					{
						ProcessMetaElementsMembers(nantElement);
					}

					// add platform extensions to a list and get a reference to the platformextensions element.
					// we need to add the extensions at the end because they are spread across multiple assemblies
					// and the assemblies are processed in a random order.
					if (nantElement.ClassType.BaseType == typeof(EA.Eaconfig.Structured.PlatformExtensionBase))
					{
						PlatformExtensionsList.Add(nantElement);
					}
					else if (nantElement.ClassType == typeof(EA.Eaconfig.Structured.PlatformExtensions))
					{
						PlatformExtensionsElement = nantElement;
					}
				}
			}
			else if (classType.IsSubclassOf(typeof(NAnt.Core.FunctionClassBase))) 
			{
				ProcessFunctionClass(classType);
			}
		}

		private void ProcessFunctionClass(Type type)
		{
			NantFunctionClass functionclass = new NantFunctionClass();
			if (Attribute.GetCustomAttribute(type, typeof(FunctionClassAttribute)) is FunctionClassAttribute attr)
			{
				if (FunctionClasses.ContainsKey(attr.FriendlyName))
				{
					functionclass = FunctionClasses[attr.FriendlyName];
				}
				else
				{
					functionclass.Name = attr.FriendlyName;
					functionclass.Description = XmlDocs.GetClassDescription(type);
				}

				MethodInfo[] methods = type.GetMethods();
				foreach (MethodInfo method in methods)
				{
					object[] attributes = method.GetCustomAttributes(typeof(FunctionAttribute), false);
					if (attributes.Length > 0)
					{
						NantFunction function = new NantFunction();
						function.Name = method.Name;
						function.MethodInfo = method;
						function.Description = XmlDocs.GetMethodDescription(function);
						functionclass.Functions.Add(function);
					}
				}

				if (!FunctionClasses.ContainsKey(functionclass.Name))
				{
					FunctionClasses.Add(functionclass.Name, functionclass);
				}
			}
		}

		private void ProcessMembers(NantElement nantElement)
		{
			foreach (MemberInfo memInfo in nantElement.ClassType.GetMembers())
			{
				if (memInfo.MemberType == MemberTypes.Property)
				{
					NantAttribute nantAttr = CreateNantAttribute(nantElement.ClassType, memInfo as PropertyInfo);
					if (nantAttr != null)
					{
						if (!nantElement.Attributes.Any(atr => atr.AttributeName == nantAttr.AttributeName && atr.IsRequired == nantAttr.IsRequired))
						{
							nantElement.Attributes.Add(nantAttr);
						}
						else
						{
							if (Log != null)
							{
								Log.ThrowWarning
								(
									Log.WarningId.SyntaxError, Log.WarnLevel.Normal,
									"Element '{0}' contains duplicate attributes {1}", nantElement.ElementName, nantAttr.AttributeName
								);
							}
						}
					}
					else
					{

						NantChildElement childElem = CreateChildElement(nantElement.ClassType, memInfo as PropertyInfo);

						if (childElem != null)
						{
							NantChildElement existingChildElem = nantElement.ChildElements.FirstOrDefault(existingChild => existingChild.Name == childElem.Name);
							if (existingChildElem != null)
							{
								if (childElem.Override && childElem.DeclaringType.IsSubclassOf(existingChildElem.DeclaringType))
								{
									nantElement.ChildElements.Remove(existingChildElem);
									nantElement.ChildElements.Add(childElem);
								}
								else if (existingChildElem.Override && existingChildElem.DeclaringType.IsSubclassOf(childElem.DeclaringType))
								{
									// current info is override and from a more derived type, keep the current definition
								}
								else
								{
									throw new Exception($"Element has duplicate nested element '{childElem.Name}'!");
								}
							}
							else
							{
								nantElement.ChildElements.Add(childElem);
							}
						}
					}
				}
			}
		}

		private void ProcessMetaElementsMembers(NantElement nantElement)
		{
			foreach (var memInfo in nantElement.ClassType.GetMembers(BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic))
			{
				if (memInfo.MemberType == MemberTypes.Method)
				{
					foreach (var attr in Attribute.GetCustomAttributes(memInfo, typeof(XmlElementAttribute)))
					{
						var childElem = CreateChildElement(nantElement, nantElement.ClassType, attr as XmlElementAttribute);

						if (childElem != null)
						{
							nantElement.ChildElements.Add(childElem);
						}
					}
				}
			}

			var containerElements = new List<NantChildElement>();

			foreach (var memInfo in nantElement.ClassType.GetMembers(BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic))
			{
				if (memInfo.MemberType == MemberTypes.Method)
				{
					foreach (var attr in Attribute.GetCustomAttributes(memInfo, typeof(XmlElementAttribute)))
					{
						var childElem = CreateContainerChildElement(nantElement, nantElement.ClassType, attr as XmlElementAttribute);

						if (childElem != null)
						{
							containerElements.Add(childElem);
						}
					}
				}
			}
			nantElement.ChildElements.AddRange(containerElements);

		}

		private void ProcessPartialModuleMembers(NantElement nantElement)
		{
			Type classType = typeof(EA.Eaconfig.Structured.PartialModuleTask.PartialModule);

			var partialModuleNantElement = CreateNantElement(classType);

			if (!NantElements.ContainsKey(partialModuleNantElement.Key))
			{
				NantElements.Add(partialModuleNantElement.Key, partialModuleNantElement);
			}
			else
			{
				partialModuleNantElement = NantElements[partialModuleNantElement.Key];
			}

			ProcessMembers(partialModuleNantElement);

			ProcessMetaElementsMembers(partialModuleNantElement);

			foreach (var attr in partialModuleNantElement.Attributes)
			{
				if (attr.AttributeName == "frompartial")
				{
					continue; // partial modules can do anything a regular module can do EXPECT be inherted from another partial so explicitly
					// skip this in schema generation
				}

				if (!nantElement.Attributes.Any(a => a.AttributeName == attr.AttributeName))
				{
					nantElement.Attributes.Add(attr);
				}
			}

			foreach (var elem in partialModuleNantElement.ChildElements)
			{
				if (!nantElement.ChildElements.Any(el => el.Name == elem.Name))
				{
					nantElement.ChildElements.Add(elem);
				}
			}
		}


		private void ProcessContainers(NantElement nantElement)
		{
			// If type has container attribute
			var containerAttributes = nantElement.ClassType.GetMembers(BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic).Where(m => m.MemberType == MemberTypes.Method).SelectMany(m => System.Attribute.GetCustomAttributes(m, typeof(XmlTaskContainerAttribute)));

			var containerElements = new List<NantChildElement>();

			var duplicateCheck = new HashSet<string>(nantElement.ChildElements.Select(cel => cel.Key));

			foreach (XmlTaskContainerAttribute attr in containerAttributes)
			{
				if (attr != null)
				{
					//Add All tasks
					foreach (var e in NantTasks)
					{
						var task = e.Value;

						if (MatchFilter(attr.Filter, task))
						{
							if (duplicateCheck.Add(task.Key))
							{
								var childElement = new NantChildElement(task.Key, task.ElementName, String.Empty, null, String.Empty, null, nantElement.ClassType, false);

								nantElement.ChildElements.Add(childElement);

								nantElement.IsTaskContainer = true;
							}
						}
					}
				}
			}
		}

		private bool MatchFilter(string filter, NantElement el, NantChildElement child=null)
		{
			var match = false;
			if (String.IsNullOrEmpty(filter) ||filter == "*")
			{
				match = true;
			}
			else if (filter == "-")
			{
				match = false;
			}
			else
			{
				//IMTODO - process more complex filters;
				match = true;
			}
			return match;
		}

		private NantElement CreateNantElement(Type type)
		{
			string elementName = String.Empty;
			bool isTask = false;
			bool isStrict = true;
			bool isSchemaElement = true;
			bool isMixed = false;

			bool isTaskContainer = false;
			string summary = GetNantElementSummary(type);
			XmlNode description = GetNantElementDescription(type);

			if (Attribute.GetCustomAttribute(type, typeof(TaskNameAttribute)) is TaskNameAttribute taskNameAttr)
			{
				// this is NAnt task
				elementName = taskNameAttr.Name;
				isTask = type.IsSubclassOf(typeof(NAnt.Core.Task));
				isStrict = taskNameAttr.StrictAttributeCheck;

				isMixed = taskNameAttr.Mixed;
				isSchemaElement = taskNameAttr.XmlSchema;
			}
			else
			{
				if (Attribute.GetCustomAttribute(type, typeof(ElementNameAttribute)) is ElementNameAttribute elementNameAttr)
				{
					// this is NAnt element
					elementName = elementNameAttr.Name;
					isTask = false;
					isStrict = elementNameAttr.StrictAttributeCheck;
					isMixed = elementNameAttr.Mixed;
					isSchemaElement = false;
				}
				else
				{
					elementName = type.Name;
					isTask = false;
					isStrict = true;
					isMixed = false;
					isSchemaElement = false;
				}
			}

			return new NantElement(type, elementName, isTask, isStrict, isSchemaElement, isMixed,  isTaskContainer, summary, description);
		}

		private NantAttribute CreateNantAttribute(Type classType, PropertyInfo propertyInfo)
		{
			if (Attribute.GetCustomAttribute(propertyInfo, typeof(TaskAttributeAttribute)) is TaskAttributeAttribute attribute)
			{
				return new NantAttribute
				(
					propertyInfo.PropertyType,
					attribute.Name,
					attribute.Required,
					GetElementAttributeSummary(classType, propertyInfo),
					GetElementAttributeDescription(classType, propertyInfo),
					GetElementAttributeValidValues(classType, propertyInfo, attribute)
				);
			}
			return null;
		}

		private NantAttribute CreateConditionalNantAttribute(string name, string description)
		{
			NantAttribute nantAttribute = nantAttribute = new NantAttribute(
					typeof(bool),
					name,
					false,
					description??String.Empty,
					null,
					null);

			return nantAttribute;
		}

		private NantChildElement CreateChildElement(Type classType, PropertyInfo propertyInfo)
		{
			if (Attribute.GetCustomAttribute(propertyInfo, typeof(BuildElementAttribute)) is BuildElementAttribute attribute)
			{
				return new NantChildElement
				(
					propertyInfo.PropertyType,
					attribute.Name,
					GetNestedElementSummary(classType, propertyInfo),
					GetNestedElementDescription(classType, propertyInfo),
					GetNestedElementTypeDescription(classType, propertyInfo, attribute),
					GetNestedElementValidValues(classType, propertyInfo, attribute),
					propertyInfo.DeclaringType,
					attribute.Override
				);
			}
			return null;
		}

		private NantChildElement CreateChildElement(NantElement parentElement, Type classType, XmlElementAttribute elementAttribute)
		{
			NantChildElement childElement = null;

			if (elementAttribute != null)
			{
				if (!elementAttribute.IsContainer())
				{
					var propertyType = GetType(elementAttribute.ElementType);
					if (propertyType != null)
					{

						childElement = new NantChildElement
						(
							propertyType,
							elementAttribute.Name,
							elementAttribute.Description,
							null,
							string.Empty, 
							null,
							classType,
							false
						);
					}
				}
			}

			return childElement;
		}

		private NantChildElement CreateContainerChildElement(NantElement parentElement, Type classType, XmlElementAttribute elementAttribute)
		{
			NantChildElement childElement = null;

			if (elementAttribute != null)
			{
				if (elementAttribute.IsContainer())
				{
					var containerType = String.Format("{0}+{1}+{2}", classType.FullName, elementAttribute.Name, elementAttribute.Container.ToString());

					childElement = new NantChildElement(
						containerType,
						elementAttribute.Name,
						elementAttribute.Description,
						null,
						string.Empty,
						null,
						classType,
						false
						);

					var containerElement = new NantElement(
						containerType,
						elementAttribute.Name,
						isTask: false,
						isStrict: true,
						isSchemaElement: false,
						isMixed: elementAttribute.Mixed,
						isTaskContainer: false,
						description: null,
						summary: elementAttribute.Description);

					if (elementAttribute.IsConditionalContainer())
					{
						containerElement.Attributes.Add(CreateConditionalNantAttribute("if", "Execute this element if true"));
						containerElement.Attributes.Add(CreateConditionalNantAttribute("unless", "Execute this element unless true"));
					}

					AddContainerChildElements(parentElement, containerElement, elementAttribute);

					if (!NantElements.ContainsKey(containerElement.Key))
					{
						NantElements.Add(containerElement.Key, containerElement);
					}
				}
			}

			return childElement;
		}


		private void AddContainerChildElements(NantElement parentElement, NantElement containerElement, XmlElementAttribute elementAttribute)
		{
			foreach (var child in parentElement.ChildElements)
			{
				if (MatchFilter(elementAttribute.Filter, parentElement, child))
				{
					containerElement.ChildElements.Add(child);
				}
			}
			if (elementAttribute.IsRecursive())
			{
				// Add self as a child element:
				var self = new NantChildElement
				(
					containerElement.Key,
					containerElement.ElementName,
					String.Empty,
					null,
					string.Empty,
					null,
					parentElement.ClassType,
					false
				);
				containerElement.ChildElements.Add(self);
			}
		}

		private static Type GetType(string typeName)
		{
			var type = Type.GetType(typeName);
			if (type != null) return type;
			foreach (var a in AppDomain.CurrentDomain.GetAssemblies())
			{
				foreach (Type t in a.GetTypes())
				{
					if (t.Name == typeName || t.FullName == typeName)
					{
						return t;
					}
				}
			}
			return null;
		}
		
		private string GetElementAttributeSummary(Type elementType, PropertyInfo propertyInfo)
		{
			var sumamry = XmlDocs.GetClassPropertySummary(elementType, propertyInfo);
			return sumamry.TrimWhiteSpace();
		}

		private XmlNode GetElementAttributeDescription(Type elementType, PropertyInfo propertyInfo)
		{
			return XmlDocs.GetClassPropertyDescription(elementType, propertyInfo);
		}

		private string GetNestedElementSummary(Type elementType, PropertyInfo propertyInfo)
		{
			var summary = XmlDocs.GetClassPropertySummary(elementType, propertyInfo);
			return summary.TrimWhiteSpace();
		}

		private XmlNode GetNestedElementDescription(Type elementType, PropertyInfo propertyInfo)
		{
			return XmlDocs.GetClassPropertyDescription(elementType, propertyInfo);
		}

		private string GetNestedElementTypeDescription(Type elementType, PropertyInfo propertyInfo, BuildElementAttribute attr)
		{
			var typeDescription = propertyInfo.PropertyType.Name;

			if (attr is FileSetAttribute)
			{
				typeDescription = "fileset";
			}
			else if (attr is OptionSetAttribute)
			{
				typeDescription = "optionset";
			}
			else if (!propertyInfo.PropertyType.IsValueType)
			{
				typeDescription = String.Empty;
			}

			return typeDescription;
		}

		private NantEnumType GetElementAttributeValidValues(Type elementType, PropertyInfo propertyInfo, TaskAttributeAttribute attr)
		{
			NantEnumType validValues = null;

			if (propertyInfo.PropertyType.IsEnum)
			{
				validValues = new NantEnumType(propertyInfo.PropertyType);
			}
			else
			{
				// IMTODO: We can add attribute field for a list of valid values for strings, for example;
			}

			if (validValues != null)
			{
				// To avoid duplicates, check if it is already stored;
				NantEnumType foundValue;
				if (EnumTypes.TryGetValue(validValues.Key, out foundValue))
				{
					validValues = foundValue;
				}
				else
				{
					EnumTypes.Add(validValues.Key, validValues);
				}
			}

			return validValues;
		}


		private NantEnumType GetNestedElementValidValues(Type elementType, PropertyInfo propertyInfo, BuildElementAttribute attr)
		{
			NantEnumType validValues = null;

			if(propertyInfo.PropertyType.IsEnum)
			{
				validValues = new NantEnumType(propertyInfo.PropertyType);
			}
			else
			{
				// IMTODO: We can add attribute field for a list of valid values for strings, for example;
			}

			if (validValues != null)
			{
				// To avoid duplicates, check if it is already stored;
				NantEnumType foundValue;
				if (EnumTypes.TryGetValue(validValues.Key, out foundValue))
				{
					validValues = foundValue;
				}
				else
				{
					EnumTypes.Add(validValues.Key, validValues);
				}
			}

			return validValues;
		}

		// For task.
		private string GetNantElementSummary(Type elementType)
		{
			return XmlDocs.GetClassSummary(elementType);
		}

		private XmlNode GetNantElementDescription(Type elementType)
		{
			return XmlDocs.GetClassDescription(elementType);
		}
	}
}
