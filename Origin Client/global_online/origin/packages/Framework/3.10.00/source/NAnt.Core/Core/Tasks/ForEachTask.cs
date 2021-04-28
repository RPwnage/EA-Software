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
// Module Maintainers:
// Scott Hernandez (ScottHernandez@hotmail.com)
// Gerry Shaw (gerry_shaw@yahoo.com)

using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Text.RegularExpressions;

using NAnt.Core.Attributes;
using NAnt.Core.Util;

namespace NAnt.Core.Tasks
{

    /// <summary>Iterates over a set of items.</summary>
    /// <remarks>
    ///   <para>Can iterate over files in directory, lines in a file, etc.</para>
    ///   <para>
    ///   The <c>property</c> value is set for each item in turn as the <c>foreach</c> task 
    ///   iterates over the given set of items. Any previously existing value in <c>property</c> 
    ///   is stored before the block of tasks specified in the <c>foreach</c> task are invoked, 
    ///   and restored when the block of tasks completes. 
    ///   </para>
    ///   <para>This storage ensures that the <c>property</c> will have the same value it had 
    ///   before the <c>foreach</c> task was invoked once the <c>foreach</c> task completes.
    ///   </para>
    ///   <para>
    ///   Valid foreach items are "File", "Folder", "Directory", "String", "Line", "FileSet",
    ///   and "OptionSet".
    ///   </para>
    ///   <para>
    ///   File - return each file name in an iterated directory<br/>
    ///   Folder - return each folder in an iterated directory<br/>
    ///   Directory - return each directory in an iterated directory<br/>
    ///   String - return each splitted string from a long string with user speicified delimiter in delim.<br/>
    ///   Line - return each line in a text file.<br/>
    ///   FileSet - return each file name in a FileSet.<br/>
    ///   OptionSet - return each option in an OptionSet.<br/>
    ///   </para>
    ///   <para>
    ///   NOTE: When iterating over strings and lines extra leading and trailing whitespace
    ///   characters will be trimmed and blank lines will be ignored.
    ///   </para>
    ///   <para>
    ///   NOTE: When iterating over option sets the property name specified is used for a
    ///   property prefix to the actual option name and values.  The name of option is available
    ///   in the <c>&lt;property&gt;.name</c> property, the value in the <c>&lt;property&gt;.value</c>
    ///   property.
    ///   </para>
    ///   <para>
    ///   NOTE: When iterating over FileSet the The BaseDirectory associated with the FileItem (see 'fileset' <c>&lt;include&gt;.name</c> help) is available
    ///   in the <c>&lt;property&gt;.basedir</c> property.</c>
    ///   property.
    ///   </para>
    /// </remarks>
    /// <include file='Examples/ForEach/LoopOverFiles.example' path='example'/>
    /// <include file='Examples/ForEach/LoopOverFilePattern.example' path='example'/>
    /// <include file='Examples/ForEach/LoopOverDirectories.example' path='example'/>
    /// <include file='Examples/ForEach/LoopOverDirectoryPattern.example' path='example'/>
    /// <include file='Examples/ForEach/LoopOverList.example' path='example'/>
    /// <include file='Examples/ForEach/LoopOverFileSet.example' path='example'/>
    /// <include file='Examples/ForEach/LoopOverOptionSet.example' path='example'/>
    [TaskName("foreach")]
    public class ForEachTask : TaskContainer
    {
        public enum ItemTypes
        {
            None,
            File,
            Folder,     // deprecated
            Directory,
            String,
            Line,
            FileSet,
            OptionSet
        }

        static char[] _defaultDelimArray = { 
            '\x0009', // tab
            '\x000a', // newline
            '\x000d', // carriage return
            '\x0020'  // space
        };

        string _propertyName = null;
        ItemTypes _itemType = ItemTypes.None;
        string _source = null;
        string _delimiter = new String(_defaultDelimArray);
        bool _local = false;

        /// <summary>The property name that holds the current iterated item value.</summary>
        [TaskAttribute("property", Required = true)]
        public string Property
        {
            get { return _propertyName; }
            set
            {
                _propertyName = value;
                if (Properties.IsPropertyReadOnly(_propertyName))
                {
                    string msg = String.Format("Property '{0}' is readonly.", _propertyName);
                    throw new BuildException(msg, Location);
                }
            }
        }

        /// <summary>
        /// The type of iteration that should be done. Valid values are: 
        /// File, Directory, String, Line, FileSet, and OptionSet.
        /// When choosing the Line option, you specify a file and the task
        /// iterates over all the lines in the text file.
        /// </summary>
        [TaskAttribute("item", Required = true)]
        public ItemTypes ItemType
        {
            get { return _itemType; }
            set { _itemType = value; }
        }

        /// <summary>
        /// The source of the iteration.
        /// </summary>
        [TaskAttribute("in", Required = true)]
        public string Source
        {
            get { return _source; }
            set { _source = value; }
        }

        /// <summary>
        /// The deliminator string array. Default is whitespace.  
        /// Multiple characters are allowed.
        /// </summary>
        [TaskAttribute("delim")]
        public string Delimiter
        {
            get { return _delimiter; }
            set { _delimiter = value; }
        }

        /// <summary>
        /// Indicates if the property that holds the iterated value is going to be defined in a local context and thus, it will be restricted to a local scope . 
        /// Default is false.
        /// </summary>
        [TaskAttribute("local")]
        public bool Local
        {
            get { return _local; }
            set { _local = value; }
        }

