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
using System.Globalization;
using System.Management.Automation.Host;

namespace NAnt.NuGet.Deprecated
{

	// this class basically reimplements Microsoft.PowerShell.DefaultHost (internal class)
	// but with a few tweaks so we can pretend to be visual studio's package manager
	// prompt
	internal class PretendPackageConsoleHost : PSHost
	{
		public override string Name { get { return "Package Manager Host"; } } // some scripts check specifically for this name
		public override Guid InstanceId { get { return m_guid; } }
		public override Version Version { get { return m_version; } }

		public override CultureInfo CurrentCulture { get { return m_cultureInfo; } }
		public override CultureInfo CurrentUICulture { get { return m_uiCultureInfo; } }
		public override PSHostUserInterface UI { get { return m_UI; } }

		private readonly Version m_version = new Version(2, 0);
		private readonly Guid m_guid = Guid.NewGuid();
		private readonly CultureInfo m_cultureInfo;
		private readonly CultureInfo m_uiCultureInfo;
		private readonly PSHostUILogRedirect m_UI;

		internal PretendPackageConsoleHost(CultureInfo cultureInfo, CultureInfo uiCultureInfo)
		{
			m_cultureInfo = cultureInfo;
			m_uiCultureInfo = uiCultureInfo;
			m_UI = new PSHostUILogRedirect();
		}

		// unsupported functions
		public override void EnterNestedPrompt() { throw new NotSupportedException(); }
		public override void ExitNestedPrompt() { throw new NotSupportedException(); }
		public override void NotifyBeginApplication() { throw new NotSupportedException(); }
		public override void NotifyEndApplication() { throw new NotSupportedException(); }
		public override void SetShouldExit(int exitCode) { throw new NotSupportedException(); }
	}
}