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
using System.IO;

namespace NAnt.Core.Util
{
	public static class Touch
	{
		public static void File(PathString path)
		{
			if (path != null && !String.IsNullOrEmpty(path.Path))
			{
				File(path.Path, DateTime.Now);
			}
		}

		public static void File(string path)
		{
			File(path, DateTime.Now);
		}

		public static void File(PathString path, DateTime touchDateTime)
		{
			if (path != null && !String.IsNullOrEmpty(path.Path))
			{
				File(path.Path, touchDateTime);
			}
		}

		public static void File(string path, DateTime touchDateTime)
		{
			if (!String.IsNullOrEmpty(path))
			{
				bool fileCreated = false;
				try
				{
					// create any directories needed
					Directory.CreateDirectory(Path.GetDirectoryName(path));

					if (!System.IO.File.Exists(path))
					{
						using (FileStream f = System.IO.File.Create(path))
						{
							fileCreated = true;
						}
					}

					// touch should set both write and access time stamp
					System.IO.File.SetLastWriteTime(path, touchDateTime);
					System.IO.File.SetLastAccessTime(path, touchDateTime);

				}
				catch (Exception e)
				{
					if (fileCreated == true && System.IO.File.Exists(path))
					{
						System.IO.File.Delete(path);
					}
					string msg = String.Format("Could not touch file '{0}'.", path);
					throw new BuildException(msg, e);
				}
			}
		}
	}
}
