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
using NAnt.Core;

namespace EA.Eaconfig
{
	public class DAGPath<T>
	{
		public readonly T Value;

		public bool Visited = false;
		public bool Completed = false;
		public bool IsInPath = false;

		public readonly List<DAGPath<T>> Dependencies = new List<DAGPath<T>>();

		public DAGPath(T value)
		{
			Value = value;
		}

		public static void FindAncestors(IEnumerable<DAGPath<T>> dag, Func<T, bool> compare, Action<DAGPath<T>, DAGPath<T>> onCircular = null)
		{
			foreach (var n in dag)
			{
				n.Visited = false;
				n.Completed = false;
				n.IsInPath = false;
			}

			foreach (var n in dag)
			{
				if (!n.Completed)
				{
					Visit(n, compare, onCircular);
				}
			}
		}

		private static bool Visit(DAGPath<T> n,  Func<T, bool> compare, Action<DAGPath<T>, DAGPath<T>> onCircular)
		{
			if (n.Visited)
			{
				return n.Completed;
			}
			n.Visited = true;
			foreach (var d in n.Dependencies)
			{
				if (compare(d.Value))
				{
					n.IsInPath = true;
				}
				else
				{
					if (!Visit(d, compare, onCircular))
					{
						if (onCircular != null)
						{
							onCircular(n, d);
						}
						else
						{
							throw new BuildException("Circular dependency detected between '" + n.ToString() + "' and '" + d.ToString() + "'");
						}
					}
					if(d.IsInPath)
					{
						n.IsInPath = true;
					}
				}
			}
			n.Completed = true;
			return true;
		}
	}



}
