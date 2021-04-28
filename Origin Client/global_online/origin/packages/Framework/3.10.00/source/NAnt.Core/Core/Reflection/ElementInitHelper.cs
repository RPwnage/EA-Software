using System;
using System.Collections.Generic;
using System.Collections.Concurrent;
using System.Reflection;
using System.Threading;
using System.Threading.Tasks;
using System.Xml;
using System.Text;

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Attributes;
using NAnt.Core.Events;


namespace NAnt.Core.Reflection
{
    public class ElementInitHelper
    {
        public class ElementInitializer
        {
            public class PerClassAttributeInfo
            {
                public PropertyInfo valSetter;			// value setter for the attrubute
                public IPropertyConverter converter;	// convert string supplied in XML into proper type
                public ValidatorAttribute validator;	// validator. may be null.
                public bool bNeedsExpansion = true;	    // indicates if the attribute is expandable.
                public bool isRequired = false;
            }

            public class PerClassNestedElementInfo
            {
                public PropertyInfo valGetter;
                public bool bInitialize;               // indicates that child element needs automatic initialization
                public bool isRequired;
                public PerClassNestedElementInfo() { }
            }

            // this indicates that we need to collect unrecogized attributes and
            // check on them later. it only applies to task elements.
            internal bool bStrictAttributeCheck = false;
            // Total number of required attributes
            internal long numRequiredAttributes = 0;
            // Total number of required elements
            internal long numRequiredElements = 0;

            // per-class initialization information items. they are immutable
            // during element initialization so we can just give away references
            // to them without copying the objects
            //
            private readonly ConcurrentDictionary<string, PerClassAttributeInfo> attributes;
            private readonly ConcurrentDictionary<string, PerClassNestedElementInfo> nestedElements;

            public ElementInitializer(Type eltType)
            {
                attributes = new ConcurrentDictionary<string, PerClassAttributeInfo>();
                nestedElements = new ConcurrentDictionary<string, PerClassNestedElementInfo>();

#if NANT_CONCURRENT
                // Under mono most parallel execution is slower than consequtive. Untill this is resolved use consequtive execution 
                bool parallel = (PlatformUtil.IsMonoRuntime == false);
#else
                bool parallel = false;
#endif
                if (parallel)
                {
                    Parallel.Invoke(
                        () => ProcessProperties(eltType),
                        () => bStrictAttributeCheck = NeedStrictAttributeCheck(eltType));
                }
                else
                {
                    ProcessProperties(eltType);
                    bStrictAttributeCheck = NeedStrictAttributeCheck(eltType);
                }
                HasNestedElements = nestedElements.Count > 0;
            }

            public void InitAttribute(Element elt, string attrName, string attrValue, ref long foundRequiredAttributes, ref long foundUnknownAttributes)
            {
                PerClassAttributeInfo attrInfo;

                if (!attributes.TryGetValue(attrName, out attrInfo))
                {
                    if (bStrictAttributeCheck)
                    {
                        Interlocked.Increment(ref foundUnknownAttributes);
                    }
                    return;
                }

                string expandedAttributeValue;

                if (attrInfo.bNeedsExpansion)
                    expandedAttributeValue = elt.Project.ExpandProperties(attrValue);
                else
                    expandedAttributeValue = attrValue;

                object convertedValue;

                /*
                 convert the xml attribute value, which is stored as a string, to the
                 expected value which is an object of the same type as defined by the
                 property.
                 */
                try
                {
                    convertedValue = attrInfo.converter.Convert(expandedAttributeValue);
                }
                catch (Exception e)
                {
                    string msg = String.Format("Failed to change attribute '{0}' with value '{1}' to type '{2}'",
                            attrName, expandedAttributeValue, attrInfo.valSetter.PropertyType);

                    throw new BuildException(msg, elt.Location, e);
                }

                if (null != attrInfo.validator)
                    attrInfo.validator.Validate(ref convertedValue);

                // finally, set the value
                attrInfo.valSetter.SetValue(elt, convertedValue, null);

                if (attrInfo.isRequired)
                {
                    Interlocked.Increment(ref foundRequiredAttributes);
                }
            }

            public void InitNestedElement(Element parentElement, XmlNode nestedElementNode, bool checkInitialize, ref long foundRequiredElements)
            {
                PerClassNestedElementInfo info;

                if (nestedElements.TryGetValue(nestedElementNode.Name, out info))
                {
                    if (info.isRequired)
                    {
                        Interlocked.Increment(ref foundRequiredElements);
                    }

                    // if the initialize flag is not set, then the element should be
                    // initialized by the task. we also need a method of overriding the
                    // initailize flag so that we can allow for initialization of these
                    // elements at later time even when the flag is set.

                    if (checkInitialize && !info.bInitialize)
                    {
                        return;
                    }

                    // Initialize any child instance elements. E.g. when Task has FileSet,
                    // this code will initialize the fileset.  This will not initialize
                    // elements that are in the collection. For example, the <includes> of a
                    // fileset needs to be initialized manually by the FileSet class by
                    // overriding the InitializeElement method

                    Element childElement = (Element)info.valGetter.GetValue(parentElement, null);
                    childElement.Project = parentElement.Project;
                    childElement.Initialize(nestedElementNode);
                }
            }

