// NAnt - A .NET build tool
// Copyright (C) 2003-2015 Electronic Arts Inc.
// Copyright (C) 2001 Gerry Shaw
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
// 
// 
// Electronic Arts (Frostbite.Team.CM@ea.com)
// Gerry Shaw (gerry_shaw@yahoo.com)
using System;

namespace NAnt.Core.Attributes
{

	/// <summary>Indicates that class should be treated as a task.</summary>
	/// <remarks>
	/// Attach this attribute to a subclass of Task to have NAnt be able
	/// to recognize it.  The name should be short but must not conflict
	/// with any other task already in use.
	/// </remarks>
	[AttributeUsage(AttributeTargets.Class, Inherited = false, AllowMultiple = false)]
	public sealed class TaskNameAttribute : Attribute
	{
		bool _NestedElementsCheck = false;

		public TaskNameAttribute(string name)
		{
			Name = name;
		}

		/// <summary>The short name of the task that appears on the XML tags in build files.</summary>
		public string Name { get; set; }

		public bool Override { get; set; } = false;

		/// <summary>This task XML node gets added to list of nestable elements. This bool property
		/// is equivalent to using XmlSchemaAttribute however using this bool is apparently faster
		/// than adding multiple attributes to a class</summary>
		public bool XmlSchema { get; set; } = true;

		/// <summary>Task XML node may contain combination of nested elements and plain text,
		/// rather than only nested elements.</summary>
		public bool Mixed { get; set; } = false;

		public bool StrictAttributeCheck { get; set; } = true;

		public bool NestedElementsCheck
		{
			get { return _NestedElementsCheck; }
			set { _NestedElementsCheck = value; }
		}

	}
}
