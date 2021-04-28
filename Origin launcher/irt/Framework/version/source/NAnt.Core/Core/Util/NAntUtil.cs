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
using System.Diagnostics;
using System.Reflection;
#if NANT_CONCURRENT
using System.Threading.Tasks;
#endif

namespace NAnt.Core.Util
{
	public static class NAntUtil
	{
        public static readonly string NantVersionString;

        // applies to masterconfig group, package and version nmes
        private static readonly char[] s_invalidNameChars = new char[] { '"', '\\', '/', ':', '*', '?', '"', '<', '>', '|', '"' }; // always invalid
        private static readonly char[] s_invalidStartChars = new char[] { '.', ' ' };                                              // names may not start with these chars
        private static readonly char[] s_invalidEndChars = s_invalidStartChars;                                                    // names may not end with these chars

        static NAntUtil()
		{
			Assembly nantCoreAssembly = Assembly.GetExecutingAssembly();
			FileVersionInfo info = FileVersionInfo.GetVersionInfo(nantCoreAssembly.Location);
			NantVersionString = String.Format("{0}.{1:D2}.{2:D2}",
								info.FileMajorPart, info.FileMinorPart, info.FileBuildPart);
		}

        public static bool IsPackageNameValid(string name, out string invalidReason)
        {
            // underscore name, only applies to package names
            if (String.Equals(name, "_", StringComparison.Ordinal))
            {
                invalidReason = "Name cannot equal '_'.";
                return false;
            }

			return IsNameValid(name, out invalidReason, s_invalidNameChars, s_invalidStartChars, s_invalidEndChars);
        }

        public static bool IsPackageVersionStringValid(string version, out string invalidReason)
        {
            return IsNameValid(version, out invalidReason, s_invalidNameChars, s_invalidStartChars, s_invalidEndChars);
        }

        public static bool IsMasterconfigGroupTypeStringValid(string name, out string invalidReason)
		{
            return IsNameValid(name, out invalidReason, s_invalidNameChars, s_invalidStartChars, s_invalidEndChars);
        }

        private static bool IsNameValid(string name, out string invalidReason, char[] inValidNameChars, char[] invalidStartChars, char[] invalidEndChars)
        {
            // empty name
            if (String.IsNullOrEmpty(name))
            {
                invalidReason = "Name is empty.";
                return false;
            }

            // always invalid characters
            foreach (char invalidChar in inValidNameChars)
            {
                if (name.IndexOf(invalidChar) >= 0)
                {
                    invalidReason = String.Format("Name contains invalid char '{0}'.", invalidChar);
                    return false;
                }
            }

            // invalid start characters
            foreach (char invalidChar in invalidStartChars)
            {
                if (name.StartsWith(invalidChar.ToString(), StringComparison.Ordinal))
                {
                    invalidReason = String.Format("Name starts with invalid char '{0}'.", invalidChar);
                    return false;
                }
            }

            // invalid end characters
            foreach (char invalidChar in invalidEndChars)
            {
                if (name.EndsWith(invalidChar.ToString(), StringComparison.Ordinal))
                {
                    invalidReason = String.Format("Name ends with invalid char '{0}'.", invalidChar);
                    return false;
                }
            }

            invalidReason = null;
            return true;
        }

		public static void ParallelForEach<T>(IEnumerable<T> source, Action<T> action, bool noParallelOnMono = false, bool forceDisableParallel = false)
		{
#if NANT_CONCURRENT
			// Under mono most parallel execution is slower than consecutive. Until this is resolved use consecutive execution 
			bool parallel = (Project.NoParallelNant == false) &&
							(noParallelOnMono == false || PlatformUtil.IsMonoRuntime == false) &&
							forceDisableParallel == false; // this should never bet set in regular code - but it's useful to temporarily set when debugging

			if (parallel)
			{
				Parallel.ForEach(source, g =>
				{
					action(g);
				});
			}
			else
#endif
			{
				foreach (T item in source)
				{
					action(item);
				}
			}
		}
	}
}
