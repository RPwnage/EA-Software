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

namespace NAnt.Perforce.Extensions
{
	namespace IEnumerable
	{
		public static class Extensions
		{
			public static IEnumerable<IEnumerable<T>> Batch<T>(this IEnumerable<T> items, uint maxItems)
			{
				return items.Select((item, inx) => new { item, inx }).GroupBy(x => x.inx / maxItems).Select(g => g.Select(x => x.item));
			}

			public static IEnumerable<T> Distinct<T>(this IEnumerable<T> items, Func<T, T, bool> comparer)
			{
				List<T> returned = new List<T>();
				foreach (T item in items)
				{
					if (!returned.Any(r => comparer(item, r)))
					{
						returned.Add(item);
						yield return item;
					}
				}
				yield break;
			}

			public static IEnumerable<T> DistinctBy<T, K>(this IEnumerable<T> items, Func<T, K> keySelector)
			{
				return items.GroupBy(keySelector).Select(g => g.First());
			}
		}
	}
}
