using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Tasks;

using System;

namespace EA.Eaconfig.Structured
{
    /// <summary>MakeStyle modules are used to exectute external build or clean commands</summary>
    [TaskName("MakeStyle")]
    public class MakeStyleTask : ModuleBaseTask
    {
        /// <summary>A makestyle build command, this should contain executable OS command(s)/script</summary>
        [Property("MakeBuildCommand", Required = false)]
        public ConditionalPropertyElement MakeBuildCommand
        {
            get { return _makebuildelem; }
            set { _makebuildelem = value; }
        } 
        private ConditionalPropertyElement _makebuildelem = new ConditionalPropertyElement();

        /// <summary>A makestyle rebuild command, this should contain executable OS command(s)/script</summary>
        [Property("MakeRebuildCommand", Required = false)]
        public ConditionalPropertyElement MakeRebuildCommand
        {
            get { return _makerebuildelem; }
            set { _makerebuildelem = value; }
        }
        private ConditionalPropertyElement _makerebuildelem = new ConditionalPropertyElement();

        /// <summary>A makestyle clean command, this should contain executable OS command(s)/script</summary>
        [Property("MakeCleanCommand", Required = false)]
        public ConditionalPropertyElement MakeCleanCommand
        {
            get { return _makecleanelem; }
            set { _makecleanelem = value; }
        }
        private ConditionalPropertyElement _makecleanelem = new ConditionalPropertyElement();

        /// <summary>Adds the list of sourcefiles, does not participate directly in the 
        /// build but can be used in generation of projects for external build systems</summary>
        [FileSet("sourcefiles", Required = false)]
        public StructuredFileSet SourceFiles
        {
            get { return _sourcefiles; }
        }
        private StructuredFileSet _sourcefiles = new StructuredFileSet();

        /// <summary>Adds the list of asmsourcefiles, does not participate directly in the 
        /// build but can be used in generation of projects for external build systems</summary>
        [FileSet("asmsourcefiles", Required = false)]
        public StructuredFileSet AsmSourceFiles
        {
            get { return _asmsourcefiles; }
        }
        private StructuredFileSet _asmsourcefiles = new StructuredFileSet();

        /// <summary>Adds the list of headerfiles, does not participate directly in the 
        /// build but can be used in generation of projects for external build systems</summary>
        [FileSet("headerfiles", Required = false)]
        public StructuredFileSet HeaderFiles
        {
            get { return _headerfiles; }
        }
        private StructuredFileSet _headerfiles = new StructuredFileSet();

        /// <summary>Adds the list of excluded build files, does not participate directly in the 
        /// build but can be used in generation of projects for external build systems</summary>
        [FileSet("excludedbuildfiles", Required = false)]
        public StructuredFileSet ExcludedBuildFiles
        {
            get { return _excludedbuildfiles; }
        }
        private StructuredFileSet _excludedbuildfiles = new StructuredFileSet();

        public MakeStyleTask() : base("MakeStyle")
        {
        }

        protected override void SetupModule()
        {
            SetModuleProperty("MakeBuildCommand", MakeBuildCommand.Value);
            SetModuleProperty("MakeCleanCommand", MakeCleanCommand.Value);
            SetModuleProperty("MakeRebuildCommand", MakeRebuildCommand.Value);

            SetModuleFileset("sourcefiles", SourceFiles);
            SetModuleFileset("asmsourcefiles", AsmSourceFiles);
            SetModuleFileset("headerfiles", HeaderFiles);
            SetModuleFileset("vcproj.excludedbuildfiles", ExcludedBuildFiles);
        }

        protected override void InitModule() { }

        protected override void FinalizeModule() { }

    }
}
