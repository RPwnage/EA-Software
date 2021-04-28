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
// Electronic Arts (Frostbite.Team.CM@ea.com)

using NAnt.Core;
using NAnt.Core.Logging;
using NAnt.Core.Attributes;
using NAnt.Core.Tasks;
using NAnt.Core.Util;
using System;
using EA.FrameworkTasks.Model;
using System.Linq;
using EA.Eaconfig.Modules;

namespace EA.Eaconfig
{
	/// <summary>(Deprecated) Sets up a number of default public data properties. This is deprecated, use the 'publicdata' task instead.</summary>
	[TaskName("ScriptInit")]
	public class ScriptInitTask : Task
	{
		/// <summary>The name of the package</summary>
		[TaskAttribute("PackageName", Required = true)]
		public string PackageName { get; set; }

		/// <summary></summary>
		[TaskAttribute("IncludeDirs", Required = false)]
		public string IncludeDirs { get; set; } = string.Empty;

		/// <summary></summary>
		[TaskAttribute("Libs", Required = false)]
		public string Libs { get; set; } = string.Empty;

		public ScriptInitTask() : base() { }

		public ScriptInitTask(Project project, string packageName, string includeDirs = null, string libs = null)
			: base()
		{
			Project = project;
			PackageName = packageName;
			IncludeDirs = includeDirs ?? string.Empty;
			Libs = libs ?? string.Empty;
		}

		protected override void ExecuteTask()
		{
			Log.ThrowDeprecation
			(
				Log.DeprecationId.ScriptInit, Log.DeprecateLevel.Minimal,
				DeprecationUtil.TaskLocationKey(this),
				"{0} <{1}> task is deprecated. Use 'publicdata' task instead. See Framework documentation for details.",
				Location, Name
			);

			if (Log.Level == Log.LogLevel.Diagnostic)
			{
				Log.Debug.WriteLine("ScriptInit('" + PackageName + "', '" + IncludeDirs + "', '" + Libs + "')");
			}

			if (!IncludeDirs.IsNullOrEmpty())
			{
				Project.Properties["package." + PackageName + ".includedirs"] = IncludeDirs;
			}
			else
			{
				Project.Properties["package." + PackageName + ".includedirs"] = Project.ExpandProperties("${package." + PackageName + ".dir}/include");
			}

			if (!Libs.IsNullOrEmpty())
			{
				FileSet libs = new FileSet();
				libs.Include(Libs);
				Project.NamedFileSets["package." + PackageName + ".libs"] = libs;
			}
			else
			{
				if (VerifyScriptInit())
				{
					string default_lib = Project.ExpandProperties("${package." + PackageName + ".builddir}/${config}/lib/${lib-prefix}" + PackageName + "${lib-suffix}");
					Project.Properties.Add("package." + PackageName + ".default.scriptinit.lib", default_lib);
					FileSet libs = new FileSet();
					libs.Include(default_lib);
					Project.NamedFileSets["package." + PackageName + ".libs"] = libs;
				}
			}
		}

		// Verify that ScriptInitTask is applied properly
		private bool VerifyScriptInit()
		{
			// Default is true, because we can't detect this on use dependencies and in some other cases;
			bool hasPackageModule = true;

			var config = Properties["config"];
			var version = Properties[String.Format("package.{0}.version", PackageName)];

			IPackage package;

			if (Project.BuildGraph().Packages.TryGetValue(Package.MakeKey(PackageName, version, config), out package))
			{
				// There is slight chance of concurrent access to modules collection. If error happens just ignore, build scripts should be cleaned so that we don't need this check at all.
				try
				{
					// If package is buildable
					if ((package.Modules.Count() > 0 && package.Modules.Where(m => (m is Module_UseDependency)).Count() == 0) && (package.IsKindOf(Package.FinishedLoading) && !package.Modules.Any(m => m.Name == PackageName)))
					{
						hasPackageModule = false;

						if (Log.WarningLevel >= Log.WarnLevel.Advise)
						{
							Log.Warning.WriteLine("Processing '{0}':  Initialize.xml from package '{1}' invokes task ScriptInit without 'Libs' parameter. ScriptInit in this case exports library '{2}', but package does not have any modules with this name. Fix your Initialize.xml file.",
								Properties["package.name"], PackageName, Project.ExpandProperties("${package." + PackageName + ".builddir}/${config}/lib/${lib-prefix}" + PackageName + "${lib-suffix}"));
						}
					}
				}
				catch (Exception)
				{
				}
			}

			return hasPackageModule;
		}

	}
}
