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
using System.Text;
using NAnt.Core;

namespace EA.Eaconfig
{
	public class DAGNode<T>
{
	public readonly T Value;

	public bool Visited = false;
	public bool Completed = false;

	public readonly List<DAGNode<T>> Dependencies = new List<DAGNode<T>>();

	public DAGNode(T value)
	{
		Value = value;
	}

	/// <summary>
	/// Returns a DAG flattened into a list of the order visited.
	/// </summary>
	/// <param name="dag">The directed acyclic graph</param>
	/// <param name="onCircular">A callback that is passed begin/end nodes of a cycle plus the complete cycle in a stack, of </param>
	/// <returns></returns>
	public static List<DAGNode<T>> Sort(IEnumerable<DAGNode<T>> dag, Action<DAGNode<T>, DAGNode<T>, LinkedList<DAGNode<T>>> onCircular = null )
	{
		var result = new List<DAGNode<T>>();
		foreach (var node in dag)
		 {
			 node.Visited = false;
			 node.Completed = false;
		 }

		foreach (var node in dag)
		 {
			 if (!node.Completed)
			 {
				 LinkedList<DAGNode<T>> path = new LinkedList<DAGNode<T>>();

				 path.AddLast(node);
				 Visit(node, result, onCircular, path);
				 path.RemoveLast();
			 }
		 }
		 return result;
	}

	public static StringBuilder AppendPathString(StringBuilder builder, LinkedList<DAGNode<T>> path)
	{
		foreach (var n in path)
		{
			builder.AppendFormat("    -> ");
			builder.Append(n.Value.ToString());
			builder.AppendLine();
		}

		return builder;
	}

	private static bool Visit(DAGNode<T> node, List<DAGNode<T>> result, Action<DAGNode<T>, DAGNode<T>, LinkedList<DAGNode<T>>> onCircular, LinkedList<DAGNode<T>> path)
	{
		if (node.Visited)
		{
			return node.Completed;
		}

		node.Visited = true;

		foreach (var dependency in node.Dependencies)
		{
			path.AddLast(dependency);

			if (!Visit(dependency, result, onCircular, path))
			{
				if (onCircular != null)
				{
					onCircular(node, dependency, path);
				}
				else
				{
					StringBuilder exceptionBuilder = new StringBuilder();

					exceptionBuilder.AppendLine("Circular dependency detected!").AppendLine();
					exceptionBuilder.AppendLine("Path:");
					AppendPathString(exceptionBuilder, path);

					throw new BuildException(exceptionBuilder.ToString());
				}
			}

			path.RemoveLast();
		}

		node.Completed = true;
		result.Add(node);

		return true;
	}
}



}
