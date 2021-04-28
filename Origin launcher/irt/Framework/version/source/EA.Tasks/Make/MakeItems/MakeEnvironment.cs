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

using NAnt.Core.Writers;


namespace EA.Make.MakeItems
{
	public class MakeEnvironment : MakeItem
	{
		private readonly List<string> export = new List<string>();
		private readonly List<string> unexport = new List<string>();

		public MakeEnvironment()
			: base(String.Empty)
		{
		}

		public void Export(string name, string value)
		{
			// Trailing slashes need to be escaped or else they will be treated as line continuation characters.
			if (value.EndsWith("\\"))
			{
				// to escape the trailing slashes we pass them to a function that does nothing but return the original value.
				export.Add(String.Format("{0}:=$(subst ,,{1})", name, value));
			}
			else
			{
				export.Add(String.Format("{0}:={1}", name, value));
			}
		}

		public void UnExport(string name)
		{
			unexport.Add(name);
		}

		public override void Write(MakeWriter writer)
		{
			if(export.Count + unexport.Count > 0)
			{
				export.Sort();
				foreach(var exp in export)
				{ 
					writer.WriteLine("export {0}", exp);
				}

				unexport.Sort();
				foreach (var unexp in unexport)
				{
					writer.WriteLine("unexport {0}", unexp);
				}
				writer.WriteLine();
			}
		}

	}
}
