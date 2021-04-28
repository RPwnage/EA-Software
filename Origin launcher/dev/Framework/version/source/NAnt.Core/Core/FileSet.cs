// NAnt - A .NET build tool
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
// File Maintainers:
// Electronic Arts (Frostbite.Team.CM@ea.com)
// Gerry Shaw (gerry_shaw@yahoo.com)

using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Collections.Specialized;
using System.IO;
using System.Linq;
using System.Runtime.ExceptionServices;
using System.Threading;
using System.Threading.Tasks;
using System.Xml;

using NAnt.Core.Attributes;
using NAnt.Core.Logging;
using NAnt.Core.Reflection;
using NAnt.Core.Threading;
using NAnt.Core.Util;

namespace NAnt.Core
{

	public class FileSetDictionary : ConcurrentDictionary<string, FileSet>
	{
		public bool Contains(string name)
		{
			return ContainsKey(name);
		}

		public bool Add(string name, FileSet set)
		{
			return TryAdd(name, set);
		}

		public bool Remove(string name)
		{
			FileSet set;
			return TryRemove(name, out set);
		}

		public new FileSet this[string name]
		{
			get
			{
				FileSet fs;
				if (!TryGetValue(name, out fs))
				{
					fs = null;
				}
				return fs;
			}
			set
			{
				base[name] = value;
			}
		}

		internal FileSetDictionary() : base()
		{
		}
		internal FileSetDictionary(ConcurrentDictionary<string, FileSet> collection)
			: base(collection)
		{
		}

	}

	/// <summary>
	/// A FileItem is a fileset item after its pattern has been expanded
	/// </summary>
	public class FileItem : IComparable<FileItem>
	{
		public string OptionSetName;
		public readonly bool AsIs;
		public readonly bool Force;
		public readonly string BaseDirectory;

		public static readonly FileItemPathEqualityComparer PathEqualityComparer = new FileItemPathEqualityComparer();

		public FileItem(PathString fileName, string optionSetName = null, bool asIs = false, bool force = false, object data = null, string basedir=null)
		{
			//IMTODO: use this
			//FileName = asIs ? fileName : PathString.MakeNormalized(fileName);
			Path = asIs ? fileName : PathString.MakeNormalized(fileName);
			OptionSetName = optionSetName;
			AsIs = asIs;
			Force = force;
			Data = data;
			BaseDirectory = basedir;
		}

		/// <summary>Internal use only for storage!</summary>
		public int Index { get; set; } = 0;

		public string FullPath
		{
			get
			{
				if (AsIs)
				{
					return Path.Path;
				}
				return Path.NormalizedPath;
			}
		}
		//IMTODO - remove this
		public string FileName
		{
			get
			{
				return Path.Path;
			}
		}

		public PathString Path { get; }

		// an object to store internal data
		public object Data { get; set; }

		public int CompareTo(FileItem a)
		{
			return String.Compare(FileName, a.FileName);
		}

		public override string ToString()
		{
			return FullPath;
		}
	}

	public class FileItemPathEqualityComparer : IEqualityComparer<FileItem>
	{
		public bool Equals(FileItem x, FileItem y)
		{
			return 0 == String.Compare(x.Path.Path, y.Path.Path, PathUtil.IsCaseSensitive ? StringComparison.Ordinal : StringComparison.OrdinalIgnoreCase);
		}

		public int GetHashCode(FileItem obj)
		{
			return PathUtil.IsCaseSensitive ? obj.Path.Path.GetHashCode() : obj.Path.Path.ToLowerInvariant().GetHashCode();
		}
	}


	/// <summary>
	/// An array list of file items
	/// </summary>
	public class FileItemList : List<FileItem>
	{
		public FileItemList()
			: base()
		{
		}

		public FileItemList(int capacity)
			: base(capacity)
		{
		}

		/// <summary>Converts this array of FileItems to a StringCollection of filenames.</summary>
		/// <remarks>Provided as a convenience for backwards compatibility and external libraries.</remarks>
		public virtual StringCollection ToStringCollection()
		{
			StringCollection files = new StringCollection();
			foreach (FileItem fileItem in this)
			{
				files.Add(fileItem.FileName);
			}
			return files;
		}
	}

	/// <summary>
	/// A FileSetItem is a fileset item before its pattern has been expanded
	/// </summary>
	public class FileSetItem
	{
		public static FileSetItemEqualityComparer EqualityComparer = new FileSetItemEqualityComparer();

		public readonly Pattern Pattern;
		public string OptionSet;
		public readonly bool AsIs;
		public readonly bool Force;
		public object Data;

		public FileSetItem(Pattern pattern, string optionSet = null, bool asIs = false, bool force = false, object data = null)
		{
			Pattern = pattern;
			OptionSet = optionSet;
			AsIs = asIs;
			Force = force;
			Data = data;
		}

