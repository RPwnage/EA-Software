using System;
using System.Collections.Generic;
using System.Linq;


namespace NAnt.Core
{
    public static class PropertyExtensions
    {

        public static string GetPropertyOrDefault(this PropertyDictionary properties, string name, string defaultValue)
        {
            string val = properties[name];

            if (val == null)
            {
                val = defaultValue;
            }
            return val;
        }

        public static string GetPropertyOrDefault(this Project project, string name, string defaultValue)
        {
            string val = project.Properties[name];

            if (val == null)
            {
                val = defaultValue;
            }
            return val;
        }


        public static string GetPropertyOrFail(this PropertyDictionary properties, string name)
        {
            var val = properties[name];

            if (val == null)
            {
                throw new BuildException(String.Format("Property '{0}' is undefined.", name));
            }
            return val;
        }


        public static string GetPropertyOrFail(this Project project, string name)
        {
            var val = project.Properties[name];

            if (val == null)
            {
                throw new BuildException(String.Format("Property '{0}' is undefined.", name));
            }
            return val;
        }

        public static bool GetBooleanProperty(this PropertyDictionary properties, string name)
        {
            bool ret = false;
            string val = properties[name];
            if (!String.IsNullOrEmpty(val))
            {
                if (val.Trim().Equals("true", StringComparison.OrdinalIgnoreCase))
                {
                    ret = true;
                }
            }
            return ret;
        }

        public static bool GetBooleanPropertyOrDefault(this PropertyDictionary properties, string name, bool defaultVal)
        {
            bool ret = defaultVal;
            string val = properties[name];
            if (!String.IsNullOrEmpty(val))
            {
                if (val.Trim().Equals("true", StringComparison.OrdinalIgnoreCase))
                {
                    ret = true;
                }
                else if (val.Trim().Equals("false", StringComparison.OrdinalIgnoreCase))
                {
                    ret = false;
                }
            }
            return ret;
        }

    }
}


