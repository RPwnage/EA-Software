using System;
using System.Xml;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Writers;
using NAnt.Core.Attributes;
using NAnt.Core.Functions;
using NAnt.Core.Logging;
using NAnt.Core.Reflection;
using NAnt.Core.Events;
using EA.FrameworkTasks.Model;

using EA.Eaconfig;
using EA.Eaconfig.Core;
using EA.Eaconfig.Build;
using EA.Eaconfig.Modules;

namespace EA.Eaconfig.Backends.VisualStudio
{
    internal abstract class VSSolutionBase : Generator
    {
        #region Constructors

        public VSSolutionBase(Log log, string name, PathString outputdir)
            : base(log, name, outputdir)
        {            
            LogPrefix = "[visualstudio] ".PadLeft(Log.IndentSize * 3);
            SolutionFolders = new SolutionFolders(LogPrefix);
            _extensibilityGlobals = new List<KeyValuePair<string,string>>();

            _solutionConfigurationNameOverrides = new Dictionary<Configuration, string>();
        }

        #endregion Constructors

        /// <summary>The prefix used when sending messages to the log.</summary>
        public readonly string LogPrefix;


        public IEnumerable<string> Platforms
        {
            get
            {
                return _platforms;
            }
        }private IEnumerable<string> _platforms;

        protected void OnSolutionFileUpdate(object sender, CachedWriterEventArgs e)
        {
            if (e != null)
            {
                Log.Status.WriteLine("{0}{1} solution file '{2}'", LogPrefix, e.IsUpdatingFile ? "    Updating" : "NOT Updating", Path.GetFileName(e.FileName));
            }
        }

        public override bool Initialize(Project project, IEnumerable<string> configurations, string startupconfigName, IEnumerable<IModule> topmodules, bool generateSingleConfig = false, GeneratorTemplatesData generatorTemplates = null, int groupnumber = 0)
        {
            PopulateVisualStudioPlatformMappings(project);

            IsPortable = project.Properties.GetBooleanPropertyOrDefault("eaconfig.generate-portable-solution", false);

            bool ret = base.Initialize(project, configurations, startupconfigName, topmodules, generateSingleConfig, generatorTemplates, groupnumber);

            if (!ret)
            {
                return ret;
            }

            _platforms = Configurations.Select(c => GetTargetPlatform(c)).Distinct().OrderBy(p => p).ToArray();

            foreach (var topmodule in topmodules.Where(m => Configurations.Contains(m.Configuration)))
            {

                 var genkey = ModuleGenerator.MakeKey(topmodule);
                 ModuleGenerator topgenerator;
                 if (ModuleGenerators.TryGetValue(genkey, out topgenerator))
                 {
                     VSProjectBase vsproject = topgenerator as VSProjectBase;
                     string configoverride;
                     if (vsproject != null && vsproject.ProjectConfigurationNameOverrides.TryGetValue(topmodule.Configuration, out configoverride))
                     {
                         string foundconfigoverride;
                         if (!_solutionConfigurationNameOverrides.TryGetValue(topmodule.Configuration, out foundconfigoverride))
                         {
                             _solutionConfigurationNameOverrides.Add(topmodule.Configuration, configoverride);
                         }
                         else if( foundconfigoverride != configoverride)
                         {
                             Log.Warning.WriteLine("Solution configuration name overrides contain conflicting settings {0} -> {1} and {0} -> {2}. Second override is ignored.", topmodule.Configuration.Name, foundconfigoverride, configoverride);
                         }
                     }
                 }
            }
            
            PopulateSolutionFolders(project);

            // --- Populate ExtensibilityGlobals
            PopulateExtensibilityGlobals(project);

            // --- find startup module generator:
            _startupModuleGenerator = GetStartupProject(project, topmodules);

            return ret;
        }

        internal virtual string ToString(Guid guid)
        {
            return guid.ToString("B").ToUpperInvariant();
        }


        protected virtual void PopulateSolutionFolders(Project project)
        {
            foreach (IPackage package in ModuleGenerators.Values.Select(mg => mg.Package).Distinct())
            {
                SolutionFolders.ProcessFolders(package.Project);

                // For all of the generators associated with this package, build a list that includes their dependents
                // IMTODO: I NEED TO REVISIT THIS. mg.Flatten() may take 70% of solution generation time
                /*
                List<ModuleGenerator> generators = new List<ModuleGenerator>();
                foreach (ModuleGenerator mg in ModuleGenerators.Values.Where(gen => gen.Package.Name == package.Name))
                {
                    generators.AddRange(mg.Flatten());
                }
                 */
                //SolutionFolders.ProcessProjects(package.Project, generators);
                SolutionFolders.ProcessProjects(package.Project, ModuleGenerators.Values);
                
            }
        }