		public override string ToString()
		{
			return Pattern.ToString();
		}

	}

	public class FileSetItemEqualityComparer : IEqualityComparer<FileSetItem>
	{
		public bool Equals(FileSetItem a, FileSetItem b)
		{
			if (a.Pattern.Value == b.Pattern.Value)
				return true;

			if (a.Pattern.Value == null && b.Pattern.Value != null)
				return false;

			if (a.Pattern.Value != null && b.Pattern.Value == null)
				return false;

			return a.Pattern.Value.Equals(b.Pattern.Value);
		}

		public int GetHashCode(FileSetItem obj)
		{
			return obj.Pattern.Value.GetHashCode();
		}
	}

	/// <summary>
	/// A collection of FileSetItems
	/// </summary>
	public class FileSetItemCollection : IList<FileSetItem>, IEnumerable<FileSetItem>
	{
		/// <summary>
		/// A specialized container class for optimizing the storage of files.
		/// A hashtable of linked lists where the key is the base directory of the file.
		/// </summary>
		class FileItemContainer
		{
			Dictionary<string, FileItemList> _hashTable = new Dictionary<string, FileItemList>();
			int _curIndex = 0;

			public void Add(FileItem fileItem)
			{
				string key = Path.GetDirectoryName(fileItem.FileName);

				FileItemList itemList = null;
				if (!_hashTable.TryGetValue(key, out itemList))
				{
					itemList = new FileItemList();
					_hashTable.Add(key, itemList);
				}
				fileItem.Index = _curIndex++;

				itemList.Add(fileItem);
			}

			public bool Contains(string fileName)
			{
				// table is very often empty, so therefore it is faster than calling the very expensive Path.GetDirectoryName
				if (_hashTable.Count == 0)
					return false;
				string key = Path.GetDirectoryName(fileName);
				FileItemList fileItemList;
				if (_hashTable.TryGetValue(key, out  fileItemList))
				{
					foreach (FileItem fileItem in fileItemList)
					{
						if (String.CompareOrdinal(fileItem.FileName, fileName) == 0)
							return true;
					}
				}
				return false;
			}

			[Obsolete("Use FileItems version instead as it is better optimized.")]
			public StringCollection FileNames
			{
				get
				{
					StringCollection files = new StringCollection();
					foreach (FileItem fi in FileItems)
					{
						files.Add(fi.FileName);
					}
					return files;
				}
			}

			public FileItemList FileItems
			{
				get
				{
					FileItem[] fileItemArray = new FileItem[_curIndex];
					Dictionary<string, FileItemList>.Enumerator enumerator = _hashTable.GetEnumerator();
					while (enumerator.MoveNext())
					{
						foreach (FileItem fi in enumerator.Current.Value)
						{
							fileItemArray[fi.Index] = fi;
						}
					}

					FileItemList fileItemList = new FileItemList(_curIndex);
					fileItemList.AddRange(fileItemArray);

					// TODO: sort the items when adding them to the array
					if (Sort)
						fileItemList.Sort();

					return fileItemList;
				}
			}

			public void Clear()
			{
				_hashTable.Clear();
				_curIndex = 0;
			}

			public bool Sort { get; set; } = false;
		}

		FileItemContainer _fileItemContainer = null;
		List<FileSetItem> _fileSetItems = new List<FileSetItem>();
		bool _initialized = false;

		public void Add(FileSetItem item)
		{
			_initialized = false;
			_fileSetItems.Add(item);
		}

		public void Add(Pattern pattern)
		{
			_initialized = false;
			FileSetItem item = new FileSetItem(pattern);
			_fileSetItems.Add(item);
		}

		public void Prepend(FileSetItem item)
		{
			_initialized = false;
			_fileSetItems.Insert(0, item);
		}

		public void AddRange(IEnumerable<FileSetItem> c)
		{
			_initialized = false;
			_fileSetItems.AddRange(c);
		}

		public void PrependRange(IEnumerable<FileSetItem> c)
		{
			_initialized = false;
			_fileSetItems.InsertRange(0, c);
		}

		public void Clear()
		{
			_initialized = false;
			if (_fileItemContainer != null)
				_fileItemContainer.Clear();

			_fileSetItems.Clear();
		}

		// Environment.CurrentDirectory has quite a lot of overhead to call so let's cache it here (we don't expect it to change for the life time of the process
		static string DefaultBaseDirectory = Environment.CurrentDirectory;