        protected override void ExecuteTask()
        {
            string oldPropValue = Properties[_propertyName];
            if (Properties.IsPropertyLocal(_propertyName))
                throw new BuildException(LogPrefix + " Iterator property is already defined as local.");

            if (Local)
                Properties.Add(_propertyName, "", false, false, true);

            try
            {
                switch (ItemType)
                {
                    case ItemTypes.None:
                        throw new BuildException("Invalid itemtype.", Location);

                    case ItemTypes.File:
                        {

                            string baseDirectory;
                            string searchPattern;

                            // check for wildcards
                            if (Pattern.IsImplicitPattern(_source))
                            {
                                baseDirectory = Path.GetDirectoryName(_source);
                                searchPattern = Path.GetFileName(_source);
                            }
                            else
                            {
                                baseDirectory = _source;
                                searchPattern = "*.*";
                            }

                            if (!Directory.Exists(Project.GetFullPath(baseDirectory)))
                            {
                                string msg = String.Format("Directory '{0}' does not exist.", baseDirectory);
                                throw new BuildException(msg, Location);
                            }
                            DirectoryInfo dirInfo = new DirectoryInfo(Project.GetFullPath(baseDirectory));
                            FileInfo[] files = dirInfo.GetFiles(searchPattern);
                            foreach (FileInfo file in files)
                            {
                                DoWork(file.FullName);
                            }
                            break;
                        }

                    case ItemTypes.Directory:
                    case ItemTypes.Folder:
                        {

                            string baseDirectory;
                            string searchPattern;

                            // check for wildcards
                            if (Pattern.IsImplicitPattern(_source))
                            {
                                baseDirectory = Path.GetDirectoryName(_source);
                                searchPattern = Path.GetFileName(_source);
                            }
                            else
                            {
                                baseDirectory = _source;
                                searchPattern = "*.*";
                            }

                            if (!Directory.Exists(Project.GetFullPath(baseDirectory)))
                            {
                                string msg = String.Format("Directory '{0}' does not exist.", baseDirectory);
                                throw new BuildException(msg, Location);
                            }
                            DirectoryInfo dirInfo = new DirectoryInfo(Project.GetFullPath(baseDirectory));
                            DirectoryInfo[] dirs = dirInfo.GetDirectories(searchPattern);
                            foreach (DirectoryInfo dir in dirs)
                            {
                                DoWork(dir.FullName);
                            }
                            break;
                        }

                    case ItemTypes.Line:
                        {
                            if (!File.Exists(Project.GetFullPath(_source)))
                            {
                                string msg = String.Format("File '{0}' does not exist.", _source);
                                throw new BuildException(msg, Location);
                            }
                            StreamReader sr = File.OpenText(Project.GetFullPath(_source));
                            while (true)
                            {
                                string line = sr.ReadLine();
                                if (line == null)
                                {
                                    break;
                                }
                                line = line.Trim();
                                if (line.Length > 0)
                                {
                                    DoWork(line);
                                }
                            }
                            sr.Close();
                            break;
                        }

                    case ItemTypes.String:
                        {
                            if (Delimiter == null || Delimiter.Length == 0)
                            {
                                string msg = String.Format("Invalid delimiter '{0}'.", _delimiter);
                                throw new BuildException(msg, Location);
                            }

                            string delimString;
                            try
                            {
                                delimString = Regex.Unescape(Delimiter);
                            }
                            catch (System.Exception e)
                            {
                                string msg = String.Format("Cannot unescape delimeter '{0}'.", Delimiter);
                                throw new BuildException(msg, e);
                            }

                            string[] items = _source.Split(delimString.ToCharArray());
                            foreach (string str in items)
                            {
                                string trimmedStr = str.Trim();
                                if (trimmedStr.Length > 0)
                                {
                                    DoWork(trimmedStr);
                                }
                            }
                            break;
                        }

                    case ItemTypes.FileSet:
                        {
                            FileSet fs;
                            if (!Project.NamedFileSets.TryGetValue(Source, out fs))
                            {
                                string msg = String.Format("Unknown file set '{0}'.", Source);
                                throw new BuildException(msg, Location);
                            }

                            if (Local)
                            {
                                Properties.Add(_propertyName + ".basedir", "", false, false, true);
                            }

                            foreach (FileItem fileItem in fs.FileItems)
                            {
                                Properties[_propertyName + ".basedir"] = fileItem.BaseDirectory ?? fs.BaseDirectory;
                                DoWork(fileItem.FileName);
                            }
                            break;
                        }

                    case ItemTypes.OptionSet:
                        {
                            OptionSet optionSet;
                            ;
                            if (!Project.NamedOptionSets.TryGetValue(Source, out optionSet))
                            {
                                string msg = String.Format("Unknown option set '{0}'.", Source);
                                throw new BuildException(msg, Location);
                            }
                            if (Local)
                            {
                                Properties.Add(_propertyName + ".name", "", false, false, true);
                                Properties.Add(_propertyName + ".value", "", false, false, true);
                            }
                            foreach (KeyValuePair<string, string> entry in optionSet.Options)
                            {
                                Properties[_propertyName + ".name"] = entry.Key;
                                Properties[_propertyName + ".value"] = entry.Value;
                                DoWork(entry.Value);
                            }
                            if (Local)
                            {
                                Properties.Remove(_propertyName + ".name");
                                Properties.Remove(_propertyName + ".value");
                            }
                            break;
                        }
                }
            }
            finally
            {
                if (Local)
                    Properties.Remove(_propertyName);
                if (oldPropValue != null)
                    Properties[_propertyName] = oldPropValue;
            }
        }

        protected virtual void DoWork(string propertyValue)
        {
            Properties[_propertyName] = propertyValue;
            base.ExecuteTask();
        }
    }
}
