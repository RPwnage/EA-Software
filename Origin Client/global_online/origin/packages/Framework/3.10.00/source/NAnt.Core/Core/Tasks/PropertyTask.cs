// NAnt - A .NET build tool
// Copyright (C) 2001-2003 Gerry Shaw
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
// 
// 
// File Maintainers:
// Gerry Shaw (gerry_shaw@yahoo.com)

using System.IO;
using System;
using System.Xml;

using NAnt.Core.Attributes;
using NAnt.Core.Logging;
using NAnt.Core.Reflection;

namespace NAnt.Core.Tasks
{

    /// <summary>Sets a property in the current project.</summary>
    /// <remarks>
    ///   <para>NAnt uses a number of predefined properties that start with nant.* or [taskname].*.  In general you should place properties into a namespace such as global.* or ProjectName.*.</para>
    ///   <para>If the property name is invalid a build error will occur.</para>
    ///   <para>The following regular expression is used to test for valid properties: <c>^([\w-\.]+)$</c>.  In English this means only A-Z, a-z, 0-9, '_', '-', and '.' characters are allowed.  The leading character should be a letter for readability.</para>
    ///   <para>The task declares the <c>${property.value}</c> property within the task itself.  The <c>${property.value}</c> property is equal to the previous value of the property if already defined, otherwise it is equal to an empty string.  By using the <c>${property.value}</c> property, user can easily insert/append to an existing property.</para>
    /// </remarks>
    /// <include file='Examples/Property/Simple.example' path='example'/>
    /// <include file='Examples/Property/Nested.example' path='example'/>
    /// <include file='Examples/Property/PropertyValue.example' path='example'/>
    /// <include file='Examples/Property/PropertyFromFile.example' path='example'/>
    [TaskName("property", Mixed = true)]
    public class PropertyTask : Task
    {
        string _name = null;
        string _value = null;
        string _fileName = null;
        bool _readOnly = false;
        bool _deferred = false;
        bool _local = false;
        private string localValue;

        /// <summary>The name of the property to set.</summary>        
        [TaskAttribute("name", Required = true)]
        public string PropertyName
        {
            get { return _name; }
            set { _name = value; }
        }

        /// <summary>The value of the property. If not specified, the default will be no value.</summary>        
        [TaskAttribute("value", ExpandProperties = false)]
        public string Value
        {
            get { return _value; }
            set { _value = value; }
        }

        /// <summary>The path to a file from which content is read into the property value.</summary>
        [TaskAttribute("fromfile", Required = false)]
        public string FileName
        {
            get { return _fileName; }
            set { _fileName = value; }
        }

        /// <summary>Indicates if the property should be read-only.  Read only properties can never be changed.  Default is false.</summary>
        [TaskAttribute("readonly")]
        public bool ReadOnly
        {
            get { return _readOnly; }
            set { _readOnly = value; }
        }

        /// <summary>Indicates if the property's value will expand encapsulated properties' value at definition time or at use time. Default is false.</summary>
        [TaskAttribute("deferred")]
        public bool Deferred
        {
            get { return _deferred; }
            set { _deferred = value; }
        }

        /// <summary>Indicates if the property is going to be defined in a local context and thus, it will be restricted to a local scope. Default is false.</summary>
        [TaskAttribute("local")]
        public bool Local
        {
            get { return _local; }
            set { _local = value; }
        }

        /// <summary>Optimization. Directly intialize</summary>
        public override void Initialize(XmlNode elementNode)
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

            string _readOnlyVal = null;
            string _deferredVal = null;
            string _localVal = null;

            try
            {
                foreach (XmlAttribute attr in elementNode.Attributes)
                {
                    switch (attr.Name)
                    {
                        case "name": // REq
                            _name = attr.Value; 
                            break;
                        case "value":  
                            _value = attr.Value;
                            break;
                        case "fromfile":
                            _fileName = attr.Value;
                            break;
                        case "readonly":
                            _readOnlyVal = attr.Value;
                            break;
                        case "deferred":
                            _deferredVal = attr.Value;
                            break;
                        case "local":
                            _localVal = attr.Value;
                            break;
                        default:
                            if (!OptimizedTaskElementInit(attr.Name, attr.Value))
                            {
                                string msg = String.Format("Unknown attribute '{0}'='{1}' in Property task", attr.Name, attr.Value);
                                throw new BuildException(msg, Location);
                            }
                            break;
                    }
                }

                if (_name == null)
                {
                    string msg = String.Format("Missing required <property> attribute 'name'");
                    throw new BuildException(msg, Location.GetLocationFromNode(elementNode));
                }


                if (IfDefined && !UnlessDefined)
                {
                    _name = Project.ExpandProperties(_name);
                    if (_fileName != null) _fileName = Project.ExpandProperties(_fileName);
                    if (_readOnlyVal != null) _readOnly = ElementInitHelper.InitBoolAttribute(this, "readonly", _readOnlyVal);
                    if (_deferredVal != null) _deferred = ElementInitHelper.InitBoolAttribute(this, "deferred", _deferredVal);
                    if (_localVal != null) _local = ElementInitHelper.InitBoolAttribute(this, "local", _localVal);

                    // Just defer for now so that everything just works
                    InitializeTask(elementNode);
                }

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
                throw new ContextCarryingException(e, Name, Location);
            }
        }
 
        ///<summary>Initializes task and ensures the supplied attributes are valid.</summary>
        ///<param name="taskNode">Xml node used to define this task instance.</param>
        protected override void InitializeTask(XmlNode taskNode)
        {
            bool expand = true;

            if (_value != null && taskNode.InnerText.Length != 0)
            {
                throw new BuildException("The property value can only appear in the value attribute or the element body.", Location);
            }

            if ((_value != null || taskNode.InnerText.Length != 0) && FileName != null)
            {
                throw new BuildException("The property value can not be defined together with the 'fromfile' attribute.", Location);
            }

            if (_name != null && _value == null && _fileName == null)
            {
                // get property value from element data (ie, <property name="foo">${bar}</property>
                _value = taskNode.InnerText;
            }
            else if (_name != null && _value == null && _fileName != null)
            {
                // get property value from a file (ie, <property name="foo" fromfile="filename.txt" /> )
                expand = false;
                try
                {
                    // If file exists but empty, property value will be left null. To prevent failure due to null property
                    // intialize it to an empty value by default:
                    Value = String.Empty;
                    string path = Project.GetFullPath(FileName);
                    StreamReader sr = File.OpenText(path);
                    sr.BaseStream.Seek(0, SeekOrigin.Begin);
                    while (sr.Peek() > -1)
                    {
                        Value += sr.ReadLine();
                        if (sr.Peek() > -1)
                            Value += System.Environment.NewLine;
                    }
                    sr.Close();
                }
                catch (System.Exception e)
                {
                    throw new BuildException(e.Message, Location);
                }
            }
            if (expand)
            {
                if (!Deferred)
                {
                    localValue = Properties[PropertyName];
                    if (localValue == null)
                    {
                       localValue = String.Empty;
                    }

                    Value = StringParser.ExpandString(Value, new StringParser.PropertyEvaluator(EvaluateParameter), new StringParser.FunctionEvaluator(Properties.EvaluateFunction));

                    localValue = null;
                }
            }
            else
            {
                Deferred = true;
            }
        }

        protected override void ExecuteTask()
        {
            Properties.Add(PropertyName, Value, ReadOnly, Deferred, Local);
        }

        private string EvaluateParameter(string name)
        {
            if (name == "property.value")
                return localValue;

            return Properties[name];
        }

    }
}