        protected virtual void PopulateExtensibilityGlobals(Project project)
        {
            string vsToMakeFile = null;
            string vsToMakeProfile = null;
            // Get properties from vstomaketools' initialize.xml
            if (!project.Properties.Contains("package.vstomaketools.vstomakefile.exe") || !project.Properties.Contains("package.vstomaketools.profile.xml"))
            {
                try
                {
                    Log.Info.WriteLine("{0}Test do dependent on vstomaketools for constructing XGEAddin info.", LogPrefix);

                    TaskUtil.Dependent(project, "vstomaketools", targetStyle: Project.TargetStyleType.Use);
                }
                catch (Exception e)
                {
                    if (e.ToString().Contains("You must specify a version for package 'vstomaketools' in your masterconfig.xml file."))
                    {
                        Log.Status.WriteLine("{0}Unable to do dependent on vstomaketools (not listed in masterconfig file).  XGEAddin info will not be added!", LogPrefix);
                    }
                    else if (e.ToString().Contains("Cannot find package '"))
                    {
                        int pkgNameStartIdx = e.ToString().IndexOf("Cannot find package '") + 21;
                        string tmpStr = e.ToString().Substring(pkgNameStartIdx);
                        int pkgNameEndIdx = tmpStr.IndexOf("'");
                        string pkgNameStr = tmpStr.Substring(0, pkgNameEndIdx);
                        Log.Status.WriteLine("{0}Unable to do dependent on vstomaketools ({1} not found in package roots).  XGEAddin info will not be added!", LogPrefix, pkgNameStr);
                    }
                    else
                    {
                        Log.Status.WriteLine("{0}Unable to do dependent on vstomaketools (redo build with -verbose for more info).  XGEAddin info will not be added!", LogPrefix);
                    }
                    Log.Debug.WriteLine("Full exception info:\n{0}", e.ToString());
                }
            }

            if (project.Properties.Contains("package.vstomaketools.vstomakefile.exe") && project.Properties.Contains("package.vstomaketools.profile.xml"))
            {
                vsToMakeFile = project.Properties["package.vstomaketools.vstomakefile.exe"];
                vsToMakeProfile = project.Properties["package.vstomaketools.profile.xml"];
            }

            if (vsToMakeFile != null && vsToMakeProfile != null)
            {
                _extensibilityGlobals.Add(new KeyValuePair<string, string>("arguments", project.Properties.GetPropertyOrDefault("xge.arguments", "")));
                _extensibilityGlobals.Add(new KeyValuePair<string, string>("maxcpus", (ConvertUtil.ToNullableInt(project.Properties["xge.maxcpus"]) ?? 40).ToString()));
                _extensibilityGlobals.Add(new KeyValuePair<string, string>("jobs", (ConvertUtil.ToNullableInt(project.Properties["xge.numjobs"]) ?? 32).ToString()));
                _extensibilityGlobals.Add(new KeyValuePair<string, string>("profile", vsToMakeProfile));
                _extensibilityGlobals.Add(new KeyValuePair<string, string>("vstomakefile", vsToMakeFile));
                _extensibilityGlobals.Add(new KeyValuePair<string, string>("nant-command-line", System.Environment.CommandLine));
                _extensibilityGlobals.Add(new KeyValuePair<string, string>("nant-current-directory", Project.NantProcessWorkingDirectory));
                _extensibilityGlobals.Add(new KeyValuePair<string, string>("framework-bin-dir", project.Properties["nant.location"]));
            }
        }

