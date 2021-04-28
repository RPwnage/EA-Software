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
using NAnt.Core.Util;

using EA.Eaconfig.Build;
using EA.Eaconfig.Core;
using EA.Eaconfig.Modules.Tools;
using EA.FrameworkTasks.Model;
using EA.Tasks.Model;
using System;

namespace EA.Eaconfig.Modules
{
	// Project types are unique powers of two so that multiple types can be applied to a bitmask
	public enum DotNetProjectTypes { Workflow = 1, UnitTest = 2, WebApp = 4};

	public enum DotNetGenerateSerializationAssembliesTypes { None, Auto, On, Off };

	public class Module_DotNet : ProcessableModule
	{	
		internal Module_DotNet(string name, string groupname, string groupsourcedir, Configuration configuration, BuildGroups buildgroup, BuildType buildType, IPackage package)
			: base(name, groupname, groupsourcedir, configuration, buildgroup, buildType, package)
		{
			SetType(Module.DotNet);

			m_copyLocal = CopyLocalType.Undefined;
			m_copyLocalUseHardLink = false;
		}

		public override CopyLocalType CopyLocal
		{
			get
			{
				return m_copyLocal;
			}
		} private CopyLocalType m_copyLocal;

		public override CopyLocalInfo.CopyLocalDelegate CopyLocalDelegate { get { return CopyLocalInfo.CommonDelegate; } }

		public override bool CopyLocalUseHardLink
		{
			get
			{
				return m_copyLocalUseHardLink;
			}
		} private bool m_copyLocalUseHardLink;

		public string TargetFrameworkVersion => DotNetTargeting.GetNetVersion(Package.Project, TargetFrameworkFamily);

		public DotNetFrameworkFamilies TargetFrameworkFamily
		{
			get
			{
				if (!m_targetFrameworkFamily.HasValue)
				{
					throw new InvalidOperationException("Trying to access target .NET family before buildgraph resolution of family.");
				}
				return m_targetFrameworkFamily.Value;
			}
			set { m_targetFrameworkFamily = value; }
		}
		private DotNetFrameworkFamilies? m_targetFrameworkFamily = null;

		public string LanguageVersion;

		public string RootNamespace;

		public string ApplicationManifest;

		public PathString AppDesignerFolder;

		public DotNetProjectTypes ProjectTypes = 0;

		public bool DisableVSHosting;

		public DotNetGenerateSerializationAssembliesTypes GenerateSerializationAssemblies;

		public string ImportMSBuildProjects;

		public OptionSet WebReferences;

		public FileSet NuGetReferences;


		//IMTODO: I need to move this into postbuild tool to be consistent in native build (if anybody cares about it anymore).
		public string RunPostBuildEventCondition;

		public bool IsProjectType(DotNetProjectTypes test)
		{
			// using bitwise & operator to test a single bit of the bitmask
			return (ProjectTypes & test) == test;
		}

		public DotNetCompiler Compiler
		{
			get { return _compiler; }
			set
			{
				SetTool(value);
				_compiler = value;
			}
		} private DotNetCompiler _compiler;


		public override void Apply(IModuleProcessor processor)
		{
			processor.Process(this);
		}

		internal void SetCopyLocal(CopyLocalType copyLocalType)
		{
			m_copyLocal = copyLocalType;
		}

		internal void SetCopyLocalUseHardLink(bool useHardLink)
		{
			m_copyLocalUseHardLink = useHardLink;
		}
	}
}

