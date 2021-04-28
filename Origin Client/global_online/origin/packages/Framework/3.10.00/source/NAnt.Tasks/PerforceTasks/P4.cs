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
using System.IO;
using System.Xml;
using System.Text;
using System.Diagnostics;

using NAnt.Core.Attributes;
using NAnt.Core.Tasks;

namespace NAnt.PerforceTasks
{
    /// <summary>
    /// This task allows you to pass arbitrary simple commands to
	/// Perforce.  Used when no predefined task exists for that
	/// Perforce command.
    /// </summary>
    /// <remarks>
    /// It will not work well with tasks that require Perforce 'form' data 
    /// input or output.
	/// <para>See the
	/// <a href="http://www.perforce.com/perforce/doc.021/manuals/cmdref">Perforce User's Guide</a> 
	/// for more information.</para>
    /// </remarks>
    /// <example>
    /// To view all the changelists in the depot:
	/// <para>Perforce command:</para>
	/// <code><![CDATA[
	///	p4 changes]]></code> 
	/// Equivalent NAnt task:
	/// <code><![CDATA[
	/// <p4 command="changes" />]]></code>
	/// </example>
    [TaskName("p4")]
	public class P4 : P4Base   
	{
        public P4() : base("p4") { }

		string	command	= null;

		/// <summary>Allows any p4 command to be entered as a command line.</summary>
		[TaskAttribute("command",Required=true)]
		public string Command			{ get { return command; }	set { command = value; } }
        
		/// <summary>Gets the command line arguments for the application.</summary>
		public override string ProgramArguments		
		{ 
			get { 	return GlobalOptions + Command; }
		}
	
	}	// end class P4
}