        protected virtual ModuleGenerator GetStartupProject(Project project, IEnumerable<IModule> topmodules)
        {
            string startupProject = project.Properties["package.sln.startupproject"];
            if (null == startupProject)
            {
                // Select executable projects.
                // Generator keys for executables. If we have more than one, try select module with name equal to package name:
                var generatorKeys = topmodules.Where(m=>m.IsKindOf(Module.Program)).Select(m=> ModuleGenerator.MakeKey(m)).Distinct();


                // If the build file does not specify which project to use as the
                // default startup project, use the package name as the start of
                // the project.  If there is no project that matches the name
                // of the package, the projects just get sorted by name makeing the startup project
                // the 1st alphabetically in the list

                var package = project.Properties["package.name"];

                ModuleGenerator gen;
                
                foreach (var key in generatorKeys)
                {
                    
                    if (ModuleGenerators.TryGetValue(key, out  gen))
                    {
                        if (gen.ModuleName == package || gen.Name == package)
                        {
                            return gen;
                        }
                    }
                }

                if (ModuleGenerators.TryGetValue(generatorKeys.FirstOrDefault()??"nonexistent", out  gen))
                {
                    return gen;
                }

                startupProject = package;
            }

            return ModuleGenerators.Values.Where(proj => proj.Name == startupProject).FirstOrDefault();
        }

        protected void PopulateVisualStudioPlatformMappings(Project project)
        {
            foreach (var module in project.BuildGraph().SortedActiveModules)
            {
                
                if (module.Package.Project != null
                    // Dynamically created packages may use exising projects from different packages. Skip such projects
                    && (module.Package.Name == module.Package.Project.Properties["package.name"] && module.Configuration.Name == module.Package.Project.Properties["config"] )
                    && !_targetPlatformMapping.ContainsKey(module.Configuration.Platform))
                {
                    _targetPlatformMapping.Add(module.Configuration.Platform, VSNAntFunctions.GetVisualStudioPlatformName(module.Package.Project));
                }
            }
        }

        protected IEnumerable<KeyValuePair<string, string>> ExtensibilityGlobals
        {
            get { return _extensibilityGlobals; }
        }

        protected VSProjectTypes GetVSProjectType(IEnumerable<IModule> modules)
        {
            var types = modules.Select(m=>
                {
                    if(m.IsKindOf(Module.Native))
                        return VSProjectTypes.Native;
                    if (m.IsKindOf(Module.ExternalVisualStudio))
                        return VSProjectTypes.ExternalProject;
                    else if (m.IsKindOf(Module.CSharp))
                        return VSProjectTypes.CSharp;
                    else if (m.IsKindOf(Module.Utility))
                        return VSProjectTypes.Utility;
                    else if (m.IsKindOf(Module.MakeStyle))
                        return VSProjectTypes.Make;
                    else if (m.IsKindOf(Module.FSharp))
                        return VSProjectTypes.FSharp;
                    else if (m.IsKindOf(Module.Python))
                        return VSProjectTypes.Python;
                    throw new BuildException("Internal error: Unknown module type in module '" + m.Name + "'");

                }).Distinct();

            if (types.Count() > 1)
            {
                throw new BuildException(String.Format("Module '{0}' has different types '{1}' in different configurations. VisualStudio does not support this feature.", modules.FirstOrDefault().Name, types.ToString(", ", t => t.ToString())));
            }

            return types.First();
        }

        protected String GetSolutionConfigName(Configuration config)
        {
            string solutionConfigName;
            if (!_solutionConfigurationNameOverrides.TryGetValue(config, out solutionConfigName))
            {
                solutionConfigName = config.Name;
            }
            return solutionConfigName;
        }

        #region Abstract Methods Implementation

        public override IEnumerable<PathString> GeneratedFiles 
        {
            get 
            { 
                return new List<PathString>() { PathString.MakeNormalized(Path.Combine(OutputDir.Path, SolutionFileName), PathString.PathParam.NormalizeOnly) }; 
            } 
        }

        public override ModuleGenerator StartupModuleGenerator 
        {
            get { return _startupModuleGenerator; }
        }

        public override void Generate()
        {

            WriteToFile();
        }


