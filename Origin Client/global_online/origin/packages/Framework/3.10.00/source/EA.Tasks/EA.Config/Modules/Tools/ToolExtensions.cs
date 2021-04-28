using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

using NAnt.Core;
using NAnt.Core.Util;
using EA.FrameworkTasks.Model;
using EA.Eaconfig.Core;


namespace EA.Eaconfig.Modules.Tools
{
    public static class ToolExtensions
    {
        private static readonly IEnumerable<char> DefaultPrefixes = new char[] { '/', '-' };

        public static string GetOptionValue(this Tool tool, string optionname, IEnumerable<char> prefixes =null)
        {
            foreach (var prefix in prefixes ?? DefaultPrefixes)
            {
                var option = prefix + optionname;
                string val = tool.Options.Find(op => op.StartsWith(option, StringComparison.OrdinalIgnoreCase));
                if (val != null)
                {
                    val = val.Substring(option.Length);
                }
                return val;
            }
            return String.Empty;
        }

        private static IDictionary<string, string> Environment(Project project)
        {
            var optionset = project.NamedOptionSets["build.env"];
            return optionset == null ? null : optionset.Options;
        }

    }
}
