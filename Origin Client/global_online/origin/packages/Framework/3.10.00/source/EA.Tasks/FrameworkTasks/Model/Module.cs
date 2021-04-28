using System;
using System.Collections.Generic;

using NAnt.Core;
using NAnt.Core.Util;

namespace EA.FrameworkTasks.Model
{
    public class Module : BitMask, IModule
    {
        public const uint Native = 1;
        public const uint DotNet = 2;
        public const uint WinRT = 4;
        public const uint Utility = 8;
        public const uint MakeStyle = 16;
        public const uint ExternalVisualStudio = 32;

        public const uint Managed = 64;
        public const uint CSharp = 128;
        public const uint FSharp = 256;
        public const uint EASharp = 512;

        public const uint Program = 1024;
        public const uint Library = 2048;
        public const uint DynamicLibrary = 4096;

        public const uint ExcludedFromBuild = 8192;
        public const uint Python = 16384;

        public static readonly IModuleEqualityComparer EqualityComparer = new IModuleEqualityComparer();

        protected Module(string name, string groupname, string groupsourcedir, Configuration configuration, BuildGroups buildgroup, IPackage package, bool isreal=true)
        {
            _name = name;
            _groupname = groupname;
            _groupsourcedir = groupsourcedir;
            _buildgroup = buildgroup;
            _configuration = configuration;
            _package = package;
            _key = Module.MakeKey(name, buildgroup, package);

            if (configuration.Name != package.ConfigurationName)
            {
                string msg = String.Format("Package '{0}-{1}' configuration name '{2}' does not match module '{3}' configuration name '{4}'.", package.Name, package.Version, package.ConfigurationName, name, configuration.Name);
                throw new BuildException(msg);
            }

            _outputname = Name;
            
            if (isreal)
            {
                _dependents = new DependentCollection(this);
                BuildSteps = new List<BuildStep>();
                _tools = new List<Tool>();

                Assets = new FileSet();
                ExcludedFromBuild_Files = new FileSet();
                ExcludedFromBuild_Sources = new FileSet();

                _package.AddModule(this);

                if (_package.Project != null)
                {
                    _outputname = _package.Project.Properties[groupname + ".outputname"] ?? Name;
                }
            }
            else
            {
                _dependents = new DependentCollection(this);
            }
        }

        public static string MakeKey(string name, BuildGroups group, IPackage package)
        {
            return package.Key + ":" + group + "-" + name;
        }

        public static string MakeKey(string name, BuildGroups group, string packageName, string packageVersion, string config)
        {
            return EA.FrameworkTasks.Model.Package.MakeKey(packageName, packageVersion, config) + ":" + group + "-" + name;
        }

        public string Name { get { return _name; } }

        public string GroupName { get { return _groupname; } }

        public string GroupSourceDir { get { return _groupsourcedir; } }

        public BuildGroups BuildGroup { get { return _buildgroup; } }

        public string Key { get { return _key; } }

        public IPackage Package { get { return _package; } }

        public Configuration Configuration { get { return _configuration; } }

        public DependentCollection Dependents { get { return _dependents; } }

        public PathString OutputDir
        {
            get { return _outputdir; }
            set { _outputdir = value; }
        }

        public String OutputName
        {
            get { return _outputname; }
        }

        public PathString IntermediateDir 
        {
            get { return _intermediatedir; }
            set { _intermediatedir = value; }
        }

        public PathString ScriptFile 
        {
            get { return _scriptfile; } 
            set { _scriptfile = value; } 
        }

        public readonly FileSet Assets;

        public readonly List<BuildStep> BuildSteps;

        public IPublicData Public(IModule parentModule)
        {
            IPublicData data = null;

            if (!Package.TryGetPublicData(out data, parentModule, this))
            {
                data = null;
            }
            return data;
        }

        public IEnumerable<Tool> Tools
        {
            get { return _tools; }
        }

        public void SetTool(Tool value)
        {
            if (value != null)
            {
                int ind = (_tools).FindIndex(tool => tool.ToolName == value.ToolName);
                if (ind >= 0)
                {
                    _tools[ind] = value;
                }
                else
                {
                    _tools.Add(value);
                }
            }
        }

        public void ReplaceTool(Tool oldTool, Tool value)
        {
            if (value != null)
            {
                var tname = (oldTool != null) ? oldTool.ToolName : value.ToolName;
                int ind = (_tools).FindIndex(tool => tool.ToolName == tname);
                if (ind >= 0)
                {
                    _tools[ind] = value;
                }
                else
                {
                    _tools.Add(value);
                }
            }
        }


        public void AddTool(Tool value)
        {
            if (value != null)
            {
                _tools.Add(value);
            }
        }

        public bool CollectAutoDependencies 
        {
            get { return IsKindOf(CSharp | FSharp | DynamicLibrary | Program); }
        }

        public ISet<IModule> ModuleSubtree(uint dependencytype, ISet<IModule> modules=null)
        {
            if (modules == null)
            {
                modules = new HashSet<IModule>(Module.EqualityComparer);
            }
            return modules;
        }

        public readonly IDictionary<string, string> MiscFlags = new Dictionary<string, string>();

        private List<Tool> _tools;

        // Data for Visual Studio and other IDEs not directly related to build, but reflected in IDE.
        public readonly FileSet ExcludedFromBuild_Files;
        public readonly FileSet ExcludedFromBuild_Sources;

        private readonly string _name;
        private readonly string _groupname;
        private readonly string _groupsourcedir;
        private readonly BuildGroups _buildgroup;
        private readonly Configuration _configuration;
        private readonly string _key;
        private readonly IPackage _package;
        private readonly DependentCollection _dependents;
        private PathString _intermediatedir;
        private PathString _outputdir;
        private String _outputname;
        private PathString _scriptfile;
    }


    public class IModuleEqualityComparer : IEqualityComparer<IModule>
    {
        public bool Equals(IModule x, IModule y)
        {
            return x.Key == y.Key;
        }

        public int GetHashCode(IModule obj)
        {
            return obj.Key.GetHashCode();
        }
    }
}