        internal void WriteToFile()
        {
            using (IMakeWriter writer = new MakeWriter(writeBOM: false))
            {
                writer.FileName = Path.Combine(OutputDir.Path, SolutionFileName);

                writer.CacheFlushed += new NAnt.Core.Events.CachedWriterEventHandler(OnSolutionFileUpdate);

                if (writer.Encoding.GetPreamble().Length > 0)
                {
                    writer.WriteLine();
                }

                WriteHeader(writer);

                // --- Write Startup module
                if (StartupModuleGenerator != null)
                {
                    WriteProject(writer, StartupModuleGenerator as VSProjectBase);
                }

                // --- Write project list
                foreach (ModuleGenerator project in ModuleGenerators.Values.OrderBy(g => g.Name, StringComparer.InvariantCultureIgnoreCase).Where(g => !Object.ReferenceEquals(g, StartupModuleGenerator)))
                {
                    WriteProject(writer, project as VSProjectBase);
                }

                WriteSolutionFoldersDefinitions(writer);

                string vsdmiName;
                var hasUnitTests = WriteUnitTestsDefinitions(writer, out vsdmiName);

                writer.WriteLine("Global");
                {
                    WriteUnitTestsSection(writer, hasUnitTests, vsdmiName);
                    WriteSolutionFoldersContent(writer);
                    WriteSourceControlIntegration(writer);

                    // --- Write global configuration section  
                    WriteSolutionPlatformMapping(writer);

                    // --- Write project configuration mappings
                    writer.WriteTabLine("GlobalSection(ProjectConfigurationPlatforms) = postSolution");
                    foreach (var project in ModuleGenerators.Values.OrderBy(g=>(g as VSProjectBase).ProjectGuidString))
                    {
                        WriteProjectConfigMapping(writer, project as VSProjectBase);
                    }
                    writer.WriteTabLine("EndGlobalSection");

                    writer.WriteTabLine("GlobalSection(SolutionProperties) = preSolution");
                    writer.WriteTab(String.Empty);
                    writer.WriteTabLine("HideSolutionNode = FALSE");
                    writer.WriteTabLine("EndGlobalSection");

                    WriteExtensibilitySolutionGlobals(writer);
                }
                writer.WriteLine("EndGlobal");

            }
        }


        protected string SolutionFileName
        {
            get
            {
                return Name + ".sln";
            }
        }

        #endregion

        #region Internal Static Methods

        internal virtual string GetTargetPlatform(Configuration configuration)
        {
            string targetPlatform = DefaultTargetPlatform;

            if (!_targetPlatformMapping.TryGetValue(configuration.Platform, out targetPlatform))
            {
                targetPlatform = DefaultTargetPlatform;
            }
             
            return targetPlatform;
        }

        /*
        internal virtual string GetVSConfigurationName(Configuration configuration)
        {
            return String.Format("{0}|{1}", configuration.Name, GetTargetPlatform(configuration));
        } 
        */
        #endregion Internal Static Methods

        #region Writing Solution

        protected virtual void WriteSolutionPlatformMapping(IMakeWriter writer)
        {
            writer.WriteTabLine("GlobalSection(SolutionConfigurationPlatforms) = preSolution");

            foreach (var config in Configurations)
            {
                foreach (var platform in Platforms)
                {
                    writer.WriteTab(String.Empty);
                    writer.WriteTabLine("{0}|{1} = {0}|{1}", GetSolutionConfigName(config), platform);
                }
            }

            writer.WriteTabLine("EndGlobalSection");
        }

        protected virtual void WriteSolutionFoldersDefinitions(IMakeWriter writer)
        {
            // Write Solution Folders:
            foreach (SolutionFolders.SolutionFolder folder in SolutionFolders.Folders.Values.OrderBy(folder=> folder.PathName.Path, StringComparer.InvariantCultureIgnoreCase))
            {
#if !FRAMEWORK_PARALLEL_TRANSITION
                if (folder.IsEmpty)
                {
                    continue;
                }
#endif
                writer.WriteLine("Project(\"{0}\") = \"{1}\", \"{2}\", \"{3}\"", ToString(SOLUTION_ITEM_GUID), folder.FolderName, folder.FolderName, ToString(folder.Guid));
                if (folder.FolderFiles.Count > 0)
                {
                    writer.WriteTabLine("ProjectSection(SolutionItems) = preProject");
                    foreach (var file in folder.FolderFiles)
                    {
                        //string relativePath = file.Path;
                        string relativePath = PathUtil.RelativePath(file.Path, OutputDir.Path, addDot:true);
                        writer.WriteTab(String.Empty);
                        writer.WriteTabLine("{0} = {0}", relativePath);
                    }
                    writer.WriteTabLine("EndProjectSection");
                }
                writer.WriteLine("EndProject");
            }
        }

