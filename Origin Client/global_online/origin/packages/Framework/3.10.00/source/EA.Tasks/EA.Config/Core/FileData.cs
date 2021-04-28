using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

using NAnt.Core;
using NAnt.Core.Util;
using EA.FrameworkTasks.Model;
using EA.Eaconfig.Core;

namespace EA.Eaconfig.Core
{
    public class FileData : BitMask
    {
        public const uint BulkBuild = 1;
        public const uint Header = 2;
        public const uint EmbeddedResource = 4;
        public const uint Asset = 8;
        public const uint Resource = 16;
        public const uint ContentFile = 32;
        public const uint ConflictObjName = 64;

        public FileData(Tool tool, PathString objectfile = null, uint flags=0) : base(flags) 
        {
            Tool = tool;
            ObjectFile = objectfile;
        }

        public static string ToString(uint type)
        {
            StringBuilder sb = new StringBuilder();

            test(type, BulkBuild, "bulkbuild", sb);
            test(type, Header, "header", sb);
            test(type, EmbeddedResource, "embedded-resource", sb);
            test(type, Resource, "resource", sb);
            test(type, Asset, "asset", sb);
            test(type, ContentFile, "content", sb);

            return sb.ToString().TrimEnd('|');
        }

        private static void test(uint t, uint mask, string name, StringBuilder sb)
        {
            if ((mask & t) != 0) sb.AppendFormat("{0}|", name);
        }


        public Tool Tool;
        public PathString ObjectFile;
        public object Data;
    }
}
