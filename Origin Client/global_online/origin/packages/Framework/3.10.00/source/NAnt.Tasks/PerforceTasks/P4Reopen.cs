// NAnt - A .NET build tool
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
// Jim Petrick (jpetrick@ea.com)

using System;
using System.Text;

using NAnt.Core.Attributes;

namespace NAnt.PerforceTasks
{
    /// <summary>
    /// Moves opened files between changelists.
	/// </summary>
	/// <remarks>
	/// <para>See the 		
	/// <a href="http://www.perforce.com/perforce/doc.021/manuals/cmdref">Perforce User's Guide</a> 
	/// for more information.</para>
	/// </remarks>
	/// <example>
	/// Move the file <c>//sandbox/testFile.cs</c> from the default changelist to changelist <c>605040</c>.
	/// <para>Perforce Command:</para>
	///	<code><![CDATA[
	///	p4 reopen -c 605040 //sandbox/testFile.cs]]></code>
	///	Equivalent NAnt Task:
	///	<code><![CDATA[
	///	<p4reopen change="605040" files="//sandbox/testFile.cs" />]]></code>
	/// </example>
    [TaskName("p4reopen")]
    public class Task_Reopen : P4Base 
	{
        public Task_Reopen() : base("p4reopen") { }

		string	_files	= String.Empty;
		string	_change	= null;

 		/// <summary>The files to operate on.</summary>
		[TaskAttribute("files", Required=true)]
		public string Files			{ get { return _files; } set {_files = value; } }
        
		/// <summary>Revert only the files in the specified changelist.</summary>
		[TaskAttribute("change", Required=true)]
		public string Change		{ get { return _change; } set {_change = value; } }
       
		/// <summary>Gets the command line arguments for the application.</summary>
		public override string ProgramArguments		
		{ 
			get	
			{ 	
				return GlobalOptions + " reopen -c " + Change + " " + Files; 
			} 
		}	//	end ProgramArguments
	}	// end class Task_Reopen
}
