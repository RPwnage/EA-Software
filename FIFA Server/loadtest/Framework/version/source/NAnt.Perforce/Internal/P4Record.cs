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
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace NAnt.Perforce
{
	namespace Internal
	{
		internal class P4Record
		{
			private static char[] Digits = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9' };
			private Dictionary<string, string> SkipArrays = new Dictionary<string,string>() // these are array fields which may not be consecutive under certain cirumstances, and the default to use when they are missing
			{ 
				{ "filesize", "0" },
				{ "digest", "" },
				{ "fromfile", "" },
				{ "fromrev", "" }
			}; 

			private readonly Dictionary<string, string> _Fields = new Dictionary<string, string>();
			private readonly Dictionary<string, string[]> _ArrayFields = new Dictionary<string, string[]>();

			internal readonly string AllText = "";

			public P4Record(Dictionary<string, string> taggedOutput, string allText)
			{
				AllText = allText;
				string[] fieldKeys = taggedOutput.Keys.ToArray();
				HashSet<string> skipFields = new HashSet<string>();
				foreach (string keyString in fieldKeys)
				{
					string arrayBase = keyString.TrimEnd(Digits);
					bool isArrayField = arrayBase != keyString;
					if (isArrayField)
					{
						if (SkipArrays.Keys.Contains(arrayBase))
						{
							skipFields.Add(arrayBase);
						}
						else
						{
							AsArray(arrayBase, taggedOutput);
						}
					}
					else
					{
						_Fields.Add(keyString, taggedOutput[keyString]);
					}
				}

				if (skipFields.Count > 0)
				{
					int arraySize = _ArrayFields.Max(af => af.Value.Length); // skip array fields represent array fields that are optionally present
																			 // anywhere they occur we can use the length of array fields to know how many
																			 // "should" be present and fill in the missings with a sane default value
																			 // normally length of all array fields will be the same so we can safely use it
																			 // but there are a few stream related commands that return different length arrays
																			 // but these do not use skip arrays so we can still use this code safely
					foreach (string skipField in skipFields)
					{
						_ArrayFields[skipField] = new string[arraySize];
						for (int i = 0; i < arraySize; ++i)
						{
							string key = String.Format("{0}{1}", skipField, i);
							if (taggedOutput.ContainsKey(key))
							{
								_ArrayFields[skipField][i] = taggedOutput[key];
							}
							else
							{
								_ArrayFields[skipField][i] = SkipArrays[skipField];
							}
						}
					}
				}
			}

			internal P4Record()
			{
			}

			#region GetField
			internal string GetField(string field)
			{
				return _Fields[field];
			}

			internal string GetField(string field, string defaultVal)
			{
				return HasField(field) ? GetField(field) : defaultVal;
			}

			internal uint GetUint32Field(string field)
			{
				return UInt32.Parse(GetField(field));
			}

			internal uint GetUint32Field(string field, UInt32 defaultVal)
			{
				return HasField(field) ? GetUint32Field(field) : defaultVal;
			}

			internal DateTime GetDateField(string field)
			{
				return P4Utils.P4TimestampToDateTime(GetField(field));
			}

			internal DateTime GetDateField(string field, DateTime defaultVal)
			{
				return HasField(field) ? GetDateField(field) : defaultVal;
			}

			internal TEnum GetEnumField<TEnum>(string field)
			{
				return (TEnum)Enum.Parse(typeof(TEnum), GetField(field));
			}

			internal TEnum GetEnumField<TEnum>(string field, TEnum defaultVal)
			{
				return HasField(field) ? GetEnumField<TEnum>(field) : defaultVal;
			}
			#endregion

			internal bool HasField(string field)
			{
				return _Fields.ContainsKey(field);
			}

			#region SetField
			internal void SetField(string field, string value)
			{
				if (value != null)
				{
					_Fields[field] = value;
				}
				else if (HasField(field))
				{
					_Fields.Remove(field);
				}
			}

			internal void SetField(string field, uint value)
			{
				_Fields[field] = value.ToString();
			}
			#endregion

			internal string[] GetFieldNames()
			{
				return _Fields.Keys.ToArray();
			}

			internal string[] GetArrayField(string field)
			{
				return _ArrayFields[field];
			}

			internal string[] SafeGetArrayField(string field)
			{
				string[] arrayFieldValue;
				if (_ArrayFields.TryGetValue(field, out arrayFieldValue))
				{
					return arrayFieldValue;
				}
				return new string[] { };
			}

			internal string GetArrayFieldElement(string field, uint index)
			{
				return _ArrayFields[field][index];
			}

			internal bool HasArrayField(string field)
			{
				return _ArrayFields.ContainsKey(field);
			}

			internal void SetArrayField(string field, string[] value)
			{
				_ArrayFields[field] = value;
			}

			internal string[] GetArrayFieldNames()
			{
				return _ArrayFields.Keys.ToArray();
			}

			internal string AsForm(string[] fieldNameExclusions)
			{
				StringBuilder str = new StringBuilder();

				HashSet<string> exclusionsSet = new HashSet<string>(fieldNameExclusions);

				foreach (string fieldKey in GetFieldNames())
				{
					if (exclusionsSet.Contains(fieldKey))
					{
						continue;
					}

					str.AppendFormat("{0}: {1}\r\n\n", GetFullFormName(fieldKey), GetField(fieldKey));
				}

				foreach (string arraykey in GetArrayFieldNames())
				{
					if (exclusionsSet.Contains(arraykey))
					{
						continue;
					}

					if (GetArrayField(arraykey).Length > 0)
					{
						str.AppendFormat("{0}:\r\n", arraykey);
						foreach (string arrayValue in GetArrayField(arraykey))
						{
							str.AppendLine("   " + arrayValue);
						}
					}
				}
				return str.ToString();
			}

			// converts a p4 record based object like a client or changelist into the forn
			// spec used for standard input commands
			internal string AsForm()
			{
				return AsForm(new string[] { });
			}

			// may need to expand this to be more generic, so far only description seems to follow different naming rules on input vs outpot
			private string GetFullFormName(string fieldKey)
			{
				if (fieldKey == "desc")
				{
					return "description";
				}
				return fieldKey;
			}

			private void AsArray(string arrayBase, Dictionary<string, string> taggedOutput)
			{
				if (!_ArrayFields.ContainsKey(arrayBase))
				{
					List<string> list = new List<string>();
					for (int i = 0; ; ++i)
					{
						string key = string.Format("{0}{1}", arrayBase, i);
				 
						if (!taggedOutput.ContainsKey(key))
						{
							break;
						}
						else
						{
							list.Add(taggedOutput[key]);
						}
					}
					_ArrayFields.Add(arrayBase, list.ToArray());
				}     
			}

			private bool IsArrayField(string arrayBase, Dictionary<string, string> taggedOutput)
			{           
				if (taggedOutput.ContainsKey(arrayBase + "0"))
				{
					return true;
				}
				return false;
			}
		}
	}
}
