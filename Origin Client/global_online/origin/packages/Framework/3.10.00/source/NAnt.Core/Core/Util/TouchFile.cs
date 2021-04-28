using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;

namespace NAnt.Core.Util
{
    public static class Touch
    {
        public static void File(PathString path)
        {
            if (path != null && !String.IsNullOrEmpty(path.Path))
            {
                File(path.Path, DateTime.Now);
            }
        }

        public static void File(string path)
        {
            File(path, DateTime.Now);
        }

        public static void File(PathString path, DateTime touchDateTime)
        {
            if (path != null && !String.IsNullOrEmpty(path.Path))
            {
                File(path.Path, touchDateTime);
            }
        }

        public static void File(string path, DateTime touchDateTime)
        {
            if (!String.IsNullOrEmpty(path))
            {
                bool fileCreated = false;
                try
                {
                    // create any directories needed
                    Directory.CreateDirectory(Path.GetDirectoryName(path));

                    if (!System.IO.File.Exists(path))
                    {
                        using (FileStream f = System.IO.File.Create(path))
                        {
                            fileCreated = true;
                        }
                    }

                    // touch should set both write and access time stamp
                    System.IO.File.SetLastWriteTime(path, touchDateTime);
                    System.IO.File.SetLastAccessTime(path, touchDateTime);

                }
                catch (Exception e)
                {
                    if (fileCreated == true && System.IO.File.Exists(path))
                    {
                        System.IO.File.Delete(path);
                    }
                    string msg = String.Format("Could not touch file '{0}'.", path);
                    throw new BuildException(msg, e);
                }
            }
        }
    }
}
