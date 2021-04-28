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

using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Util;

// Base class with utility methods for eaconfig tasks.
namespace EA.Eaconfig
{
	[TaskName("EAConfigTask", XmlSchema = false)]
	public abstract class EAConfigTask : Task
	{
		protected string Config;
		protected string ConfigSystem;
		protected string ConfigPlatform;
		protected string ConfigCompiler;

		protected string PackageName
		{
			get
			{
				if (_package_name == null)
				{
					_package_name = Properties["package.name"];
				}
				return _package_name;
			}
		} private string _package_name = null;

		protected string PackageVersion
		{
			get
			{
				if (_package_version == null)
				{
					_package_version = Properties["package.version"];
				}
				return _package_version;
			}
		} private string _package_version = null;

		protected EAConfigTask(Project project)
			: base()
		{
			Project = project;
			InternalInit();
		}

		protected EAConfigTask()
			: base()
		{
		}

		protected override void InitializeTask(System.Xml.XmlNode taskNode)
		{
			base.InitializeTask(taskNode);
			InternalInit();
		}

		protected virtual void InternalInit()
		{
			using (PropertyDictionary.PropertyReadScope scope = Properties.ReadScope())
			{
				Config = scope["config"];
				ConfigSystem = scope["config-system"];
				ConfigPlatform = scope["config-platform"];
				ConfigCompiler = scope["config-compiler"];
			}
		}

		protected OptionSet CreateBuildOptionSetFrom(string buildSetName, string optionSetName, string fromOptionSet)
		{
			OptionSet optionset = new OptionSet();
			optionset.FromOptionSetName = fromOptionSet;
			optionset.Project = Project;
			optionset.Append(OptionSetUtil.GetOptionSet(Project, fromOptionSet));
			optionset.Options["buildset.name"] = buildSetName;
			if (Project.NamedOptionSets.Contains(optionSetName))
			{
				Project.NamedOptionSets[optionSetName].Append(optionset);
			}
			else
			{
				Project.NamedOptionSets[optionSetName] = optionset;
			}
			return optionset;
		}
	}
}
