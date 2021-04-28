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
	/// Discard changes made to open files.
	/// </summary>
	/// <remarks>
	/// <para>
	/// <c>revert</c> causes perforce to discard all changes including checked-in changes made after the revision to which the reverted files are returned.  These changes are lost, and cannot be recovered in any way.
	/// </para>
	/// <para>See the 		
	/// <a href="http://www.perforce.com/perforce/doc.021/manuals/cmdref">Perforce User's Guide</a> 
	/// for more information.</para>
	/// </remarks>
	/// <example>
	/// Revert all unchanged files from changelist <c>12345</c> that match the filter <c>//depot/test/...</c>.
	/// <para>Perforce Command:</para>
	///	<code><![CDATA[
	///	p4 revert -a -c 12345 //depot/test/...]]></code>
	///	Equivalent NAnt Task:
	///	<code><![CDATA[
	///	<p4revert onlyUnchanged="true" change="12345" files="//depot/test/..." />]]></code>
	/// </example>
	[TaskName("p4revert")]
	public class P4RevertTask : P4Base 
	{
		public P4RevertTask() : base("p4revert") { }

		/// <summary>The files to revert.</summary>
		[TaskAttribute("files", Required = true)]
		public string Files { get; set; } = null;

		/// <summary>Revert only the files in the specified changelist.</summary>
		[TaskAttribute("change")]
		public string Change { get; set; } = null;

		/// <summary>Revert only unchanged files (open for edit, but not altered).</summary>
		[TaskAttribute("onlyUnchanged")]
		public bool Unchanged { get; set; } = false;

		//	------------------------------------------------------------------------
		/// <summary>
		/// Returns the command line arguments for the application.
		/// </summary>
		//	------------------------------------------------------------------------
		public override string ProgramArguments		
		{ 
			get 
			{ 
				string options = GlobalOptions + " revert";
				if (Unchanged)
					options += " -a";
				if (Change != null)
					options += " -c " + Change;
				options += " ";
				return options + Files; 
			} 
		}	//	end ProgramArguments
	}	// end class P4RevertTask
}
