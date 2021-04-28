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

using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;

using NAnt.Shared.Properties;

namespace NAnt.Core.Util
{
	public static class DeprecationUtil
	{
		// get a deprecation key for the callsite of a deprecated function
		public static IEnumerable<object> GetCallSiteKey(Location location = null, uint frameOffset = 0)
		{
			return new object[] { GetCallSiteLocation(location, frameOffset + 1) };
		}

		public static IEnumerable<object> PackageConfigKey(Project project)
		{
			string package = project.Properties[PackageProperties.PackageNamePropertyName];
			string config = project.Properties[PackageProperties.ConfigNameProperty];
			if (package == null || config == null)
			{
				return Enumerable.Empty<string>();
			}
			else
			{
				return new object[] { package, config };
			}
		}

		public static IEnumerable<object> TaskLocationKey(Task task)
		{
			return new object[] { task.Location };
		}

		// get location for reporting purposes
		public static Location GetCallSiteLocation(Location currentLocation = null, uint frameOffset = 0)
		{
			if (currentLocation != null && currentLocation != Location.UnknownLocation)
			{
				return currentLocation;
			}

			StackFrame callingFrame = new StackTrace((int)(2 + frameOffset), fNeedFileInfo: true).GetFrame(0); // go up 2 frames + offset - one for this function, one for deprecated function, then we have the frame calling deprecated function
			if (callingFrame.GetFileName() != null)
			{
				return new Location(callingFrame.GetFileName(), callingFrame.GetFileLineNumber(), callingFrame.GetFileColumnNumber());
			}
			else if (callingFrame.GetMethod() != null)
			{
				// if we can't resolve file, use calling method name
				return new Location(callingFrame.GetMethod().DeclaringType.FullName + "." + callingFrame.GetMethod().Name);
			}
			else
			{
				// pdb probably isn't available, return unknown location
				return Location.UnknownLocation;
			}
		}
	}
}
