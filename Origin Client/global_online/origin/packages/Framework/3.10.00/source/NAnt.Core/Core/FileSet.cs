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
using System.Linq;
using System.Collections.Generic;
using System.Collections.Concurrent;
using System.Collections.Specialized;
using System.IO;
using System.Xml;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading.Tasks;


using NAnt.Core.Util;
using NAnt.Core.Logging;
using NAnt.Core.Threading;
using NAnt.Core.Reflection;
using NAnt.Core.Attributes;

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

        //public readonly PathString FileName;
        private readonly PathString _fileName;
        public string OptionSetName;
        public readonly bool AsIs;
        public readonly bool Force;
        private object _data;
        private int _index = 0;
        public readonly string BaseDirectory;

        public static readonly FileItemPathEqualityComparer PathEqualityComparer = new FileItemPathEqualityComparer();

        public FileItem(PathString fileName, string optionSetName = null, bool asIs = false, bool force = false, object data = null, string basedir=null)
        {
            //IMTODO: use this
            //FileName = asIs ? fileName : PathString.MakeNormalized(fileName);
            _fileName = asIs ? fileName : PathString.MakeNormalized(fileName);
            OptionSetName = optionSetName;
            AsIs = asIs;
            Force = force;
            _data = data;
            BaseDirectory = basedir;
        }

        /// <summary>Internal use only for storage!</summary>
        public int Index
        {
            get { return _index; }
            set { _index = value; }
        }


        public string FullPath
        {
            get
            {
                return _fileName.NormalizedPath;
            }
        }
        //IMTODO - remove this
        public string FileName
        {
            get
            {
                return _fileName.Path;
            }
        }

        public PathString Path
        {
            get
            {
                return _fileName;
            }
        }

        // an object to store internal data
        public object Data
        {
            get { return _data; }
            set { _data = value; }
        }

        public int CompareTo(FileItem a)
        {
            return String.Compare(FileName, a.FileName);
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
        /// <remarks>Provided as a convinience for backwards compatability and external libraries.</remarks>
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
    }

    /// <summary>
    /// A collection of FileSetItems
    /// </summary>
    public class FileSetItemCollection : IList<FileSetItem>, IEnumerable<FileSetItem>
    {
        /// <summary>
        /// A specialized container class for optimizing the storage of files.
        /// A hashtable of linked lists where the key is the basedirectory of the file.
        /// </summary>
        class FileItemContainer
        {
            Dictionary<string, FileItemList> _hashTable = new Dictionary<string, FileItemList>();
            int _curIndex = 0;
            bool _sort = false;

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
                string key = Path.GetDirectoryName(fileName);
                FileItemList fileItemList;
                if (_hashTable.TryGetValue(key, out  fileItemList))
                {
                    foreach (FileItem fileItem in fileItemList)
                    {
                        if (String.Compare(fileItem.FileName, fileName) == 0)
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

            public bool Sort
            {
                get { return _sort; }
                set { _sort = value; }
            }
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

        internal class InitializeProcParams
        {
            internal InitializeProcParams(FileSetItemCollection items, int start, int end, List<FileItem> fileItems, FileSetItemCollection excludePatterns, bool failOnMissingFile,
                                          List<Pattern.RegexPattern> excludeRegexPatterns, StringCollection excludeFileNames, StringCollection asIsExcludeFileNames)
            {
                Items = items;
                Start = start;
                End = end;
                ExcludePatterns = excludePatterns;
                FailOnMissingFile = failOnMissingFile;
                FileItems = fileItems;
                ExcludeRegexPatterns = excludeRegexPatterns;
                ExcludeFileNames = excludeFileNames;
                AsIsExcludeFileNames = asIsExcludeFileNames;
            }

            internal readonly FileSetItemCollection Items;
            internal readonly int Start;
            internal readonly int End;
            internal readonly FileSetItemCollection ExcludePatterns;
            internal readonly bool FailOnMissingFile;
            internal readonly List<FileItem> FileItems;
            internal readonly List<Pattern.RegexPattern> ExcludeRegexPatterns;
            internal readonly StringCollection ExcludeFileNames;
            internal readonly StringCollection AsIsExcludeFileNames;
        }

        private object InitializeProc(InitializeProcParams parameters)
        {
            FileSetItemCollection items = parameters.Items;

            for (int i = parameters.Start; i < parameters.End; i++)
            {
                FileSetItem item = (FileSetItem)items[i];
                bool failOnMissingFile = parameters.FailOnMissingFile;

                if (item.Pattern.BaseDirectory == null)
                {
                    item.Pattern.BaseDirectory = Environment.CurrentDirectory;
                }

                List<Pattern.RegexPattern> excludeRegexPatterns = new List<Pattern.RegexPattern>();
                StringCollection excludeFileNames = new StringCollection();

                if (!item.Force)
                {
                    if (!item.AsIs)
                    {
                        excludeRegexPatterns = parameters.ExcludeRegexPatterns;
                        excludeFileNames = parameters.ExcludeFileNames;
                    }
                    else
                    {
                        excludeFileNames = parameters.AsIsExcludeFileNames;
                    }
                }

                List<string> includedFiles = item.Pattern.GetMatchingFiles(failOnMissingFile, excludeRegexPatterns, excludeFileNames);


                foreach (string file in includedFiles)
                {
                    //IMTODO: Use PathString consistently. Make GetMatchingFiles to return PathString.
                    PathString fileStr = new PathString(file, PathString.PathState.File);
                    parameters.FileItems.Add(new FileItem(fileStr, item.OptionSet, item.AsIs, item.Force, item.Data, item.Pattern.PreserveBaseDirValue?item.Pattern.BaseDirectory:null));
                }
            }

            return null;
        }

        private void Initialize(FileSetItemCollection excludePatterns, bool failOnMissingFile, bool sort)
        {
            _fileItemContainer = new FileItemContainer();
            _fileItemContainer.Sort = sort;

            if (Count == 0)
                return;

            int patterncount = 0;

            var fileitems = new List<FileItem>[Count];
            List<Pattern.RegexPattern> excludeRegexPatterns;
            StringCollection excludeFileNames;
            StringCollection asIsExcludeFileNames;

            ComputeExcludedRegexPatters(excludePatterns, out excludeRegexPatterns, out excludeFileNames, out asIsExcludeFileNames);

#if NANT_CONCURRENT
            // Under mono most parallel execution is slower than consequtive. Untill this is resolved use consequtive execution 
            bool threading = (PlatformUtil.IsMonoRuntime == false);
#else
            bool threading = false;
#endif

            // If more than one pattern run multiple threads.
            if (Count > 1)
            {
                int interval = Count / (2 * Math.Max(Environment.ProcessorCount, 1));
                interval = Math.Max(interval, 1);

                int start = 0;
                int end = Math.Min(start + interval, Count);

                if (threading)
                {
                    try
                    {
                        var tasks = new List<System.Threading.Tasks.Task>();

                        while (start < end)
                        {
                            List<FileItem> itemList = fileitems[patterncount] = new List<FileItem>();

                            var initializeProcParams = new InitializeProcParams(this, start, end, itemList, excludePatterns, failOnMissingFile, excludeRegexPatterns, excludeFileNames, asIsExcludeFileNames);

                            tasks.Add(System.Threading.Tasks.Task.Factory.StartNew(() => InitializeProc(initializeProcParams)));

                            patterncount++;
                            start = end;
                            end = Math.Min(start + interval, Count);
                        }

                        System.Threading.Tasks.Task.WaitAll(tasks.ToArray());
                    }
                    catch (Exception e)
                    {
                        ThreadUtil.RethrowThreadingException("An error occured expanding a fileset.", e);
                    }
                }
                else
                {
                    while (start < end)
                    {
                        List<FileItem> itemList = fileitems[patterncount] = new List<FileItem>();

                        InitializeProc(new InitializeProcParams(this, start, end, itemList, excludePatterns, failOnMissingFile, excludeRegexPatterns, excludeFileNames, asIsExcludeFileNames));

                        patterncount++;
                        start = end;
                        end = Math.Min(start + interval, Count);
                    }
                }
            }
            else if (Count > 0)
            {
                List<FileItem> itemList = fileitems[0] = new List<FileItem>();
                InitializeProc(new InitializeProcParams(this, 0, 1, itemList, excludePatterns, failOnMissingFile, excludeRegexPatterns, excludeFileNames, asIsExcludeFileNames));
            }

            foreach (List<FileItem> itemList in fileitems)
            {
                if (itemList == null) break;

                foreach (FileItem item in itemList)
                {
                    if (!item.AsIs && !_fileItemContainer.Contains(item.FileName))
                    {
                        _fileItemContainer.Add(item);
                    }
                    else if (item.AsIs)
                    {
                        _fileItemContainer.Add(item);
                    }
                }
            }
        }

        /// <summary>Returns a collection of fileitems which match the set of patterns</summary>
        public FileItemList GetMatchingItems(FileSetItemCollection excludePatterns, bool failOnMissingFile, bool sort)
        {
            if (_initialized == false)
            {
                Initialize(excludePatterns, failOnMissingFile, sort);
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
        bool _defaultExcludes = false;
        string _baseDirectory = null;
        bool _failOnMissingFile = true;
        bool _sort = false;
        string _fromset = null;
        GroupElement _defaultGroup = null;

        FileSetItemCollection _includes = new FileSetItemCollection();
        FileSetItemCollection _excludes = new FileSetItemCollection();

        public string LogPrefix = "[fileset] ";

        public FileSet() : this((Project)null)
        {
        }

        public FileSet(Project project)
        {
            _project = project;
            _defaultGroup = new GroupElement(this, null, Project);
        }

        // copy constructor 
        public FileSet(FileSet source)
        {
            _defaultExcludes = source._defaultExcludes;
            _location = source._location;
            // _parent = source._parent;
            _project = source._project;
            _baseDirectory = source._baseDirectory;
            _sort = source._sort;
            _failOnMissingFile = source._failOnMissingFile;
            _defaultGroup = new GroupElement(this, null, Project);

            Clear();
            Append(source);
        }

        public void Append(FileSet fileSet)
        {
            if (fileSet != null)
            {
                this.Includes.AddRange(fileSet.Includes);
                this.Excludes.AddRange(fileSet.Excludes);
            }
        }

        public void Clear()
        {
            Includes.Clear();
            Excludes.Clear();
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
        public bool DefaultExcludes
        {
            get { return _defaultExcludes; }
            set { _defaultExcludes = value; }
        }

        /// <summary>The base of the directory of this file set.  Default is project base directory.</summary>
        [TaskAttribute("basedir")]
        public string BaseDirectory
        {
            get { return _baseDirectory; }
            set { _baseDirectory = value; }
        }

        /// <summary>Indicates if a build error should be raised if an explictly included file does not exist.  Default is true.</summary>
        [TaskAttribute("failonmissing")]
        public bool FailOnMissingFile
        {
            get { return _failOnMissingFile; }
            set { _failOnMissingFile = value; }
        }

        /// <summary>The name of a file set to include.</summary>
        [TaskAttribute("fromfileset", Required = false)]
        public string FromFileSetName
        {
            get { return _fromset; }
            set { _fromset = value; }
        }

        /// <summary>Sort the fileset by filename. Default is false.</summary>
        [TaskAttribute("sort", Required = false)]
        public bool Sort
        {
            get { return _sort; }
            set { _sort = value; }
        }

        public FileSetItemCollection Includes
        {
            get { return _includes; }
        }

        public FileSetItemCollection Excludes
        {
            get { return _excludes; }
        }

        /// <summary>Collection of file set items that match the file set.</summary>
        public FileItemList FileItems
        {
            get { return Includes.GetMatchingItems(Excludes, FailOnMissingFile, Sort); }
        }

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

        /// <summary>Optimization. Directly intialize</summary>
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
            if (BaseDirectory == null)
            {
                BaseDirectory = Project.BaseDirectory;
            }
            BaseDirectory = Project.GetFullPath(BaseDirectory);

            if (_initialized)
            {
                return;
            }
            _initialized = true;

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
            string _pattern = null;
            string _fromset = null;
            string _optionSet = null;
            string _fromFile = null;
            string _baseDirectory = null;

            // These bools are stored as objects so we have a reference as to whether or 
            // not they were specifcally set. They will be cast back down to bool when accessed.
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
            public string Pattern
            {
                get { return _pattern; }
                set { _pattern = value; }
            }

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
            public string FromFileSetName
            {
                get { return _fromset; }
                set { _fromset = value; }
            }

            /// <summary>The name of a file containing a newline delimited set of files/patterns to include/exclude..</summary>
            [TaskAttribute("fromfile", Required = false)]
            public string FromFile
            {
                get { return _fromFile; }
                set { _fromFile = value; }
            }

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
            /// Use this attribute to set custo basedir value which is different from the FileSet basedir, 
            /// In addition to evaluating patterns, basedir is used to set Link elements and file folders in Visual Studio,
            /// compiler resources, in CopyTask, etc.
            /// </summary>
            [TaskAttribute("basedir")]
            public string BaseDirectory
            {
                get { return _baseDirectory; }
                set { _baseDirectory = value; }
            }


            /// <summary>Optimization. Directly intialize</summary>
            public override void Initialize(XmlNode elementNode)
            {
                if (Project == null)
                {
                    throw new InvalidOperationException("Element " + elementNode.Name + " has invalid (null) Project property.");
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
                    string asis = null;
                    string force = null;
                    foreach (XmlAttribute attr in elementNode.Attributes)
                    {
                        switch (attr.Name)
                        {
                            case "name":
                                _pattern = attr.Value;
                                break;
                            case "optionset":
                                _optionSet = attr.Value;
                                break;
                            case "fromfileset":
                                _fromset = attr.Value;
                                break;
                            case "fromfile":
                                _fromFile = attr.Value;
                                break;
                            case "asis":
                                asis = attr.Value;
                                break;
                            case "force":
                                force = attr.Value;
                                break;
                            case "basedir":
                                _baseDirectory = attr.Value;
                                break;


                            default:
                                if (!OptimizedConditionalElementInit(attr.Name, attr.Value))
                                {
                                    Log.Warning.WriteLine("Element <{0}> has unrecognized attributes '{1}=\"{2}\".", elementNode.Name, attr.Name, attr.Value);
                                }
                                break;
                        }
                    }

                    if (Log.Level > Log.LogLevel.Normal)
                    {
                        XmlElement element = (XmlElement)elementNode;
                        if (element.InnerText.Trim() != "")
                        {
                            Log.Warning.WriteLine("Invalid inner text inside of file set element: {0}", element.InnerText);
                        }
                    }


                    if (IfDefined && !UnlessDefined)
                    {
                        if (_pattern != null) _pattern = Project.ExpandProperties(_pattern);
                        if (_optionSet != null) _optionSet = Project.ExpandProperties(_optionSet);
                        if (_fromset != null) _fromset = Project.ExpandProperties(_fromset);
                        if (_fromFile != null) _fromFile = Project.ExpandProperties(_fromFile);
                        if (_baseDirectory != null) _baseDirectory = Project.ExpandProperties(_baseDirectory);
                        if (asis != null) _asIs = ElementInitHelper.InitBoolAttribute(this, "asis", asis);
                        if (force != null) _force = ElementInitHelper.InitBoolAttribute(this, "force", force);

                        InitializeElement(elementNode);
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



            public virtual FileSetItemCollection GetFileSetItems(string baseDirectory)
            {
                FileSetItemCollection fileSetItemCollection = new FileSetItemCollection();

                // If <includes> does not define base directory - use fileset value
                var patternBaseDirectory = _baseDirectory ?? baseDirectory;
                var preserveBasedir = _baseDirectory != null;

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
                    catch (System.Exception e)
                    {
                        throw new BuildException("Error reading fileset FromFile.", e);
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
                        Pattern pattern = PatternFactory.Instance.CreatePattern(fileItem.FileName, patternBaseDirectory, fileItem.AsIs);
                        pattern.FailOnMissingFile = false; // This item was already verified once. No need to do it second time
                        pattern.PreserveBaseDirValue = preserveBasedir;
                        FileSetItem newFileSetItem = new FileSetItem(pattern, ConvertUtil.ValueOrDefault(fileItem.OptionSetName, this.OptionSet), fileItem.AsIs, fileItem.Force);
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
            FileSet _fileSet;

            public GroupElement(FileSet fileSet, object parent, Project project)
                : base(parent, project)
            {
                _fileSet = fileSet;
            }

            private FileSet FileSet
            {
                get { return _fileSet; }
                set { _fileSet = value; }
            }

            protected override void InitializeElement(XmlNode elementNode)
            {
                OptimizedInitializeGroupElement(elementNode);
            }

            /// <summary>Optimization for default group. Directly intialize</summary>
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
                        if (e.Location == Location.UnknownLocation)
                        {
                            throw new BuildException(e.Message, this.Location, e.InnerException, e.StackTraceFlags);
                        }
                        throw;
                    }
                    catch (System.Exception e)
                    {
                        string msg = String.Format("An error occured initializing the '{0}' element.", elementNode.Name);
                        throw new BuildException(msg, this.Location, e);
                    }

                    OptimizedInitializeGroupElement(elementNode);
            }


            internal void OptimizedInitializeGroupElement(XmlNode elementNode)
            {
                // The Element class will initialize the marked xml attributes but
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

                                    if (include.IfDefined && !include.UnlessDefined)
                                    {
                                        FileSet.Includes.AddRange(include.GetFileSetItems(_fileSet.BaseDirectory));
                                    }

                                    // if <includes> specifies an attribute but fails to contain at least 1 of the following valid attributes: name, fromfile, or fromfileset
                                    if (include.Pattern == null && include.FromFile == null && include.FromFileSetName == null && node.Attributes.Count > 0)
                                    {
                                        // then notify of an error
                                        string msg = String.Format("If a fileset <includes> statement specifies an attribute, but the <includes> lacks at least one of the following valid attributes: name, fromfileset, or fromfile, then throw an error");
                                        throw new BuildException(msg, Location);
                                    }
                                    break;

                                case "excludes":

                                    ExcludesElement exclude = new ExcludesElement(this, Project);
                                    exclude.Initialize(node);

                                    if (exclude.IfDefined && !exclude.UnlessDefined)
                                    {
                                        FileSet.Excludes.AddRange(exclude.GetFileSetItems(_fileSet.BaseDirectory));
                                    }
                                    break;

                                case "group":
                                case "do":
                                    GroupElement group = new GroupElement(_fileSet, this, Project);
                                    group.Initialize(node);
                                    break;
                                default:
                                    {
                                        string msg = String.Format("Illegal FileSet element '{0}'.", node.Name);
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
