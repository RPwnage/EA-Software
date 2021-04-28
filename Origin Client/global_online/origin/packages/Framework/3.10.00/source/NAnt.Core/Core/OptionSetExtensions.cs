using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace NAnt.Core
{
    public static class OptionSetExtensions
    {
        public static bool GetBooleanOption(this OptionSet optionset, string optionname)
        {
            return optionset.GetBooleanOptionOrDefault(optionname, false);
        }

        public static bool GetBooleanOptionOrDefault(this OptionSet optionset, string optionname, bool defaultval)
        {
            bool result = defaultval;
            if (optionset != null)
            {
                string option = optionset.Options[optionname];
                if (!String.IsNullOrEmpty(option))
                {
                    result = option.Equals("on", StringComparison.OrdinalIgnoreCase) || option.Equals("true", StringComparison.OrdinalIgnoreCase);
                }

            }
            return result;
        }


        public static string GetOptionOrDefault(this OptionSet set, string optionName, string defaultVal)
        {
            if (set == null || optionName == null)
            {
                return defaultVal;
            }

            string val = set.Options[optionName];
            if (val == null)
            {
                val = defaultVal;
            }
            return val;
        }


    }
}