        protected virtual bool WriteUnitTestsDefinitions(IMakeWriter writer, out string vsdmiName)
        {
            vsdmiName = null;
            bool hasUnitTests =  ModuleGenerators.Values.Any(mg => mg.Modules.Any(m=> 
                    ((m is Module_DotNet) && (m as Module_DotNet).IsProjectType(DotNetProjectTypes.UnitTest))
                ||  ((m is Module_ExternalVisualStudio) && (m as Module_ExternalVisualStudio).IsProjectType(DotNetProjectTypes.UnitTest))));

                if (hasUnitTests)
                {
                    vsdmiName = Path.GetFileNameWithoutExtension(Name) + ".vsmdi";
                    var testrunconfigName = "LocalTestRun.testrunconfig";

                    var testProjectGuid = Hash.MakeGUIDfromString(vsdmiName + "UnitTests");

                    writer.WriteLine("Project(\"{0}\") = \"{1}\", \"{1}\", \"{2}\"", ToString(SOLUTION_ITEM_GUID), "Solution Items", ToString(testProjectGuid));
                    writer.WriteTabLine("ProjectSection(SolutionItems) = postProject");

                    writer.WriteTab(String.Empty);
                    writer.WriteTabLine("{0} = {0}", testrunconfigName);
                    writer.WriteTab(String.Empty);
                    writer.WriteTabLine("{0} = {0}", vsdmiName);
                    writer.WriteTabLine("EndProjectSection");
                    writer.WriteLine("EndProject");

                    WriteUnitTestConfigurationFiles(vsdmiName, testrunconfigName);
                }
                return hasUnitTests;
        }

        protected void WriteUnitTestConfigurationFiles(string vsdmiName, string testrunconfigName)
        {
            string vsdmiPath = Path.Combine(OutputDir.Path, vsdmiName);
            string testrunconfigPath = Path.Combine(OutputDir.Path, testrunconfigName);

            string vsdmiContent = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n" +
                                  "<TestLists xmlns=\"http://microsoft.com/schemas/VisualStudio/TeamTest/" + TeamTestSchemaVersion + "\">\r\n" +
                                  "  <TestList name=\"Lists of Tests\" id=\"8c43106b-9dc1-4907-a29f-aa66a61bf5b6\">\r\n" +
                                  "     <RunConfiguration id=\"18004645-256a-4e97-8af1-7f6b0a4fa907\" name=\"Local Test Run\" storage=\"{0}\" type=\"Microsoft.VisualStudio.TestTools.Common.TestRunConfiguration, Microsoft.VisualStudio.QualityTools.Common,   PublicKeyToken=b03f5f7f11d50a3a\" />\r\n" +
                                  "  </TestList>\r\n" +
                                  "</TestLists>\r\n";

            using (StreamWriter vsdmiOut = new StreamWriter(vsdmiPath))
            {
                vsdmiOut.Write(vsdmiContent, testrunconfigName.ToLower());
            }

            string testrunconfigContent = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n" +
                                          "<TestRunConfiguration name=\"Local Test Run\" id=\"18004645-256a-4e97-8af1-7f6b0a4fa907\" xmlns=\"http://microsoft.com/schemas/VisualStudio/TeamTest/" + TeamTestSchemaVersion + "\">\r\n" +
                                          "   <Description>This is a default test run configuration for a local test run.</Description>\r\n" +
                                          "   <TestTypeSpecific />\r\n" +
                                          "</TestRunConfiguration>\r\n";

            using (StreamWriter testrunconfigOut = new StreamWriter(testrunconfigPath))
            {
                testrunconfigOut.Write(testrunconfigContent);
            }
        }

        protected virtual void WriteProject(IMakeWriter writer, VSProjectBase project)
        {
            var relativePath = Path.Combine(project.RelativeDir, project.ProjectFileName);

            if (!Path.IsPathRooted(relativePath))
            {
                relativePath = "." + Path.DirectorySeparatorChar + relativePath;
            }
            
            writer.WriteLine(@"Project(""{0}"") = ""{1}"", ""{2}"", ""{3}""",
#if FRAMEWORK_PARALLEL_TRANSITION
                ToString(project.ProjectTypeGuid), project.Name, (project.RelativeDir.StartsWith(".") ? "" : ".\\") + Path.Combine(project.RelativeDir, project.ProjectFileName), project.ProjectGuidString);
#else
                ToString(project.ProjectTypeGuid), project.Name, relativePath, project.ProjectGuidString);
#endif


#if FRAMEWORK_PARALLEL_TRANSITION
#else
            if (project.Dependents.Count > 0)
#endif
            {
                writer.WriteTabLine("ProjectSection(ProjectDependencies) = postProject");
#if FRAMEWORK_PARALLEL_TRANSITION
                foreach (var dependent in project.Dependents.OrderBy(mg => (mg as VSProjectBase).ProjectGuidString, StringComparer.InvariantCultureIgnoreCase))
#else
                foreach (var dependent in project.Dependents)
#endif
                {
                    WriteDependent(writer, dependent as VSProjectBase);
                }
                writer.WriteTabLine("EndProjectSection");
            }
            writer.WriteLine("EndProject");
        }

