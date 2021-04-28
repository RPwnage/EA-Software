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

using System.Collections.Generic;
using System.Linq;
using System.Text;

using NAnt.Core;
using NAnt.Core.Util;
using EA.FrameworkTasks.Model;

using EA.Eaconfig.Build;

namespace EA.Eaconfig.Backends.VisualStudio
{
	internal class FileEntry
	{
		internal class ConfigFileEntry : BitMask
		{
			internal readonly Configuration Configuration;
			internal readonly FileItem FileItem;
			internal readonly Tool Tool;
			internal readonly Project Project;

			internal ConfigFileEntry(Project project, FileItem fileitem, Configuration configuration, Tool tool, uint flags = 0) : base(flags)
			{
				Configuration = configuration;
				FileItem = fileitem;
				Tool = tool;
				Project = project;
			}
		}

		internal const uint Buildable = 1;
		internal const uint ExcludedFromBuild = 2;
		internal const uint BulkBuild = 4;
		internal const uint Headers = 8;
		internal const uint Assets = 16;
		internal const uint Resources = 32;
		internal const uint CopyLocal = 64;
		internal const uint CustomBuildTool = 128;
		internal const uint NonBuildable = 256;
		//VSDotNetProject defines masks starting with 1024.

		internal readonly string FileSetBaseDir;
		internal readonly PathString Path;
		internal readonly List<ConfigFileEntry> ConfigEntries = new List<ConfigFileEntry>();
		internal readonly List<CopyLocalInfo> CopyLocalOperations = new List<CopyLocalInfo>();

		internal FileEntry(PathString path, string filesetbasedir) 
		{
			Path = path;
			FileSetBaseDir = PathNormalizer.Normalize(filesetbasedir, false);
		}

		internal bool TryAddConfigEntry(Project project, FileItem fileitem, Configuration configuration, Tool tool, IEnumerable<CopyLocalInfo> copyOperations, uint flags = 0)
		{
			if (ConfigEntries.Any(obj => obj.Configuration.Name == configuration.Name))
			{
				return false;
			}

			CopyLocalOperations.AddRange(copyOperations);
			ConfigEntries.Add(new ConfigFileEntry(project, fileitem, configuration, tool, flags));

			return true;
		}

		internal ConfigFileEntry FindConfigEntry(Configuration configuration)
		{
			return ConfigEntries.Find(obj => obj.Configuration.Name == configuration.Name);
		}


		internal string ConfigSignature
		{
			get
			{
				return ConfigEntries.OrderBy(ce => ce.Configuration.Name).Aggregate(new StringBuilder(), (result, ce)=> result.AppendLine(ce.Configuration.Name + ":" +ce.Type.ToString())).ToString();
			}
		}

		internal bool IsKindOf(Configuration config, uint flags)
		{
			var kind = ConfigEntries.FirstOrDefault(ce=> ce.Configuration == config);

			return (kind != null && kind.IsKindOf(flags));
		}
	}
}
