using System;
using System.Linq;
using System.Collections.Concurrent;
using System.Reflection;
using System.IO;
using System.Xml;
using System.Threading;
using System.Threading.Tasks;
using System.Diagnostics;


using NAnt.Core.Util;
using NAnt.Core.Threading;
using NAnt.Core.Attributes;
using NAnt.Core.Events;
using System.Collections.Generic;

namespace NAnt.Core.Reflection
{
    public static class AssemblyLoader
    {
        static AssemblyLoader()
        {
            AppDomain.CurrentDomain.AssemblyResolve += ResolveHandler;
        }

        public static Assembly Get(string path, bool fromMemory = false)
        {
            AssemblyCacheInfo assembly;
            if (_cache.TryGetValue(Path.GetFileName(path), out assembly))
                return assembly.Assembly;

            return Load(path, fromMemory);
        }

        public static bool TryGetCached(string path, out Assembly assembly)
        {
            
            AssemblyCacheInfo assemblyInfo;
            if(_cache.TryGetValue(Path.GetFileName(path), out assemblyInfo))
            {
                assembly = assemblyInfo.Assembly;
                return true;
            }
            assembly = null;
            return false;
        }

        public static void PreloadAssembliesContaining(params Type[] types) { PreloadAssembliesContaining((IEnumerable<Type>)types); }

        public static void PreloadAssembliesContaining(IEnumerable<Type> types)
        {
            if (PlatformUtil.IsMonoRuntime)
            {
                foreach(var type in types)
                {
                    var assembly = type.Assembly;
                    Add(assembly, assembly.Location);
                }
            }
            else
            {
                Parallel.ForEach(types, type =>
                {
                    var assembly = type.Assembly;
                    Add(assembly, assembly.Location);
                });
            }
        }

        public static bool isLoaded(string filename)
        {
            // In case this assembly has been previously loaded by other tasks, only
            // add this assembly file if it is not already loaded in CurrentDomain.
            bool alreadyLoaded = false;
            try
            {
                string asmFilename = Path.GetFileName(filename);
                foreach (Assembly asm in AppDomain.CurrentDomain.GetAssemblies())
                {
                    if (!asm.IsDynamic)
                    {
                        string currDomainAsmFilename = Path.GetFileName(asm.Location);
                        if (String.Compare(asmFilename, currDomainAsmFilename) == 0)
                        {
                            alreadyLoaded = true;
                            break;
                        }
                    }
                }
            }
            catch (NotSupportedException)
            {
                // Ignore - this error is sometimes thrown by asm.Location for certain dynamic assemblies
            }

            return alreadyLoaded;
        }

        public static IEnumerable<AssemblyLoader.AssemblyCacheInfo> GetAssemblyCacheInfo()
        {
            var info = new List<AssemblyLoader.AssemblyCacheInfo>();
            foreach (var e in _cache)
            {
                info.Add(e.Value);
            }
            return info;
        }

        public static void Add(Assembly assembly, string path, string name = null)
        {
            if (_cache.TryAdd(name ?? assembly.GetName().Name + ".dll", new AssemblyCacheInfo() { Assembly = assembly, Path = path }))
                // This fills the reflection cache. We will deal with any errors later in task Factory.
                new System.Threading.Tasks.Task(() => { try { assembly.GetTypes(); } catch (Exception) { } }).Start();
        }

        // Directory.EnumerateFiles() on Mono skips readonly files. 
        // Provide custom implementation;
        private static string FindFile(string path, string pattern, SearchOption searchoption)
        {
             var file = Directory.GetFiles(path, pattern).FirstOrDefault();

             if (SearchOption.AllDirectories == searchoption && file == null)
             {
                 foreach (var dir in Directory.GetDirectories(pattern, "*", SearchOption.AllDirectories))
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

        private static Assembly Load(string path, bool fromMemory = false)
        {
            if (!Path.IsPathRooted(path))
            {
                path = FindFile(Path.GetDirectoryName(Assembly.GetEntryAssembly().Location),
                                                Path.GetFileName(path),
                                                SearchOption.AllDirectories) ?? path;
            }

            if (path == null)
                throw new Exception("Error, unable to resolve assembly: " + path);

            // Let fusion resolve file name;
            if (fromMemory && !File.Exists(path))
            {
                fromMemory = false;
            }

            var assembly = AppDomain.CurrentDomain.GetAssemblies().SingleOrDefault( a => 
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

            if(assembly == null)
            {
                assembly = fromMemory ? Assembly.Load(File.ReadAllBytes(path)) : Assembly.LoadFrom(path);
            }

            Add(assembly , path, fromMemory ? Path.GetFileName(path) : null);

            return assembly;
        }

        private static Assembly OnResolve(object sender, ResolveEventArgs args)
        {
            Assembly assembly = null;

            // Try to find required assembly in our cache:
            int ind = args.Name.IndexOf(",");
            string name = (ind > 0 ? args.Name.Substring(0, args.Name.IndexOf(",")) : args.Name) + ".dll";
            AssemblyCacheInfo assemblyInfo;
            if (_cache.TryGetValue(name, out assemblyInfo))
            {
                assembly = assemblyInfo.Assembly;
            }
            {
                // Try to find loaded assembly:

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

        private static ResolveEventHandler ResolveHandler = new ResolveEventHandler(OnResolve);

        private static ConcurrentDictionary<string, AssemblyCacheInfo> _cache = new ConcurrentDictionary<string, AssemblyCacheInfo>();

        public struct AssemblyCacheInfo
        {
            public Assembly Assembly;
            public string Path;
        }
    }
}
