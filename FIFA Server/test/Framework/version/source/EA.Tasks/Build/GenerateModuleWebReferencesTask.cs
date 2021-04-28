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

using System.IO;

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Attributes;
using NAnt.Core.Tasks;
using EA.FrameworkTasks.Model;

namespace EA.Eaconfig
{
	///<summary>Use wsdl to generate proxy classes for ASP.NET web services. The generated code is placed in ${workingdir}.</summary>
	[TaskName("task-generatemodulewebreferences")]
	public class GenerateModuleWebReferencesTask : Task
	{
		/// <summary>The name of the module</summary>
		[TaskAttribute("module", Required = true)]
		public string Module { get; set; }

		/// <summary>The group the module is in</summary>
		[TaskAttribute("group")]
		public string Group { get; set; } = "runtime";

		/// <summary>The fileset (C#) or property (managed) where the output will be stored</summary>
		[TaskAttribute("output")]
		public string Output { get; set; }

		public string WebCodeDir;

		public GenerateModuleWebReferencesTask() : base() { }

		public GenerateModuleWebReferencesTask(Project project, string module, string group = "runtime", string output = null) 
			: base()
		{
			Project = project;
			Module = module;
			Group = group;
			Output = output;
		}

		public static string GetWsdlExe(Project project)
		{
			string wsdl = project.GetPropertyOrDefault("wsdl.exe", null);
			if (wsdl == null)
			{
				if (project.Properties.Contains("package.WindowsSDK.tools.wsdl"))
				{
					wsdl = project.Properties["package.WindowsSDK.tools.wsdl"];
				}
				else
				{
					int majorVersion = 0;
					if (int.TryParse(project.GetPropertyOrDefault("package.WindowsSDK.MajorVersion", "0"), out majorVersion) && majorVersion >= 8)
					{
						wsdl = project.ExpandProperties("${package.WindowsSDK.dotnet.tools.dir}/wsdl.exe");
					}
					else if (project.Properties.Contains("package.WindowsSDK.appdir"))
					{
						wsdl = project.ExpandProperties("${package.WindowsSDK.appdir}/bin/wsdl.exe");
					}
				}
				project.Properties["wsdl.exe"] = wsdl;
			}
			return wsdl;
		}

		protected virtual void RunWsdl(string exe, string directory, string name, string value, string lang)
		{
			new ExecTask(Project, program: exe, workingdir: directory,
						message: "Converting '" + name + "' ...",
						stdout: Project.Properties.GetBooleanPropertyOrDefault("nant.verbose", false),
						commandline: string.Format("/namespace:{0} {1} /language:{2} /nologo", name, value, lang)).Execute();
		}

		protected override void ExecuteTask()
		{
			string groupAndModule = Group + "." + Module;

			string WsdlExe = GetWsdlExe(Project);
			if (!File.Exists(WsdlExe))
			{
				throw new BuildException(string.Format("Unable to find wsdl.exe in '{0}'", WsdlExe));
			}

			string buildType = Project.GetPropertyOrFail(groupAndModule + ".buildtype");
			string moduleBaseType = GetModuleBaseType.Execute(Project, buildType).BaseName;

			string webreflang = null;
			if (moduleBaseType.StartsWith("CSharp"))
			{
				webreflang = "CS";
			}
			else if (moduleBaseType.StartsWith("Managed"))
			{
				webreflang = "CPP";
			}
			else
			{
				throw new BuildException(string.Format("Buildtype '{0}' for module '{1}' in group '{2}' does not support web references.", buildType, Module, Group));
			}

			WebCodeDir = Project.ExpandProperties(Path.Combine(Project.Properties["package.builddir"], Module, "webreferences"));
			if (!Directory.Exists(WebCodeDir)) Directory.CreateDirectory(WebCodeDir);

			foreach (var webref in Project.NamedOptionSets[groupAndModule + ".webreferences"].Options)
			{
				// We need to make sure that this is being executed only once for a specific module build
				DoOnce.Execute(Project, string.Format("WebReference_{0}_{1}_{2}_{3}", WebCodeDir, webref.Key, webref.Value, webreflang), () =>
				{
					// Generate web service wrapper
					RunWsdl(WsdlExe, WebCodeDir, webref.Key, webref.Value, webreflang);
				});
			}

			if (Output != null)
			{
				// these properties are only being set for potential backward compatibility 
				// because they were set by the old xml version of this task, I don't know if anyone is still using these
				Project.Properties["webref.lang"] = webreflang;
				Project.Properties["webcodedir"] = WebCodeDir;

				// The generated code needs to compile with the module's source code
				if (moduleBaseType.StartsWith("CSharp"))
				{
					FileSet outputFileset = null;
					if (!Project.NamedFileSets.TryGetValue(Output, out outputFileset))
					{
						outputFileset = new FileSet();
					}
					outputFileset.BaseDirectory = WebCodeDir;
					outputFileset.Include("**/*.cs");
					Project.NamedFileSets[Output] = outputFileset;
				}
				// For managed projects, the generated header file's path needs to be added to the module's include dirs
				else if (moduleBaseType.StartsWith("Managed"))
				{
					Project.Properties[Output] = WebCodeDir;
				}
			}
		}
	}
}
