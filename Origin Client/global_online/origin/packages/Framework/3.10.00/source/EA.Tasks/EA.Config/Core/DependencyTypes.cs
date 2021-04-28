using System;
using System.Text;
using NAnt.Core;

namespace EA.Eaconfig.Core
{ 
    public struct DependencyTypes
    {
        public const uint Undefined = 0;
        public const uint Build = 2;
        public const uint Interface = 4;
        public const uint Link = 8;
        public const uint AutoBuildUse = 16;
        public const uint CopyLocal = 32;

        public static string ToString(uint type)
        {
            StringBuilder sb = new StringBuilder();

            test(type, Build, "build", sb);
            test(type, Interface, "interface", sb);
            test(type, Link, "link", sb);
            test(type, AutoBuildUse, "auto", sb);

            return sb.ToString().TrimEnd('|');
        }

        private static void test(uint t, uint mask, string name, StringBuilder sb)
        {
            if ((mask & t) != 0) sb.AppendFormat("{0}|", name);
        }

    }
}
