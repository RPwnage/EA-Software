// Originally based on NAnt - A .NET build tool
// Copyright (C) 2003-2015 Electronic Arts Inc.
// Copyright (C) 2001-2003 Gerry Shaw
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
// Kosta Arvanitis (karvanitis@ea.com)

using System;
using NAnt.Core.Attributes;
using NAnt.Core.Logging;
using NAnt.Core.Util;
using NAnt.Core.Reflection;

namespace NAnt.Core.Tasks
{

	/// <summary>Loads functions from a specified assembly.</summary>
	/// <remarks>
	/// <para>
	/// NAnt can only use .NET assemblies; other types of files which
	/// end in .dll won't work.
	/// </para>
	/// </remarks>
	/// <include file='Examples/FuncDef/FuncDef.example' path='example'/>
	[TaskName("funcdef")]
	public class FuncDefTask : Task
	{
		bool _failOnErrorFuncDef = false;

		/// <summary>File name of the assembly containing the NAnt functions.</summary>
		[TaskAttribute("assembly", Required = true)]
		public string AssemblyFileName { get; set; } = null;

		/// <summary>
		/// Override function(s) with the same name.
		/// Default is false. When override is 'false' &lt;funcdef&gt; will fail on duplicate function names.
		/// </summary>
		[TaskAttribute("override", Required = false)]
		public bool Override { get; set; } = false;

		[TaskAttribute("failonerror")]
		public override bool FailOnError
		{
			get { return _failOnErrorFuncDef; }
			set { _failOnErrorFuncDef = value; }
		}

		protected override void ExecuteTask()
		{
			string assemblyFileName = Project.GetFullPath(AssemblyFileName);
			try
			{
				int num = Project.FuncFactory.AddFunctions(AssemblyLoader.Get(assemblyFileName), Override, FailOnError, AssemblyFileName);

				Log.Debug.WriteLine(LogPrefix + "Added {0} functions from {1}.", num, UriFactory.CreateUri(assemblyFileName));

			}
			catch (Exception e)
			{
				string msg = String.Format("Could not add functions from '{0}'.", assemblyFileName);
				throw new BuildException(msg, Location, e);
			}
		}
	}
}
