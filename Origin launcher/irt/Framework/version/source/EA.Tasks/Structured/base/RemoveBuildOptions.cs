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

using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Structured;

namespace EA.Eaconfig.Structured
{
	/// <summary>Sets options to be removed from final configuration</summary>
	[ElementName("remove", StrictAttributeCheck = true, NestedElementsCheck = true)]
	public class RemoveBuildOptions : ConditionalElementContainer
	{
		/// <summary>Defines to be removed</summary>
		[Property("defines", Required = false)]
		public ConditionalPropertyElement Defines { get; } = new ConditionalPropertyElement();

		/// <summary>C/CPP compiler options to be removed</summary>
		[Property("cc.options", Required = false)]
		public ConditionalPropertyElement CcOptions { get; } = new ConditionalPropertyElement();

		/// <summary>Assembly compiler options to be removed</summary>
		[Property("as.options", Required = false)]
		public ConditionalPropertyElement AsmOptions { get; } = new ConditionalPropertyElement();

		/// <summary>Linker options to be removed</summary>
		[Property("link.options", Required = false)]
		public ConditionalPropertyElement LinkOptions { get; } = new ConditionalPropertyElement();

		/// <summary>librarian options to be removed</summary>
		[Property("lib.options", Required = false)]
		public ConditionalPropertyElement LibOptions { get; } = new ConditionalPropertyElement();
	}

}
