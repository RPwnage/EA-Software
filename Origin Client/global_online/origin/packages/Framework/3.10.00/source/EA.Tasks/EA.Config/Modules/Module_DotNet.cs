using System;
using System.Collections.Generic;
using System.Linq;
using System.IO;

using NAnt.Core;
using NAnt.Core.Util;
using EA.FrameworkTasks.Model;
using EA.Eaconfig.Core;

using EA.Eaconfig.Modules.Tools;


namespace EA.Eaconfig.Modules
{
    // Project types are unique powers of two so that multiple types can be applied to a bitmask
    public enum DotNetProjectTypes { Workflow = 1, UnitTest = 2, WebApp = 4};

    public enum DotNetGenerateSerializationAssembliesTypes { None, Auto, On, Off };

    public class Module_DotNet : ProcessableModule
    {
        
        internal Module_DotNet(string name, string groupname, string groupsourcedir, Configuration configuration, BuildGroups buildgroup, BuildType buildType, IPackage package)
            : base(name, groupname, groupsourcedir, configuration, buildgroup, buildType, package)
        {
            SetType(Module.DotNet);
            CopyLocal = CopyLocalType.False;
        }

        public CopyLocalType CopyLocal;

        public string TargetFrameworkVersion;

        public string RootNamespace;

        public string ApplicationManifest;

        public PathString AppDesignerFolder;

        public DotNetProjectTypes ProjectTypes = 0;

        public bool DisableVSHosting;

        public DotNetGenerateSerializationAssembliesTypes GenerateSerializationAssemblies;

        public string ImportMSBuildProjects;

        public OptionSet WebReferences;


        //IMTODO: I need to move this into postbuild tool to be consistent in native build (if anybody cares about it anymore).
        public string RunPostBuildEventCondition;

        public bool IsProjectType(DotNetProjectTypes test)
        {
            // using bitwise & operator to test a single bit of the bitmask
            return (ProjectTypes & test) == test;
        }

        public DotNetCompiler Compiler
        {
            get { return _compiler; }
            set
            {
                SetTool(value);
                _compiler = value;
            }
        } private DotNetCompiler _compiler;


        public override void Apply(IModuleProcessor processor)
        {
            processor.Process(this);
        }

    }
}

