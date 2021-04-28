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
using NAnt.Core.Tasks;

namespace NAnt.PerforceTasks 
{
    /// <summary>
    /// This task lists submitted and pending changelists.
	/// </summary>
	/// <remarks>
	/// If no parameters are given, this task 
	/// reports the list of all pending and submitted changelists currently known to the perforce system.
	/// The parameters serve to filter the output down to a manageable level.
	/// <para>See the
	/// <a href="http://www.perforce.com/perforce/doc.021/manuals/cmdref">Perforce User's Guide</a> 
	/// for more information.</para>
	/// </remarks>
	/// <example>
	/// List the last 3 submitted changelists in <c>//sandbox/...</c>
    /// <para>Perforce Command:</para><code><![CDATA[
    /// p4 changes -m 3 //sandbox/...]]></code>
    /// Equivalent NAnt Task:<code><![CDATA[
    /// <p4changes maxnum=3 files="//sandbox/..." />]]></code>
    /// </example>
    [TaskName("p4changes")]
    public class P4ChangesTask : P4Base 
	{
        public P4ChangesTask() : base("p4changes") { }

		//	Note: the case of this enum is set to match the perforce expected keyword case.
		public enum P4Status { None, pending, submitted };

		string		_files		= String.Empty;
		bool		_integrated	= false;
		int			_maxnum		= 0;
		P4Status	_status		= P4Status.None;

		/// <summary>Optional Perforce file spec to limit the output to the given files.</summary>
		[TaskAttribute("files")]
		public string Files							{ get { return _files; } set { _files = value; } }       

		/// <summary>Include changelists that were integrated with the specified files.</summary>
		[TaskAttribute("integrated")]
		public bool Integrated						{ get { return _integrated; } set { _integrated = value; } }  
     
		/// <summary>Lists only the highest maxnum changes.</summary>
		[TaskAttribute("maxnum"), Int32Validator()]
		public int Maxnum							{ get { return _maxnum; } set { _maxnum = value; } }  
     
		/// <summary>
		/// Limit the list to the changelists with the given status (pending or submitted).
		/// </summary>
		[TaskAttribute("status")]
		public P4Status Status						{ get { return _status; } set { _status = value; } }  
    

		/// <summary>Gets the command line arguments for the application.</summary>
		public override string ProgramArguments
		{
			get 
			{ 
				string options = GlobalOptions + " changes";
				if (Integrated)
					options += " -i";
				if (Verbose)
					options += " -l";
				if (Maxnum >= 0)
					options += " -m " + Maxnum;
				if (Status != P4Status.None)
					options += " -s " + Status.ToString().ToLower();
				options += " ";
				return options + Files; 
			} 
		}	// end ProgramArguments
   }	// end class P4ChangesTask
}
