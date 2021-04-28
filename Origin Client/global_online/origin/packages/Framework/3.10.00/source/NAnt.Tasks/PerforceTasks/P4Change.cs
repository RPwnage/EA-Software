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
using System.Diagnostics;

using NAnt.Core;
using NAnt.Core.Logging;
using NAnt.Core.Attributes;

namespace NAnt.PerforceTasks
{
    /// <summary>
    /// Creates a new Perforce changelist or edits an existing one.
    /// </summary>
    /// <remarks>
    /// Files to be added to a changelist must already be opened for
    /// 'add' or for 'edit' on the perforce server or this
    /// command will fail.
    /// <para>This task sets the <c>${p4.change}</c> property which can be used with 
    /// the <c>P4Submit</c>, <c>P4Edit</c>, or <c>P4Add</c> tasks.
    /// </para>
    /// If the <c>files</c> parameter is not specified, all files from the default changelist will be 
    /// moved to the new changelist.
    /// <para>See the 		
    /// <a href="http://www.perforce.com/perforce/doc.021/manuals/cmdref">Perforce User's Guide</a> 
    /// for more information.</para>
    /// </remarks>
    /// <example>
    /// Create a new changelist with the given description.
    /// <para>Perforce Command:</para>
    /// <code><![CDATA[
    /// p4 change "This is a new changelist created by NAnt"]]></code>
    /// Equivalent NAnt Task:<code><![CDATA[
    /// <p4change desc="This is a new changelist created by NAnt"/>]]></code>
    /// </example>
    [TaskName("p4change")]
    public class P4ChangeTask : P4Base
    {
        public P4ChangeTask() : base("p4change") { }

        public enum ChangeType { None, Restricted, Public };

        string _files = null;
        string _desc = null;
        string _change = null;
        bool _delete = false;
        ChangeType _type = ChangeType.None;

        /// <summary>Textual description of the changelist.</summary>
        [TaskAttribute("desc")]
        public string Desc { get { return _desc; } set { _desc = value; } }

        /// <summary>
        /// A Perforce file specification for the files to add to the changelist.
        /// </summary>
        [TaskAttribute("files")]
        public string Files { get { return _files; } set { _files = value; } }

        /// <summary>Changelist to modify.  Required for delete.</summary>
        [TaskAttribute("change")]
        public string Change { get { return _change; } set { _change = value; } }

        /// <summary>Delete the named changelist.  This only works for empty changelists.</summary>
        [TaskAttribute("delete")]
        public bool Delete { get { return _delete; } set { _delete = value; } }

        /// <summary>Set change list type to be restricted or public. This only works starting with P4 2012.1.</summary>
        [TaskAttribute("type")]
        public ChangeType Type { get { return _type; } set { _type = value; } }

        /// <summary>
        /// Executes the P4Change task
        /// </summary>
        protected override void ExecuteTask()
        {
            P4Form form = new P4Form(ProgramFileName, BaseDirectory, FailOnError, Log);

            //	Build a command line to send to p4.
            string command = GlobalOptions;

            if (!Delete && Type == ChangeType.None)
                command += " change -o";
            else
            {
                if (Delete && (Type != ChangeType.None))
                {
                    throw new BuildException(this.GetType().ToString() + ": Both 'delete' and 'type' cannot be specified. Only one of these attributes can be set.", Location);
                }
                if (Change == null || Change == String.Empty)
                {
                    throw new BuildException(this.GetType().ToString() + ": changeList must be specified when deleting or setting type.", Location);
                }

                if (Delete)
                {
                    command += " change -d ";
                }
                else if (Type == ChangeType.Restricted)
                {
                    command += " change -t restricted";
                }
                else if (Type == ChangeType.Public)
                {
                    command += " change -t public";
                }
            }

            if (Change != null)
                command += Change;

            try
            {
                //	Execute the command and retreive the form.
                Log.Info.WriteLine(LogPrefix + "{0}>{1} {2}", form.WorkDir, form.ProgramName, command);
                form.GetForm(command);

                if (Delete)
                {
                    //					string result = form.GetField("Change");
                    //					if (result != Change + " deleted.")
                    //						throw new BuildException(this.GetType().ToString() + ": problem deleting, " + form.Form, Location);
                    Properties.Add("p4.change", "");
                }
                else
                {
                    //	Replace the data in the form with the new data.
                    form.ReplaceField("Description:", Desc);
                    form.ReplaceField("Files:", Files);

                    //	Now send the modified form back to Perforce.
                    command = GlobalOptions + " change -i";
                    Log.Info.WriteLine(LogPrefix + "{0}>{1} {2}", form.WorkDir, form.ProgramName, command);
                    form.PutForm(command);

                    //	Now set the ${p4.change} property based on the output.  It's not really a field, but we can isolate
                    //	the change number with this routine, only it returns more info than we want.
                    string changeProp = form.GetField("Change");
                    if (changeProp == null || changeProp == String.Empty)
                        throw new BuildException(this.GetType().ToString() + ": Changelist not returned properly", Location);
                    int endIndex = changeProp.IndexOf(' ');
                    string prop = changeProp.Remove(endIndex, changeProp.Length - endIndex);
                    Properties.Add("p4.change", prop.Trim());
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

    }	// end class P4ChangeTask
}
