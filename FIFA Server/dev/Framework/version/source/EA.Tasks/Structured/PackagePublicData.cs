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

using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Logging;
using NAnt.Core.Util;

namespace EA.Eaconfig.Structured
{
	/// <summary>
	/// Container task to declare module public data in Initialize.xml file.  Refer to the Module
	/// task for a full description of the NAnt script that may be placed inside the publicdata task.
	/// You can also look at the documentation for Initialize.xml (Reference/Packages/Initialize.xml file)
	/// for some examples of how to use the publicdata task.
	/// </summary>
	[TaskName("publicdata")]
	public class PackagePublicData : TaskContainer
	{
		/// <summary>Name of the package.</summary>
		[TaskAttribute("packagename", Required = true)]
		public String PackageName { get; set; } = String.Empty;

		/// <summary>
		/// List of supported processors to be used in adding default include directories and libraries.
		/// </summary>
		[TaskAttribute("processors", Required = false)]
		public String Processors { get; set; }

		/// <summary>
		/// List of additional library names to be added to default set of libraries.
		/// </summary>
		[TaskAttribute("libnames", Required = false)]
		public String LibNames { get; set; }

		/// <summary>
		/// (Deprecated) If set to true default values for include directories and libraries will be added to all modules. Can be overwritten by 'add-defaults' attribute in module.
		/// </summary>
		[TaskAttribute("add-defaults", Required = false)]
		public bool AddDefaults
		{
			get { return _adddefaults ?? false; }
			set { _adddefaults = value; }
		} internal bool? _adddefaults;

		/// <summary>
		/// If your package overrides the "package.configbindir" property this should be set if you want public data paths to be auto resolved.
		/// </summary>
		/// <example>
		/// <para>If your package .build file modifies configbindir:
		/// <code><![CDATA[
		/// <property name="package.configbindir" value="${overridebindir}"/>
		/// ]]></code>
		/// Then publicdata task will not return correct default paths unless it is passed the new configbindir:
		/// <code><![CDATA[
		/// <publicdata packagename="MyPackage" configbindir="${overridebindir}">
		///		<moddule name="MyAssembly">
		///			<assemblies/> <!-- this will be able to resolve the correct default path -->
		///		</module>
		/// </publicdata>
		/// ]]></code>
		/// </para>
		/// </example>
		[TaskAttribute("configbindir", Required = false)]
		public string ConfigBinDir { get; set; }

		// TODO publicdata configlibdir
		// -dvaliant 2016/05/17
		/*
			it's unlikely but user may also want to override configlibdir in which case we should add wiring for this as well
		*/

		protected override void ExecuteTask()
		{
			if (_adddefaults.HasValue)
			{
				Log.ThrowDeprecation
				(
					Log.DeprecationId.PublicDataDefaults, Log.DeprecateLevel.Normal,
					DeprecationUtil.TaskLocationKey(this),
					"{0} Use of 'add-defaults' attribute in <{1}> task in is deprecated. To add default libs use empty libs fileset '<libs/>' instead.", 
					Location, Name
				);
			}

			string packageDotName = "package." + PackageName + ".";

			string package_builddir = Project.GetPropertyValue(packageDotName + "builddir");
			if (package_builddir == null)
			{
				throw new BuildException(String.Format(LogPrefix + "{0} Property '{1}' is not defined, verify that package name '{2}' is correct.", Location, packageDotName + "builddir", PackageName));
			}

			// The libdir and bindir need to be created in advance because the <module> task assume that these two properties exists.
			// However, we should only set these properties if user haven't already provided an override define for it.
			string libdirPropname = packageDotName + "libdir";
			string configPropertyName = Project.GetPropertyValue("config");

			if (configPropertyName == null)
			{
				throw new BuildException(String.Format(LogPrefix + "Framework is expecting the 'config' property to be defined but it isn't visible at this point. This may be because the top level package build file is missing the <package> task."));
			}

			if (!Project.Properties.Contains(libdirPropname))
			{
				Project.Properties[libdirPropname] = Path.Combine(package_builddir, Project.GetPropertyValue("config"), "lib");
			}
			string bindirPropname = packageDotName + "bindir";
			if (!Project.Properties.Contains(bindirPropname))
			{
				string configbindirOverride = null;
				OptionSet pkgOutputMapping = Build.ModulePath.Public.PackageOutputMapping(Project, PackageName);
				if (pkgOutputMapping != null && pkgOutputMapping.Options != null && pkgOutputMapping.Options.ContainsKey("configbindir"))
				{
					// Using the GetFullPath to get rid of the relative paths business.
					configbindirOverride = Path.GetFullPath(pkgOutputMapping.Options["configbindir"]);
				}
				Project.Properties[bindirPropname] = string.IsNullOrEmpty(configbindirOverride) 
					? Path.Combine(package_builddir, Project.GetPropertyValue("config"), "bin")
					: configbindirOverride;
			}

			base.ExecuteTask();
		}
	}
}