		private void InitializeProc(FileSetItem item, List<FileItem> fileItems, FileSetItemCollection excludePatterns, bool failOnMissingFile, List<Pattern.RegexPattern> excludeRegexPatterns, StringCollection excludeFileNames, StringCollection asIsExcludeFileNames)
		{
			if (item.Pattern.BaseDirectory == null)
			{
				item.Pattern.BaseDirectory = DefaultBaseDirectory;
			}

			if (!item.Force)
			{
				if (item.AsIs)
				{
					excludeRegexPatterns = new List<Pattern.RegexPattern>();
					excludeFileNames = asIsExcludeFileNames;
				}
			}
			else
			{
				excludeRegexPatterns = new List<Pattern.RegexPattern>();
				excludeFileNames = new StringCollection();
			}

			List<string> includedFiles = item.Pattern.GetMatchingFiles(failOnMissingFile, excludeRegexPatterns, excludeFileNames);

			FileItem[] items = new FileItem[includedFiles.Count];
			Parallel.For(0, includedFiles.Count, (index) =>
			{
				PathString fileStr = new PathString(includedFiles[index], PathString.PathState.File);
				items[index] = new FileItem(fileStr, item.OptionSet, item.AsIs, item.Force, item.Data, item.Pattern.PreserveBaseDirValue ? item.Pattern.BaseDirectory : null);
			});
			fileItems.AddRange(items);
		}

		class ProcessCollectionInfo
		{
			public int Count;
			public long PatternIndex = 0;
			public long PatternProcessed = 0;
			public Exception Exception;
			public FileSetItem[] Items;

			public ProcessCollectionInfo(FileSetItemCollection collection)
			{ 
				Items = collection._fileSetItems.ToArray();
				Count = Items.Length;
			}
		}

		private FileItemContainer Initialize(FileSetItemCollection excludePatterns, bool failOnMissingFile, bool sort)
		{
			// Use local variable for thread safety on expansion;
			var fileItemContainer = new FileItemContainer();
			fileItemContainer.Sort = sort;

			if (Count == 0)
				return fileItemContainer;
		
			List<Pattern.RegexPattern> excludeRegexPatterns;
			StringCollection excludeFileNames;
			StringCollection asIsExcludeFileNames;

			ComputeExcludedRegexPatters(excludePatterns, out excludeRegexPatterns, out excludeFileNames, out asIsExcludeFileNames);

			var processInfo = new ProcessCollectionInfo(this);
			var fileitems = new List<FileItem>[processInfo.Count];

			try
			{
				Action<ProcessCollectionInfo> processInterval = (info) =>
				{
					while (true)
					{
						long index = Interlocked.Increment(ref info.PatternIndex) - 1;

						if (index >= info.Count || index < 0) // paranoia - older version of this code used to enter this function while spinning incrementing pattern index until it overflowed
							break;

						List<FileItem> itemList = fileitems[index] = new List<FileItem>();

						try
						{
							InitializeProc(processInfo.Items[index], itemList, excludePatterns, failOnMissingFile, excludeRegexPatterns, excludeFileNames, asIsExcludeFileNames);
						}
						catch (Exception e)
						{
							info.Exception = e;
						}
						Interlocked.Increment(ref info.PatternProcessed);
					}
				};

				// If more than one pattern run multiple threads.
				// Only start jobs if we have more than one Count.
				// Note that we don't need to wait for the tasks to start or finish since we process the intervals ourselves as well further down
				if (Count > 1)
				{
#if NANT_CONCURRENT
					// Under mono most parallel execution is slower than consecutive. Until this is resolved use consecutive execution 
					bool threading = (PlatformUtil.IsMonoRuntime == false);

					//If the file set contains only explicit and absolute patterns and there are no exclusion patterns, then we do not need to parallelize.
					if (!_fileSetItems.Any(item => item.Pattern is ImplicitPattern) && !excludePatterns.Any())
					{
						threading = false;
					}
#else
					bool threading = false;
#endif
					if (threading)
						for (int i = 0, e = Math.Min(Environment.ProcessorCount - 1, processInfo.Count - 1); i != e; ++i)
							System.Threading.Tasks.Task.Factory.StartNew(() => processInterval(processInfo));
				}

				while (Interlocked.Read(ref processInfo.PatternProcessed) < processInfo.Count)
					if (Interlocked.Read(ref processInfo.PatternIndex) < processInfo.Count) // if this condition isn't true then this thread just spins, could probably refactor to wait on tasks started above
						processInterval(processInfo);

				if (processInfo.Exception != null)
					ExceptionDispatchInfo.Capture(processInfo.Exception).Throw();
			}
			catch (Exception e)
			{
				ThreadUtil.RethrowThreadingException("An error occurred expanding a fileset.", e);
			}

			foreach (List<FileItem> itemList in fileitems)
			{
				if (itemList == null) break;

				foreach (FileItem item in itemList)
				{
					try
					{
						if (!item.AsIs && !fileItemContainer.Contains(item.FileName))
						{
							fileItemContainer.Add(item);
						}
						else if (item.AsIs)
						{
							fileItemContainer.Add(item);
						}
					}
					catch (PathTooLongException e)
					{
						throw new PathTooLongException(string.Format("The following path is longer than the 260 character file path limit: '{0}'", item.FileName), e);
					}
				}
			}

			return fileItemContainer;

		}

