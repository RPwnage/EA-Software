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
using System.Text;
using System.Collections.Generic;
using System.Linq;

namespace NAnt.Core.Threading
{
	public static class ThreadUtil
	{
		public static Exception ProcessAggregateException(Exception e)
		{
			Exception res = e;
			if (e is AggregateException)
			{
				AggregateException ae = e as AggregateException;
				res = ae.GetBaseException();
				ae = res as AggregateException;
				if (ae != null)
				{
					//Try to extract root cause exception:
					if (ae.InnerExceptions.Count > 0)
					{
						res = ae.InnerExceptions[0];
						foreach (Exception ex in ae.InnerExceptions)
						{
							if (ex is BuildException)
							{
								res = ex;
								break;
							}
							else if (ex is ContextCarryingException)
							{
								res = ex;
								break;
							}
						}
					}
				}
			}
			return res;
		}

		public static string GetStackTrace(Exception e)
		{
			var output = new StringBuilder();

			if (e is AggregateException)
			{
				AggregateException ae = e as AggregateException;
				ae = ae.Flatten();
				foreach (var inner in ae.InnerExceptions)
				{
					if (!(inner is BuildException))
					{
						output.AppendLine();
						output.AppendLine("********************************************************");
						output.AppendLine("    " + inner.Message);
						output.AppendLine("--------------------------------------------------------");
						output.AppendLine(inner.StackTrace);
						output.AppendLine();
					}
				}
			}
			else if (!(e is BuildException))
			{
				output.AppendLine();
				output.AppendLine("********************************************************");
				output.AppendLine("    " + e.Message);
				output.AppendLine("--------------------------------------------------------");
				output.AppendLine(e.StackTrace);
				output.AppendLine();

			}
			return output.ToString();
		}

		public static void RethrowThreadingException(String message, Location location, Exception e)
		{
			Exception ex = ProcessAggregateException(e);

			if (typeof(BuildException).IsAssignableFrom(ex.GetType()))
			{
				((BuildException)ex).SetLocationIfMissing(location);
				throw new BuildExceptionStackTraceSaver((BuildException)ex);
			}

			var new_be = new BuildException(message, location, ex);
			throw new BuildExceptionStackTraceSaver(new_be);

		}

		public static void RethrowThreadingException(String message, Exception e)
		{
			Exception ex = ProcessAggregateException(e);

			if (ex is BuildException)
			{
				throw new BuildExceptionStackTraceSaver(ex as BuildException);
			}

			var new_ex = new Exception(message, ex);
			throw new ExceptionStackTraceSaver(new_ex);
		}
	}
}
