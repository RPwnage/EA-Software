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
// Jim Petrick (jpetrick@ea.com)

using System;

using NAnt.Core.Attributes;

namespace NAnt.PerforceTasks
{
	/// <summary>
	/// Synchronize files between the depot and the workspace.
	/// </summary>
	/// <remarks>
	/// <para>See the 		
	/// <a href="http://www.perforce.com/perforce/doc.021/manuals/cmdref">Perforce User's Guide</a> 
	/// for more information.</para>
	/// </remarks>
	/// <example>
	/// The first command brings the entire workspace into sync with the depot.  
	/// <para>The second command merely lists the files that are out of sync with the label <c>labelname</c>.</para>
	/// <para>Perforce Commands:</para>
	///	<code><![CDATA[
	///	p4 sync
	///	p4 sync -f -n @labelname.]]></code>
	///	Equivalent NAnt Tasks:
	///	<code><![CDATA[
	///	<p4sync />
	///	<p4sync files="@labelname" force="true" execute="false" />]]></code>
	/// </example>
	[TaskName("p4sync")]
	public class P4SyncTask : P4Base 
	{
		public P4SyncTask() : base("p4sync") { }

		/// <summary>The files to sync.</summary>
		[TaskAttribute("files")]
		public string Files { get; set; } = String.Empty;

		/// <summary>Perform the sync even if files are not writable or are already in sync.</summary>
		[TaskAttribute("force")]
		public bool Force { get; set; } = false;

		/// <summary>If false, displays the results of this sync without actually executing it (defaults true).</summary>
		[TaskAttribute("execute")]
		public bool Exec { get; set; } = true;

		/// <summary>Gets the command line arguments for the application.</summary>
		public override  string ProgramArguments	
		{ 
			get	
			{ 
				string options = GlobalOptions + " sync";
				if (Force)
					options += " -f";
				if (!Exec)
					options += " -n";
				options += " ";
				return options + Files;
			} 
		}	//	end ProgramArguments
	}	// end class Task_NameTask
}
