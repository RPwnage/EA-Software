using System;
using System.Collections.Generic;
using System.IO;

using NAnt.Core;
using NAnt.Core.Util;


namespace EA.Eaconfig.Backends.VisualStudio
{
    
    public class FileFilterDef : BitMask
    {
        public const uint Regular = 0;
        public const uint Flatten = 1;

        public readonly PathString Root;
        public readonly string Name;
        public readonly string Path;

        public FileFilterDef(PathString root, string path, uint type = Regular)
            : base(type)
        {
            Root = root;
            
            if (String.IsNullOrEmpty(path))
            {
                Path = String.Empty;
                Name = String.Empty;
            }
            else
            {
                Path = PathString.MakeNormalized(path, PathString.PathParam.NormalizeOnly).Path;
                Name = System.IO.Path.GetFileName(path);
            }
        }
    }

    public class FileFilterDefEqualityComparer : IEqualityComparer<FileFilterDef>
    {
        public bool Equals(FileFilterDef x, FileFilterDef y)
        {
            return x.Root.Path == y.Root.Path;
        }

        public int GetHashCode(FileFilterDef obj)
        {
            return obj.Root.Path.GetHashCode();
        }
    }

}
