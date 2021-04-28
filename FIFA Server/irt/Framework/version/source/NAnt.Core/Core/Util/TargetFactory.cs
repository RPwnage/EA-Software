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
using System.Xml;
using System.Collections.Concurrent;

namespace NAnt.Core.Reflection
{
	internal class TargetFactory
	{
		private class TargetInfo
		{
			public Target CachedTarget;
			public DateTime LastWriteTime;

			public TargetInfo(Target target, DateTime lastWriteTime)
			{
				CachedTarget = target;
				LastWriteTime = lastWriteTime;
			}
		};

		private static ConcurrentDictionary<string, Lazy<TargetInfo>> s_targets = new ConcurrentDictionary<string, Lazy<TargetInfo>>();

		internal static Target CreateTarget(Project project, string buildFileDirectory, string filename, XmlNode targetNode, Location location)
		{
			if (String.IsNullOrEmpty(filename))
			{
				return CreateNewTarget(project, buildFileDirectory, targetNode, location);
			}

			string targetName = targetNode.Attributes["name"].Value;

			TargetInfo info = s_targets.GetOrAddBlocking(GetKey(filename, targetName), key =>
			{
				return new TargetInfo(CreateNewTarget(project, buildFileDirectory, targetNode, location), DateTime.Now);
			});

			return info.CachedTarget.Copy();
		}

		private static string GetKey(string filename, string targetName)
		{
			return filename + ":" + targetName;
		}

		private static Target CreateNewTarget(Project project, string buildFileDirectory, XmlNode targetNode, Location location)
		{
			Target target = new Target();
			target.Project = project;
			target.Parent = project;
			target.BaseDirectory = buildFileDirectory;
			if (location != null)
				target.Location = location;
			target.Initialize(targetNode);

			return target;
		}
	}
}
