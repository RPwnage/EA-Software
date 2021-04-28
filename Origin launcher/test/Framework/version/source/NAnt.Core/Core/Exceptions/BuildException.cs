// NAnt - A .NET build tool
// Copyright (C) 2003-2015 Electronic Arts Inc.
// Copyright (C) 2001 Gerry Shaw
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
// Gerry Shaw (gerry_shaw@yahoo.com)
// Ian MacLean (ian_maclean@another.com)

using System;

namespace NAnt.Core
{
	/// <summary>
	/// Thrown whenever an error occurs during the build.
	/// </summary>
	public class BuildException : Exception
	{
		public enum StackTraceType
		{
			None,
			XmlOnly,
			Full
		};

		public string Type { get; } = String.Empty; // type is primarily used by <trycatch> tasks that is allowed to catch certain "types" set by the <fail> task
		public string BaseMessage { get { return base.Message; } }
		public Location Location { get; private set; } = Location.UnknownLocation;

		public readonly StackTraceType StackTraceFlags;

		public BuildException(string message, Exception innerException) //TODO: document constructor signature change
			: this(message, Location.UnknownLocation, innerException)
		{
		}

		public BuildException(string message, Location location = null, Exception innerException = null, string type = null, StackTraceType stackTrace = StackTraceType.Full)
			: base(message, innerException)
		{
			Location = location ?? Location.UnknownLocation;
			Type = type ?? String.Empty;
			StackTraceFlags = stackTrace;
		}

		public Location SetLocationIfMissing(Location location)
		{
			if (Location == Location.UnknownLocation)
			{
				Location = location ?? Location.UnknownLocation;
			}
			return Location;
		}

		public static StackTraceType GetBaseStackTraceType(Exception e)
		{
			StackTraceType type = StackTraceType.Full;

			while (e != null)
			{
				if (e is BuildException)
				{
					type = ((BuildException)e).StackTraceFlags;
				}
				e = e.InnerException;
			}
			return type;
		}
	}
}
