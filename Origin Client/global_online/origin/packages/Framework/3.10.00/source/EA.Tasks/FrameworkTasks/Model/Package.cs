using System;
using System.Collections.Generic;
using System.Collections.Concurrent;
using System.Linq;

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Attributes;

namespace EA.FrameworkTasks.Model
{
    public class Package : BitMask, IPackage
    {
        public const uint AutoBuildClean = 1; // Should we try to build/clean it
        public const uint Framework2 = 2;
        public const uint Buildable = 4;  // Is package buildable at all (declared in the manifest)
        public const uint FinishedLoading = 8;  

        public Package(string name, string version, PathString dir, string configurationname, FrameworkVersions frameworkVersion)
        {
            _name = name;
            _version = version;
            _dir = dir;
            _configurationname = configurationname;
            _key = MakeKey(name, version, configurationname);
            _frameworkVersion = frameworkVersion;
        }

        public string Name
        {
            get { return _name; }
        }

        public string Version
        {
            get { return _version; }
        }

        public PathString Dir 
        {
            get { return _dir; }
        }

        public string ConfigurationName
        {
            get { return _configurationname; }
        }

        public string Key
        {
            get { return _key; }
        }

        public void AddModule(IModule module)
        {
            _modules.Add(module);
        }

        public IEnumerable<IModule> Modules
        {
            get { return _modules; }
        }

        public Project Project
        {
            get { return _project; }

            set
            {
                if (value != null)
                {
                    if (_project != null)
                    {
                 //       throw new BuildException("INTERNAL ERROR, package project already set");
                    }

                    _project = new Project(value); // Create new local context project
                }
                else
                {
                    _project = null;
                }
            }

        }

        public FrameworkVersions FrameworkVersion 
        {
            get { return _frameworkVersion; }
        }

        public IEnumerable<Dependency<IPackage>> DependentPackages
        {
            get 
            {
                //IM: returning _depPackages.Values causes runtime crash under Mono 2.10.8 
                //    when building some of the Blaze projects. The code below seem to prevent the crash,
                //    however it is not clear whether it is really fixing the problem or just masking it.
                //    THe problem seems to be due to some bug in Mono (JIT bug?)
                var deps = new List<Dependency<IPackage>>();
                foreach (var e in _depPackages)
                {
                    deps.Add(e.Value);
                }
                return deps; 
            }
        }

        internal bool TryAddDependentPackage(IPackage dependent, uint deptype)
        {
            _depPackages.AddOrUpdate(dependent.Key, new Dependency<IPackage>(dependent, BuildGroups.runtime, deptype), (key, val) => { val.SetType(deptype); return val; });

            return true;
        }


        public static string MakeKey(string name, string version, string config)
        {
            return config + "-" + name + "-" + version;
        }

        public bool TryGetPublicData(out IPublicData data, IModule parentModule, IModule dependentModule)
        {
            return _publicdata.TryGetValue(makepublicdatakey(parentModule, dependentModule), out data);
        }

        public bool TryAddPublicData(IPublicData data, IModule parentModule, IModule dependentModule)
        {
            return _publicdata.TryAdd(makepublicdatakey(parentModule, dependentModule), data);
        }

        public IEnumerable<IPublicData> GetModuleAllPublicData(IModule owner)
        {
               return _publicdata.Where(e => e.Key.EndsWith((owner != null) ? owner.Key : "package")).Select(e=>e.Value);
        }

        public PathString PackageBuildDir 
        {
            get { return _packagebuilddir; }
            set { _packagebuilddir = value; }
        }

        public PathString PackageConfigBuildDir
        {
            get { return _packageconfigbuilddir; }
            set { _packageconfigbuilddir = value; }
        }

        public SourceControl SourceControl
        {
            get { return _sourceControl ?? NOT_USING_SOURCE_CONTROL; }
            set { _sourceControl = value; }
        }

        private string makepublicdatakey(IModule parentModule, IModule dependentModule)
        {
            return "public_data_" + parentModule.Key + "_" + dependentModule.Key;
        }

        private readonly string _name;
        private readonly string _version;
        private readonly PathString _dir;
        private readonly string _configurationname;
        private readonly string _key;
        private PathString _packagebuilddir;
        private PathString _packageconfigbuilddir;
        private SourceControl _sourceControl;
        private Project _project;
        private FrameworkVersions _frameworkVersion;

        private readonly List<IModule> _modules = new List<IModule>();
        private readonly ConcurrentDictionary<string, Dependency<IPackage>> _depPackages = new ConcurrentDictionary<string, Dependency<IPackage>>(); 
        private readonly ConcurrentDictionary<string, IPublicData> _publicdata = new ConcurrentDictionary<string, IPublicData>();

        private static readonly SourceControl NOT_USING_SOURCE_CONTROL = new SourceControl();
    }
}
