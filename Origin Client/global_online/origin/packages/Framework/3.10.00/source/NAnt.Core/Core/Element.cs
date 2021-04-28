using System;
using System.Collections.Concurrent;
using System.Xml;
using System.Threading;
using System.Threading.Tasks;

using NAnt.Core.Reflection;
using NAnt.Core.Threading;
using NAnt.Core.Logging;
using NAnt.Core.Util;

namespace NAnt.Core
{
    /// <summary>Models a NAnt XML element in the build file.</summary>
    /// <remarks>
    ///   <para>Automatically validates attributes in the element based on Attribute settings in the derived class.</para>
    /// </remarks>
    abstract public class Element
    {
        public string Name
        {
            get { return _elementName; }
        } internal string _elementName;

        public StringParser.PropertyEvaluator LocalContext = null;

        protected Location _location = Location.UnknownLocation;
        protected Project _project = null;
        
        private object _parent = null;

        public Element()
        {
        }

        public Element(string name)
        {
            _elementName = name;
        }

        public Element(Element e) : this(e.Name)
        {
            _location = e._location;
            _project = e._project;
            _parent = e._parent;
        }

        /// <summary><see cref="Location"/> in the build file where the element is defined.</summary>
        public virtual Location Location
        {
            get { return _location; }
            set { _location = value; }
        }

        /// <summary>The <see cref="Project"/> this element belongs to.</summary>
        public Project Project
        {
            get { return _project; }
            set
            {
                _project = value;
            }
        }

        /// <summary>The Parent object. This will be your parent Task, Target, or Project depeding on where the element is defined.</summary>
        public object Parent
        {
            get { return _parent; }
            set { _parent = value; }
        }

        /// <summary>The properties local to this Element and the project.</summary>
        public PropertyDictionary Properties
        {
            get { return _project.Properties; }
        }

        /// <summary>The Log instance associated with the project.</summary>
        public Log Log
        {
            get { return _project.Log; }
        }

        /// <summary>Performs default initialization.</summary>
        /// <remarks>
        ///   <para>Derived classes that wish to add custom initialization should override <see cref="InitializeElement"/>.</para>
        /// </remarks>
        public virtual void Initialize(XmlNode elementNode)
        {
            if (Project == null)
            {
                throw new InvalidOperationException("Element has invalid (null) Project property.");
            }

            // Save position in buildfile for reporting useful error messages.
            if (!String.IsNullOrEmpty(elementNode.BaseURI))
            {
                try
                {
                    Location = Location.GetLocationFromNode(elementNode);
                }
                catch (ArgumentException ae)
                {
                    Log.Warning.WriteLine("Can't find node '{0}' location, file: '{1}'{2}{3}", elementNode.Name, elementNode.BaseURI, Environment.NewLine, ae.ToString());
                }
            }
                        
            try
            {
                InitializeAttributes(elementNode);

                // Allow inherited classes a chance to do some custom initialization.
                InitializeElement(elementNode);

            }
            catch (BuildException e)
            {
                if (e.Location == Location.UnknownLocation)
                {
                    e = new BuildException(e.Message, this.Location, e.InnerException, e.StackTraceFlags);
                }
                throw new ContextCarryingException(e, Name);
            }
            catch (System.Exception e)
            {
                Exception ex = ThreadUtil.ProcessAggregateException(e);

                if (ex is BuildException)
                {
                    BuildException be = ex as BuildException;
                    if (be.Location == Location.UnknownLocation)
                    {
                        be = new BuildException(be.Message, Location, be.InnerException);
                    }
                    throw new ContextCarryingException(be, Name);
                }

                throw new ContextCarryingException(ex, Name, Location);
            }
        }

        /// <summary>Helper task for manual initialization of a build element.(nested into this element)</summary>
        protected void InitializeBuildElement(XmlNode elementNode, string xmlName, bool isRequired=false)
        {
            // get value from xml node
            XmlNode nestedElementNode = elementNode[xmlName, elementNode.OwnerDocument.DocumentElement.NamespaceURI];

            if (nestedElementNode != null)
            {                
                long foundRequiredElements = 0;
                ElementInitHelper.GetElementInitializer(this).InitNestedElement(this, nestedElementNode, false, ref foundRequiredElements);
            }
            else if (isRequired) {
                string msg = String.Format("'{0}' is a required element.", xmlName);
                throw new BuildException(msg, Location);
            }
        }


        protected virtual void InitializeAttributes(XmlNode elementNode)
        {
            ElementInitHelper.ElementInitializer initializer = ElementInitHelper.GetElementInitializer(this);

            long foundRequiredAttributes = 0;
            long foundUnknownAttributes = 0;
            long foundRequiredElements = 0;

            // init attributes
#if NANT_CONCURRENT_DISABLE // looks like in most cases running this in parallel does not give any advantage
            Parallel.For(0, elementNode.Attributes.Count, i =>
            {
                XmlAttribute attr = elementNode.Attributes[i];
                initializer.InitAttribute(this, attr.Name, attr.Value, ref foundRequiredAttributes, ref foundUnknownAttributes);
            });
#else
            for(int i = 0; i < elementNode.Attributes.Count; i++)
            {
                XmlAttribute attr = elementNode.Attributes[i];

                initializer.InitAttribute(this, attr.Name, attr.Value, ref foundRequiredAttributes, ref foundUnknownAttributes);
            }
#endif

            initializer.CheckRequiredAttributes(elementNode, foundRequiredAttributes);
            initializer.CheckUnknownAttributes(elementNode, foundUnknownAttributes);

            if (initializer.HasNestedElements)
            {
#if NANT_CONCURRENT_DISABLE // Can't do parallel. Structured XML and some other elements will produce wrong data if nested elements are processed out of order.
                Parallel.For(0, elementNode.ChildNodes.Count, j =>
                {
                    initializer.InitNestedElement(this, elementNode.ChildNodes[j], true, ref foundRequiredElements);
                });
#else
                foreach(XmlNode childNode in elementNode.ChildNodes)
                {
                    initializer.InitNestedElement(this, childNode, true, ref foundRequiredElements);
                }
#endif
                initializer.CheckRequiredNestedElements(elementNode, foundRequiredElements);
            }
        }

        /// <summary>Allows derived classes to provide extra initialization and validation not covered by the base class.</summary>
        /// <param name="elementNode">The xml node of the element to use for initialization.</param>
        protected virtual void InitializeElement(XmlNode elementNode) { }
    }
}