        protected virtual void WriteDependent(IMakeWriter writer, VSProjectBase project)
        {
            writer.WriteTab(String.Empty);
            writer.WriteTabLine("{0} = {0}", project.ProjectGuidString);
        }

        protected virtual void WriteProjectConfigMapping(IMakeWriter writer, VSProjectBase project)
        {
            //for each global configuration find corresponging project configurations.
            foreach (Configuration config in Configurations)
            {
                //IMTODO: I can mark wrong name|platfortm combinations as unbuildable. Now I just map them to correct project configs
                //string activesolutionconfig = project.GetVSProjConfigurationName(config);
                string activeprojectconfig = null;
                Configuration active = null;
                if (project.ConfigurationMap.TryGetValue(config, out active))
                {
                    activeprojectconfig = project.GetVSProjConfigurationName(active);
                    //IMTODO: do better implementation. This looks a bit hacky.
                    if (project is ExternalVSProject || project is VSDotNetProject || project.ProjectTypeGuid == VSProjectBase.CSPROJ_GUID || project.ProjectTypeGuid == VSProjectBase.FSPROJ_GUID)
                    {
                        activeprojectconfig = activeprojectconfig.Replace("AnyCPU", "Any CPU");
                    }
                }
                foreach (var solutionplatform in Platforms)
                {
                    var solutionplatformName = (solutionplatform == "AnyCPU") ? "Any CPU" : solutionplatform;

                    string solutioncfgName = GetSolutionConfigName(config) + "|" + solutionplatformName;


                    //IMTODO: I don't understand why I put this extra condition activesolutionconfig == solutioncfgName. 
                    // IN C# (and maybe other cases) dependent module may not have the same config as solution config at all! 
                    //if (activesolutionconfig == solutioncfgName && activeprojectconfig != null)
                    if (activeprojectconfig != null)
                    {
                        writer.WriteTab(String.Empty);
                        writer.WriteTabLine("{0}.{1}.ActiveCfg = {2}", project.ProjectGuidString, solutioncfgName, activeprojectconfig);
                        writer.WriteTab(String.Empty);
                        writer.WriteTabLine("{0}.{1}.Build.0 = {2}", project.ProjectGuidString, solutioncfgName, activeprojectconfig);
                        if (project.SupportsDeploy(active))
                        {
                            writer.WriteTab(String.Empty);
                            writer.WriteTabLine("{0}.{1}.Deploy.0 = {2}", project.ProjectGuidString, solutioncfgName, activeprojectconfig);
                        }
                    }
                    else
                    {
                        //Just select any project config to map as non active. Match with the same config name if possible:
                        var projectconfig = project.AllConfigurations.SingleOrDefault(c => c == config);
                        if (projectconfig == null)
                        {
                            projectconfig = project.AllConfigurations.FirstOrDefault();
                        }
                        if (projectconfig != null)
                        {
                            writer.WriteTab(String.Empty);
                            writer.WriteTabLine("{0}.{1}.ActiveCfg = {2}", project.ProjectGuidString, solutioncfgName, project.GetVSProjConfigurationName(projectconfig));
                        }
                    }
                }
            }
        }

        protected virtual void WriteUnitTestsSection(IMakeWriter writer, bool hasUnitTests, string vsdmiName)
        {
            if (hasUnitTests)
            {
                writer.WriteTabLine("GlobalSection(TestCaseManagementSettings) = postSolution");
                writer.WriteTab();
                writer.WriteTabLine("CategoryFile = {0}", vsdmiName);
                writer.WriteTabLine("EndGlobalSection");
            }
        }


