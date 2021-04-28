using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;

using NAnt.Core;
using NAnt.Core.Util;
using EA.FrameworkTasks.Model;
using EA.Eaconfig.Core;

using EA.Eaconfig.Modules.Tools;

namespace EA.Eaconfig.Modules
{
    public class Module_Native : ProcessableModule
    {
        internal Module_Native(string name, string groupname, string groupsourcedir, Configuration configuration, BuildGroups buildgroup, BuildType buildType, IPackage package)
            : base(name, groupname, groupsourcedir, configuration, buildgroup, buildType, package)
        {
            SetType(Module.Native);
            CopyLocal = CopyLocalType.False;
        }

        public override void Apply(IModuleProcessor processor)
        {
            processor.Process(this);
        }

        public string PrecompiledHeaderFile;    // Precomp header file .
        public PathString PrecompiledFile;     // Precompiled obj file. (.pch)
        public CopyLocalType CopyLocal;
        public string TargetFrameworkVersion;
        public string RootNamespace;

        public FileSet CustomBuildRuleFiles;
        public OptionSet CustomBuildRuleOptions;

        public CcCompiler Cc
        {
            get { return _cc; }
            set 
            { 
                SetTool(value); 
                _cc = value; 
            }
        } private CcCompiler _cc;

        public AsmCompiler Asm
        {
            get { return _as; }
            set
            {
                SetTool(value);
                _as = value;
            }
        } private AsmCompiler _as;

        public Linker Link
        {
            get { return _link; }
            set
            {
                SetTool(value);
                _link = value;
            }
        } private Linker _link;

        public PostLink PostLink
        {
            get { return _postlink; }
            set
            {
                ReplaceTool(_postlink, value);
                _postlink = value;
            }
        } private PostLink _postlink;

        public Librarian Lib
        {
            get { return _lib; }
            set
            {
                SetTool(value);
                _lib = value;
            }
        } private Librarian _lib;
    }
}
