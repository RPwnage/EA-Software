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
using EA.FrameworkTasks.Model;

namespace EA.Make
{
	public static class MakeExtensions
	{

		public static string MakeFileName(this IModule module)
		{
			return String.Format("{0}.nant.mak", module.Name);
		}

		public static string MakeFileDir(this IModule module)
		{
			return module.IntermediateDir.Path;
		}

		public static string ToMakeValue(this string variable)
		{
			return "$(" + variable + ")";
		}

		public static string MkDir(this string dir)
		{
			if (!String.IsNullOrEmpty(dir))
			{
				if (PlatformUtil.IsWindows)
				{
					bool use_nant_mkdir = true;
					if (use_nant_mkdir)
					{
						return String.Format("-@$(MKDIR) /es {0}", dir.Quote());
					}
					else
					{
						return String.Format(@"-@if not exist $(subst /,\,{0}) mkdir $(subst /,\,{0})", dir);
					}
				}
				else
				{
					return String.Format(@"-@mkdir -p {0}", dir);
				}
			}
			return String.Empty;
		}

		public static string DelFile(this string file)
		{
			if (!String.IsNullOrEmpty(file))
			{

				if (PlatformUtil.IsWindows)
				{
					return String.Format(@"@if exist $(subst /,\,{0}) del /F $(subst /,\,{0})", file);
				}
				else
				{
					return String.Format(@"-@rm -f {0}", file);
				}
			}
			return String.Empty;
		}

		public static string QuoteOptionAsNeeded(this string option)
		{
			var res = option;
			if(!String.IsNullOrWhiteSpace(res))
			{
				if(-1 != res.IndexOfAny(OPTION_CHARTS_TO_QUOTE))
				{
					var quotechar = '"';
					if(-1 != res.IndexOf('"'))
					{
						quotechar = '\'';
					}
					res = quotechar + res + quotechar;
				}
			}
			return res;
		}

		private static readonly char[] OPTION_CHARTS_TO_QUOTE = new char[] { '<', '>', '|'};

	}
}
