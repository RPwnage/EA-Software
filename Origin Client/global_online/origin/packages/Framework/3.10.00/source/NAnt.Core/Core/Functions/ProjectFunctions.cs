// NAnt - A .NET build tool
// Copyright (C) 2001-2002 Gerry Shaw
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
// Kosta Arvanitis (karvanitis@ea.com)
using System;
using System.Collections.Specialized;

using NAnt.Core;
using NAnt.Core.Attributes;

namespace NAnt.Core.Functions
{
	/// <summary>
	/// Collection of NAnt Project routines.
	/// </summary>
	[FunctionClass("Project Functions")]
	public class ProjectFunctions : FunctionClassBase
	{
		/// <summary>
		/// Returns the previous build exception message.
		/// </summary>
		/// <param name="project"></param>
		/// <returns>The previous build exception message that was thrown.</returns>
		/// <remarks>If no error message exists an empty string is returned.</remarks>
        /// <include file='Examples\Project\ProjectGetLastError.example' path='example'/>        
        [Function()]
		public static string ProjectGetLastError(Project project)
		{
			if (project.LastError == null) {
				return String.Empty;
			}
			return project.LastError.Message;
		}

		/// <summary>
		/// Returns the previous inner build exception message.
		/// </summary>
		/// <param name="project"></param>
		/// <returns>The previous inner build exception message that was thrown.</returns>
		/// <remarks>If no error message exists an empty string is returned.</remarks>
        /// <include file='Examples\Project\ProjectGetLastInnerError.example' path='example'/>        
        [Function()]
		public static string ProjectGetLastInnerError(Project project)
		{
			if (project.LastError == null || project.LastError.InnerException == null) {
				return String.Empty;
			}
			return project.LastError.InnerException.Message;
		}

		/// <summary>
		/// Gets the name of the project.
		/// </summary>
		/// <param name="project"></param>
		/// <returns>The project name or empty string if none exists.</returns>
        /// <include file='Examples\Project\ProjectGetName.example' path='example'/>        
        [Function()]
		public static string ProjectGetName(Project project)
		{
			return project.ProjectName;
		}

		/// <summary>
		/// Gets the name of the default target.
		/// </summary>
		/// <param name="project"></param>
		/// <returns>The name of the default target or empty string if none exists.</returns>
        /// <include file='Examples\Project\ProjectGetDefaultTarget.example' path='example'/>        
        [Function()]
		public static string ProjectGetDefaultTarget(Project project)
		{
			if (project.DefaultTargetName == null)
				return String.Empty;
			return project.DefaultTargetName;
		}
	}
}