            public readonly bool HasNestedElements;

            public bool IsNestedElement(string name)
            {
                return nestedElements.ContainsKey(name);
            }

            public void CheckRequiredAttributes(XmlNode eltNode, long foundRequiredAttributes)
            {
                if (numRequiredAttributes != foundRequiredAttributes)
                {
                    int count = 0;
                    StringBuilder sb = new StringBuilder();
                    foreach (KeyValuePair<string, PerClassAttributeInfo> item in attributes)
                    {
                        if (item.Value.isRequired)
                        {
                            if (null == eltNode.Attributes[item.Key])
                            {
                                if (sb.Length != 0)
                                    sb.Append(", ");
                                sb.Append(item.Key);
                                count++;
                            }
                        }
                    }
                    string msg = String.Format("'{0}' {1} required attribute{2}.", sb.ToString(), count > 1 ? "are" : "is a", count > 1 ? "s" : String.Empty);
                    throw new BuildException(msg, Location.GetLocationFromNode(eltNode));
                }
            }

            public void CheckUnknownAttributes(XmlNode eltNode, long foundUnknownAttributes)
            {
                if (bStrictAttributeCheck && foundUnknownAttributes > 0)
                {
                    StringBuilder sb = new StringBuilder();
                    foreach (XmlAttribute attr in eltNode.Attributes)
                    {
                        if (!attributes.ContainsKey(attr.Name))
                        {
                            if (sb.Length != 0)
                                sb.Append(", ");
                            sb.Append(attr.Name);
                        }
                    }

                    throw new BuildException(String.Format("Task <{0}> has unrecognized attributes: '{1}'.",
                                                            eltNode.Name, sb.ToString()), Location.GetLocationFromNode(eltNode));
                }
            }

            public void CheckRequiredNestedElements(XmlNode eltNode, long foundRequiredElements)
            {
                if (numRequiredElements != foundRequiredElements)
                {
                    int count = 0;
                    bool found = false;
                    StringBuilder sb = new StringBuilder();
                    foreach (KeyValuePair<string, PerClassNestedElementInfo> item in nestedElements)
                    {
                        if (item.Value.isRequired)
                        {
                            found = false;
                            foreach (XmlNode node in eltNode.ChildNodes)
                            {
                                if (node.Name == item.Key)
                                {
                                    found = true;
                                    break;
                                }
                            }
                            if (!found)
                            {
                                if (sb.Length != 0)
                                    sb.Append(", ");
                                sb.Append(item.Key);
                                count++;
                            }
                        }
                    }

                    string msg = String.Format("'{0}' {1} required element{2}.", sb.ToString(), count > 1 ? "are" : "is a", count > 1 ? "s" : String.Empty);
                    throw new BuildException(msg, Location.GetLocationFromNode(eltNode));
                }
            }


            private void ProcessProperties(Type type)
            {
#if NANT_CONCURRENT
                // Under mono most parallel execution is slower than consequtive. Untill this is resolved use consequtive execution 
                bool parallel = (PlatformUtil.IsMonoRuntime == false);
#else
                bool parallel = false;
#endif

                if (parallel)
                {
                    Parallel.ForEach(type.GetProperties(BindingFlags.Public | BindingFlags.Instance), prop =>
                    {
                        if (!StoreBuildAttributeInfo(prop, type))
                        {
                            StoreNestedElementInfo(prop);
                        }
                    });
                }
                else
                {
                    foreach (PropertyInfo prop in type.GetProperties(BindingFlags.Public | BindingFlags.Instance))
                    {
                        if (!StoreBuildAttributeInfo(prop, type))
                        {
                            StoreNestedElementInfo(prop);
                        }
                    }
                }
            }

            private bool StoreBuildAttributeInfo(PropertyInfo propertyInfo, Type type)
            {
                BuildAttributeAttribute buildAttribute = Attribute.GetCustomAttribute(propertyInfo, typeof(BuildAttributeAttribute)) as BuildAttributeAttribute;

                if (buildAttribute == null || !propertyInfo.CanWrite)		// do not care about readonly properties
                {
                    return (buildAttribute != null); // signal that this is an attribute
                }

                PerClassAttributeInfo attrInfo = new PerClassAttributeInfo();

                attrInfo.bNeedsExpansion = buildAttribute.ExpandProperties;

                attrInfo.converter = PropertyConverter.Create(propertyInfo.PropertyType);

                attrInfo.validator = Attribute.GetCustomAttribute(propertyInfo, typeof(ValidatorAttribute)) as ValidatorAttribute;

                attrInfo.valSetter = propertyInfo;

                if (buildAttribute.Required)
                {
                    attrInfo.isRequired = buildAttribute.Required;
                    Interlocked.Increment(ref numRequiredAttributes);
                }

                if (!attributes.TryAdd(buildAttribute.Name, attrInfo))
                {
#if DEBUG
                    PerClassAttributeInfo redefined = attributes[buildAttribute.Name];
                    Console.WriteLine(String.Format("attribute {0} of element {1} has been redefined in element {2}",
                            buildAttribute.Name, type.Name, redefined.valSetter.DeclaringType.Name));
#endif
                }
                return true;
            }

