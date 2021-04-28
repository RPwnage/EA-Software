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
using NAnt.Core.Util;

using EA.Eaconfig.Build;
using EA.Eaconfig.Core;
using EA.Eaconfig.Modules.Tools;
using EA.FrameworkTasks.Model;
using EA.Tasks.Model;

namespace EA.Eaconfig.Modules
{
	public class Module_Native : ProcessableModule
	{
		internal Module_Native(string name, string groupname, string groupsourcedir, Configuration configuration, BuildGroups buildgroup, BuildType buildType, IPackage package)
			: base(name, groupname, groupsourcedir, configuration, buildgroup, buildType, package)
		{
			SetType(Module.Native);

			m_copyLocalUseHardLink = false;
			m_copyLocal = CopyLocalType.Undefined;
		}

		public override void Apply(IModuleProcessor processor)
		{
			processor.Process(this);
		}

		public bool UsingSharedPchModule;       // This module is using the <sharedpch> element.
		public bool UseForcedIncludePchHeader;  // Use compiler forced include syntax so that source files don't need to be updated with #incldue "Pch.h"
		public string PrecompiledHeaderFile;    // The actual pch header (.h) file (no path information)
		public PathString PrecompiledHeaderDir; // Directory to the header (.h) file.
		public PathString PrecompiledFile;      // The precompiled binary object file (.pch)

		public string RootNamespace;
		public string ImportMSBuildProjects;

		public FileSet NuGetReferences; // needs to work for C++/CLI

		public FileSet CustomBuildRuleFiles;
		public OptionSet CustomBuildRuleOptions;

		public FileSet ResourceFiles;
		public FileSet NatvisFiles;
		public FileSet JavaSourceFiles;
		public FileSet ShaderFiles;

		public string TargetFrameworkVersion => DotNetTargeting.GetNetVersion(Package.Project, TargetFrameworkFamily);
		
		public DotNetFrameworkFamilies TargetFrameworkFamily
		{
			get
			{
				if (!IsKindOf(Managed))
				{
					// I really wish this was compile time enforce but Module_Native and managed C++ are too tied together at this point
					throw new InvalidOperationException("Trying to access target .NET family of a non-managed module.");
				}
				if (!m_targetFrameworkFamily.HasValue)
				{
					throw new InvalidOperationException("Trying to access target .NET family before buildgraph resolution of family.");
				}
				return m_targetFrameworkFamily.Value;
			}
			set
			{
				if (!IsKindOf(Managed))
				{
					// I really wish this was compile time enforce but Module_Native and managed C++ are too tied together at this point
					throw new InvalidOperationException("Trying set .NET family of a non-managed module.");
				}
				m_targetFrameworkFamily = value;
			}
		}
		private DotNetFrameworkFamilies? m_targetFrameworkFamily = null;

		public override CopyLocalType CopyLocal
		{
			get
			{
				return m_copyLocal;
			}
		} private CopyLocalType m_copyLocal;

		public override CopyLocalInfo.CopyLocalDelegate CopyLocalDelegate 
		{
			get
			{
				if (Configuration.System == "pc" || Configuration.System == "pc64")
				{
					// On PC platform, we can have a native module depends on a Managed/DotNet module and we
					// still want the dependency
					return CopyLocalInfo.CommonDelegate;
				}
				else
				{
					// For non-pc platforms, there are no Managed/DotNet modules.
					return CopyLocalInfo.NativeDelegate;
				}
			}
		}

		public override bool CopyLocalUseHardLink
		{
			get
			{
				return m_copyLocalUseHardLink;
			}
		} private bool m_copyLocalUseHardLink;

		public CcCompiler Cc
		{
			get { return _cc; }
			set 
			{ 
				SetTool(value); 
				_cc = value; 
			}
		} private CcCompiler _cc;

		public AsmCompiler Asm
		{
			get { return _as; }
			set
			{
				SetTool(value);
				_as = value;
			}
		} private AsmCompiler _as;

		public Linker Link
		{
			get { return _link; }
			set
			{
				SetTool(value);
				_link = value;
			}
		} private Linker _link;

		public PostLink PostLink
		{
			get { return _postlink; }
			set
			{
				ReplaceTool(_postlink, value);
				_postlink = value;
			}
		} private PostLink _postlink;

		public Librarian Lib
		{
			get { return _lib; }
			set
			{
				SetTool(value);
				_lib = value;
			}
		} private Librarian _lib;

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
