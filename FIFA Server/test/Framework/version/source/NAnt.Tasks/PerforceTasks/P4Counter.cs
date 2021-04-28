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


using NAnt.Core;
using NAnt.Core.Logging;
using NAnt.Core.Attributes;

namespace NAnt.PerforceTasks
{
	/// <summary>
	/// Obtain or set the value of a Perforce counter.
	/// </summary>
	/// <remarks>
	/// NOTE: The user performing this task must have Perforce 'review' permission in order for this 
	/// task to succeed. 
	/// <para>See the 		
	/// <a href="http://www.perforce.com/perforce/doc.021/manuals/cmdref">Perforce User's Guide</a> 
	/// for more information.</para>
	/// </remarks>
	/// <example>
	///	The first command prints the value of the counter <c>last-clean-build</c>.
	/// <para>The second sets the value to <c>999</c>.</para>
	/// <para>The third sets the NAnt property value <c>${p4.LastCleanBuild}</c> based on the value retrieved 
	/// from Perforce.</para>
	/// <para>Perforce Commands:</para><code><![CDATA[
	/// p4 counter last-clean-build
	/// p4 counter last-clean-build 999
	/// [no real equivalent command]]]></code>
	/// Equivalent NAnt Tasks:<code><![CDATA[
	/// <p4counter counter="last-clean-build" />
	/// <p4counter counter="last-clean-build" value="${TSTAMP}" />
	/// <p4counter counter="last-clean-build" property="p4.LastCleanBuild" />]]></code>
	/// </example>
	[TaskName("p4counter")]
	public class Task_P4Counter : P4Base 
	{
		public Task_P4Counter() : base("p4counter") { }

		/// <summary>The name of the counter.</summary>
		[TaskAttribute("counter", Required = true)]
		public string Counter { get; set; } = null;

		/// <summary>The new value for the counter.</summary>
		[TaskAttribute("value")]
		public string Value { get; set; } = null;

		/// <summary>The property to set with the value of the counter.</summary>
		[TaskAttribute("property")]
		public string Property { get; set; } = null;

		/// <summary>
		/// Executes the P4Counter task
		/// </summary>
		protected override void ExecuteTask() 
		{
			P4Form	form	= new P4Form(ProgramFileName, BaseDirectory, FailOnError, Log);

			//	Build a command line to send to p4.
			string	command = GlobalOptions + " counter " + Counter + " ";
			if (Value != null)
				command += Value;
			try	
			{
				//	Execute the command and retrieve the form (perforce output).
				Log.Info.WriteLine(LogPrefix + "{0}>{1} {2}", form.WorkDir, form.ProgramName, command);
				form.GetForm(command);

				//	Extract any useful data from the output and so things with it depending on the command options.
				string result = form.Form.Trim();
				if (result != null && result.Length > 0)
				{
					Log.Status.WriteLine("{0}: {1} = '{2}'", LogPrefix, Counter, result);
					if (Property != null)
						Properties.Add(Property, result);
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
	}	// end class Task_P4Counter
}
