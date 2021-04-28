using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Reflection;
using System.IO;

using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Logging;
using System.Xml;

namespace EA.Eaconfig
{
    public class SchemaMetadata
    {
        public readonly IDictionary<string, NantElement> NantTasks = new Dictionary<string, NantElement>();
        public readonly IDictionary<string, NantElement> NantElements = new Dictionary<string, NantElement>();
        public readonly IDictionary<string, NantEnumType> EnumTypes = new Dictionary<string, NantEnumType>();
        public readonly IDictionary<string, NantFunctionClass> FunctionClasses = new Dictionary<string, NantFunctionClass>();

        private readonly Log Log;
        private readonly string LogPrefix;

        public readonly XmlDocDataParser XmlDocs;
        private readonly List<Tuple<string, Assembly>> Assemblies = new List<Tuple<string, Assembly>>();

        public SchemaMetadata(Log log, string logprefix)
        {
            Log = log;
            LogPrefix = logprefix;

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
            Log.Debug.WriteLine(LogPrefix + "ShemaMetadata: processing assembly " + assembly.FullName);

            Type[] classTypes = assembly.GetTypes();
            foreach (Type classType in classTypes)
            {
                ProcessClassMetaData(classType);
            }
        }

        private bool IsValidNantXmlElement(Type type)
        {
            bool ret =
                type.IsSubclassOf(typeof(NAnt.Core.Element)) &&
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

                    if (nantElement.IsTask)
                    {
                        NantTasks.Add(nantElement.Key, nantElement);
                    }

                    ProcessMembers(nantElement);

                    ProcessMetaElementsMembers(nantElement);
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
            FunctionClassAttribute attr = Attribute.GetCustomAttribute(type, typeof(FunctionClassAttribute)) as FunctionClassAttribute;
            if (attr != null)
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
            foreach (var memInfo in nantElement.ClassType.GetMembers())
            {
                if (memInfo.MemberType == MemberTypes.Property)
                {
                    var nantAttr = CreateNantAttribute(nantElement.ClassType, memInfo as PropertyInfo);
                    if (nantAttr != null)
                    {
                        if (!nantElement.Attributes.Any(atr => atr.AttributeName == nantAttr.AttributeName))
                        {
                            nantElement.Attributes.Add(nantAttr);
                        }
                        else
                        {
                            if (Log != null)
                            {
                            Log.Warning.WriteLine("Element '{0}' contains duplicate attributes {1}", nantElement.ElementName, nantAttr.AttributeName);
                        }
                    }
                    }
                    else
                    {
                        var childElem = CreateChildElement(nantElement.ClassType, memInfo as PropertyInfo);

                        if (childElem != null)
                        {
                            nantElement.ChildElements.Add(childElem);
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
                    foreach (var attr in System.Attribute.GetCustomAttributes(memInfo, typeof(XmlElementAttribute)))
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
                    foreach (var attr in System.Attribute.GetCustomAttributes(memInfo, typeof(XmlElementAttribute)))
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
                                var childElement = new NantChildElement(task.Key, task.ElementName, String.Empty, null, String.Empty, null);

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

            var taskNameAttr = Attribute.GetCustomAttribute(type, typeof(TaskNameAttribute)) as TaskNameAttribute;

            if (taskNameAttr != null)
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
                var elementNameAttr = Attribute.GetCustomAttribute(type, typeof(ElementNameAttribute)) as ElementNameAttribute;

                if (elementNameAttr != null)
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
            NantAttribute nantAttribute = null;

            var attribute = Attribute.GetCustomAttribute(propertyInfo, typeof(TaskAttributeAttribute)) as TaskAttributeAttribute;

            if (attribute != null)
            {
                nantAttribute = new NantAttribute(
                    propertyInfo.PropertyType, 
                    attribute.Name, 
                    attribute.Required, 
                    GetElementAttributeSummary(classType, propertyInfo),
                    GetElementAttributeDescription(classType, propertyInfo),
                    GetElementAttributeValidValues(classType, propertyInfo, attribute));
            }

            return nantAttribute;
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
            NantChildElement childElement = null;

            var attribute = System.Attribute.GetCustomAttribute(propertyInfo, typeof(BuildElementAttribute)) as BuildElementAttribute;

            if (attribute != null)
            {
                childElement = new NantChildElement(
                    propertyInfo.PropertyType, 
                    attribute.Name, 
                    GetNestedElementSummary(classType, propertyInfo),
                    GetNestedElementDescription(classType, propertyInfo),
                    GetNestedElementTypeDescription(classType, propertyInfo, attribute),
                    GetNestedElementValidValues(classType, propertyInfo, attribute)
                    );
            }

            return childElement;
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

                        childElement = new NantChildElement(
                            propertyType,
                            elementAttribute.Name,
                            elementAttribute.Description,
                            null,
                            string.Empty, 
                            null
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
                        null
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
                var self = new NantChildElement(
                    containerElement.Key,
                    containerElement.ElementName,
                    String.Empty,
                    null,
                    string.Empty,
                    null
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
