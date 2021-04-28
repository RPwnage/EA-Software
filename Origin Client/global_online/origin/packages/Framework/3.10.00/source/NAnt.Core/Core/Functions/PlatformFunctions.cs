// NAnt - A .NET build tool
// Copyright (C) 2001-2009 Gerry Shaw
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
// Clayton Harbour (charbour@ea.com)
using System;

using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Util;

namespace NAnt.Core.Functions
{
    /// <summary>
    /// Collection of string manipulation routines. 
    /// </summary>
    [FunctionClass("Platform Functions")]
    public class PlatformFunctions : FunctionClassBase
    {
        /// <summary>
        /// Returns 'true' if the current platform is Linux.
        /// </summary>
        /// <param name="project"></param>
        /// <returns>'true' if the current platform is Linux, otherwise 'false'.</returns>
        [Function()]
        public static string PlatformIsUnix(Project project)
        {
            if (PlatformUtil.IsUnix)
            {
                return Boolean.TrueString;
            }
            return Boolean.FalseString;
        }

        /// <summary>
        /// Returns 'true' if the current platform is Windows.
        /// </summary>
        /// <param name="project"></param>
        /// <returns>'true' if the current platform is Windows, otherwise 'false'.</returns>
        [Function()]
        public static string PlatformIsWindows(Project project)
        {
            if (PlatformUtil.IsWindows)
            {
                return Boolean.TrueString;
            }
            return Boolean.FalseString;
        }

        /// <summary>
        /// Returns 'true' if the current platform is OSX.
        /// </summary>
        /// <param name="project"></param>
        /// <returns>'true' if the current platform is Windows, otherwise 'false'.</returns>
        [Function()]
        public static string PlatformIsOSX(Project project)
        {
            if (PlatformUtil.IsOSX)
            {
                return Boolean.TrueString;
            }
            return Boolean.FalseString;
        }

        /// <summary>
        /// Returns the Operating System we are running on.
        /// </summary>
        /// <returns>name of the platform. Supported values are: Windows, Unix, OSX, Xbox, Unknown. </returns>
        [Function()]
        public static string PlatformPlatform(Project project)
        {
            return PlatformUtil.Platform;
        }
    }
}
