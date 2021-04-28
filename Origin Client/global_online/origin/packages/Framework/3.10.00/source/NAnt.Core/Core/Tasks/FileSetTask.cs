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
// Paul Lalonde (plalonde@ea.com)

using System;
using System.Collections.Specialized;
using System.Xml;

using NAnt.Core.Attributes;
using NAnt.Core.Logging;
using NAnt.Core.Reflection;
using NAnt.Core.Threading;

namespace NAnt.Core.Tasks
{

    /// <summary>
    /// Filesets are groups of files defined by a set of patterns.
    /// </summary>
    /// <include file="Examples/FileSet/Filesets.remarks" path="remarks"/>
    /// 
    /// <include file="Examples/FileSet/Simple.example" path="example"/>
    /// <include file="Examples/FileSet/Group.example" path="example"/>
    [TaskName("fileset")]
    public class FileSetTask : Task
    {
        string _name = null;
        FileSet _fileset = new FileSet();
        bool _append = false;

        /// <summary>Name for fileset. Preceding and trailing spaces will be trimmed off.</summary>
        [TaskAttribute("name", Required = true)]
        [StringValidator(Trim = true)]
        public string FileSetName
        {
            get { return _name; }
            set { _name = value; }
        }

        /// <summary>If append is true, the patterns specified by
        /// this task are added to the patterns contained in the
        /// named file set.  If append is false, the named file set contains
        /// the patterns specified by this task.
        /// Default is "false".</summary>
        [TaskAttribute("append", Required = false)]
        public bool Append
        {
            get { return _append; }
            set { _append = value; }
        }

        /// <summary>The base directory of the file set.  Default is the directory where the
        /// build file is located.</summary>
        [TaskAttribute("basedir", Required = false)]
        public string BaseDirectory
        {
            get { return _fileset.BaseDirectory; }
            set { _fileset.BaseDirectory = value; }
        }

        /// <summary>The name of a file set to include.</summary>
        [TaskAttribute("fromfileset", Required = false)]
        public string FromFileSetName
        {
            get { return _fileset.FromFileSetName; }
            set { _fileset.FromFileSetName = value; }
        }

        /// <summary>Sort the file set by filename. Default is false.</summary>
        [TaskAttribute("sort", Required = false)]
        public bool Sort
        {
            get { return _fileset.Sort; }
            set { _fileset.Sort = value; }
        }

        /// <summary>Indicates if a build error should be raised if an explictly included file does not exist.  Default is true.</summary>
        [TaskAttribute("failonmissing", Required = false)]
        public bool FailOnMissing
        {
            get { return _fileset.FailOnMissingFile; }
            set { _fileset.FailOnMissingFile = value; }
        }

        /// <summary>
        /// If true then the file name will be added to the fileset without pattern matching or checking if the file exists. Default is "false".
        /// </summary>
        [TaskAttribute("asis", Required = false)]  // For documentation
        public bool DummyAsis
        {
            set { }
        }

        /// <summary>
        /// The name of an optionset to associate with this set of excludes.
        /// </summary>
        [TaskAttribute("optionset", Required = false)] // For documentation
        public string DummyOptionset
        {
            set { }
        }

        /// <summary>
        /// If true a the file name will be added to the fileset regardless if it is already included. Default is "false".
        /// </summary>
        [TaskAttribute("force", Required = false)] // For documentation
        public bool DummyForce
        {
            set { }
        }

        /// <summary>
        /// <b>Invalid element in Framework</b>. Added to prevent existing scripts failing.
        /// </summary>
        [TaskAttribute("depends", Required = false)]
        public string DummyDepends
        {
            set { Log.Warning.WriteLine(LogPrefix + "*** invalid <fileset> attribute:  depends."); }
        }

        /// <summary>
        /// If "false", default excludes are not used otherwise default excludes are used. Default is "true"
        /// <list type="bullet">
        /// <item>
        ///    <item>**/CVS/*</item>
        ///    <item>**/.cvsignore</item>
        ///    <item>**/*~</item>
        ///    <item>**/#*#</item>
        ///    <item>**/.#*</item>
        ///    <item>**/%*%</item>
        ///    <item>**/SCCS</item>
        ///    <item>**/SCCS/**</item>
        ///    <item>**/vssver.scc</item>
        /// </list>
        /// </summary>
        [TaskAttribute("defaultexcludes", Required = false)]  
        public string DummyDefaultexcludes
        {
            set { }
        }


#if BPROFILE
		/// <summary>Return additional info identifying the task to the BProfiler.</summary>
		public override string BProfileAdditionalInfo 
		{
			get { return FileSetName; }
		}
#endif

        /// <summary>Optimization. Directly intialize</summary>
        [XmlElement("includes", "NAnt.Core.FileSet+IncludesElement", AllowMultiple = true)]
        [XmlElement("excludes", "NAnt.Core.FileSet+ExcludesElement", AllowMultiple = true)]
        [XmlElement("do", Container = XmlElementAttribute.ContainerType.ConditionalContainer, AllowMultiple = true)]
        [XmlElement("group", Container = XmlElementAttribute.ContainerType.ConditionalContainer, AllowMultiple = true)]
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
            string _failonmissingVal = null;
            try
            {
                foreach (XmlAttribute attr in elementNode.Attributes)
                {
                    switch (attr.Name)
                    {
                        case "name": // REq
                            _name = attr.Value;
                            break;
                        case "basedir":
                            BaseDirectory = attr.Value;
                            break;
                        case "fromfileset":
                            FromFileSetName = attr.Value;
                            break;
                        case "append":
                            _appendVal = attr.Value;
                            break;
                        case "failonmissing":
                            _failonmissingVal = attr.Value;
                            break;
                        case "sort":
                            _fileset.Sort = ElementInitHelper.InitBoolAttribute(this, attr.Name, attr.Value);
                            break;
                        case "defaultexcludes":
                            _fileset.DefaultExcludes = ElementInitHelper.InitBoolAttribute(this, attr.Name, attr.Value);
                            break;
                        case "depends":
                            Log.Warning.WriteLine("{0}invalid <fileset> attribute:  '{1}'.", LogPrefix, attr.Name);
                            break;
                        case "asis":
                        case "force":
                            //These elements are initialized in the GroupElement (default group)
                            break;
                        default:
                            if (!OptimizedTaskElementInit(attr.Name, attr.Value))
                            {
                                string msg = String.Format("Unknown attribute '{0}'='{1}' in fileset task", attr.Name, attr.Value);
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
                    _name = Project.ExpandProperties(_name.Trim());

                    if (BaseDirectory != null) BaseDirectory = Project.ExpandProperties(BaseDirectory);
                    if (FromFileSetName != null) FromFileSetName = Project.ExpandProperties(FromFileSetName);
                    if (_appendVal != null) _append = ElementInitHelper.InitBoolAttribute(this, "append", _appendVal);
                    if (_failonmissingVal != null) _fileset.FailOnMissingFile = ElementInitHelper.InitBoolAttribute(this, "failonmissing", _failonmissingVal);

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
            _fileset.Project = Project;
            _fileset.LogPrefix = LogPrefix;
            _fileset.IfDefined = IfDefined;
            _fileset.UnlessDefined = UnlessDefined;
            _fileset.OptimizedInitializeFileSetElement(elementNode);
        }

        protected override void ExecuteTask()
        {
            Project.NamedFileSets.AddOrUpdate(FileSetName, _fileset, (name, oldfileset) =>
                {
                    if (Append)
                    {
                        oldfileset.Append(_fileset);
                        return oldfileset;
                    }
                    return _fileset;
                });
        }
    }
}
