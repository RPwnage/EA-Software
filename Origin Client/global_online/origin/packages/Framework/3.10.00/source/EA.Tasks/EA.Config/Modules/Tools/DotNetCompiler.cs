using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.DotNetTasks;
using EA.FrameworkTasks.Model;
using EA.Eaconfig.Core;

namespace EA.Eaconfig.Modules.Tools
{
    public class DotNetCompiler : Tool
    {
        public enum Targets { Exe, WinExe, Library,  Module }

        protected DotNetCompiler(string toolname, PathString toolexe, Targets target)
            : base(toolname, toolexe, Tool.TypeBuild)
        {
            SourceFiles = new FileSet();
            Defines = new SortedSet<string>();
            Resources = new ResourceFileSet();
            NonEmbeddedResources = new FileSet();
            ContentFiles = new FileSet();
            Modules = new FileSet();
            Assemblies = new FileSet();
            DependentAssemblies = new FileSet();
            ComAssemblies = new FileSet();
            Target = target;

            Assemblies.FailOnMissingFile = false;
            DependentAssemblies.FailOnMissingFile = false;
        }

        public string GetOptionValues(string name, string sep)
        {
            var val = new StringBuilder();
            if (!String.IsNullOrEmpty(name))
            {
                var cleanname = name.TrimStart(OPTION_PREFIX_CHARS).TrimEnd(':') + ':';
                foreach(var option in Options.Where(o => OPTION_PREFIXES.Any(prefix => o.StartsWith(prefix + cleanname))))
                {
                        var ind = option.IndexOf(':');
                        if (ind < option.Length - 1)
                        {
                            var oval = option.Substring(ind + 1).TrimWhiteSpace();
                            if(!String.IsNullOrEmpty(oval))
                            {
                                if(val.Length > 0)
                                {
                                    val.Append(sep);
                                }
                                val.Append(oval);
                            }
                        }
                }
            }
            return val.ToString();
        }


        public string GetOptionValue(string name)
        {
            var val = String.Empty;
            if(!String.IsNullOrEmpty(name))
            {
                var cleanname = name.TrimStart(OPTION_PREFIX_CHARS).TrimEnd(':') + ':';
                var option = Options.FirstOrDefault(o => OPTION_PREFIXES.Any(prefix=> o.StartsWith(prefix+cleanname)));
                if (!String.IsNullOrEmpty(option))
                {
                    var ind = option.IndexOf(':');
                    if(ind < option.Length-1)
                    {
                        val = option.Substring(ind+1);
                    }
                }
            }
            return val;
        }

        public bool GetBoolenOptionValue(string name)
        {
            return HasOption(name) && (GetOptionValue(name) != "-");
        }


        public bool HasOption(string name)
        {
            var cleanname = name.TrimStart(OPTION_PREFIX_CHARS);
            return Options.Any(o => OPTION_PREFIXES.Any(prefix=> o.StartsWith(prefix+cleanname)));
        }

        private static readonly char[] OPTION_PREFIX_CHARS = new char[] { '-', '/'};
        private static readonly string[] OPTION_PREFIXES = new string[] { "--", "/", "-" };

        public readonly Targets Target;
        public string OutputName;
        public string OutputExtension;
        public readonly SortedSet<string> Defines;
        public PathString DocFile;
        public bool GenerateDocs;
        public PathString ApplicationIcon;
        public string AdditionalArguments; //IMTODO: ideally I want to get rid of this and parse .csc-args and .fsc-args into Options. But I don't have time now.

        public readonly FileSet SourceFiles;
        public readonly FileSet Assemblies;
        public readonly FileSet DependentAssemblies;
        public readonly ResourceFileSet Resources;
        public readonly FileSet NonEmbeddedResources;
        public readonly FileSet ContentFiles;
        public readonly FileSet Modules;
        public readonly FileSet ComAssemblies;
    }
}
