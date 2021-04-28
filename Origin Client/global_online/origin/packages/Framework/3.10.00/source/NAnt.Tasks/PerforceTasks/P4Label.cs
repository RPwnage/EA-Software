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
using System.Text;
using System.Diagnostics;

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Logging;
using NAnt.Core.Attributes;

namespace NAnt.PerforceTasks 
{
	/// <summary>
	/// Creates a new label and associates that label with the current revisions of a list of files on the Perforce depot.
	/// </summary>
	/// <remarks>
	/// Note: This command only creates a label and associates files with it.  Use <c>p4labelsync</c>
	/// to make the label permanent.
	/// <para>See the 		
	/// <a href="http://www.perforce.com/perforce/doc.021/manuals/cmdref">Perforce User's Guide</a> 
	/// for more information.</para>
	/// </remarks>
	/// <example>
	/// Create a label named <c>Nightly build</c>, with description <c>Built on ${TIMESTAMP}</c>.  
	/// Lock the label once created, and associate the files in <c>//sandbox/...</c> with the label.
	/// <para>Perforce Command:</para>
	///	<code><![CDATA[
	///	p4 label -o "Nightly build" 
	///	[edit the form put out by the previous command to include the description, lock and file list data].
	///	p4 label -i
	///	p4 labelsync]]></code>
	///	Equivalent NAnt Task:
	///	<code><![CDATA[
	///	<p4label label="Nightly build" files="//sandbox/..." desc="Built on ${TIMESTAMP}" lock="true"/>
	///	<p4labelsync />]]></code>
	/// </example>
    [TaskName("p4label")]
	public class P4LabelTask : P4Base 
	{
        public P4LabelTask() : base("p4label") { }

		string	_label	= null;	
		string	_desc	= null;
		bool	_lock	= false;
		string	_files	= null;
		bool	_delete	= false;
		bool	_force	= false;

		/// <summary>Name to use for the label (required).</summary>
		[TaskAttribute("label", Required=true)]
		public string Label			{ get { return _label; } set {_label = value; } }

		/// <summary>Description of the label.</summary>
		[TaskAttribute("description")]
		public string Desc			{ get { return _desc; } set {_desc = value; } }

		/// <summary>Lock the label once created.</summary>
		[TaskAttribute("lock")]
		public bool Lock			{ get { return _lock; } set {_lock = value; } }
        
		/// <summary>Optional Perforce file spec to limit the output to the given files.</summary>
		[TaskAttribute("files")]
		public string Files			{ get { return _files; } set { _files = value; } }       

		/// <summary>Deletes an existing label.</summary>
		[TaskAttribute("delete")]
		public bool Delete			{ get { return _delete; } set {_delete = value; } }
        
		/// <summary>For delete, force delete even if locked.  Ignored otherwise.</summary>
		[TaskAttribute("force")]
		public bool Force			{ get { return _force; } set {_force = value; } }
        
		/// <summary>
		/// Executes the P4Change task
		/// </summary>
		protected override void ExecuteTask() 
		{
			P4Form	form	= new P4Form(ProgramFileName, BaseDirectory, FailOnError, Log);

			//	Build a command line to send to p4.
			string	command = GlobalOptions + " label";

			if (!Delete)
				command += " -o";
			else
			{
				command += " -d";
				if (Force)
					command += " -f";
			}
			command += " " + Label;

			try	
			{
				//	Execute the command and retreive the form.
                Log.Info.WriteLine(LogPrefix + "{0}>{1} {2}", form.WorkDir, form.ProgramName, command);
				form.GetForm(command);

				if (!Delete)
				{
					//	Replace the data in the form with the new data.
					form.ReplaceField("Description:", Desc );
					form.ReplaceField("Options:", Lock ? "locked" : "unlocked" );
					form.ReplaceField("View:", Files );

					//	Now send the modified form back to Perforce.
					command = GlobalOptions + " label -i";
                    Log.Info.WriteLine(LogPrefix + "{0}>{1} {2}", form.WorkDir, form.ProgramName, command);
					form.PutForm(GlobalOptions + command);
				}
			}
			catch (BuildException e) 
			{
				if (FailOnError)
					throw new BuildException(this.GetType().ToString() + ": Error during P4 program execution.", Location, e);
				else 
					Log.Error.WriteLine(LogPrefix + "P4 returned an error. {0}", Location);
			}
		}	// end ExecuteTask
	}	// end class P4LabelTask
}

/*
Here's what this form looks like:

# A Perforce Label Specification.
#
#  Label:       The label name.
#  Update:      The date this specification was last modified.
#  Access:      The date of the last 'labelsync' on this label.
#  Owner:       The user who created this label.
#  Description: A short description of the label (optional).
#  Options:     Label update options: 'locked' or 'unlocked'.
#  View:        Lines to select depot files for the label.

Label:	<label here>

Owner:	<owner here>

Description:
	<desc here>

Options:	<locked|unlocked>

View:
	<view here>
*/
