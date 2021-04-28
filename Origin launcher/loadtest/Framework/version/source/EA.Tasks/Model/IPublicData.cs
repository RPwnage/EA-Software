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

using System.Collections.Generic;

using NAnt.Core;
using NAnt.Core.Util;


namespace EA.FrameworkTasks.Model
{
	public interface IPublicData
	{
		IEnumerable<string> Defines { get; }

		IEnumerable<PathString> IncludeDirectories  { get; }
		IEnumerable<PathString> UsingDirectories { get; }
		IEnumerable<PathString> LibraryDirectories { get; }

		FileSet Assemblies {  get; }
		FileSet ContentFiles { get; set; }
		FileSet DynamicLibraries { get; }
		FileSet Libraries { get; }
		FileSet NatvisFiles { get; }
		FileSet SnDbsRewriteRulesFiles { get; }
	}

	// To allow backward compatibility with having Framework dropping to older version of Frostbite with older eaconfig,
	// we are adding a new interface class.  The current IPublicData interface was being used by 
	// eaconfig's create-prebuilt-package task.  That task don't really need to use IPublicData interface so will
	// be removed in new version of eaconfig to avoid future unnecessary dependency to the Framework version changes.
	// Don't think IPublicData is being used by any other users custom task.
	public interface IPublicDataExtension
	{
		FileSet Programs { get; }
	}

}
