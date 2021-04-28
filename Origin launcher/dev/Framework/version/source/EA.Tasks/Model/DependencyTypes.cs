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

using System.Text;

namespace EA.Eaconfig.Core
{ 
	public struct DependencyTypes
	{
		public const uint Undefined = 0;
		public const uint Build			= 1 << 1;
		public const uint Interface		= 1 << 2;
		public const uint Link			= 1 << 3;
		public const uint AutoBuildUse	= 1 << 4;
		public const uint CopyLocal		= 1 << 5;

		// TODO copylocal dependencies
		// -dvaliant 2016/04/18
		// copy local dependencies aren't actually a very useful concept in nant, originally I think they were meant so that
		// transitive relationships i.e A -> B -> C could be represented for .net projects because visual studio requires
		// A -> B and B -> C to be copy local in order for C's outputs to be copied to A. Now that we no longer rely on VS
		// to do these copies we only really need the mechansim where A declares itself as copy local and outputs from B
		// and C are automatically copied (without unnecessary copies from C to B)

		public static string ToString(uint type)
		{
			StringBuilder sb = new StringBuilder();

			test(type, Build, "build", sb);
			test(type, Interface, "interface", sb);
			test(type, Link, "link", sb);
			test(type, AutoBuildUse, "auto", sb);
			test(type, CopyLocal, "copylocal", sb);

			return sb.ToString().TrimEnd('|');
		}

		private static void test(uint t, uint mask, string name, StringBuilder sb)
		{
			if ((mask & t) != 0) sb.AppendFormat("{0}|", name);
		}
	}
}
