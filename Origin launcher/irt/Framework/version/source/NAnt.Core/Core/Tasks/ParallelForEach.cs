// Originally based on NAnt - A .NET build tool
// Copyright (C) 2003-2015 Electronic Arts Inc.
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
// Gerry Shaw (gerry_shaw@yahoo.com)
// Ian MacLean (ian_maclean@another.com)
// Electronic Arts (Frostbite.Team.CM@ea.com)

using System;
using System.Collections.Generic;
using System.IO;
using System.Xml;
using System.Text.RegularExpressions;
using System.Threading.Tasks;

using NAnt.Core.Attributes;
using NAnt.Core.Util;
using NAnt.Core.Threading;

namespace NAnt.Core.Tasks
{

	/// <summary>Iterates over a set of items in parallel. Similar to &lt;foreach&gt; task.</summary>
	/// 
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
	///   String - return each splitted string from a long string with user specified delimiter in delim.<br/>
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
	/// </remarks>
	/// <include file='Examples/ForEach/LoopOverFiles.example' path='example'/>
	/// <include file='Examples/ForEach/LoopOverFilePattern.example' path='example'/>
	/// <include file='Examples/ForEach/LoopOverDirectories.example' path='example'/>
	/// <include file='Examples/ForEach/LoopOverDirectoryPattern.example' path='example'/>
	/// <include file='Examples/ForEach/LoopOverList.example' path='example'/>
	/// <include file='Examples/ForEach/LoopOverFileSet.example' path='example'/>
	/// <include file='Examples/ForEach/LoopOverOptionSet.example' path='example'/>
	[TaskName("parallel.foreach")]
	public class ParallelForEachTask : TaskContainer
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
		public ItemTypes ItemType { get; set; } = ItemTypes.None;

		/// <summary>
		/// The source of the iteration.
		/// </summary>
		[TaskAttribute("in", Required = true)]
		public string Source { get; set; } = null;

		/// <summary>
		/// The delimiter string array. Default is whitespace.  
		/// Multiple characters are allowed.
		/// </summary>
		[TaskAttribute("delim")]
		public string Delimiter { get; set; } = new String(_defaultDelimArray);

		private object m_scriptInitLock = new object(); // create a single script init lock for all tasks - we don't want parallel tasks to include multiple scripts
														// at once, but calling context may already have current project's script init lock held so we can't use that

