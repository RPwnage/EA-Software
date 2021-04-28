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

using NAnt.Core.Attributes;

using System;

namespace EA.Eaconfig.Structured
{
	/// <summary></summary>
	[ElementName("resourcefiles", StrictAttributeCheck = true)]
	public class ResourceFilesElement : StructuredFileSet
	{
		public ResourceFilesElement() : base(){
		}

		public ResourceFilesElement(ResourceFilesElement source)
			: base(source)
		{
			Prefix = source.Prefix;
			ResourceBasedir = source.ResourceBasedir;
			ResourceIncludeDirs = source.ResourceIncludeDirs;
			ResourceDefines = source.ResourceDefines;
		}

		/// <summary>Indicates the prefix to prepend to the actual resource.  This is usually the 
		/// default namespace of the assembly.</summary>
		[TaskAttribute("prefix")]
		public string Prefix { get; set; } = String.Empty;

		/// <summary></summary>
		[TaskAttribute("resourcebasedir")]
		public string ResourceBasedir { get; set; }

		/// <summary>Additional include directories to pass to the resource compiler</summary>
		[TaskAttribute("includedirs", Required = false)]
		public string ResourceIncludeDirs { get; set; }

		/// <summary>Additional defines to pass to the resource compiler</summary>
		[TaskAttribute("defines", Required = false)]
		public string ResourceDefines { get; set; }
	}
}
