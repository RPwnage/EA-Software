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
// Gerry Shaw (gerry_shaw@yahoo.com)
// Ian MacLean (ian_maclean@another.com)
// Electronic Arts (Frostbite.Team.CM@ea.com)

using NAnt.Core.Attributes;
using NAnt.Core.Logging;

namespace NAnt.Core.Tasks 
{
	/// <summary>A task that execute the RegularExpression's Match function.
	/// <para>
	///   <b>NOTE</b> This task is new in Framework version 3.28.00.  So if you use this task in your build script, 
	///   you need to make sure that your project don't need to be build by older version of Framework.
	/// </para>
	/// </summary>
	/// <remarks>
	/// This task executes the System.Text.RegularExpressions.Regex.Match .Net function for 
	/// the given input and pattern.  The match result will be sent to the properties provided.
	/// The first property provided will always store the match result.  If a match is not
	/// found, the properties provided will be set to empty string.
	/// </remarks>
	/// <include file='Examples/RegexMatch/Simple.example' path='example'/>
	[TaskName("RegexMatch")]
	public class RegexMatchTask : Task 
	{
		/// <summary>The input string.</summary>
		[TaskAttribute("Input", Required = true)]
		public string Input { get; set; }

		/// <summary>The search pattern.</summary>
		[TaskAttribute("Pattern", Required = true)]
		public string Pattern { get; set; }

		/// <summary>The output property names (separated by semi-colon ';') to store the match output.</summary>
		[TaskAttribute("Properties", Required = true)]
		public string OutProperties { get; set; }

		public static void Execute(Project project, string input, string pattern, string properties)
		{
			RegexMatchTask task = new RegexMatchTask();
			task.Project = project;
			task.Input = input;
			task.Pattern = pattern;
			task.OutProperties = properties;
			task.Execute();
		}

		protected override void ExecuteTask() 
		{
			if (!string.IsNullOrEmpty(Input) && !string.IsNullOrEmpty(Pattern))
			{
				Log.Debug.WriteLine("RegexMatch input: {0}", Input);
				Log.Debug.WriteLine("RegexMatch pattern: {0}", Pattern);

				System.Text.RegularExpressions.Match mt = System.Text.RegularExpressions.Regex.Match(Input, Pattern);
				if (mt.Success)
				{
					Log.Debug.WriteLine("RegexMatch success.  Groups.Count: {0}", mt.Groups.Count);
					var grpIter = mt.Groups.GetEnumerator();
					foreach (string outProp in OutProperties.Split(new char[]{';'}))
					{
						string result = string.Empty;
						bool iteratorAdvanceOk = grpIter.MoveNext();
						if (iteratorAdvanceOk)
						{
							result = grpIter.Current.ToString();
							Log.Debug.WriteLine("    RegexMatch result {0}", result);
						}
						Project.Properties.Add(outProp, result);
					}
				}
				else
				{
					Log.Debug.WriteLine("RegexMatch failed");
					foreach (string outProp in OutProperties.Split(new char[]{';'}))
					{
						Project.Properties.Add(outProp, string.Empty);
					}
				}
			}
	   
		}
	}
}