        protected virtual void WriteSolutionFoldersContent(IMakeWriter writer)
        {

            if (SolutionFolders.Folders.Count > 0)
            {
                writer.WriteTabLine("GlobalSection(NestedProjects) = preSolution");

#if FRAMEWORK_PARALLEL_TRANSITION
                // OrderBy to match slnmerger.cs
                List<string> lines = new List<string>();
                foreach (var folder in SolutionFolders.Folders.Values)
                {
                    foreach (var child in folder.children.Values)
                    {
                        lines.Add(String.Format("{0} = {1}", ToString(child.Guid), ToString(folder.Guid)));
                    }
                    foreach (var gen in folder.FolderProjects.Values)
                    {
                        lines.Add(String.Format("{0} = {1}", gen.ProjectGuidString, ToString(folder.Guid)));
                    }
                }
                lines.Sort();

                foreach (string line in lines)
                {
                    writer.WriteTab(String.Empty);
                    writer.WriteTabLine(line);
                }
#else
                foreach (var folder in SolutionFolders.Folders.Values.OrderBy(f => f.PathName.Path, StringComparer.InvariantCultureIgnoreCase))
                {
                    if (folder.IsEmpty)
                    {
                        continue;
                    }

                    foreach (var gen in folder.FolderProjects.Values.OrderBy(g => g.Name, StringComparer.InvariantCultureIgnoreCase))
                    {
                        writer.WriteTab(String.Empty);
                        writer.WriteTabLine("{0} = {1}", gen.ProjectGuidString, ToString(folder.Guid));
                    }
                    foreach (var child in folder.children.Values.OrderBy(c=> c.FolderName))
                    {
                        writer.WriteTab(String.Empty);
                        writer.WriteTabLine("{0} = {1}", ToString(child.Guid), ToString(folder.Guid));
                    }
                }
#endif
                writer.WriteTabLine("EndGlobalSection");
            }
        }

        protected virtual void WriteSourceControlIntegration(IMakeWriter writer)
        {
            var sscgenerators = ModuleGenerators.Values.Where(g => g.Package.SourceControl.UsingSourceControl);

            var count = sscgenerators.Count();

            if (count > 0)
            {
                writer.WriteTabLine("GlobalSection(SourceCodeControl) = preSolution");
                writer.WriteTab();
                writer.WriteTabLine("SccNumberOfProjects = {0}", count);

                int ind = 0;
                foreach (var generator in sscgenerators)
                {
                    writer.WriteTab();
                    writer.WriteTabLine("SccProjectUniqueName{0} = {1}", ind, generator.SourceControl.SccProjectName);
                    writer.WriteTab();
                    writer.WriteTabLine("SccProjectName{0} = {1} Project", ind, generator.Package.SourceControl.Type.ToString());
                    writer.WriteTab();
                    writer.WriteTabLine("SccLocalPath{0} = {1}", ind, PathUtil.RelativePath(generator.SourceControl.SccLocalPath, OutputDir.Path));
                    writer.WriteTab();
                    writer.WriteTabLine("SccProvider{0} = {1}", ind, generator.SourceControl.SccProvider);
                    writer.WriteTab();
                    writer.WriteTabLine("SccProjectFilePathRelativizedFromConnection{0} = {1}", ind, generator.RelativeDir);
                    ind++;
                }
                writer.WriteTabLine("EndGlobalSection");
            }
        }

        protected virtual void WriteExtensibilitySolutionGlobals(IMakeWriter writer)
        {
            writer.WriteTabLine("GlobalSection(ExtensibilityGlobals) = postSolution");
            foreach (var global in ExtensibilityGlobals)
            {
                writer.WriteTab();
                writer.WriteTabLine("{0} = {1}", global.Key, global.Value);
            }
            writer.WriteTabLine("EndGlobalSection");
        }

        #endregion Writing Solution


        #region Abstract Methods

        protected abstract void WriteHeader(IMakeWriter writer);

        protected abstract string TeamTestSchemaVersion
        {
            get;
        }

        #endregion Abstract Methods


        #region Private Fields
        private const string DefaultTargetPlatform = "Win32";
        protected readonly IDictionary<string, string> _targetPlatformMapping = new Dictionary<string, string>();
        private readonly List<KeyValuePair<string,string>> _extensibilityGlobals;
        private ModuleGenerator _startupModuleGenerator;

        private readonly IDictionary<Configuration, string> _solutionConfigurationNameOverrides;
        #endregion Private Fields

        protected readonly SolutionFolders SolutionFolders;

        protected enum VSProjectTypes { Native, Make, Utility, CSharp, FSharp, Python, ExternalProject }

        #region Solution Types Guid definitions
        protected static readonly Guid SOLUTION_ITEM_GUID = new Guid("2150E333-8FDC-42A3-9474-1A3956D46DE8");
        #endregion Solution Types Guid definitions

    }

}
