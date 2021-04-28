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

using System;

namespace EA.Eaconfig
{
	[Obsolete("Error class is no longer supported. It is recommend you use standard logging APIs or just throw a BuildException for errors.")]
	public static class Error
	{
		public static void Throw(Project project, string module, string message)
		{
			throw new BuildException(Format(project, module, "ERROR", message));
		}

		public static void Throw(Project project, Location location, string module, string message)
		{
			throw new BuildException(Format(project, module, "ERROR", message), location);
		}

		public static void Throw(Project project, string module, string format, params object[] args)
		{
			string message = String.Format(format, args);
			Throw(project, module, message);
		}

		public static void Throw(Project project, Location location, string module, string format, params object[] args)
		{
			string message = String.Format(format, args);
			Throw(project, location, module, message);
		}

		public static string Format(Project project, string module, string type, string message)
		{
			String package_name = project.Properties["package.name"];
			String package_version = project.Properties["package.version"];
			String build_module = project.Properties["build.module"];

			if (String.IsNullOrEmpty(package_name))
			{
				package_name = "????";
			}
			if (String.IsNullOrEmpty(package_version))
			{
				package_version = "??";
			}
			if (!String.IsNullOrEmpty(build_module) && build_module != package_name)
			{
				package_version += " (" + build_module + ") ";
			}
			String err_header = type;
			if (!String.IsNullOrEmpty(module))
			{
				err_header = type + " in " + module;
			}

			String msg = String.Format("\n  [{0}-{1}] {2} : {3}\n", package_name, package_version, err_header, message);
			return msg;
		}

		public static string Format(Project project, string module, string type, string format, params object[] args)
		{
			string message = String.Format(format, args);
			return Format(project, module, type, message);
		}

		public static string Format(Project project, string module, string type, Location location, string format, params object[] args)
		{
			string message = String.Format(format, args);
			if (location != null && location != Location.UnknownLocation)
			{
				message = location.ToString() + " " + message;
			}
			return Format(project, module, type, message);
		}
	}
}


