using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace NAnt.Core
{
    public static class FileSetExtensions
    {
        public static FileSet GetFileSet(this Project project, string name)
        {
            FileSet res = null;

            if (!String.IsNullOrEmpty(name))
            {
                project.NamedFileSets.TryGetValue(name, out res);
            }
            return res;
        }

        public static FileSet SafeClone(this FileSet fileSet)
        {
            if (fileSet != null)
            {
                return new FileSet(fileSet);
            }

            return new FileSet();
        }

        public static FileSet AppendIfNotNull(this FileSet fileSet, FileSet fromfileSet, string optionsetName = null)
        {
                if (fileSet != null && fromfileSet != null)
                {
                    if (optionsetName == null)
                    {
                        fileSet.Append(fromfileSet);
                    }
                    else
                    {
                        foreach (var inc in fromfileSet.Includes)
                        {
                            fileSet.Includes.Add(new FileSetItem(inc.Pattern, inc.OptionSet ?? optionsetName, inc.AsIs, inc.Force, inc.Data));
                        }
                        fileSet.Excludes.AddRange(fromfileSet.Excludes);
                    }
                }
            return fileSet;

        }


    }
}
