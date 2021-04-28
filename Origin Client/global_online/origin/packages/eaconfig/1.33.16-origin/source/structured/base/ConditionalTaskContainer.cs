using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Tasks;
using EA.FrameworkTasks;

using System;
using System.Reflection;
using System.Collections;
using System.Collections.Specialized;
using System.Collections.Generic;
using System.Xml;


namespace EA.Eaconfig.Structured
{
    public abstract class ConditionalTaskContainer : TaskContainer
    {
        static ConditionalTaskContainer()
        {
            MethodInfo mi = typeof(ElementInitHelper).GetMethod("CheckRequiredAttributes", BindingFlags.Public | BindingFlags.Static);
            if (mi == null)
            {
                throw new BuildException("This version of Framework (nant) package does not support Structured XML input. Update Framework package");
            }
        }
        protected StringCollection _parentSubXMLElements = null;
        protected Element _parentElement = null;

        protected override void InitializeAttributes(XmlNode elementNode) {

            // init attributes
            foreach( XmlAttribute attr in elementNode.Attributes )
            {
                ElementInitHelper.InitAttribute( this, attr.Name, attr.Value ) ;
            }

            ElementInitHelper.CheckRequiredAttributes( this ) ;

            if (ElementInitHelper.NeedInitChildren(this))
            {
                // init nested elements
                foreach (XmlNode node in elementNode.ChildNodes)
                {
                    ElementInitHelper.InitNestedElement(this, node, true);
                }

                // Do this check at a later stage in initializeTask
                //ElementInitHelper.CheckRequiredNestedElements( this ) ;
            }
        }

        protected virtual void InitializeParentTask(System.Xml.XmlNode taskNode, Element parent)
        {
            _parentSubXMLElements = new StringCollection();

             _parentElement = parent;

            if (_parentElement != null)
            {
                foreach (MemberInfo memInfo in _parentElement.GetType().GetMembers(BindingFlags.Instance | BindingFlags.Public))
                {
                    if (memInfo.DeclaringType.Equals(typeof(object)))
                    {
                        continue;
                    }

                    BuildElementAttribute buildElemAttr = (BuildElementAttribute)Attribute.GetCustomAttribute(memInfo, typeof(BuildElementAttribute), true);
                    if (buildElemAttr != null)
                    {
                        _parentSubXMLElements.Add(buildElemAttr.Name);
                    }
                }
            }
        }

        protected override void ExecuteChildTasks()
        {
            foreach (XmlNode childNode in _xmlNode)
            {
                if (childNode.Name.StartsWith("#") &&
                   childNode.NamespaceURI.Equals(Project.BuildFileDocument.DocumentElement.NamespaceURI))
                {
                    continue;
                }

                if(IsPrivateXMLElement(childNode)) continue;

                if (IsParentXMLElement(childNode))
                {
                    NAnt.Core.ElementInitHelper.InitNestedElement((Element)_parentElement, childNode, true);
                }
                else
                {
                    Task task = CreateChildTask(childNode);
                    // we should assume null tasks are because of incomplete metadata about the xml.
                    if (task != null)
                    {
                        task.Parent = this;
                        task.Execute();
                    }
                }
            }
            try
            {
                ElementInitHelper.CheckRequiredNestedElements(this);
            }
            catch (BuildException bex)
            {
                throw new BuildException("Error in task  <" + this.Name + ">: " + bex.BaseMessage, bex.Location);
            }
        }

        protected override Task CreateChildTask(XmlNode node)
        {
            Task task = null;

            if (node.NodeType != XmlNodeType.Comment)
            {

                if (node.Name == "do")
                {
                    task = new SectionTask();

                    task.Project = Project;
                    task.Parent = null;
                    if (!Project.ProbeOnly || (Project.ProbeOnly && task.ProbingSupported))
                    {
                        task.Initialize(node);
                    }
                }
                else
                {
                    task = base.CreateChildTask(node);
                }

                ConditionalTaskContainer taskContainer = task as ConditionalTaskContainer;

                Element parent = _parentElement == null ? this : _parentElement;

                if (taskContainer != null)
                {
                    taskContainer.InitializeParentTask(node, parent);
                }
                else if (task != null && !IsValidElement(task))
                {
                    Location loc = (task != null) ? task.Location : Location.GetLocationFromNode(node);
                    Error.Throw(Project, loc, "<" + parent.Name + ">", "XML element <{0}> is not allowed inside <{1}> element.", node.Name, parent.Name);
                }
            }

            return task;
        }

        protected virtual bool IsParentXMLElement(XmlNode node)
        {
            return (_parentSubXMLElements != null && _parentSubXMLElements.Contains(node.Name));
        }

        protected virtual bool IsValidElement(Element task)
        {
            bool valid = task != null && task is EchoTask;
            return valid;
        }

    }
}
