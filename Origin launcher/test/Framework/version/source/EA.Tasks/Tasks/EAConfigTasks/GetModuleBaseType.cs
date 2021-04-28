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

using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Logging;
using NAnt.Core.Tasks;
using NAnt.Core.Util;

namespace EA.Eaconfig
{
	/// <summary>
	/// Evaluates base buildtype name for the given buildtype optionset.
	/// </summary>
	/// <remarks>
	/// Base build type name can be one of the following
	/// <list type="bullet">
	/// <item>Library</item>
	/// <item>Program</item>
	/// <item>DynamicLibrary</item>
	/// <item>WindowsProgram</item>
	/// <item>CSharpLibrary</item>
	/// <item>CSharpProgram</item>
	/// <item>CSharpWindowsProgram</item>
	/// <item>FSharpLibrary</item>
	/// <item>FSharpProgram</item>
	/// <item>FSharpWindowsProgram</item>
	/// <item>ManagedCppAssembly</item>
	/// <item>ManagedCppProgram</item>
	/// <item>MakeStyle</item>
	/// <item>VisualStudioProject</item>
	/// <item>Utility</item>
	/// </list>
	/// </remarks>
	[TaskName("GetModuleBaseType")]
	public class GetModuleBaseType : Task
	{
		/// <summary>Name of the buildtype optionset to examine</summary>
		[TaskAttribute("buildtype-name")]
		public string BuildTypeName { get; set; }

		/// <summary>(deprecated) Duplicate of the buildtype-name field for backward compatibility</summary>
		[TaskAttribute("Name")]
		public string BuildTypeNameAlt 
		{
			get { return BuildTypeName; }
			set
			{
				Log.ThrowDeprecation
				(
					Log.DeprecationId.GetModuleBaseTypeName, Log.DeprecateLevel.Normal,
					DeprecationUtil.TaskLocationKey(this),
					"{0} The 'Name' attribute of GetModuleBaseType task is deprecated.Please use the 'buildtype-name' attribute instead.",
					Location		
				);
				BuildTypeName = value;
			}
		}

		/// <summary>Evaluated buildtype. Saved in property "GetModuleBaseType.RetVal"</summary>
		[TaskAttribute("base-buildtype")]
		public string BaseBuildType
		{
			get { return BuildType.BaseName; }
		}

		public BuildType BuildType { get; private set; }

		public static BuildType Execute(Project project, string buildTypeName)
		{
			GetModuleBaseType task = new GetModuleBaseType(project, buildTypeName);
			task.ExecuteTask();
			return task.BuildType;
		}

		public GetModuleBaseType(Project project, string buildTypeName)
			: base()
		{
			Project = project;
			BuildTypeName = buildTypeName;

			if (String.IsNullOrEmpty(BuildTypeName))
			{
				throw new BuildException("'BuildTypeName' parameter is empty.", Location);
			}
		}

		public GetModuleBaseType() : base() { }

		/// <summary>Execute the task.</summary>
		protected override void ExecuteTask()
		{
			string baseBuildType = String.Empty;

			// java is a special case - we don't bother with an optionset because
			// everything is configured in the gradle file
			if (BuildTypeName == "AndroidApk" || BuildTypeName == "JavaLibrary" || BuildTypeName == "AndroidAar")
			{
				BuildType = new BuildType(BuildTypeName, BuildTypeName);

				// For compatibility with C# script
				Properties["GetModuleBaseType.RetVal"] = baseBuildType;
				return;
			}

			if (BuildTypeName == "Program")
			{
				BuildTypeName = "StdProgram";
			}
			else if (BuildTypeName == "Library")
			{
				BuildTypeName = "StdLibrary";
			}
			OptionSet buildOptionSet = OptionSetUtil.GetOptionSetOrFail(Project, BuildTypeName);

			if (OptionSetUtil.GetBooleanOption(buildOptionSet, "buildset.protobuildtype"))
			{
				throw new BuildException($"[GetModuleBaseType] Buildset '{BuildTypeName}' is a protobuild set and can not be used for build.\nUse 'GenerateBuildOptionset' task to create final build type.", Location);
			}

			string buildTasks = OptionSetUtil.GetOption(buildOptionSet, "build.tasks");
			if (buildTasks == null)
			{
				throw new BuildException($"[GetModuleBaseType] BuildType '{BuildTypeName}' does not have 'build.tasks' option.",  Location);
			}

			if (buildTasks.Contains("lib"))
			{
				baseBuildType = "StdLibrary";
			}
			else if (buildTasks.Contains("csc"))
			{
				switch (OptionSetUtil.GetOption(buildOptionSet,"csc.target"))
				{
					case "library":
						baseBuildType = "CSharpLibrary";
						break;
					case "exe":
						baseBuildType = "CSharpProgram";
						break;
					case "winexe":
						baseBuildType = "CSharpWindowsProgram";
						break;
				}
			}
			else if (buildTasks.Contains("fsc"))
			{
				// baseBuildType must be set here...
				switch (OptionSetUtil.GetOption(buildOptionSet, "fsc.target"))
				{
					case "library":
						baseBuildType = "FSharpLibrary";
						break;
					case "exe":
						baseBuildType = "FSharpProgram";
						break;
					case "winexe":
						baseBuildType = "FSharpWindowsProgram";
						break;
				}
			}
			else if (buildTasks.Contains("link"))
			{
				string ccDefines = OptionSetUtil.GetOption(buildOptionSet, "cc.defines") ?? String.Empty;
				string ccOptions = OptionSetUtil.GetOption(buildOptionSet, "cc.options") ?? String.Empty;
				string linkOptions = OptionSetUtil.GetOption(buildOptionSet, "link.options") ?? String.Empty;

				if (ccOptions.Contains("clr"))
				{
					if (linkOptions.Contains("-dll"))
					{
						baseBuildType = "ManagedCppAssembly";
					}
					else
					{
						baseBuildType = "ManagedCppProgram";
					}
				}
				else if (OptionSetUtil.GetBooleanOption(buildOptionSet, "generatedll") ||

						 linkOptions.Contains("-dll") ||
						 linkOptions.Contains("-mprx") ||
						 linkOptions.Contains("-oformat=fsprx"))
				{
					baseBuildType = "DynamicLibrary";
				}
				else if (ccDefines.Contains("_WINDOWS"))
				{
					baseBuildType = "WindowsProgram";
				}
				else
				{
					baseBuildType = "StdProgram";
				}
			}
			else if (buildTasks.Contains("makestyle"))
			{
				baseBuildType = "MakeStyle";
			}
			else if (buildTasks.Contains("visualstudio") || BuildTypeName == "VisualStudioProject")
			{
				baseBuildType = "VisualStudioProject";
			}
			else if (buildTasks.Contains("pythonprogram") || BuildTypeName == "PythonProgram")
			{
				baseBuildType = "PythonProgram";
			}
			else if (String.IsNullOrEmpty(buildTasks) || BuildTypeName == "Utility")
			{
				baseBuildType = "Utility";  // Seems that's what eaconfig expects for Util
			}
			else
			{
				baseBuildType = "StdLibrary";
			}

			bool ismakestyle = baseBuildType == "MakeStyle";

			BuildType = new BuildType(BuildTypeName, baseBuildType, isMakestyle:ismakestyle);

			// For compatibility with C# script
			Properties["GetModuleBaseType.RetVal"] = baseBuildType;
		}
	}
}
