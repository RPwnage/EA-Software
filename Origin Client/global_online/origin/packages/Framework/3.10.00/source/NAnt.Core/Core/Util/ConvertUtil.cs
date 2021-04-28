using System;
using System.Xml;
using System.Text;
using NAnt.Core;

namespace NAnt.Core.Util
{    
    public static class ConvertUtil
    {
        public static bool? ToNullableBoolean(string value, XmlNode node = null)
        {
            if (String.IsNullOrEmpty(value))
            {
                return null;
            }
            bool result;

            if (!Boolean.TryParse(value, out result))
            {
                Error(value, typeof(Boolean), node);
            }

            return result;
        }

        public static bool? ToNullableBoolean(string value, Action<string> onerror)
        {
            if (String.IsNullOrEmpty(value))
            {
                return null;
            }
            bool result;

            if (!Boolean.TryParse(value, out result))
            {
                onerror(value);
            }

            return result;
        }


        public static bool ToBoolean(string value, XmlNode node = null)
        {
            bool result = false;

            if (!Boolean.TryParse(value, out result))
            {
                Error(value, typeof(Boolean), node);
            }

            return result;
        }

        public static bool ToBooleanOrDefault(string value, bool default_val, XmlNode node = null)
        {
            bool result = default_val;

            if (!String.IsNullOrEmpty(value))
            {
                result = ToBoolean(value, node);
            }
            return result;
        }



        public static T ToEnum<T>(string value) where T : struct
        {
            T result;
            if (!Enum.TryParse<T>(value, true, out result))
            {
                // catch all type conversion exceptions here, and rethrow with a friendlier message
                StringBuilder sb = new StringBuilder("Invalid value '" + value + "'. Valid values for are: ");
                Array array = Enum.GetValues(typeof(T));
                for (int i = 0; i < array.Length; i++)
                {
                    sb.Append(array.GetValue(i).ToString());
                    if (i <= array.Length - 1)
                    {
                        sb.Append(", ");
                    }
                }
                throw new BuildException(sb.ToString());

            }
            return result;
        }

        public static int? ToNullableInt(string value)
        {
            int parsed = 0;
            if (value == null || !Int32.TryParse(value, out parsed))
            {
                return null;
            }
            return parsed;
        }

        public static T ValueOrDefault<T>(T val, T def) 
        {
            return (val == null) ? def : val;
        }

        private static void Error(string value, Type type, XmlNode node)
        {
            Location loc = Location.UnknownLocation;
            if (node != null)
            {
                loc = Location.GetLocationFromNode(node);
            }
            throw new BuildException(String.Format("Failed to convert '{0}' to '{1}' {2}", value, type.Name, loc == null ? Location.UnknownLocation: loc));
        }
    }
    
}