		/// <summary>Returns a collection of file items which match the set of patterns</summary>
		public FileItemList GetMatchingItems(FileSetItemCollection excludePatterns, bool failOnMissingFile, bool sort)
		{
			
			if (_initialized == false)
			{
				_fileItemContainer = Initialize(excludePatterns, failOnMissingFile, sort);

				Thread.MemoryBarrier();

				_initialized = true;
			}

			return _fileItemContainer.FileItems;
		}

		private void ComputeExcludedRegexPatters(FileSetItemCollection excludePatterns, out List<Pattern.RegexPattern> excludeRegexPatterns, out StringCollection excludeFileNames, out StringCollection asIsExcludeFileNames)
		{
			bool hasNonAsIs = false;
			bool hasAsIs = false;
			excludeRegexPatterns = null;
			excludeFileNames = null;
			asIsExcludeFileNames = null;

			foreach (FileSetItem item in this)
			{
				if (item.AsIs)
				{
					hasAsIs = true;
					if (hasNonAsIs)
						break;
				}
				else
				{
					hasNonAsIs = true;
					if (hasAsIs)
						break;
				}
			}

			if (hasNonAsIs)
			{
				excludeRegexPatterns = new List<Pattern.RegexPattern>();
				excludeFileNames = new StringCollection();

				string excludedFileName = null;

				foreach (FileSetItem excludePattern in excludePatterns)
				{
					if (!excludePattern.AsIs)
					{
						excludedFileName = null;
						Pattern.RegexPattern p = excludePattern.Pattern.ConvertPattern(excludePattern.Pattern.Value, out excludedFileName);
						if (p != null)
							excludeRegexPatterns.Add(p);
						else if (excludedFileName != null)
							excludeFileNames.Add(excludedFileName);
					}
				}
			}

			if (hasAsIs)
			{
				asIsExcludeFileNames = new StringCollection();
				foreach (FileSetItem excludePattern in excludePatterns)
				{
					if (excludePattern.AsIs)
					{
						asIsExcludeFileNames.Add(excludePattern.Pattern.Value);
					}
				}
			}
		}

		public int IndexOf(FileSetItem item)
		{
			return _fileSetItems.IndexOf(item);
		}

		public void Insert(int index, FileSetItem item)
		{
			_initialized = false;
			_fileSetItems.Insert(index, item);
		}

		public void RemoveAt(int index)
		{
			_initialized = false;
			_fileSetItems.RemoveAt(index);
		}

		public FileSetItem this[int index]
		{
			get
			{
				return _fileSetItems[index];
			}
			set
			{
				_initialized = false;
				_fileSetItems[index] = value;
			}
		}

		public bool Contains(FileSetItem item)
		{
			return _fileSetItems.Contains(item);
		}

		public void CopyTo(FileSetItem[] array, int arrayIndex)
		{
			_fileSetItems.CopyTo(array, arrayIndex);
		}

		public int Count { get { return _fileSetItems.Count; } }

		public bool IsReadOnly { get { return false; } }

		public bool Remove(FileSetItem item)
		{
			_initialized = false;
			return _fileSetItems.Remove(item);
		}

		public IEnumerator<FileSetItem> GetEnumerator()
		{
			return _fileSetItems.GetEnumerator();
		}

		System.Collections.IEnumerator System.Collections.IEnumerable.GetEnumerator()
		{
			return _fileSetItems.GetEnumerator();
		}
	}

	public class FileSet : ConditionalElement
	{
		bool _initialized = false;
		GroupElement _defaultGroup = null;
		public string LogPrefix = "[fileset] ";

		public FileSet() : this((Project)null)
		{
		}

		public FileSet(Project project)
		{
			Project = project;
			_defaultGroup = new GroupElement(this, null, Project);
		}

		// copy constructor
		public FileSet(FileSet source)
		{
			DefaultExcludes = source.DefaultExcludes;
			Location = source.Location;
			// _parent = source._parent;
			Project = source.Project;
			BaseDirectory = source.BaseDirectory;
			Sort = source.Sort;
			FailOnMissingFile = source.FailOnMissingFile;
			_defaultGroup = new GroupElement(this, null, Project);

			Clear();
			Append(source);
		}

		public void Append(FileSet fileSet)
		{
			if (fileSet != null)
			{
				IncludesDefined = IncludesDefined || fileSet.IncludesDefined;
				Includes.AddRange(fileSet.Includes);
				Excludes.AddRange(fileSet.Excludes);
			}
		}

