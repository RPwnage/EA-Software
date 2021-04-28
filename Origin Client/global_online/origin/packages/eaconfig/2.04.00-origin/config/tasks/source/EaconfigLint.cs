
using System;
using System.Xml;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using System.Threading;
using Threading = System.Threading.Tasks;

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Tasks;
using NAnt.Core.Attributes;
using NAnt.Core.Functions;
using NAnt.Core.Logging;
using NAnt.Core.PackageCore;
using EA.CPlusPlusTasks;
using EA.FrameworkTasks.Model;

using EA.Eaconfig;
using EA.Eaconfig.Core;
using EA.Eaconfig.Build;
using EA.Eaconfig.Modules;
using EA.Eaconfig.Modules.Tools;


namespace EA.Eaconfig
{
    [TaskName("eaconfig-lint")]
    sealed class EaconfigLint : Task
    {
        [TaskAttribute("configs", Required = true)]
        public string Configurations
        {
            get { return _configs; }
            set { _configs = value; }
        }
        private string _configs;


        protected override void ExecuteTask()
        {
            var stepsLog = (Log.Level >= Log.LogLevel.Detailed) ? new BufferedLog(Log) : null;
            if (stepsLog != null)
            {
                stepsLog.Info.WriteLine(LogPrefix + "Executing '{0}' + '{1}' steps", "backend.nant.pregenerate", "backend.pregenerate");
            }

            Project.ExecuteGlobalProjectSteps((Properties["backend.nant.pregenerate"] + Environment.NewLine + Properties["backend.pregenerate"]).ToArray(), stepsLog);

            LintAllModules();
        }

        private void LintAllModules()
        {
            // Because package-lint target may not be thread safe:
            bool parallel = false;

            ForEachModule.Execute(GetModulesToLint(), DependencyTypes.Build, (module) =>
                {
                    module.Package.Project.Log.Status.WriteLine(LogPrefix + "{0}-{1} ({2}) build  '{3}.{4}'", module.Package.Name, module.Package.Version, module.Configuration.Name, module.BuildGroup.ToString(), module.Name);

                    TaskUtil.Dependent(module.Package.Project, "pclintconfig", targetStyle: Project.TargetStyleType.Use);

                    if(!module.Package.Project.Properties.Contains("pclintconfig-targetfile"))
                    {
                        throw new BuildException(String.Format("Property '{0}' does not exist after executing <dependent> task on package '{1}'", "pclintconfig-targetfile", "pclintconfig"));
                    }

                    IncludeTask.IncludeFile(module.Package.Project, module.Package.Project.GetPropertyValue("pclintconfig-targetfile"));

                    HashSet<string> tempOptionsets = null;
                    try
                    {
                        // Set module level properties.
                        module.SetModuleBuildProperties();

                        SetLintTargetProperties(module as Module_Native, out tempOptionsets);

                        module.Package.Project.ExecuteTarget("package-lint", force: true);
                    }
                    finally
                    {
                        if (tempOptionsets != null)
                        {
                            foreach (var tempset in tempOptionsets)
                            {
                                module.Package.Project.NamedOptionSets.Remove(tempset);
                            }
                        }
                    }


                },
                LogPrefix,
                parallelExecution: parallel);
        }

        private void SetLintTargetProperties(Module_Native module, out HashSet<string> tempOptionsets)
        {
            tempOptionsets = null;
            if (module != null)
            {
                    tempOptionsets = new HashSet<string>();

                    var lintOptionfile = (module.Package.Project.Properties["package." + module.Name + ".lint-optionfile"] ?? module.Package.Project.Properties["package.lint-optionfile"] ?? module.Name + ".lnt").TrimWhiteSpace();

                    var lintOptiondir = Path.Combine(module.Package.Dir.Path, "config", "pclint");

                    if (!String.IsNullOrEmpty(lintOptionfile))
                    {
                        var lintOptionFolder = Path.GetDirectoryName(lintOptionfile);

                        if (!String.IsNullOrEmpty(lintOptionFolder))
                        {
                            lintOptiondir = lintOptionFolder;
                            lintOptionfile = Path.GetFileName(lintOptionfile);
                        }
                    }

                    module.Package.Project.Properties["lint.buildname"] = module.Package.Project.Properties[module.GroupName + ".outputname"] ?? module.Name;

                    if (module.IsKindOf(Module.Program))
                    {
                        module.Package.Project.Properties["lint.buildtype"] = "LintProgram";
                    }
                    else if (module.IsKindOf(Module.Library | Module.DynamicLibrary))
                    {
                        module.Package.Project.Properties["lint.buildtype"] = "LintLibrary";
                    }
                    else
                    {
                        var msg = String.Format("Unable to lint package {0} module {1}. Lint cannot run on the package's specified build type '{3}'.", module.Package.Name, module.Name, module.BuildType.BaseName);
                        throw new BuildException(msg, BuildException.StackTraceType.XmlOnly);
                    }

                    module.Package.Project.Properties["lint.usedependencies"] = module.Dependents.Where(d => !d.IsKindOf(DependencyTypes.Build) && d.Dependent.Package.Name != module.Package.Name).Select(d => d.Dependent.Package.Name).ToString(" ");
                    module.Package.Project.Properties["lint.builddependencies"] = module.Dependents.Where(d => d.IsKindOf(DependencyTypes.Build) && d.Dependent.Package.Name != module.Package.Name).Select(d => d.Dependent.Package.Name).ToString(" ");
                    module.Package.Project.Properties["lint.optiondir"] = lintOptiondir;
                    module.Package.Project.Properties["lint.optionfile"] = lintOptionfile;
                    module.Package.Project.Properties["lint.outputdir"] = Path.Combine(module.Package.PackageConfigBuildDir.Path, "pclint", module.Name);
                    module.Package.Project.Properties["lint.includedirs"] = module.Cc.IncludeDirs.ToNewLineString();
                    module.Package.Project.Properties["lint.defines"] = module.Cc.Defines.ToNewLineString();

                    var build_buildtype = "__private_build.buildtype.lint";

                    tempOptionsets.Add(build_buildtype);

                    module.ToOptionSet(module.Package.Project, build_buildtype);
                    module.Package.Project.Properties["build.buildtype"] = build_buildtype;

                    var lintSourcefiles = new FileSet();
                    lintSourcefiles.IncludeWithBaseDir(module.Cc.SourceFiles);

                    // Set custom options for each source file:
                    foreach (var fileitem in lintSourcefiles.FileItems)
                    {
                        if (!String.IsNullOrWhiteSpace(fileitem.OptionSetName))
                        {
                            var optionsetname = "___private.cc.lint." + fileitem.OptionSetName;

                            if (!tempOptionsets.Contains(optionsetname))
                            {
                                if ((fileitem.GetTool() as CcCompiler).ToOptionSet(module, module.Package.Project, optionsetname))
                                {
                                    tempOptionsets.Add(optionsetname);
                                }
                            }

                            fileitem.OptionSetName = optionsetname;
                        }
                    }
                    module.Package.Project.NamedFileSets["lint.sourcefiles"] = lintSourcefiles;
            }
        }

        private IEnumerable<IModule> GetModulesToLint()
        {
            var lintModules = Project.BuildGraph().TopModules;

            if (lintModules.Count() == 0)
            {
                Log.Warning.WriteLine(LogPrefix + "Found no modules for lint target.");
            }
            return lintModules;
        }

        /// <summary>The prefix used when sending messages to the log.</summary>
        public override string LogPrefix
        {
            get
            {
                string prefix = "[lint] ";
                return prefix.PadLeft(prefix.Length + Log.IndentSize);
            }
        }


    }
}

