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

using NAnt.Core;
using NAnt.Core.Attributes;

namespace NAnt.PerforceTasks
{
	/// <summary>
	/// This task causes the named label to reflect the current contents of the client workspace.
	/// </summary>
	/// <remarks>
	/// A label records the last revision of each file it contains and can subsequently be used in 
	/// a revision specification as <c>@label</c>.
	/// <para>
	/// If the files parameter is omitted, the sync will cause the label to reflect the entire 
	/// contents of the client by adding, deleting and updating the list of files in the label.
	/// If the file parameter is included, the sync updates only the named files.
	/// </para><para>
	/// NOTE: You can only update labels you own, and labels that are locked cannot be updated 
	/// with <c>p4labelsync</c>.
	/// </para>
	/// <para>See the 		
	/// <a href="http://www.perforce.com/perforce/doc.021/manuals/cmdref">Perforce User's Guide</a> 
	/// for more information.</para>
	/// </remarks>
	/// <example>
	/// The first command lists what would happen if it were to add the files in <c>//sandbox/...</c> to the label <c>labelName</c>.
	/// <para>The second deletes files matching <c>//sandbox/*.txt</c> from label <c>labelName2</c>.</para>
	/// <para>Perforce Commands:
	/// </para><code><![CDATA[
	/// p4 labelsync -a -n -l labelName //sandbox/...
	/// p4 labelsync -d -l labelName2 //sandbox/*.txt]]></code>
	/// Equivalent NAnt Tasks:<code><![CDATA[
	/// <p4labelsync  label="labelName" files="//sandbox/..." add="true" execute="false" />
	/// <p4labelsync  label="labelName2" files="//sandbox/*.txt" delete="true"  />]]></code>
	/// </example>
	[TaskName("p4labelsync")]
	public class P4LabelsyncTask : P4Base 
	{
		public P4LabelsyncTask() : base("p4labelsync") { }

		bool _delete		= false;
		bool		_add		= false;

		/// <summary>Name to use for the label (required).</summary>
		[TaskAttribute("label", Required = true)]
		public string Label { get; set; } = null;

		/// <summary>Optional Perforce file spec to limit the operation to the given files.</summary>
		[TaskAttribute("files")]
		public string Files { get; set; } = String.Empty;

		/// <summary>Deletes the named files from the label.  Defaults false.</summary>
		[TaskAttribute("delete")]
		public bool Delete
		{
			get { return _delete; }
			set
			{
				if (value && Add)
				{
					throw new BuildException(this.GetType().ToString() + ": Add and Delete cannot both be specified at once.", Location);
				}

				_delete = value; 
			}
		}  
	 
		/// <summary>
		/// Adds the named files to the label without deleting any files, 
		/// even if some of the files have been deleted at the head revision.  Defaults false.
		/// </summary>
		[TaskAttribute("add")]
		public bool Add
		{
			get { return _add; }
			set
			{
				if (value && Delete)
				{
					throw new BuildException(this.GetType().ToString() + ": Add and Delete cannot both be specified at once.", Location);
				}

				_add = value; 
			}
		}

		/// <summary>
		/// If false, displays the results of this sync without actually executing it.  
		/// Defaults true.
		/// </summary>
		[TaskAttribute("execute")]
		public bool Exec { get; set; } = true;


		/// <summary>Gets the command line arguments for the application.</summary>
		public override string ProgramArguments
		{
			get 
			{ 
				string options = GlobalOptions + " labelsync";
				if (Add)
					options += " -a";
				if (Delete)
					options += " -d";
				if (!Exec)
					options += " -n";
				return options + " -l" + Label + " " + Files;
			} 
		}	// end ProgramArguments
   }	// end class P4LabelsyncTask
}
