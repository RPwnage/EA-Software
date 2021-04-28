// Originally based on NAnt - A .NET build tool
// Copyright (C) 2003-2015 Electronic Arts Inc.
// Copyright (C) 2001-2003 Gerry Shaw
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
// 
// As a special exception, the copyright holders of this software give you 
// permission to link the assemblies with independent modules to produce 
// new assemblies, regardless of the license terms of these independent 
// modules, and to copy and distribute the resulting assemblies under terms 
// of your choice, provided that you also meet, for each linked independent 
// module, the terms and conditions of the license of that module. An 
// independent module is a module which is not derived from or based 
// on these assemblies. If you modify this software, you may extend 
// this exception to your version of the software, but you are not 
// obligated to do so. If you do not wish to do so, delete this exception 
// statement from your version. 
// 
// 
// 
// Gerry Shaw (gerry_shaw@yahoo.com)
// Ian MacLean (ian_maclean@another.com)
// Electronic Arts (Frostbite.Team.CM@ea.com)

using System;
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
			protected Type m_type = null;

			public DefaultConverter(Type type)
			{
				m_type = type;
			}

			public object Convert(string value)
			{
				return System.Convert.ChangeType(value, m_type);
			}
		}

		protected class StringConverter : IPropertyConverter
		{
			public object Convert(string value)
			{
				return value;
			}
		};

		protected class BoolConverter : IPropertyConverter
		{
			public object Convert(string value)
			{
				return Expression.Evaluate(value);
			}
		}

		protected class NullableBoolConverter : IPropertyConverter
		{
			public object Convert(string value)
			{
				return (bool?)Expression.Evaluate(value);
			}
		}

		protected class EnumConverter : IPropertyConverter
		{
			private Type m_type;

			public EnumConverter(Type type)
			{
				m_type = type;
			}

			public object Convert(string value)
			{
				try
				{
					return Enum.Parse(m_type, value, true);
				}
				catch (Exception)
				{
					// catch all type conversion exceptions here, and rethrow with a friendlier message
					StringBuilder sb = new StringBuilder("Invalid value '" + value + "'. Valid values for this attribute are: ");
					Array array = Enum.GetValues(m_type);
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
		}

		private static IPropertyConverter s_stringConverter = new StringConverter();
		private static IPropertyConverter s_boolConverter = new BoolConverter();
		private static IPropertyConverter s_nullableBoolCovnerter = new BoolConverter();

		public static IPropertyConverter Create(Type type)
		{
			if (type == typeof(String))
			{
				return s_stringConverter;
			}
			if (type == typeof(Boolean))
			{
				return s_boolConverter;
			}
			if (type == typeof(Nullable<Boolean>))
			{
				return s_nullableBoolCovnerter;
			}
			else if (type == typeof(Enum) || type.IsSubclassOf(typeof(Enum)))
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
	}
}
