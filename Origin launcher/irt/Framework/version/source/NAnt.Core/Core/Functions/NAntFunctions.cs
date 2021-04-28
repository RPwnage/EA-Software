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
// 
// 
// Gerry Shaw (gerry_shaw@yahoo.com)
// Ian MacLean (ian_maclean@another.com)
// Electronic Arts (Frostbite.Team.CM@ea.com)

using System;
using System.Collections.Generic;
using System.Linq;

using NAnt.Core.Attributes;
using NAnt.Core.Logging;
using NAnt.Core.Util;

namespace NAnt.Core.Functions
{
	/// <summary>
	/// Collection of NAnt Project routines.
	/// </summary>
	[FunctionClass("NAnt Functions")]
	public class NAntFunctions : FunctionClassBase
	{
		/// <summary>
		/// Tests whether Framework (NAnt) is running in parallel mode.
		/// </summary>
		/// <param name="project"></param>
		/// <returns>Returns true if Framework is in parallel mode (default).</returns>
		/// <remarks>Parallel mode can be switched off by NAnt command line parameter -noparallel.</remarks>
		[Function()]
		public static string NAntIsParallel(Project project)
		{
			return Project.NoParallelNant ? "false" : "true";
		}

		/// <summary>
		/// Gets log file name paths.
		/// </summary>
		/// <param name="project"></param>
		/// <returns>Returns new line separated list of log file names or an empty string when log is not redirected to a file</returns>
		[Function()]
		public static string GetLogFilePaths(Project project)
		{
			HashSet<string> paths = new HashSet<string>();

			foreach (ILogListener listener in project.Log.Listeners.Concat(project.Log.Listeners))
			{
				FileListener fileListener = listener as FileListener;
				if (fileListener != null)
				{
					paths.Add(fileListener.LogFilePath);
				}
			}

			return paths.OrderedDistinct().ToString(Environment.NewLine);
		}


	}
}