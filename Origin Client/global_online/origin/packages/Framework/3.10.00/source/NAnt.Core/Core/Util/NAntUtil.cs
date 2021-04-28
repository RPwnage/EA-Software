using System;
using System.Reflection;
using System.Diagnostics;

namespace NAnt.Core.Util
{
    public static class NAntUtil
    {
        static NAntUtil()
        {
            Assembly nantCoreAssembly = Assembly.GetExecutingAssembly();
            FileVersionInfo info = FileVersionInfo.GetVersionInfo(nantCoreAssembly.Location);
            NantVersionString = String.Format("Version {0}.{1}.{2} ({3})",
                                info.FileMajorPart, info.FileMinorPart, info.FileBuildPart,
                                nantCoreAssembly.Location);
        }

        static public readonly string NantVersionString;

        static public bool IsPackageNameValid(string name)
        {
            if (String.IsNullOrEmpty(name))
            {
                return false;
            }
            return !(name.IndexOfAny(invalidNameChars) != -1 || name.StartsWith(".", StringComparison.Ordinal) || name.EndsWith(".", StringComparison.Ordinal) || name.StartsWith(" ", StringComparison.Ordinal) || name.EndsWith(" ", StringComparison.Ordinal) || String.Equals(name,"_", StringComparison.Ordinal));
        }

        static public bool IsPackageVersionStringValid(string version)
        {
            if (String.IsNullOrEmpty(version))
            {
                return false;
            }
            return !(version.IndexOfAny(invalidNameChars) != -1 || version.StartsWith(".", StringComparison.Ordinal) || version.EndsWith(".", StringComparison.Ordinal) || version.StartsWith(" ", StringComparison.Ordinal) || version.EndsWith(" ", StringComparison.Ordinal));
        }

        static public bool IsMasterconfigGrouptypeStringValid(string name)
        {
            if (String.IsNullOrEmpty(name))
            {
                return false;
            }
            return !(name.IndexOfAny(invalidNameChars) != -1 || name.StartsWith(".", StringComparison.Ordinal) || name.EndsWith(".", StringComparison.Ordinal) || name.StartsWith(" ", StringComparison.Ordinal) || name.EndsWith(" ", StringComparison.Ordinal));
        }



        private static readonly char[] invalidNameChars = new char[] { '"', '\\', '/', ':', '*', '?', '"', '<', '>', '|', '"' };
    }
}
