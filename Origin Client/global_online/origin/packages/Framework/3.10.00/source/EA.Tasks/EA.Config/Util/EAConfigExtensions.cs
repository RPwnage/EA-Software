using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

using NAnt.Core;

namespace EA.Eaconfig
{
    public static class EAConfigExtensions
    {
        public static string UnescapeDefine(this string def)
        {
            // When defines are passed through command line quotes are  stripped for us by cmd line processor.
            // In response file we need them stripped.
            int ind = def.IndexOf('=');
            if (ind != -1 && ind < def.Length - 1)
            {
                string def_val = def.Substring(ind + 1);
                if (!String.IsNullOrEmpty(def_val) && def_val.Length > 1)
                {
                    // Trim one quote:
                    if ((def_val[0] == '"' || def_val[0] == '\'') && def_val[0] == def_val[def_val.Length - 1])
                    {
                        def_val = def_val.Substring(1, def_val.Length - 2);
                    }
                    // Unescape def_val
                    if (def_val.Length > 3 && ((def_val[0] == '\\') && (def_val[0] == def_val[def_val.Length - 2])))
                    {
                        def_val = def_val.Substring(1, def_val.Length - 3) + def_val[def_val.Length - 1];
                    }

                    def_val = def.Substring(0, ind + 1) + def_val;
                }
                else
                {
                    def_val = def;
                }

                if (!String.IsNullOrEmpty(def_val))
                {
                    def = def_val;
                }
            }
            return def;

        }

        public static IEnumerable<string> UnescapeDefines(this IEnumerable<string> defines)
        {
            var cleandefines = new List<string>();
            foreach (string def in defines)
            {
                if (String.IsNullOrEmpty(def))
                {
                    continue;
                }
                string def_val = def.TrimWhiteSpace();

                if (String.IsNullOrEmpty(def_val))
                {
                    continue;
                }

                def_val = UnescapeDefine(def_val);

                cleandefines.Add(def_val);
            }
            return cleandefines;
        }

        public static IDictionary<string, string> BuildEnvironment(this Project project)
        {
            if (project != null)
            {
                var optionset = project.NamedOptionSets["build.env"];
                return optionset == null ? null : optionset.Options;
            }
            return null;
        }

        public static IDictionary<string, string> ExecEnvironment(this Project project)
        {
            if (project != null)
            {
                var optionset = project.NamedOptionSets["exec.env"];
                return optionset == null ? null : optionset.Options;
            }
            return null;
        }


        public static void AddNonEmpty(this IDictionary<string, string> data, string name, string value)
        {
            if (data != null && !String.IsNullOrWhiteSpace(value))
            {
                data.Add(name, value);
            }
        }

        public static void Add(this IDictionary<string, string> data, string name, bool value)
        {
            if (data != null)
            {
                data.Add(name, value.ToString().ToLowerInvariant());
            }
        }
    }
}
