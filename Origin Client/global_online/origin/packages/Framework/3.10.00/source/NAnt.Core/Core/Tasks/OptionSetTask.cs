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

using System;
using System.Collections.Specialized;
using System.Xml;

using NAnt.Core.Util;
using NAnt.Core.Logging;
using NAnt.Core.Attributes;
using NAnt.Core.Reflection;



namespace NAnt.Core.Tasks
{

    /// <summary>Optionset is a dictionary containing [name, value] pairs called options.</summary>
    /// <include file="Examples/OptionSet/OptionSet.remarks" path="remarks"/>
    /// <include file="Examples/OptionSet/Simple.example" path="example"/>
    [TaskName("optionset")]
    public class OptionSetTask : Task
    {
        private string      _name   = null;
        private string      _fromOptionSetName   = null;
        private OptionSet _optionSet = null;
        private bool _append = true;
        private bool _needAdd = true;

        /// <summary>The name of the option set.</summary>        
        [TaskAttribute("name", Required = true)]
        public string OptionSetName
        {
            get { return _name; }
            set { _name = value; }
        }

        /// <summary>If append is true, the options specified by
        /// this option set task are added to the options contained in the
        /// named option set.  Options that already exist are replaced.
        /// If append is false, the named option set contains the options 
        /// specified by this option set task.
        /// Default is "true".</summary>
        [TaskAttribute("append", Required = false)]
        public bool Append
        {
            get { return _append; }
            set { _append = value; }
        }

        /// <summary>The name of a file set to include.</summary>
        [TaskAttribute("fromoptionset", Required = false)]
        public string FromOptionSetName
        {
            get { return _fromOptionSetName; }
            set { _fromOptionSetName = value; }
        }

        /// <summary>Optimization. Directly intialize</summary>
        [XmlElement("option", "NAnt.Core.OptionSet+Option", AllowMultiple = true)]
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
            string _appendVal = null;
            try
            {
                foreach (XmlAttribute attr in elementNode.Attributes)
                {
                    switch (attr.Name)
                    {
                        case "name":
                            _name = attr.Value;
                            break;
                        case "fromoptionset":
                            _fromOptionSetName = attr.Value;
                            break;
                        case "append":
                            _appendVal = attr.Value;
                            break;
                        default:
                            if (!OptimizedTaskElementInit(attr.Name, attr.Value))
                            {
                                string msg = String.Format("Unknown attribute '{0}'='{1}' in OptionSet task", attr.Name, attr.Value);
                                throw new BuildException(msg, Location);
                            }
                            break;
                    }
                }

                if (_name == null)
                {
                    string msg = String.Format("Missing required <optionset> attribute 'name'");
                    throw new BuildException(msg, Location.GetLocationFromNode(elementNode));
                }

                if (IfDefined && !UnlessDefined)
                {
                    _name = Project.ExpandProperties(_name);
                    if (_fromOptionSetName != null) _fromOptionSetName = Project.ExpandProperties(_fromOptionSetName);
                    if (_appendVal != null) _append = ElementInitHelper.InitBoolAttribute(this, "append", _appendVal);

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

        protected override void InitializeTask(XmlNode elementNode)
        {
            // Since each optionset task starts with an empty optionset, so before initializing this optionset, 
            // we need to populate it with its existing options if this optionset already exists.
            // This makes sure that the ${option.value} property always works.
            if (!(Append == true && Project.NamedOptionSets.TryGetValue(OptionSetName, out _optionSet)))
            {
                _optionSet = new OptionSet();
                _elementName = Name;
                _optionSet.Location = Location;
                _optionSet.FromOptionSetName = FromOptionSetName;
            }
            else
            {
                _needAdd = false;
            }
            _optionSet.Project = Project;
            _optionSet.SetOptions(elementNode, FromOptionSetName);
        }

        protected override void ExecuteTask()
        {
            if (_needAdd)
            {
                Project.NamedOptionSets[OptionSetName] = _optionSet;
            }
        }
    }
}
