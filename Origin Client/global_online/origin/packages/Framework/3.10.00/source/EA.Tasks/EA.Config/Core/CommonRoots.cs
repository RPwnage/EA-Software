using System;
using System.Xml;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using System.Globalization;

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Writers;
using NAnt.Core.Attributes;
using NAnt.Core.Functions;
using NAnt.Core.Logging;
using NAnt.Core.Reflection;
using EA.FrameworkTasks.Model;

using EA.Eaconfig;
using EA.Eaconfig.Core;
using EA.Eaconfig.Build;
using EA.Eaconfig.Modules;

namespace EA.Eaconfig.Core
{
    public class CommonRoots<T>
    {
        public readonly IDictionary<string, string> Roots;        
        internal readonly IEnumerable<string> CustomPrefixes;

        private Func<T, string> GetPath;

        public CommonRoots(IOrderedEnumerable<T> files, Func<T,string> getPath, IEnumerable<string> prefixes = null)
        {
            GetPath = getPath;
            CustomPrefixes = prefixes;
            StringComparison stringComp = PathUtil.IsCaseSensitive ? StringComparison.Ordinal : StringComparison.OrdinalIgnoreCase;
            Roots = new Dictionary<string, string>();

            foreach (var file in files)
            {
                string currentPath = GetPath(file);
                if (String.IsNullOrEmpty(currentPath))
                {
                    continue;
                }
                // drive of current path
                string currentDrive = GetPrefix(currentPath);

                currentPath = Path.GetDirectoryName(currentPath);

                if (currentPath != Path.GetPathRoot(currentPath))
                {
                    currentPath += Path.DirectorySeparatorChar;
                }

                if (!Roots.ContainsKey(currentDrive))
                {
                    // there is no common path set for this drive yet
                    Roots[currentDrive] = currentPath;
                }

                while (Roots[currentDrive] != Path.GetPathRoot(Roots[currentDrive])
                       && !currentPath.StartsWith(Roots[currentDrive], stringComp))
                {
                    string newPath = Path.GetDirectoryName(Roots[currentDrive].TrimEnd(new char[] { Path.DirectorySeparatorChar }));

                    if (newPath != Path.GetPathRoot(newPath))
                    {
                        newPath += Path.DirectorySeparatorChar;
                    }
                    Roots[currentDrive] = newPath;
                }
            }
        }

        internal IDictionary<string, bool> FilterMap(IOrderedEnumerable<T> files, IDictionary<string, string> predefinedFilterMap = null)
        {
            var duplicates = new Dictionary<string, bool>(PathUtil.IsCaseSensitive ? StringComparer.Ordinal : StringComparer.OrdinalIgnoreCase);
            var temp = new Dictionary<string, string>(PathUtil.IsCaseSensitive ? StringComparer.Ordinal : StringComparer.OrdinalIgnoreCase);

            foreach (var file in files)
            {
                string basedir = Path.GetDirectoryName(GetPath(file));

                string prefix = GetPrefix(basedir);

                string root;

                if (!Roots.TryGetValue(prefix, out root))
                {
                    throw new BuildException(String.Format("Internal error: common root for '{0}' is not defined", prefix));
                }

                string dirName = String.Empty;

                if (predefinedFilterMap == null || !predefinedFilterMap.TryGetValue(prefix, out dirName))
                {
                    if (String.Compare(basedir, 0, root, 0, basedir.Length, !PathUtil.IsCaseSensitive) != 0)
                    {
                        // work out what the name of the next directory down should be
                        int endIdx = basedir.IndexOf(Path.DirectorySeparatorChar, root.Length);

                        if (endIdx >= 0)
                        {
                            dirName = basedir.Substring(root.Length, endIdx - root.Length);
                        }
                        else
                        {
                            dirName = basedir.Substring(root.Length);
                        }
                    }
                }

                if (!String.IsNullOrEmpty(dirName))
                {
                    string previousRoot;
                    if (temp.TryGetValue(dirName, out previousRoot))
                    {
                        if (0 != String.Compare(root, previousRoot, !PathUtil.IsCaseSensitive))
                        {
                            duplicates[dirName] = true;
                        }
                    }
                    else
                    {
                        temp.Add(dirName, root);
                    }
                }
            }

            return duplicates;
        }

        public string GetPrefix(string path)
        {
            var prefix = CustomPrefixes == null ? null : CustomPrefixes.FirstOrDefault(p => path.StartsWith(p, (PathUtil.IsCaseSensitive ? StringComparison.Ordinal : StringComparison.OrdinalIgnoreCase)));
            
            if(prefix == null)
            {
                return PathUtil.GetDriveLetter(path);
            }

            return prefix;
        }
    }
}