		public void Clear()
		{
			Includes.Clear();
			Excludes.Clear();
		}

		public bool ContentEquals(FileSet other)
		{
			if (Includes != null && !Includes.OrderBy(kvp => kvp.ToString()).SequenceEqual(other.Includes.OrderBy(kvp => kvp.ToString()), FileSetItem.EqualityComparer))
				return false;

			if (Excludes != null && !Excludes.OrderBy(kvp => kvp.ToString()).SequenceEqual(other.Excludes.OrderBy(kvp => kvp.ToString()), FileSetItem.EqualityComparer))
				return false;

			if (DefaultExcludes != other.DefaultExcludes || FailOnMissingFile != other.FailOnMissingFile || Sort != other.Sort)
				return false;

			if (BaseDirectory != null && other.BaseDirectory == null)
				return false;

			if (BaseDirectory == null && other.BaseDirectory != null)
				return false;

			if (BaseDirectory != null && !BaseDirectory.Equals(other.BaseDirectory, StringComparison.CurrentCultureIgnoreCase))
				return false;

			if (FromFileSetName != null && other.FromFileSetName == null)
				return false;

			if (FromFileSetName == null && other.FromFileSetName != null)
				return false;

			if (FromFileSetName != null && !FromFileSetName.Equals(other.FromFileSetName))
				return false;

			return true;
		}

		/// <summary>Determines if a file has a more recent last write time than the given time.</summary>
		/// <param name="fileNames">A collection of filenames to check last write times against.</param>
		/// <param name="targetLastWriteTime">The datetime to compare against.</param>
		/// <returns>The name of the first file that has a last write time greater than <c>targetLastWriteTime</c>; otherwise null.</returns>
		public static string FindMoreRecentLastWriteTime(StringCollection fileNames, DateTime targetLastWriteTime)
		{
			foreach (string fileName in fileNames)
			{
				// only check fully file names that have a full path
				if (Path.IsPathRooted(fileName))
				{
					FileInfo fileInfo = new FileInfo(fileName);
					if (!fileInfo.Exists)
					{
						return fileName;
					}
					if (fileInfo.LastWriteTime > targetLastWriteTime)
					{
						return fileName;
					}
				}
			}
			return null;
		}

		/// <summary>Indicates whether default excludes should be used or not.  Default "false".</summary>
		[TaskAttribute("defaultexcludes")]
		public bool DefaultExcludes { get; set; } = false;

		/// <summary>The base of the directory of this file set.  Default is project base directory.</summary>
		[TaskAttribute("basedir")]
		public string BaseDirectory { get; set; } = null;

		/// <summary>Indicates if a build error should be raised if an explicitly included file does not exist.  Default is true.</summary>
		[TaskAttribute("failonmissing")]
		public bool FailOnMissingFile { get; set; } = true;

		/// <summary>The name of a file set to include.</summary>
		[TaskAttribute("fromfileset", Required = false)]
		public string FromFileSetName { get; set; } = null;

		/// <summary>Sort the fileset by filename. Default is false.</summary>
		[TaskAttribute("sort", Required = false)]
		public bool Sort { get; set; } = false;
		public FileSetItemCollection Includes { get; } = new FileSetItemCollection();

		/// <summary>True if any includes were specified even if their conditional attributes did not evaluate to true.</summary>
		public bool IncludesDefined { get; private set; }

		public FileSetItemCollection Excludes { get; } = new FileSetItemCollection();

		/// <summary>Collection of file set items that match the file set.</summary>
		public FileItemList FileItems
		{
			get { return Includes.GetMatchingItems(Excludes, FailOnMissingFile, Sort); }
		}

		/// <summary>True if any excludes were specified even if their conditional attributes did not evaluate to true.</summary>
		public bool ExcludesDefined { get; private set; }

		private static readonly string[] ExcludedDefaults = {
			"**/*~", 
			"**/#*#", 
			"**/.#*", 
			"**/%*%", 
			"**/CVS", 
			"**/CVS/**",
			"**/.cvsignore", 
			"**/SCCS", 
			"**/SCCS/**", 
			"**/vssver.scc"
		};

