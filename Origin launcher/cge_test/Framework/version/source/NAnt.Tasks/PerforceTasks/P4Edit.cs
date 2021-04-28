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


using NAnt.Core.Attributes;

namespace NAnt.PerforceTasks
{
	/// <summary>
	/// Open file(s) for edit in the local workspace.
	/// </summary>
	/// <remarks>
	/// The specified files will be linked to the a changelist, but changes to the depot will not 
	/// occur until a <c>p4submit</c> has been completed.  
	/// <para>If the <c>change</c> parameter is not specified, the files will go onto the <c>default</c> changelist.
	/// If the files were already open for edit and the <c>change</c> parameter is given, the files will be moved from their current changelist to the specified changelist. 
	/// </para>
	/// <para>See the 		
	/// <a href="http://www.perforce.com/perforce/doc.021/manuals/cmdref">Perforce User's Guide</a> 
	/// for more information.</para>
	/// </remarks>
	/// <example>
	/// Open the files in <c>//depot/test/...</c> for edit and place them on changelist <c>12345</c>.
	/// <para>Perforce Command:</para>
	///	<code><![CDATA[
	///	p4 edit -c 12345 //depot/test/...]]></code>
	///	Equivalent NAnt Task:
	///	<code><![CDATA[
	///	<p4edit change="12345" files="//depot/test/..."/>]]></code>
	/// </example>
	[TaskName("p4edit")]
	public class P4EditTask : P4Base 
	{
		public P4EditTask() : base("p4edit") { }

		/// <summary>The files to edit.</summary>
		[TaskAttribute("files", Required = true)]
		public string Files { get; set; } = null;

		/// <summary>Assign these files to this pre-existing changelist.</summary>
		[TaskAttribute("change")]
		public string Change { get; set; } = null;

		/// <summary>Overrides the default type of the files to this type.</summary>
		[TaskAttribute("type")]
		public string Type { get; set; } = null;

		//	------------------------------------------------------------------------
		/// <summary>
		/// Returns the command line arguments for the application.
		/// </summary>
		//	------------------------------------------------------------------------
		public override string ProgramArguments		
		{ 
			get 
			{ 
				string options = GlobalOptions + " edit";
				if (Change != null)
					options += " -c " + Change;
				if (Type != null)
					options += " -t " + Type;
				options += " ";
				return options + Files; 
			} 
		}	//	end ProgramArguments


	}	// end class P4EditTask
}
