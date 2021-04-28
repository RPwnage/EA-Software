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
using System.Collections;
using System.Collections.Generic;
using System.Linq;

namespace EA.Eaconfig
{

	public class OrderedDictionary<TKey, TValue> : IDictionary<TKey, TValue>
	{
		private readonly List<KeyValuePair<TKey, TValue>> _values = new List<KeyValuePair<TKey, TValue>>();

		public OrderedDictionary()
		{
		}

		public TValue this[TKey key]
		{
			get
			{
				int ind = _values.FindIndex(e => e.Key.Equals(key));
				if (ind < 0)
				{
					throw new ArgumentException(String.Format("Key is not present in the dictionary: {0}", key));
				}
				return _values[ind].Value;
			}
			set
			{
				int ind = _values.FindIndex(e => e.Key.Equals(key));
				if (ind < 0)
				{
					_values.Add(new KeyValuePair<TKey, TValue>(key, value));
				}
				else
				{
					_values[ind] = new KeyValuePair<TKey, TValue>(key, value);
				}
			}
		}

		public int Count
		{
			get { return _values.Count; }
		}

		public ICollection<TKey> Keys
		{
			get
			{
				return _values.Select(e => e.Key).ToList();
			}
		}

		public ICollection<TValue> Values
		{
			get
			{
				return _values.Select(e => e.Value).ToList();
			}
		}

		public IEqualityComparer<TKey> Comparer
		{
			get;
			private set;
		}

		public void Add(KeyValuePair<TKey, TValue> entry)
		{
			Add(entry.Key, entry.Value);
		}

		public void Add(TKey key, TValue value)
		{
			int ind = _values.FindIndex(e => e.Key.Equals(key));
			if (ind < 0)
			{
				_values.Add(new KeyValuePair<TKey, TValue>(key, value));
			}
			else
			{
				throw new ArgumentException(String.Format("Key is already present in the dictionary: {0}", key));
			}
		}

		public void Clear()
		{
			_values.Clear();
		}

		public void Insert(int index, TKey key, TValue value)
		{
			_values[index] = new KeyValuePair<TKey, TValue>(key, value);
		}

		public bool Contains(KeyValuePair<TKey, TValue> entry)
		{
			return ContainsKey(entry.Key);
		}


		public bool ContainsValue(TValue value)
		{
			return this.Values.Contains(value);
		}

		public bool ContainsValue(TValue value, IEqualityComparer<TValue> comparer)
		{
			return this.Values.Contains(value, comparer);
		}

		public bool ContainsKey(TKey key)
		{
			return -1 != _values.FindIndex(e => e.Key.Equals(key));
		}

		public IEnumerator<KeyValuePair<TKey, TValue>> GetEnumerator()
		{
			return _values.GetEnumerator();
		}

		IEnumerator IEnumerable.GetEnumerator()
		{
			return GetEnumerator();
		} 

		public bool Remove(TKey key)
		{
			var ret = false;
			int ind = _values.FindIndex(e => e.Key.Equals(key));
			if (ind > -1)
			{
				_values.RemoveAt(ind);
				ret = true;
			}
			return ret;
		}

		public bool Remove(KeyValuePair<TKey, TValue> entry)
		{
			var ret = false;
			int ind = _values.FindIndex(e => e.Key.Equals(entry.Key));
			if (ind > -1)
			{
				_values.RemoveAt(ind);
				ret = true;
			}
			return ret;
		}



		public bool TryGetValue(TKey key, out TValue value)
		{
			bool ret = false;
			int ind = _values.FindIndex(e => e.Key.Equals(key));
			if (ind > -1)
			{
				value = _values[ind].Value;
				ret = true;
			}
			else
			{
				value = default(TValue);
			}
			return ret;
		}

		public bool IsReadOnly
		{
			get { return false; }
		}

		public void CopyTo(KeyValuePair<TKey, TValue>[] array, int arrayIndex)
		{
			
		} 
   }
}