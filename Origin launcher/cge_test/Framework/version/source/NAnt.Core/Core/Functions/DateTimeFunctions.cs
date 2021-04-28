// NAnt - A .NET build tool
// Copyright (C) 2003-2015 Electronic Arts Inc.
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
// Electronic Arts (Frostbite.Team.CM@ea.com)
// Kosta Arvanitis (karvanitis@ea.com)
using System;
using NAnt.Core.Attributes;

namespace NAnt.Core.Functions
{
	/// <summary>
	/// Collection of time and date manipulation routines.
	/// </summary>
	[FunctionClass("DateTime Functions")]
	public class DateTimeFunctions: FunctionClassBase
	{
		/// <summary>
		/// Gets the date and time that is the current local date and time on this computer.  NOTE: The output format will depend on your current OS's locale setting!
		/// </summary>
		/// <param name="project"></param>
		/// <returns>The current local date and time on this computer.</returns>
		/// <include file='Examples/DateTime/DateTimeNow.example' path='example'/>        
		[Function()]
		public static string DateTimeNow(Project project)
		{
			return DateTime.Now.ToString();
		}

		/// <summary>
		/// Gets the date and time that is the current local date and time on this computer expressed as coordinated universal time (UTC).  NOTE: The output format will depend on your current OS's locale setting!
		/// </summary>
		/// <param name="project"></param>
		/// <returns>The current local date and time on this computer expressed as UTC.</returns>
		/// <include file='Examples/DateTime/DateTimeUtcNow.example' path='example'/>        
		[Function()]
		public static string DateTimeUtcNow(Project project)
		{
			return DateTime.UtcNow.ToString();
		}

		/// <summary>
		/// Gets the date and time that is the current local date and time on this computer expressed as a file timestamp.  NOTE: The output format will depend on your current OS's locale setting!
		/// </summary>
		/// <param name="project"></param>
		/// <returns>The current local date and time on this computer expressed as UTC.</returns>
		/// <include file='Examples/DateTime/DateTimeNowAsFiletime.example' path='example'/>        
		[Function()]
		public static string DateTimeNowAsFiletime(Project project)
		{
			return DateTime.Now.ToFileTime().ToString();
		}


		/// <summary>
		/// Gets the current date.  NOTE: The output format will depend on your current OS's locale setting!
		/// </summary>
		/// <param name="project"></param>
		/// <returns>The current date and time with the time part set to 00:00:00.</returns>
		/// <include file='Examples/DateTime/DateTimeToday.example' path='example'/>        
		[Function()]
		public static string DateTimeToday(Project project)
		{
			return DateTime.Today.ToString();
		}

		/// <summary>
		/// Returns the current day of the week.  NOTE: This function always returns the name in English.
		/// </summary>
		/// <param name="project"></param>
		/// <returns>The current day of the week.</returns>
		/// <include file='Examples/DateTime/DateTimeDayOfWeek.example' path='example'/>        
		[Function()]
		public static string DateTimeDayOfWeek(Project project)
		{
			return DateTime.Today.DayOfWeek.ToString();
		}


		/// <summary>
		/// Compares two datetime stamps.  NOTE: The input DateTime format is expected to match your OS's locale setting or in ISO 8601 format that DateTime.Parse can handle.
		/// </summary>
		/// <param name="project"></param>
		/// <param name="t1">The first datetime stamp.</param>
		/// <param name="t2">The second datetime stamp.</param>
		/// <returns>
		///		Less than zero - t1 is less than t2.  &lt;br/&gt;
		///		Zero - t1 equals t2. &lt;br/&gt;
		///		Greater than zero - t1 is greater than t2. &lt;br/&gt;
		/// </returns>
		/// <include file='Examples/DateTime/DateTimeCompare.example' path='example'/>        
		[Function()]
		public static string DateTimeCompare(Project project, DateTime t1, DateTime t2)
		{
			return DateTime.Compare(t1, t2).ToString();
		}
		
	}
}