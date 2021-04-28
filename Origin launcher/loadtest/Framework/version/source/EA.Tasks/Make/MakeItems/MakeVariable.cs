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

using NAnt.Core;
using NAnt.Core.Writers;


namespace EA.Make.MakeItems
{
	public class MakeVariableScalar : MakeItem
	{
		public string Value;

		public MakeVariableScalar(string name, string value = null)
			: base(name)
		{
			Value = value;
		}

		public override void Write(MakeWriter writer)
		{
			writer.WriteLine("{0} = {1}", Label, Value);
			if(-1 != Value.IndexOfAny(new char[]{'\n','\r'}))
			{
				writer.WriteLine();
			}
		}
	}


	public class MakeVariableArray : MakeItem
	{
		public readonly List<string> Values = new List<string>();

		public MakeVariableArray(string name, IEnumerable<string> values = null)
			: base(name)
		{
			if (values != null)
			{
				Values.AddRange(values);
			}
		}

		public override void Write(MakeWriter writer)
		{
			if (Values.Count == 1)
			{
				writer.Write("{0} = {1}", Label, Values.ToString(" "));
			}
			else if(Values.Count > 1)
			{
				writer.Write("{0} = ", Label);
				foreach (var v in Values)
				{
					writer.WriteLine("\\");
					writer.Write(" {0}", v);
				}
				writer.WriteLine();
			}
			writer.WriteLine();
		}
	}

}
