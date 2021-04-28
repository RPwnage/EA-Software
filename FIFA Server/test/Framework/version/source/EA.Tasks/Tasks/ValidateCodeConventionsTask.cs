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

using System;
using System.Collections.Generic;
using System.IO;
using System.Text;
using System.Text.RegularExpressions;
using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Logging;

using EA.FrameworkTasks.Model;

namespace EA.Eaconfig
{
	[TaskName("validate-code-conventions")]
	public class ValidateCodeConventionsTask : Task
	{
		private static readonly Regex InvalidTrailingWhitespace = new Regex(@"\s+$", RegexOptions.Compiled);
		private static readonly Regex ValidLeadingWhitespace = new Regex(@"^\t* {0,3}\S", RegexOptions.Compiled);
		private static readonly Regex InvalidTabs = new Regex(@"\S\t", RegexOptions.Compiled);
		private static readonly Regex InvalidVariableName = new Regex(@"\b([amsg][A-Z]\w*)", RegexOptions.Compiled);
		private static readonly string[] SupportedExtensions = new string[] { ".BUILD", ".H", ".CPP", ".INL", ".CS", ".XAML", ".EDF", ".ADF" };

		protected override void ExecuteTask()
		{
			HashSet<string> filenames = new HashSet<string>();
			foreach (BuildGroups group in Enum.GetValues(typeof(BuildGroups)))
			{
				var buildModules = Project.Properties[group + ".buildmodules"].ToArray();
				if (buildModules.Count > 0)
				{
					foreach (string module in buildModules)
					{
						string group_module_str = group + "." + module;
						if (Project.Properties.GetBooleanPropertyOrDefault(group_module_str + ".structured-xml-used", false) == false)
						{
							string message = String.Format("Structured XML has not been used for module '{0}'.", group_module_str);
							Log.ThrowWarning
							(
								Log.WarningId.StructuredXmlNotUsed, Log.WarnLevel.Normal,
								message + " Please refer to the Framework docs to learn how structured XML can benefit your build scripts."
							);
						}

						GatherFiles(group, module, "sourcefiles", filenames);
						GatherFiles(group, module, "headerfiles", filenames);
					}
				}
				else
				{
					GatherFiles(group, String.Empty, "sourcefiles", filenames);
					GatherFiles(group, String.Empty, "headerfiles", filenames);
				}
			}

			if (ValidateFiles(filenames))
			{
				string name = Project.Properties["package.name"];
				Log.Status.WriteLine(name + " passes code conventions.");
			}
			else
			{
				throw new BuildException("Violates code conventions.");
			}
		}

		private void GatherFiles(BuildGroups group, string module, string filesetname, ICollection<string> filenames)
		{
			var modulesep = ".";
			if(!String.IsNullOrEmpty(module))
			{
				modulesep = "." + module + ".";
			}
			// gather source files.
			FileSet files = Project.NamedFileSets[group + modulesep + filesetname];
			
			if(files != null)
			{
				foreach (FileItem file in files.FileItems)
				{
					filenames.Add(file.FileName);
				}
			}
			files = Project.NamedFileSets[group + modulesep + filesetname + "." + Project.GetPropertyValue("config-system")];

			if (files != null)
			{
				foreach (FileItem file in files.FileItems)
				{
					filenames.Add(file.FileName);
				}
			}
		}

		private bool ValidateFiles(IEnumerable<string> filenames)
		{
			// validate files
			bool success = true;
			foreach (string filename in filenames)
			{
				string ext = Path.GetExtension(filename).ToUpperInvariant();
				if (Array.IndexOf(SupportedExtensions, ext) != -1)
				{
					success = Validate(filename) && success;
				}
			}
			return success;
		}

		private bool Validate(string path)
		{
			bool success = true;
			string line;
			int linenum = 0;

			StreamReader reader = new StreamReader(path);
			while ((line = reader.ReadLine()) != null)
			{
				++linenum;
				Match m;

				if (line == "")
				{
					continue;
				}

				m = InvalidVariableName.Match(line);
				if (m.Success)
				{
					string value = m.Groups[1].ToString();
					string recommended;
					if (value.StartsWith("a"))
					{
						StringBuilder s = new StringBuilder();
						s.Append(Char.ToLower(value[1]));
						s.Append(value.Substring(2));
						recommended = s.ToString();
					}
					else
					{
						StringBuilder s = new StringBuilder();
						s.Append(value[0]).Append("_");
						s.Append(Char.ToLower(value[1]));
						s.Append(value.Substring(2));
						recommended = s.ToString();
					}

					string msg = string.Format("{0}({1},{2}): bad variable name, replace '{3}' with '{4}'.", path, linenum, m.Index, value, recommended);
					Log.Status.WriteLine(msg);
					success = false;
				}

				m = ValidLeadingWhitespace.Match(line);
				if (!m.Success)
				{
					string msg = string.Format("{0}({1},{2}): error, leading whitespace", path, linenum, m.Index);
					Log.Status.WriteLine(msg);
					success = false;
				}

				m = InvalidTrailingWhitespace.Match(line);
				if (m.Success)
				{
					string msg = string.Format("{0}({1},{2}): error, trailing whitespace", path, linenum, m.Index);
					Log.Status.WriteLine(msg);
					success = false;
				}

				m = InvalidTabs.Match(line);
				if (success && m.Success)
				{
					string msg = string.Format("{0}({1},{2}): error, contains tabs interleaved with code", path, linenum, m.Index);
					Log.Status.WriteLine(msg);
					success = false;
				}
			}
			return success;
		}
	}
}
