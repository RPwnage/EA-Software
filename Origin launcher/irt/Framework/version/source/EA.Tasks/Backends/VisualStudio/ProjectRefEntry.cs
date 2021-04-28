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

using NAnt.Core.Util;
using EA.FrameworkTasks.Model;

namespace EA.Eaconfig.Backends.VisualStudio
{
	internal class ProjectRefEntry 
	{
		// these to flags were used to setup VS copy local, however we implement our own and so these should not be used
		// - simply left here to document that these flags had a prior meaning
		/*
		internal const uint CopyLocal = 1;
		internal const uint CopyLocalSatelliteAssemblies = 2;
		*/
		internal const uint LinkLibraryDependencies = 4;
		internal const uint UseLibraryDependencyInputs = 8;
		internal const uint ReferenceOutputAssembly = 16;

		internal class ConfigProjectRefEntry : BitMask
		{
			internal readonly Configuration Configuration;

			internal ConfigProjectRefEntry(Configuration configuration, uint flags = 0) : base(flags)
			{
				Configuration = configuration;
			}
		}

		internal ProjectRefEntry(IVSProject projectRef)
		{
			ProjectRef = projectRef;
			ConfigEntries = new List<ConfigProjectRefEntry>();
		}

		internal bool TryAddConfigEntry(Configuration configuration, uint flags = 0)
		{
			if (-1 != ConfigEntries.FindIndex(obj => obj.Configuration.Name == configuration.Name))
			{
				return false;
			}

			ConfigEntries.Add(new ConfigProjectRefEntry(configuration, flags));

			return true;
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

		internal readonly IVSProject ProjectRef;
		internal readonly List<ConfigProjectRefEntry> ConfigEntries;
	}
}
