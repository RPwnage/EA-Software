using System;
using System.Collections.Generic;
using System.Linq;
using System.IO;

namespace NAnt.Core.Reflection
{
    internal static class ReflectionUtil
    {
        internal static IEnumerable<string> GetAssemblyFiles(string path, string[] patterns)
        {
            return Directory.GetFiles(path).Where(file => FitsPattern(file, patterns));
        }

        private static bool FitsPattern(string file, string[] patterns)
        {
            if (file != null && patterns != null)
            {
                foreach (var pattern in patterns)
                {
                    if (file.EndsWith(pattern, StringComparison.OrdinalIgnoreCase))
                    {
                        return true;
                    }
                }
            }
            return false;
        }
    }
}
