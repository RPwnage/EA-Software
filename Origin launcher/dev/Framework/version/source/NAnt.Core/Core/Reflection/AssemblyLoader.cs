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
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;

namespace NAnt.Core.Reflection
{
	// TODO, a lot of this class seems to be trying to subvert assembly loading
	// rules to based on simpler names - though it is used as a check when determine to rebuild
	// certain dynamic dlls. Wonder how much is really necessary.

	public static class AssemblyLoader
	{
		public static readonly IEnumerable<Assembly> WellKnownAssemblies;

		private static readonly ConcurrentDictionary<string, AssemblyCacheInfo> s_cache = new ConcurrentDictionary<string, AssemblyCacheInfo>();

		public struct AssemblyCacheInfo
		{
			public Assembly Assembly;
			public string Path;
		}

		static AssemblyLoader()
		{
			AppDomain.CurrentDomain.AssemblyResolve += new ResolveEventHandler(OnResolve);

			// build up well know assemblies
			{
				Assembly thisAssembly = Assembly.GetExecutingAssembly();

				// Add current assembly to the cache.  The following TryLoad() functions will add the 
				// loaded assemblies to the cache as well.  We need to add current assembly to the cache
				// or the intellisense schema generator (which access the cache only) would fail because 
				// it didn't know about NAnt.Core.dll.
				Add(thisAssembly, thisAssembly.Location);

				List<Assembly> wellKnownAssemblies = new List<Assembly>() { thisAssembly };
				// this is a slightly gross dependency inversion where NAnt.Core "knows" about things
				// that depend on it, but doing this here simplifies other code
				if (TryLoad("NAnt.Tasks.dll", out Assembly nantTasksAssembly, Path.GetDirectoryName(thisAssembly.Location)))
				{
					wellKnownAssemblies.Add(nantTasksAssembly);
				}
				if (TryLoad("EA.Tasks.dll", out Assembly eaTasksAssembly, Path.GetDirectoryName(thisAssembly.Location)))
				{
					wellKnownAssemblies.Add(eaTasksAssembly);
				}
				if (TryLoad("NAnt.Intellisense.exe", out Assembly nantIntellisenseAssembly, Path.GetDirectoryName(thisAssembly.Location)))
				{
					wellKnownAssemblies.Add(nantIntellisenseAssembly);
				}
				WellKnownAssemblies = wellKnownAssemblies;
			}
		}

		public static Assembly Get(string path, bool fromMemory = false)
		{
			if (s_cache.TryGetValue(Path.GetFileName(path), out AssemblyCacheInfo assembly))
			{
				return assembly.Assembly;
			}

			return Load(path, fromMemory: fromMemory);
		}

		public static bool TryGetCached(string path, out Assembly assembly)
		{
			if (s_cache.TryGetValue(Path.GetFileName(path), out AssemblyCacheInfo assemblyInfo))
			{
				assembly = assemblyInfo.Assembly;
				return true;
			}
			assembly = null;
			return false;
		}

		public static IEnumerable<AssemblyCacheInfo> GetAssemblyCacheInfo()
		{
			return s_cache.Values;
		}

		public static void Add(Assembly assembly, string path, string name = null)
		{
			if (s_cache.TryAdd(name ?? assembly.GetName().Name + ".dll", new AssemblyCacheInfo() { Assembly = assembly, Path = path }))
			{
				new System.Threading.Tasks.Task(() => { try { assembly.GetTypes(); } catch (Exception) { } }).Start();
			}
		}

		// Directory.EnumerateFiles() on Mono skips readonly files - provide custom implementation;
		private static string FindFile(string path, string pattern, SearchOption searchoption)
		{
			 string file = Directory.GetFiles(path, pattern).FirstOrDefault();

			 if (SearchOption.AllDirectories == searchoption && file == null)
			 {
				 foreach (string dir in Directory.GetDirectories(pattern, "*", SearchOption.AllDirectories))
				 {
					 file = Directory.GetFiles(path, pattern).FirstOrDefault();
					 if (file != null)
					 {
						 break;
					 }
				 }
			 }
			 return file;
		}

		private static bool TryLoad(string path, out Assembly assembly, string relativeTo = null, bool fromMemory = false)
		{
			try
			{
#if NETFRAMEWORK
#else
				// .NET Core exes are stub executables, the real assembly is the dll
				if (path.EndsWith(".exe"))
					path = path.Substring(0, path.Length - 3) + "dll";
#endif
				assembly = Load(path, relativeTo, fromMemory);
				return true;
			}
			catch (Exception ex) when (ex is FileNotFoundException || ex is DirectoryNotFoundException)
			{
				assembly = null;
				return false;
			}
		}

		private static Assembly Load(string path, string relativeTo = null, bool fromMemory = false)
		{
			if (!Path.IsPathRooted(path))
			{
				relativeTo = relativeTo ?? Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location);
				path = FindFile
				(
					relativeTo,
					Path.GetFileName(path),
					SearchOption.AllDirectories
				);

				if (path == null)
				{
					throw new FileNotFoundException("Error, unable to resolve assembly: " + path);
				}
			}

			Assembly assembly = AppDomain.CurrentDomain.GetAssemblies().SingleOrDefault
			( 
				a => 
				{
					try
					{
						if (!a.IsDynamic)
						{
							return String.Equals(a.Location, path, StringComparison.OrdinalIgnoreCase);
						}
					}
					catch (NotSupportedException)
					{
						// Ignore - this error is sometimes thrown by asm.Location for certain dynamic assemblies
					}
					return false;
				}
			);

			if (assembly == null)
			{
				if (fromMemory)
				{
					string pdbPath = Path.ChangeExtension(path, ".pdb");
					try
					{
						byte[] pdbBytes = File.ReadAllBytes(pdbPath);
						byte[] assemblyBytes = File.ReadAllBytes(path);
						assembly = Assembly.Load(assemblyBytes, pdbBytes);
					}
					catch
					{
						byte[] assemblyBytes = File.ReadAllBytes(path);
						assembly = Assembly.Load(assemblyBytes);
					}
				}
				else
				{
					assembly = Assembly.LoadFrom(path);
				}
			}

			Add(assembly, path, fromMemory ? Path.GetFileName(path) : assembly.GetName().Name + ".dll");

			return assembly;
		}

		private static Assembly OnResolve(object sender, ResolveEventArgs args)
		{
			Assembly assembly = null;

			// strip down to just the .dll name
			int ind = args.Name.IndexOf(",");
			string name = (ind > 0 ? args.Name.Substring(0, ind) : args.Name) + ".dll";

			if (s_cache.TryGetValue(name, out AssemblyCacheInfo assemblyInfo))
			{
				assembly = assemblyInfo.Assembly;
			}
			{
				Assembly[] currentAssemblies = AppDomain.CurrentDomain.GetAssemblies();
				for (int i = 0; i < currentAssemblies.Length; i++)
				{
					if (currentAssemblies[i].FullName == args.Name)
					{
						assembly = currentAssemblies[i];
						break;
					}
				}

			}
			return assembly;
		}
	}
}
