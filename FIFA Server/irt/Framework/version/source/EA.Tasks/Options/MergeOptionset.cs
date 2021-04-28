// Originally based on NAnt - A .NET build tool
// Copyright (C) 2003-2018 Electronic Arts Inc.
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
// Electronic Arts (Frostbite.Team.CM@ea.com)

//-----------------------------------------------------------------------------
// mergeoption.cs
//
// NAnt custom task which merges options  from a NAnt code-build script.
// This differs from the built-in Framework mergeoption in that it is using C#
// instead of nAnt tasks to do the work in order to increase performance
//
// Written by:
//		Alex Van der Star (avdstar@ea.com)
//
//-----------------------------------------------------------------------------

using System;
using System.Collections.Generic;
using System.Xml;

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Attributes;

namespace EA.Eaconfig
{
	/// <summary>
	/// NAnt custom task which merges options from a NAnt code-build script.
	/// This differs from the built-in Framework mergeoption in that it is using C#
	/// instead of nAnt tasks to do the work in order to increase performance
	/// </summary>
	[TaskName("MergeOptionset")]
	public class MergeOptionset : Task
	{
		//bool _verbose				=	false;

		public static void Execute(Project project, string optionsetName, string baseOptionsetName)
		{
			MergeOptionset task = new MergeOptionset();
			task.Project = project;
			task.OriginalOptionset = optionsetName;
			task.FromOptionset = baseOptionsetName;
			task.Execute();
		}

		[TaskAttribute("OriginalOptionset", Required = true)]
		public string OriginalOptionset { get; set; } = null;

		[TaskAttribute("FromOptionset", Required = true)]
		public string FromOptionset { get; set; } = null;

		/// <summary>Execute the task.</summary>
		protected override void ExecuteTask()
		{
			OptionSet fromOS = Project.NamedOptionSets[FromOptionset];
			OptionSet originalOS = Project.NamedOptionSets[OriginalOptionset];

			if (fromOS == null)
			{
				string errorMessage = String.Format("ERROR:  OptionSet '{0}' specified as input to MergeOptionset task does not exist.", FromOptionset);
				throw new BuildException(errorMessage);
			}

			if (originalOS == null)
			{
				string errorMessage = String.Format("ERROR:  OptionSet '{0}' specified for as output for the MergeOptionset task does not exist.", OriginalOptionset);
				throw new BuildException(errorMessage);
			}

			foreach (KeyValuePair<string, string> entry in fromOS.Options)
			{
				string name  = (string) entry.Key;
				MergeOptionTask.MergeOptionValue( originalOS, name, entry.Value.ToString() ) ;
			}
		}

	}


	[TaskName("MergeOption")]
	public class MergeOptionTask : Task
	{
		[TaskAttribute("Optionset", Required = true)]
		public string Optionset { get; set; } = null;

		[TaskAttribute("OptionName", Required = true)]
		public string OptionName { get; set; } = null;

		//[Property("OptionValue", Required=true)]
		public string OptionValue { get; set; } = null;

		protected override void InitializeTask( XmlNode taskNode )
		{
			var valueElement = taskNode.GetChildElementByName("OptionValue");
			if ( valueElement == null )
			{
				throw new BuildException("Missing required <OptionValue> element.");
			}
			OptionValue = Project.ExpandProperties( valueElement.InnerText ) ;
		}

		protected override void ExecuteTask()
		{
			OptionSet optionset = Project.NamedOptionSets[Optionset];
			if( null != optionset )
			{
				MergeOptionValue( optionset, OptionName, OptionValue ) ;
			}
		}

		static public void MergeOptionValue( OptionSet optionset, string name, string val )
		{
			string origVal ;
			origVal = optionset.Options[name] ;

			if( null != origVal && origVal.Length > 0 )
			{
				// replicate original MergeOption's behaviour, merge the option
				// only if it does not already exist. it is not entirely correct as 
				// the value may be part of substring, as in:
				//   "include/foo" "include/foobar"
				// employ simple substring approach for now...
				if( origVal.IndexOf( val ) >= 0 )
				{
					// do nothing: we already have it
					return;
				}

				// If the original value is a true/false, on/off, or custom, we'll just keep the original
				// value
				string origValCaps = origVal.ToUpper();
				if (origValCaps == "TRUE" || origValCaps == "FALSE" || origValCaps == "ON" || origValCaps == "OFF" ||
					origValCaps == "CUSTOM")
				{
					// do nothing.  These are mutually exclusive values and cannot be combined.  So
					// we're just going to keep the original value.
					return;
				}

				// append new value to the end, along with separator
				
				val = origVal + " " + val ;
			}

			optionset.Options[name] = val;
		}

	}

}
