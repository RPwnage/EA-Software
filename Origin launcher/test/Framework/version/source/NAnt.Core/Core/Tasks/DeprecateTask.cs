// NAnt - A .NET build tool
// Copyright (C) 2003-2015 Electronic Arts Inc.
// Copyright (C) 2001-2003 Gerry Shaw
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

using NAnt.Core.Attributes;
using NAnt.Core.Logging;

namespace NAnt.Core.Tasks
{

	/// <summary>Display a deprecation message in the current build.</summary>
	/// <remarks>
	///   <para>Displays a deprecation message and location in the build file then continues with the build.</para>
	/// </remarks>
	/// <include file='Examples/Deprecate/Simple.example' path='example'/>
	[TaskName("deprecate", NestedElementsCheck = true)]
	public class DeprecateTask : Task
	{
		/// <summary>The deprecation message to display.</summary>
		[TaskAttribute("message")]
		public string Message { get; set; } = null;

		/// <summary>
		/// Categorizes the deprecation for the framework build messaging system. 
		/// Allows users to disable all deprecations in a category.
		/// (Designed to be used only by internal Framework packages for now)
		/// </summary>
		[TaskAttribute("group")]
		public Log.DeprecationId Group { get; set; } = Log.DeprecationId.NoId;

		protected override void ExecuteTask()
		{
			string message = Message ?? "No message.";
			if (Group == Log.DeprecationId.NoId)
			{
				Log.Warning.WriteLine("{0} {1}", Location.ToString(), message);
			}
			else
			{
				Log.ThrowDeprecation(Group, Log.DeprecateLevel.Normal, "{0} {1}", Location.ToString(), message);
			}
		}
	}
}