		/// <summary>Optimization. Directly initialize</summary>
		[XmlElement("includes", "NAnt.Core.FileSet+IncludesElement", AllowMultiple = true, Description="Defines a file pattern to be included in the fileset.")]
		[XmlElement("excludes", "NAnt.Core.FileSet+ExcludesElement", AllowMultiple = true, Description = "Defines a file pattern of files to be excluded from the fileset.")]
		[XmlElement("do", Container = XmlElementAttribute.ContainerType.ConditionalContainer, AllowMultiple = true, Description="Groups help you organize related files into groups that can be conditionally included or excluded using if/unless attributes. Note that groups can be nested.")]
		[XmlElement("group", Container = XmlElementAttribute.ContainerType.ConditionalContainer, AllowMultiple = true, Description = "Groups help you organize related files into groups that can be conditionally included or excluded using if/unless attributes. Note that groups can be nested.")]
		protected override void InitializeElement(XmlNode elementNode)
		{
			if (IfDefined && !UnlessDefined)
			{
				OptimizedInitializeFileSetElement(elementNode);
			}
		}

		internal void OptimizedInitializeFileSetElement(XmlNode elementNode)
		{
			if (_initialized)
			{
				return;
			}

			_initialized = true;

			if (BaseDirectory == null)
			{
				BaseDirectory = Project.BaseDirectory;
			}
			BaseDirectory = Project.GetFullPath(BaseDirectory);

			// initialize the fromset
			if (FromFileSetName != null)
			{
				FileSet fromSet;
				if (!Project.NamedFileSets.TryGetValue(FromFileSetName, out fromSet))
				{
					string msg = String.Format("Using unknown named file set '{0}'.", FromFileSetName);
					throw new BuildException(msg, Location);
				}
				Append(fromSet);
			}

			// initialize all includes and excludes
			_defaultGroup.Project = Project;
			_defaultGroup.IfDefined = IfDefined;
			_defaultGroup.UnlessDefined = UnlessDefined;
			_defaultGroup.OptimizedInitializeDefaultGroup(elementNode);

			// initialize default exclude patterns
			if (DefaultExcludes)
			{
				foreach (string pattern in ExcludedDefaults)
				{
					Pattern p = PatternFactory.Instance.CreatePattern(pattern, BaseDirectory);
					Excludes.Add(p);
				}
			}
		}

		/// <summary>Base class for includes and excludes file set elements.</summary>
		abstract class FileSetElement : ConditionalElement
		{
			string _optionSet = null;

			// These bools are stored as objects so we have a reference as to whether or 
			// not they were specifically set. They will be cast back down to bool when accessed.
			// We need to know if they have been specifically set so as not to look further up
			// the tree for their values.
			bool? _asIs = null;
			bool? _force = null;

			static readonly bool DefaultAsIs = false;
			static readonly bool DefaultForce = false;


			public FileSetElement(object parent, Project project)
			{
				Parent = parent;
				Project = project;
			}

			/// <summary>The filename or pattern used for file inclusion/exclusion. Default specifies no file.</summary>
			[TaskAttribute("name")]
			public string Pattern { get; set; } = null;

			/// <summary>
			/// The name of an optionset to associate with this set of includes or excludes.
			/// </summary>
			[TaskAttribute("optionset")]
			public string OptionSet
			{
				get
				{
					if (_optionSet == null && Parent != null)
					{
						_optionSet = ((GroupElement)Parent).OptionSet;
					}
					return _optionSet;
				}
				set { _optionSet = value; }
			}

			/// <summary>
			/// The name of a fileset defined by the &lt;fileset&gt; task. This fileset will be used for file inclusion/exclusion. Default is empty.
			/// </summary>
			[TaskAttribute("fromfileset")]
			public string FromFileSetName { get; set; } = null;

			/// <summary>The name of a file containing a newline delimited set of files/patterns to include/exclude..</summary>
			[TaskAttribute("fromfile", Required = false)]
			public string FromFile { get; set; } = null;

			/// <summary>If true then the file name will be added to the fileset without pattern matching or checking if the file exists. Default is "false".</summary>
			[TaskAttribute("asis")]
			public bool AsIs
			{
				get
				{
					if (_asIs == null && Parent != null)
					{
						_asIs = ((GroupElement)Parent).AsIs;
					}
					if (_asIs == null)
					{
						return DefaultAsIs;
					}
					return (bool)_asIs;
				}
				set { _asIs = value; }
			}

			/// <summary>If true the file name will be added to the file set regardless if it is already included. Default is false.</summary>
			[TaskAttribute("force")]
			public bool Force
			{
				get
				{
					if (_force == null && Parent != null)
					{
						_force = ((GroupElement)Parent).Force;
					}
					if (_force == null)
					{
						return DefaultForce;
					}
					return (bool)_force;
				}
				set { _force = value; }
			}

			/// <summary>The base of the directory of this include pattern. 
			/// Default is fileset base directory. When 'basedir' is specified here it will be propagated to fileitems.
			/// Use this attribute to set custom basedir value which is different from the FileSet basedir, 
			/// In addition to evaluating patterns, basedir is used to set Link elements and file folders in Visual Studio,
			/// compiler resources, in CopyTask, etc.
			/// </summary>
			[TaskAttribute("basedir")]
			public string BaseDirectory { get; set; } = null;


