using System;
using System.Collections.Generic;
using System.Collections.Concurrent;
using System.Xml;
using System.IO;

using NAnt.Core.Logging;

namespace NAnt.Core.Util
{
    public class PathStringCollection : IEnumerable<string>
    {
        public PathStringCollection(Log log)
        {
            _log = log;
            _files = new ConcurrentDictionary<string, IncludedNode>();
        }

        public PathStringCollection(PathStringCollection other, Log log = null)
        {
            _log = log??other._log;
            _files = new ConcurrentDictionary<string, IncludedNode>(other._files);
        }


        public int Count
        {
            get
            {
                return _files.Count;
            }
        }

        public bool TryAdd(string parent, Uri uri)
        {
            bool ret = false;

            if (uri != null)
            {
                try
                {
                    ret = TryAdd(parent, new FileInfo(uri.LocalPath).FullName);

                }
                catch (Exception)
                {
                    // do nothing
                }                
            }
            return ret;
        }


        public bool TryAdd(string parent, string value)
        {
            bool ret = false;

            if (!String.IsNullOrEmpty(value))
            {
                if (!(ret = _files.TryAdd(GetKey(value), new IncludedNode(value))))
                {
                    _log.Warning.WriteLine("PathStringCollection: ignoring duplicate include {0}", value);
                }
                else
                {
                    IncludedNode parentNode;
                    if (_files.TryGetValue(GetKey(parent), out parentNode))
                    {
                        parentNode.IncludedFiles.Push(value);
                    }
                }
            }
            return ret;
        }

        public IEnumerable<string> GetIncludedFiles(string value)
        {
            IncludedNode parentNode;
            if (_files.TryGetValue(GetKey(value), out parentNode) && parentNode != null)
            {
                return parentNode.IncludedFiles;
            }
            return null;
        }



        public bool Contains(string value)
        {
            return _files.ContainsKey(GetKey(value));
        }

        public IEnumerator<string> GetEnumerator()
        {
            foreach (var pair in _files)
            {
                yield return pair.Value.FilePath;
            }
        }

        System.Collections.IEnumerator System.Collections.IEnumerable.GetEnumerator()
        {
            return GetEnumerator();
        }


        private string GetKey(string value)
        {
            string key = value;
            if (!PathUtil.IsCaseSensitive)
            {
                key = key.ToLower();
            }
            return key;
        }

        private readonly Log _log;
        private ConcurrentDictionary<string, IncludedNode> _files;

        class IncludedNode
        {
            internal IncludedNode(string filepath)
            {
                FilePath = filepath;
            }
            public readonly string FilePath;
            public readonly ConcurrentStack<string> IncludedFiles = new ConcurrentStack<string>();

        }
    }
}
