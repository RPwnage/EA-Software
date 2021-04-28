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

namespace EA.Eaconfig.Structured
{
	/// <summary>Precompiled headers input.   NOTE: To use this option, you still need to list a source file and specify that file with 'create-precompiled-header' optionset.</summary>
	[ElementName("pch", StrictAttributeCheck = true)]
	public class PrecompiledHeadersElement : ConditionalElement
	{
		public PrecompiledHeadersElement()
		{
			_enable = null;
		}

		/// <summary>enable/disable using precompiled headers.</summary>
		[TaskAttribute("enable")]
		public bool Enable
		{
			get { return _enable??false; }
			set { _enable = value; }
		} internal bool? _enable;

		/// <summary>Name of output precompiled header (Note that some platform's VSI don't provide this setting.  So this value may not get used!</summary>
		[TaskAttribute("pchfile", Required = false)]
		public string PchFile { get; set; }

		/// <summary>Name of the precompiled header file</summary>
		[TaskAttribute("pchheader", Required = false)]
		public string PchHeaderFile { get; set; }

		/// <summary>Directory to the precompiled header file</summary>
		[TaskAttribute("pchheaderdir", Required = false)]
		public string PchHeaderFileDir { get; set; }

		/// <summary>
		/// Use forced include command line switches to set up include to the header file so that people don't need to modify
		/// all source files to do #include "pch.h"
		/// </summary>
		[TaskAttribute("use-forced-include", Required = false)]
		public bool UseForcedInclude { get; set; }
	}

	/// <summary>
	/// Shared Precompiled Header module input.
	/// </summary>
	[ElementName("usesharedpch", StrictAttributeCheck = true)]
	public class UseSharedPchElement : ConditionalElement
	{
		public UseSharedPchElement() : base()
		{
		}

		/// <summary>The shared pch module (use the form [package_name]/[build_group]/[module_name]).  The build_group field can be ignored if it is runtime.</summary>
		[TaskAttribute("module", Required = true)]
		public string SharedModule { get; set; } = null;

		/// <summary>For platforms that support using common shared pch binary, this attribute allow module level control of using the shared pch binary. (default = true)</summary>
		[TaskAttribute("use-shared-binary", Required = false)]
		public bool UseSharedBinary { get; set; } = true;
	}

}
