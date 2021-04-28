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
using System.IO;
using System.Text;
using System.Collections;
using System.Text.RegularExpressions;

using NAnt.Core;
using NAnt.Core.Attributes;

namespace EA.GetConfigPlatform
{
	/// <summary>
	/// A Custom task which checks whether an existing solution file
	/// contains the given configuration and platform, and on missing
	/// platform in the solution, assign an existing platform with that config
	/// to that platform property in the calling project instance
	/// </summary>
	[TaskName("GetConfigPlatform")]
	public class GetConfigPlatformTask : Task
	{
		[TaskAttribute("configname", Required = true)]
		public string ConfigName { get; set; } = string.Empty;

		[TaskAttribute("platformname", Required = true)]
		public string PlatformName { get; set; } = string.Empty;

		[TaskAttribute("file", Required = true)]
		public string FileName { get; set; } = string.Empty;

		protected override void ExecuteTask()
		{
			FileInfo fo = new FileInfo(FileName);
			StringBuilder fileText = new StringBuilder((int)fo.Length);
			using (StreamReader reader = new StreamReader(FileName))
			{
				String line;
				while ((line = reader.ReadLine()) != null)
				{
					fileText.Append(line);
					fileText.Append(Environment.NewLine);
				}
				reader.Close();
			}
			string fileString = fileText.ToString();

			const string globalSectionRegxString = @"GlobalSection\(SolutionConfigurationPlatforms\)\s*=\s*preSolution[\s|\n|\t|\r\n]*(([^\n=]*\s*=\s*[^\n=]*[\s|\n|\t|\r\n]*)*)[\s|\n|\t|\r\n]*EndGlobalSection";
			const string configPlatformRegxString = @"([^\n=\|]+)\|([^\n=\|]+)\s*=\s*[^\n=]+";

			// Find all solution configurations - preSolution
			Match match = Regex.Match(fileString, globalSectionRegxString);
			if (match.Success)
			{
				Hashtable slnConfigPlatform = new Hashtable();
				// Example of matching string: ps3-gcc-debug Win32 = ps3-gcc-debug Win32
				MatchCollection slnConfigMatchColl = Regex.Matches(match.Groups[1].ToString().Trim(), configPlatformRegxString);
				for (int i = 0; i < slnConfigMatchColl.Count; i++)
				{
					string config = slnConfigMatchColl[i].Groups[1].ToString().Trim();
					string platform = slnConfigMatchColl[i].Groups[2].ToString().Trim();

					if (!slnConfigPlatform.Contains(config))
					{
						ArrayList platforms = new ArrayList();
						platforms.Add(platform);
						slnConfigPlatform.Add(config, platforms);
					}
					else if (!((ArrayList)slnConfigPlatform[config]).Contains(platform))
					{
						((ArrayList)slnConfigPlatform[config]).Add(platform);
					}
				}

				string configValue = Project.Properties[ConfigName];
				string platformValue = Project.Properties[PlatformName];
				if (configValue != null && platformValue != null)
				{
					if (slnConfigPlatform.Contains(configValue))
					{
						if (!((ArrayList)slnConfigPlatform[configValue]).Contains(platformValue))
						{
							// Set platform to an existing platform in the solution file.
							Project.Properties[PlatformName] = (string)((ArrayList)slnConfigPlatform[configValue])[0];
						}
					}
				}
				else
				{
					throw new BuildException("ERROR: Properties not set '" + ConfigName + "', '" + PlatformName + "'!");
				}
			}
			else
			{
				string errorString = String.Format("ERROR: {0} is missing SolutionConfiguration Section", FileName);
				throw new BuildException(errorString);
			}
		}
	}
}