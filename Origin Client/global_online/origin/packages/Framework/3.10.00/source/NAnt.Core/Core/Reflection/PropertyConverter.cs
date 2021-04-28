using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace NAnt.Core.Reflection
{
    public interface IPropertyConverter
    {
        object Convert(string value);
    }

    class PropertyConverter
    {

        protected class DefaultConverter : IPropertyConverter
        {
            protected Type _type = null;

            public DefaultConverter(Type type)
            {
                _type = type;
            }

            public object Convert(string value)
            {
                return System.Convert.ChangeType(value, _type);
            }
        }

        protected class StringConverter : IPropertyConverter
        {
            public StringConverter(Type type)
            {
            }

            public object Convert(string value)
            {
                return (value);
            }
        };

        /// <summary>
        /// Special case booleans to handle expressions
        /// </summary>
        protected class BoolConverter : IPropertyConverter
        {
            public BoolConverter(Type type)
            {
            }
            public object Convert(string value)
            {
                return Expression.Evaluate(value);
            }
        }

        protected class EnumConverter : IPropertyConverter
        {
            public EnumConverter(Type type)
            {
                mType = type;
            }

            public object Convert(string value)
            {
                try
                {
                    return Enum.Parse(mType, value, true);
                }
                catch (Exception)
                {
                    // catch all type conversion exceptions here, and rethrow with a friendlier message
                    StringBuilder sb = new StringBuilder("Invalid value '" + value + "'. Valid values for this attribute are: ");
                    Array array = Enum.GetValues(mType);
                    for (int i = 0; i < array.Length; i++)
                    {
                        sb.Append(array.GetValue(i).ToString());
                        if (i <= array.Length - 1)
                        {
                            sb.Append(", ");
                        }
                    }
                    throw new Exception(sb.ToString());
                }
            }

            private Type mType;
        }

        public static IPropertyConverter Create(Type type)
        {
            if (type == typeof(System.String))
            {
                return (sStringConverter);
            }
            if (type == typeof(System.Boolean))
            {
                return (sBoolConverter);
            }
            else if (type == typeof(System.Enum) || type.IsSubclassOf(typeof(System.Enum)))
            {
                return new EnumConverter(type);
            }
            else
            {
                return new DefaultConverter(type);
            }
        }

        public static object Convert(Type type, string value)
        {
            if (type == typeof(System.String))
            {
                return (value);
            }
            else if (type == typeof(System.Boolean))
            {
                return (Expression.Evaluate(value));
            }
            else if (type == typeof(System.Enum) || type.IsSubclassOf(typeof(System.Enum)))
            {
                return ((new EnumConverter(type)).Convert(value));
            }
            else
            {
                return (System.Convert.ChangeType(value, type));
            }
        }

        private static IPropertyConverter sStringConverter = new StringConverter(typeof(System.String));
        private static IPropertyConverter sBoolConverter = new BoolConverter(typeof(System.Boolean));
    }
}