            private void StoreNestedElementInfo(PropertyInfo propertyInfo)
            {
                BuildElementAttribute buildElementAttribute = Attribute.GetCustomAttribute(propertyInfo, typeof(BuildElementAttribute)) as BuildElementAttribute;

                if (buildElementAttribute == null)
                {
                    // it is not a build attribute
                    return;
                }

                PerClassNestedElementInfo info = new PerClassNestedElementInfo();

                info.bInitialize = buildElementAttribute.Initialize;
                info.isRequired = buildElementAttribute.Required;
                info.valGetter = propertyInfo;

                if (buildElementAttribute.Required)
                {
                    info.isRequired = buildElementAttribute.Required;
                    Interlocked.Increment(ref numRequiredElements);
                }


                if (!nestedElements.TryAdd(buildElementAttribute.Name, info))
                {
#if DEBUG
                    throw new BuildException("Internal error: duplicate sub-element definition has been found.");
#endif
                }

            }

            private bool NeedStrictAttributeCheck(Type type)
            {
                if (type.IsSubclassOf(typeof(Task)) && !type.IsAbstract)
                {
                    // check if the class has defined a TaskName attribute
                    TaskNameAttribute attribute =
                            Attribute.GetCustomAttribute(type, typeof(TaskNameAttribute)) as TaskNameAttribute;

                    if (attribute != null && attribute.StrictAttributeCheck)
                        return true;
                }
                else if (type.IsSubclassOf(typeof(Element)) && !type.IsAbstract)
                {
                    // check if the class has defined ElementNameAttribute attribute and StrictAttributeCheck
                    ElementNameAttribute attribute =
                        Attribute.GetCustomAttribute(type, typeof(ElementNameAttribute)) as ElementNameAttribute;

                    if (attribute != null && attribute.StrictAttributeCheck)
                    {
                        return true;
                    }
                }

                return false;
            }
        }

        static ElementInitHelper()
        {
            _initializers = new ConcurrentDictionary<Type, Lazy<ElementInitializer>>();
        }

        static public ElementInitializer GetElementInitializer(Element elt)
        {
            return _initializers.GetOrAddBlocking(elt.GetType(), (type) => new ElementInitializer(type));
        }

        static public T InitAttribute<T>(Element elt, string attrName, string value, bool expand = true)
        {
            if (expand)
            {
                value = elt.Project.ExpandProperties(value);
            }

            try
            {
                T result = (T)System.Convert.ChangeType(value, typeof(T));
                return result;
            }
            catch (Exception e)
            {
                string msg = String.Format("Failed to change attribute '{0}' with value '{1}' to type '{2}'",
                        attrName, value, typeof(T).Name);

                throw new BuildException(msg, elt.Location, e);
            }
        }

        static public bool InitBoolAttribute(Element elt, string attrName, string value, bool expand = true)
        {
            if (expand)
            {
                value = elt.Project.ExpandProperties(value);
            }
            try
            {
                return Expression.Evaluate(value);
            }
            catch (Exception e)
            {
                string msg = String.Format("Failed to change attribute '{0}' with value '{1}' to type '{2}'",
                        attrName, value, "bool");

                throw new BuildException(msg, elt.Location, e);
            }
        }

        static public string InitStringAttribute(Element elt, string attrName, string value, bool expand = true)
        {
            if (expand)
            {
                value = elt.Project.ExpandProperties(value);
            }
            return value;
        }

        static public int InitIntAttribute(Element elt, string attrName, string value, bool expand = true)
        {
            if (expand)
            {
                value = elt.Project.ExpandProperties(value);
            }

            int result;
            if (!Int32.TryParse(value, out result))
            {
                string msg = String.Format("Failed to change attribute '{0}' with value '{1}' to type '{2}'",
                    attrName, value, "int");

                throw new BuildException(msg, elt.Location);
            }

            return result;
        }

        static public T InitEnumAttribute<T>(Element elt, string attrName, string value, bool expand = true) where T : struct
        {
            if (expand)
            {
                value = elt.Project.ExpandProperties(value);
            }

            T result;
            if (!Enum.TryParse<T>(value, out result))
            {
                // catch all type conversion exceptions here, and rethrow with a friendlier message
                StringBuilder sb = new StringBuilder("Invalid value '" + value + "'. Valid values for this attribute are: ");
                Array array = Enum.GetValues(typeof(T));
                for (int i = 0; i < array.Length; i++)
                {
                    sb.Append(array.GetValue(i).ToString());
                    if (i <= array.Length - 1)
                    {
                        sb.Append(", ");
                    }
                }

                string msg = String.Format("Failed to change attribute '{0}' with value '{1}' to type '{2}'{3}{4}",
                    attrName, value, typeof(T).Name, Environment.NewLine, sb.ToString());

                throw new BuildException(msg, elt.Location);
            }

            return result;

        }

        private static readonly ConcurrentDictionary<Type, Lazy<ElementInitializer>> _initializers;
    }
}
