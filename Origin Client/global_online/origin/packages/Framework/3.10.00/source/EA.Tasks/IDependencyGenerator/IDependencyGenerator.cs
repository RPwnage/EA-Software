using System;
using System.Collections.Specialized;
using System.IO;
using NAnt.Core.Reflection;

namespace EA
{
    namespace DependencyGenerator
    {
        public abstract class IDependencyGenerator : IDisposable
        {
            public abstract void Dispose();

            public abstract void Reset();

            public abstract void AddIncludePath(String includePath);

            public abstract void AddIncludePath(String includePath, bool systemInclude);

            public abstract void AddUsingPath(String usingPath);

            public abstract void AddDefine(String define);

            public abstract StringCollection ParseFile(String path);

            public abstract bool SkipDuplicates { get; set; }

            public abstract bool ShowDuplicates { get; set; }

            public abstract bool IgnoreMissingIncludes { get; set; }

            public abstract bool SuppressWarnings { get; set; }

            // when using DependencyGenerator.GCC path to compiler generated dependency file.
            public virtual string DependencyFilePath
            {
                get { return String.Empty; }
                set { }
            }

        }

        public class DependencyGeneratorFactory
        {
            private static System.Reflection.Assembly _dependencyAssembly = GetDependencyAssembly();

            private static string depAssemblyName = "";

            public static IDependencyGenerator Create()
            {
                IDependencyGenerator dependencyGenerator = null;

                System.Reflection.Assembly dependencyAssembly = GetDependencyAssembly();

                if (dependencyAssembly != null)
                {
                    dependencyGenerator = dependencyAssembly.CreateInstance("EA.DependencyGenerator.DependencyGenerator") as IDependencyGenerator;

                    if (dependencyGenerator == null)
                    {
                        throw new ApplicationException(String.Format("Failed to create DependencyGenerator instance from assembly {0}", depAssemblyName));
                    }

                }

                return dependencyGenerator;
            }

            private static System.Reflection.Assembly GetDependencyAssembly()
            {
                if (_dependencyAssembly == null)
                {
                    switch (Environment.OSVersion.Platform)
                    {
                        case PlatformID.MacOSX:
                        case PlatformID.Unix:
                            depAssemblyName = "EA.DependencyGenerator.GCC.dll";
                            break;
                        case PlatformID.Xbox:
                            depAssemblyName = String.Empty;
                            break;
                        case PlatformID.Win32NT:
                        case PlatformID.Win32S:
                        case PlatformID.Win32Windows:
                            depAssemblyName = System.IntPtr.Size == 8 ? "EA.DependencyGenerator_64.dll" : "EA.DependencyGenerator_32.dll";
                            break;
                        default:
                            depAssemblyName = String.Empty;
                            break;
                    }
                    if (!String.IsNullOrEmpty(depAssemblyName))
                    {
                        string depPath = Path.GetDirectoryName(System.Reflection.Assembly.GetExecutingAssembly().Location);
                        depPath = Path.Combine(depPath, depAssemblyName);
                        _dependencyAssembly = AssemblyLoader.Get(depPath);
                    }
                }

                return _dependencyAssembly;
            }
        }
    }
}