		protected override void ExecuteTask()
		{
			string oldPropValue = Properties[_propertyName];
			try
			{
				if (oldPropValue != null) Properties.Remove(_propertyName);

				switch (ItemType)
				{
					case ItemTypes.None:
						throw new BuildException("Invalid item type.", Location);

					case ItemTypes.File:
						{

							string baseDirectory;
							string searchPattern;

							// check for wild cards
							if (Pattern.IsImplicitPattern(Source))
							{
								baseDirectory = Path.GetDirectoryName(Source);
								searchPattern = Path.GetFileName(Source);
							}
							else
							{
								baseDirectory = Source;
								searchPattern = "*.*";
							}

							if (!Directory.Exists(Project.GetFullPath(baseDirectory)))
							{
								string msg = String.Format("Directory '{0}' does not exist.", baseDirectory);
								throw new BuildException(msg, Location);
							}
							DirectoryInfo dirInfo = new DirectoryInfo(Project.GetFullPath(baseDirectory));
							FileInfo[] files = dirInfo.GetFiles(searchPattern);

							try
							{
#if NANT_CONCURRENT
								if (Project.NoParallelNant)
								{
									foreach (var file in files) DoWork(file.FullName);
								}
								else
								{
									Parallel.ForEach(files, file => DoWork(file.FullName));
								}
#else
								foreach (var file in files) DoWork(file.FullName);
#endif
							}
							catch (Exception ex)
							{
								ThreadUtil.RethrowThreadingException(String.Empty, Location, ex);
							}

							break;
						}

					case ItemTypes.Directory:
					case ItemTypes.Folder:
						{

							string baseDirectory;
							string searchPattern;

							// check for wild cards
							if (Pattern.IsImplicitPattern(Source))
							{
								baseDirectory = Path.GetDirectoryName(Source);
								searchPattern = Path.GetFileName(Source);
							}
							else
							{
								baseDirectory = Source;
								searchPattern = "*.*";
							}

							if (!Directory.Exists(Project.GetFullPath(baseDirectory)))
							{
								string msg = String.Format("Directory '{0}' does not exist.", baseDirectory);
								throw new BuildException(msg, Location);
							}
							DirectoryInfo dirInfo = new DirectoryInfo(Project.GetFullPath(baseDirectory));
							DirectoryInfo[] dirs = dirInfo.GetDirectories(searchPattern);

							try
							{
#if NANT_CONCURRENT
								if (Project.NoParallelNant)
								{
									foreach (var dir in dirs) DoWork(dir.FullName);
								}
								else
								{
									Parallel.ForEach(dirs, dir => DoWork(dir.FullName));
								}
#else
								foreach (var dir in dirs) DoWork(dir.FullName);
#endif

							}
							catch (Exception ex)
							{
								//ThreadUtil.RethrowThreadingException(String.Format("Error in task '{0}'", this.Name), Location, ex);
								ThreadUtil.RethrowThreadingException(String.Empty, Location, ex);
							}

							break;
						}

					case ItemTypes.Line:
						{
							if (!File.Exists(Project.GetFullPath(Source)))
							{
								string msg = String.Format("File '{0}' does not exist.", Source);
								throw new BuildException(msg, Location);
							}
							StreamReader sr = File.OpenText(Project.GetFullPath(Source));
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
								string msg = String.Format("Invalid delimiter '{0}'.", Delimiter);
								throw new BuildException(msg, Location);
							}

							string delimString;
							try
							{
								delimString = Regex.Unescape(Delimiter);
							}
							catch (System.Exception e)
							{
								string msg = String.Format("Cannot unescape delimiter '{0}'.", Delimiter);
								throw new BuildException(msg, e);
							}

							string[] items = Source.Split(delimString.ToCharArray());

							try
							{
								if (Project.NoParallelNant)
								{
									foreach (var str in items)
									{
										string trimmedStr = str.Trim();
										if (trimmedStr.Length > 0)
										{
											DoWork(trimmedStr);
										}
									}
								}
								else
								{
									NAntUtil.ParallelForEach(items, str =>
									{
										string trimmedStr = str.Trim();
										if (trimmedStr.Length > 0)
										{
											DoWork(trimmedStr);
										}
									});
								}
							}
							catch (Exception ex)
							{
								//ThreadUtil.RethrowThreadingException(String.Format("Error in task '{0}'", this.Name), Location, ex);
								ThreadUtil.RethrowThreadingException(String.Empty, Location, ex);
							}
							break;
						}

					case ItemTypes.FileSet:
						{
							if (!Project.NamedFileSets.TryGetValue(Source, out FileSet fs))
							{
								string msg = String.Format("Unknown file set '{0}'.", Source);
								throw new BuildException(msg, Location);
							}

							try
							{
#if NANT_CONCURRENT
								if (Project.NoParallelNant)
								{
									foreach (var fileItem in fs.FileItems) DoWork(fileItem.FileName, null, fileItem.BaseDirectory??fs.BaseDirectory);
								}
								else
								{
									Parallel.ForEach(fs.FileItems, fileItem => DoWork(fileItem.FileName, null, fileItem.BaseDirectory??fs.BaseDirectory));
								}
#else
								foreach(var fileItem in fs.FileItems) DoWork(fileItem.FileName, null, fileItem.BaseDirectory ?? fs.BaseDirectory);
#endif
							}
							catch (Exception ex)
							{
								//ThreadUtil.RethrowThreadingException(String.Format("Error in task '{0}'", this.Name), Location, ex);
								ThreadUtil.RethrowThreadingException(String.Empty, Location, ex);
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

							foreach (KeyValuePair<string, string> entry in optionSet.Options)
							{
								DoWork(entry.Value, entry);
							}

#if NANT_CONCURRENT
							if (Project.NoParallelNant)
							{
								foreach (var option in optionSet.Options) DoWork(option.Value, option);
							}
							else
							{
								Parallel.ForEach(optionSet.Options, option => DoWork(option.Value, option));
							}
#else
							foreach(var option in optionSet.Options) DoWork(option.Value, option);
#endif
							break;
						}
				}
			}
			finally
			{
				if (oldPropValue != null)
					Properties[_propertyName] = oldPropValue;
			}
		}

		protected virtual void DoWork(string propertyValue, KeyValuePair<string, string>? option=null, string basedir = null)
		{
            Project taskContextProject = new Project(Project, m_scriptInitLock);
			{
				taskContextProject.Properties.Add(_propertyName, propertyValue, local: true);

				if (option != null)
				{
					taskContextProject.Properties.UpdateReadOnly(_propertyName + ".name", option.Value.Key, local: true, inheritable: true);
					taskContextProject.Properties.UpdateReadOnly(_propertyName + ".value", option.Value.Value, local: true, inheritable: true);
				}
				else if (basedir != null)
				{
					taskContextProject.Properties.Add(_propertyName + ".basedir", basedir, local: true, inheritable: true);
				}

				foreach (XmlNode childNode in _xmlNode)
				{
					if (childNode.NodeType == XmlNodeType.Element && childNode.NamespaceURI.Equals(_xmlNode.NamespaceURI))
					{
						if (!IsPrivateXMLElement(childNode))
						{
							Task task = taskContextProject.CreateTask(childNode, this);
							task.Execute();
						}
					}
				}
			}
		}

	}
}
