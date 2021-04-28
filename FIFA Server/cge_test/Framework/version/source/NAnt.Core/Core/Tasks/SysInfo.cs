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
// File Maintainer:
// Electronic Arts (Frostbite.Team.CM@ea.com)
// Gerry Shaw (gerry_shaw@yahoo.com)

using NAnt.Core.Attributes;
using NAnt.Core.Logging;
using NAnt.Core.Util;

namespace NAnt.Core.Tasks
{
	/// <summary>(Deprecated) Set properties with system information.</summary>
	/// <remarks>
	///   <para>This task is Deprecated and no longer preforms any function. All of the properties that were setup by this task are now setup automatically for each project.</para>
	///   <para>Sets a number of properties with information about the system environment.  The intent of this task is for nightly build logs to have a record of the system information that the build was performed on.</para>
	///   <list type="table">
	///     <listheader><term>Property</term>      <description>Value</description></listheader>
	///     <item><term>sys.clr.version</term>     <description>Common Language Runtime version number.</description></item>
	///     <item><term>sys.env.*</term>           <description>Environment variables, stored both in upper case or their original case.(e.g., sys.env.Path or sys.env.PATH).</description></item>
	///     <item><term>sys.os.folder.system</term><description>The System directory.</description></item>
	///     <item><term>sys.os.folder.temp</term>  <description>The temporary directory.</description></item>
	///     <item><term>sys.os.platform</term>     <description>Operating system platform ID.</description></item>
	///     <item><term>sys.os.version</term>      <description>Operating system version.</description></item>
	///     <item><term>sys.os</term>              <description>Operating system version string.</description></item>
	///   </list>
	/// </remarks>
	/// <include file='Examples/SysInfo/DefaultPrefix.example' path='example'/>
	/// <include file='Examples/SysInfo/NoPrefix.example' path='example'/>
	/// <include file='Examples/SysInfo/Verbose.example' path='example'/>
	/// <include file='Examples/SysInfo/Enviro.example' path='example'/>
	[TaskName("sysinfo", NestedElementsCheck = true)]
	public class SysInfoTask : Task
	{
		/// <summary>The string to prefix the property names with.  Default is "sys."</summary>
		[TaskAttribute("prefix", Required = false)]
		public string Prefix { get; set; } = "sys.";

		protected override void ExecuteTask() 
		{
			Log.ThrowDeprecation
			(
				Log.DeprecationId.SysInfo, Log.DeprecateLevel.Normal,
				DeprecationUtil.TaskLocationKey(this),
				"{0}: The {1} task is deprecated, and no longer does anything. The properties setup by this task are now loaded automatically.",
				Location, Name
			);
		}
	}
}