			/// <summary>Optimization. Directly initialize</summary>
			public override void Initialize(XmlNode elementNode)
			{
				if (Project == null)
				{
					throw new InvalidOperationException("Element " + elementNode.Name + " has invalid (null) Project property.");
				}

				// Save position in build file for reporting useful error messages.
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
					string asis = null;
					string force = null;
					foreach (XmlAttribute attr in elementNode.Attributes)
					{
						switch (attr.Name)
						{
							case "name":
								Pattern = attr.Value;
								break;
							case "optionset":
								_optionSet = attr.Value;
								break;
							case "fromfileset":
								FromFileSetName = attr.Value;
								break;
							case "fromfile":
								FromFile = attr.Value;
								break;
							case "asis":
								asis = attr.Value;
								break;
							case "force":
								force = attr.Value;
								break;
							case "basedir":
								BaseDirectory = attr.Value;
								break;


							default:
								if (!OptimizedConditionalElementInit(attr.Name, attr.Value))
								{
									Log.ThrowWarning(Log.WarningId.SyntaxError, Log.WarnLevel.Normal,
										"Element <{0}> has unrecognized attributes '{1}=\"{2}\".", elementNode.Name, attr.Name, attr.Value);
								}
								break;
						}
					}

					XmlElement element = (XmlElement)elementNode;
					if (element.InnerText.Trim() != "")
					{
						Log.ThrowWarning(Log.WarningId.SyntaxError, Log.WarnLevel.Normal, "Invalid inner text inside of file set element: {0}", element.InnerText);
					}

					if (IfDefined && !UnlessDefined)
					{
						if (Pattern != null) Pattern = Project.ExpandProperties(Pattern);
						if (_optionSet != null) _optionSet = Project.ExpandProperties(_optionSet);
						if (FromFileSetName != null) FromFileSetName = Project.ExpandProperties(FromFileSetName);
						if (FromFile != null) FromFile = Project.ExpandProperties(FromFile);
						if (BaseDirectory != null) BaseDirectory = Project.ExpandProperties(BaseDirectory);
						if (asis != null) _asIs = ElementInitHelper.InitBoolAttribute(this, "asis", asis);
						if (force != null) _force = ElementInitHelper.InitBoolAttribute(this, "force", force);

						InitializeElement(elementNode);
					}
				}
				catch (BuildException ex)
				{
					throw new ContextCarryingException(ex, Name, Location);
				}
			}

			public virtual FileSetItemCollection GetFileSetItems(string baseDirectory)
			{
				FileSetItemCollection fileSetItemCollection = new FileSetItemCollection();

				// If <includes> does not define base directory - use fileset value
				var patternBaseDirectory = BaseDirectory ?? baseDirectory;
				var preserveBasedir = BaseDirectory != null;

				// add items from the FromFile
				if (FromFile != null)
				{
					try
					{
						using (StreamReader sr = new StreamReader(FromFile))
						{
							string line;
							while ((line = sr.ReadLine()) != null)
							{
								line = line.Trim();
								if (line.Length != 0)
								{
									Pattern pattern = PatternFactory.Instance.CreatePattern(line, patternBaseDirectory, AsIs);
									fileSetItemCollection.Add(new FileSetItem(pattern, OptionSet, AsIs, Force));
								}
							}
						}
					}
					catch (Exception e)
					{
						throw new BuildException("Error reading fileset FromFile '" + FromFile + "'.", e);
					}
				}

				// add items from the FromSet
				if (FromFileSetName != null)
				{
					FileSet fileSet;
					if (!Project.NamedFileSets.TryGetValue(FromFileSetName, out fileSet))
					{
						string msg = String.Format("Using unknown named FileSet '{0}'.", FromFileSetName);
						throw new BuildException(msg, Location);
					}

					foreach (FileItem fileItem in fileSet.FileItems)
					{
						Pattern pattern = PatternFactory.Instance.CreatePattern(fileItem.FileName, fileItem.BaseDirectory ?? patternBaseDirectory, fileItem.AsIs);
						pattern.FailOnMissingFile = false; // This item was already verified once. No need to do it second time
						pattern.PreserveBaseDirValue = (preserveBasedir || fileItem.BaseDirectory != null);
						FileSetItem newFileSetItem = new FileSetItem(pattern, fileItem.OptionSetName ?? OptionSet, fileItem.AsIs, fileItem.Force);
						fileSetItemCollection.Add(newFileSetItem);
					}
				}

				// Add pattern
				if (Pattern != null)
				{
					Pattern pattern = PatternFactory.Instance.CreatePattern(Pattern, patternBaseDirectory, AsIs);
					pattern.PreserveBaseDirValue = preserveBasedir;
					fileSetItemCollection.Add(new FileSetItem(pattern, OptionSet, AsIs, Force));
				}

				return fileSetItemCollection;
			}
		}

