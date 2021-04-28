// Originally based on NAnt - A .NET build tool
// Copyright (C) 2003-2018 Electronic Arts Inc.
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
// Electronic Arts (Frostbite.Team.CM@ea.com)

using System;
using System.Text;

namespace NAnt.Core.Util
{
	public static class ConvertUtil
	{
		public static bool ToBoolean(string value, Location location = null, Action<string> onError = null)
		{
			if (!Boolean.TryParse(value, out bool result))
			{
				onError(value);
				ConversionError(value, typeof(Boolean), location); // we typically expect onError (if not null) to throw custom exception, but in case it doesn't throw here instead
			}

			return result;
		}

		public static bool? ToNullableBoolean(string value, Location location = null, Action<string> onError = null)
		{
			if (String.IsNullOrEmpty(value))
			{
				return null;
			}

			return ToBoolean(value, location, onError);
		}


		public static bool ToBooleanOrDefault(string value, bool defaultValue, Location location = null, Action<string> onError = null)
		{
			if (String.IsNullOrEmpty(value))
			{
				return defaultValue;
			}

			return ToBoolean(value, location, onError);
		}

		public static T ToEnum<T>(string value) where T : struct
		{
			if (!Enum.TryParse<T>(value, true, out T result))
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
			if (value == null || !Int32.TryParse(value, out int parsed)) // DAVE-FUTURE-REFACTOR-TODO - why no ConversionError here?
			{
				return null;
			}
			return parsed;
		}

		private static void ConversionError(string value, Type type, Location location = null)
		{
			throw new BuildException($"Failed to convert '{value}' to '{type.Name}' {location ?? Location.UnknownLocation}.");
		}
	}
	
}
