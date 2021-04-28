using System;
using System.Collections.Generic;
using System.Collections.Concurrent;
using System.Linq;
using System.Text;

using NAnt.Core;
using NAnt.Core.Util;
using EA.FrameworkTasks.Model;
using EA.Eaconfig.Core;

namespace EA.Eaconfig.Modules
{
    public static class ModuleExtensions
    {

        public static bool IsCopyLocal(this IModule module)
        {
            return (module is Module_Native && (module as Module_Native).CopyLocal != CopyLocalType.False) || (module is Module_DotNet && (module as Module_DotNet).CopyLocal != CopyLocalType.False);
        }

        public static SortedSet<string> AddUnique(this SortedSet<string> target, IEnumerable<string> source)
        {
            if (target != null && source != null)
            {
                
                foreach (var str in source)
                {
                        target.Add(str);
                }
            }
            return target;
        }

        public static SortedSet<string> AddRange(this SortedSet<string> target, IEnumerable<string> source)
        {
            if (target != null && source != null)
            {
                foreach (var str in source)
                {
                    target.Add(str);
                }
            }
            return target;
        }

        public static SortedSet<string> RemoveAll(this SortedSet<string> target, Predicate<string> match)
        {
            if (target != null)
            {
                target.RemoveWhere(match);
            }
            return target;
        }

        public static List<PathString> AddUnique(this List<PathString> target, IEnumerable<PathString> source)
        {
            if (target != null && source != null)
            {
                foreach (var path in source)
                {
                    if (!target.Contains(path))
                        target.Add(path);
                }
            }
            return target;
        }

        public static List<PathString> AddUnique(this List<PathString> target, IEnumerable<string> source)
        {
            if (target != null && source != null)
            {
                foreach (var str in source)
                {
                    PathString path = PathString.MakeNormalized(str);
                    if (!target.Contains(path))
                        target.Add(path);
                }
            }
            return target;
        }


        public static List<string> AddUnique(this List<string> target, IEnumerable<string> source)
        {
            if (target != null && source != null)
            {
                foreach (var item in source)
                {
                    if (!target.Contains(item))
                        target.Add(item);
                }
            }
            return target;
        }

    }
}
