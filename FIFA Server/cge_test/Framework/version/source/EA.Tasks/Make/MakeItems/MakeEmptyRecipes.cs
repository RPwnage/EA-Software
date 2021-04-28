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
using System.IO;
using System.Collections.Generic;
using System.Linq;

using NAnt.Core.Util;
using NAnt.Core.Writers;


namespace EA.Make.MakeItems
{
	public class MakeEmptyRecipes : MakeItem
	{
		public readonly HashSet<string> Suffixes;

		public MakeEmptyRecipes()
			: base(String.Empty)
		{
			Suffixes = new HashSet<string>();
		}

		public void AddSuffixRecipy(PathString path)
		{
			if(path != null)
			{
				AddSuffixRecipy(path.Path);
			}
		}

		public void AddSuffixRecipy(string path)
		{
			if(!String.IsNullOrEmpty(path))
			{
				try
				{
					Suffixes.Add(Path.GetExtension(path));
				}
				catch(Exception)
				{ }
			}
		}
		public void AddSuffixRecipy(IEnumerable<string> paths)
		{
			if (paths != null)
			{
				foreach (var p in paths)
				{
					AddSuffixRecipy(p);
				}
			}
		}

		public void AddSuffixRecipy(IEnumerable<PathString> paths)
		{
			if (paths != null)
			{
				foreach (var p in paths)
				{
					AddSuffixRecipy(p);
				}
			}
		}


		public override void Write(MakeWriter writer)
		{
			foreach (var suffix in Suffixes.OrderBy(s => s, StringComparer.OrdinalIgnoreCase))
			{
				writer.WriteLine("%{0}: ;", suffix);
			}
		}
	}

}
