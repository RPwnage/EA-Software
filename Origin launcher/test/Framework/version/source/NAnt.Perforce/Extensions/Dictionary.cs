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
using System.Linq;
using System.Collections.Generic;
using System.Collections;

namespace NAnt.Perforce.Extensions
{
	namespace Dictionary
	{
		public static class Extensions
		{
			public static TKey ReserveLookup<TKey, TValue>(this Dictionary<TKey, TValue> source, TValue val)
			{
				return source.First(kvp => kvp.Value.Equals(val)).Key;
			}

			public static TValue Pop<TKey, TValue>(this Dictionary<TKey, TValue> source, TKey key)
			{
				TValue val = source[key];
				source.Remove(key);
				return val;
			}

			public static Dictionary<TKey, TValue> Update<TKey, TValue>(this Dictionary<TKey, TValue> source, Dictionary<TKey, TValue> dict)
			{
				Dictionary<TKey, TValue> updated = new Dictionary<TKey, TValue>(source);
				dict.ToList().ForEach(x => updated[x.Key] = x.Value);
				return updated;
			}

			public static ReadOnlyDictionary<TKey, TValue> AsReadOnly<TKey, TValue>(this IDictionary<TKey, TValue> dictionary)
			{
				return new ReadOnlyDictionary<TKey, TValue>(dictionary);
			}
		}

		public class ReadOnlyDictionary<TKey, TValue> : IDictionary<TKey, TValue>
		{
			private IDictionary<TKey, TValue> _InternalDictionary;

			public ReadOnlyDictionary()
			{
				_InternalDictionary = new Dictionary<TKey, TValue>();
			}

			public ReadOnlyDictionary(IDictionary<TKey, TValue> dictionary)
			{
				_InternalDictionary = dictionary;
			}

			void IDictionary<TKey, TValue>.Add(TKey key, TValue value)
			{
				throw ReadOnlyException();
			}

			public bool ContainsKey(TKey key)
			{
				return _InternalDictionary.ContainsKey(key);
			}

			public ICollection<TKey> Keys
			{
				get { return _InternalDictionary.Keys; }
			}

			bool IDictionary<TKey, TValue>.Remove(TKey key)
			{
				throw ReadOnlyException();
			}

			public bool TryGetValue(TKey key, out TValue value)
			{
				return _InternalDictionary.TryGetValue(key, out value);
			}

			public ICollection<TValue> Values
			{
				get { return _InternalDictionary.Values; }
			}

			public TValue this[TKey key]
			{
				get
				{
					return _InternalDictionary[key];
				}
			}

			TValue IDictionary<TKey, TValue>.this[TKey key]
			{
				get
				{
					return this[key];
				}
				set
				{
					throw ReadOnlyException();
				}
			}

			void ICollection<KeyValuePair<TKey, TValue>>.Add(KeyValuePair<TKey, TValue> item)
			{
				throw ReadOnlyException();
			}

			void ICollection<KeyValuePair<TKey, TValue>>.Clear()
			{
				throw ReadOnlyException();
			}

			public bool Contains(KeyValuePair<TKey, TValue> item)
			{
				return _InternalDictionary.Contains(item);
			}

			public void CopyTo(KeyValuePair<TKey, TValue>[] array, int arrayIndex)
			{
				_InternalDictionary.CopyTo(array, arrayIndex);
			}

			public int Count
			{
				get { return _InternalDictionary.Count; }
			}

			public bool IsReadOnly
			{
				get { return true; }
			}

			bool ICollection<KeyValuePair<TKey, TValue>>.Remove(KeyValuePair<TKey, TValue> item)
			{
				throw ReadOnlyException();
			}

			public IEnumerator<KeyValuePair<TKey, TValue>> GetEnumerator()
			{
				return _InternalDictionary.GetEnumerator();
			}

			IEnumerator System.Collections.IEnumerable.GetEnumerator()
			{
				return GetEnumerator();
			}

			private static System.Exception ReadOnlyException()
			{
				return new NotSupportedException("This dictionary is read-only");
			}
		}

		public class ReadThroughCacheDictionary<TKey, TValue>
		{
			private readonly Func<TKey, TValue> ReadThroughAction;
			private Dictionary<TKey, TValue> Cache = new Dictionary<TKey, TValue>();

			public ReadThroughCacheDictionary(Func<TKey, TValue> readThroughAction)
			{
				ReadThroughAction = readThroughAction;
			}

			public TValue this[TKey key]
			{
				get
				{
					TValue realVal;
					if (Cache.TryGetValue(key, out realVal))
					{
						return realVal;
					}
					realVal = ReadThroughAction(key);
					Cache[key] = realVal;
					return realVal;
				}
			}

			public void Clear()
			{
				Cache.Clear();
			}
		}
	}
}