		[ElementName("excludes", StrictAttributeCheck = true)]
		class ExcludesElement : FileSetElement
		{
			public ExcludesElement(object parent, Project project)
				: base(parent, project)
			{
			}
		}

		[ElementName("includes", StrictAttributeCheck = true)]
		class IncludesElement : FileSetElement
		{
			public IncludesElement(object parent, Project project)
				: base(parent, project)
			{
			}
		}

		/// <summary>Represents groups of files in a file set.</summary>
		[ElementName("group")]
		class GroupElement : FileSetElement
		{
			public GroupElement(FileSet fileSet, object parent, Project project)
				: base(parent, project)
			{
				FileSet = fileSet;
			}

			private FileSet FileSet { get; set; }

			protected override void InitializeElement(XmlNode elementNode)
			{
				OptimizedInitializeGroupElement(elementNode);
			}

			/// <summary>Optimization for default group. Directly initialize</summary>
			internal void OptimizedInitializeDefaultGroup(XmlNode elementNode)
			{
					Location = FileSet.Location;
					IfDefined = FileSet.IfDefined;
					UnlessDefined = FileSet.UnlessDefined;
					try
					{
						foreach (XmlAttribute attr in elementNode.Attributes)
						{
							switch (attr.Name)
							{
								case "optionset":
									OptionSet = Project.ExpandProperties(attr.Value);
									break;
								case "asis":
									AsIs = ElementInitHelper.InitBoolAttribute(this, "asis", attr.Value);
									break;
								case "force":
									Force = ElementInitHelper.InitBoolAttribute(this, "force", attr.Value);
									break;

								default:
									break;
							}
						}
					}
					catch (BuildException e)
					{
						e.SetLocationIfMissing(Location);
						throw;
					}
					catch (Exception e)
					{
						string msg = String.Format("An error occurred initializing the '{0}' element.", elementNode.Name);
						throw new BuildException(msg, Location, e);
					}

					OptimizedInitializeGroupElement(elementNode);
			}


			internal void OptimizedInitializeGroupElement(XmlNode elementNode)
			{
				// The Element class will initialize the marked XML attributes but
				// not the unmarked <includes> and <excludes> elements.  We have to
				// initialize them ourselves.

				if (this.IfDefined && !this.UnlessDefined)
				{
					foreach (XmlNode node in elementNode)
					{
						if (node.NodeType == XmlNodeType.Element && node.NamespaceURI.Equals(elementNode.NamespaceURI))
						{
							switch (node.Name)
							{
								case "includes":
									IncludesElement include = new IncludesElement(this, Project);
									include.Initialize(node);

									FileSet.IncludesDefined = true;
									if (include.IfDefined && !include.UnlessDefined)
									{
										FileSet.Includes.AddRange(include.GetFileSetItems(FileSet.BaseDirectory));
									}

									// if <includes> specifies an attribute but fails to contain at least 1 of the following valid attributes: name, fromfile, or fromfileset
									if (include.Pattern == null && include.FromFile == null && include.FromFileSetName == null && node.Attributes.Count > 0)
									{
										// then notify of an error
										const string msg = "If a fileset <includes> statement specifies an attribute, but the <includes> lacks at least one of the following valid attributes: name, fromfileset, or fromfile, then throw an error";
										throw new BuildException(msg, Location);
									}
									break;

								case "excludes":

									ExcludesElement exclude = new ExcludesElement(this, Project);
									exclude.Initialize(node);

									FileSet.ExcludesDefined = true;
									if (exclude.IfDefined && !exclude.UnlessDefined)
									{
										FileSet.Excludes.AddRange(exclude.GetFileSetItems(FileSet.BaseDirectory));
									}
									break;

								case "group":
								case "do":
									GroupElement group = new GroupElement(FileSet, this, Project);
									group.Initialize(node);
									break;
								default:
									{
										string msg = String.Format("Illegal FileSet element '{0}'.", node.Name);
										if (node.Name == "include")
										{
											msg = String.Format("Illegal FileSet element 'include', should be 'includes'.");
										}
										else if (node.Name == "exclude")
										{
											msg = String.Format("Illegal FileSet element 'exclude', should be 'excludes'.");
										}
										throw new BuildException(msg, Location);
									}
							}
						}
						else if (node.NodeType == XmlNodeType.Text || node.NodeType == XmlNodeType.CDATA)
						{
							string msg = String.Format("Invalid inner text inside FileSet element: {0}", node.Value);
							throw new BuildException(msg, Location);
						}
					}
				}
			}
		}
	}
}
