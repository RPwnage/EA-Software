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

using System.Collections.Generic;


namespace EA.Make
{
	
	public class MakeStringCollection : IList<string>, IEnumerable<string>
	{

		public void Add(string item)
		{
			_collection.Add(item);
		}

		public void AddRange(IEnumerable<string> c)
		{
			_collection.AddRange(c);
		}

		public void Clear()
		{
			_collection.Clear();
		}

		public int IndexOf(string item)
		{
			return _collection.IndexOf(item);
		}

		public void Insert(int index, string item)
		{
			_collection.Insert(index, item);
		}

		public void RemoveAt(int index)
		{
			_collection.RemoveAt(index);
		}

		public string this[int index]
		{
			get
			{
				return _collection[index];
			}
			set
			{
				_collection[index] = value;
			}
		}

		public bool Contains(string item)
		{
			return _collection.Contains(item);
		}

		public void CopyTo(string[] array, int arrayIndex)
		{
			_collection.CopyTo(array, arrayIndex);
		}

		public int Count { get { return _collection.Count; } }

		public bool IsReadOnly { get { return false; } }

		public bool Remove(string item)
		{

			return _collection.Remove(item);
		}

		public IEnumerator<string> GetEnumerator()
		{
			return _collection.GetEnumerator();
		}

		System.Collections.IEnumerator System.Collections.IEnumerable.GetEnumerator()
		{
			return _collection.GetEnumerator();
		}

		public static MakeStringCollection operator +(MakeStringCollection collection, string toadd)
		{
			collection.Add(toadd);
			return collection;
		}

		public static MakeStringCollection operator +(MakeStringCollection collection, IEnumerable<string> toadd)
		{
			collection.AddRange(toadd);
			return collection;
		}

		private readonly List<string> _collection = new List<string>();
	}

}
