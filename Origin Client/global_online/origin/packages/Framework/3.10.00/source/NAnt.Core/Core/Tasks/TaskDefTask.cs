// NAnt - A .NET build tool
// Copyright (C) 2001-2003 Gerry Shaw
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
// 
// As a special exception, the copyright holders of this software give you 
// permission to link the assemblies with independent modules to produce 
// new assemblies, regardless of the license terms of these independent 
// modules, and to copy and distribute the resulting assemblies under terms 
// of your choice, provided that you also meet, for each linked independent 
// module, the terms and conditions of the license of that module. An 
// independent module is a module which is not derived from or based 
// on these assemblies. If you modify this software, you may extend 
// this exception to your version of the software, but you are not 
// obligated to do so. If you do not wish to do so, delete this exception 
// statement from your version. 
// 
//
// Gerry Shaw (gerry_shaw@yahoo.com)

using System;
using System.Collections.Generic;
using System.Collections.Concurrent;
using System.IO;
using System.Text;
using System.Linq;
using System.Reflection;
using System.CodeDom.Compiler;
using Microsoft.CSharp;
using System.Xml;

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Threading;
using NAnt.Core.Attributes;
using NAnt.Core.Logging;
using NAnt.Core.Reflection;
using NAnt.Core.PackageCore;

namespace NAnt.Core.Tasks
{

    /// <summary>Loads tasks from a specified assembly. When source files are provided as input assembly is built first.</summary>
    /// <remarks>
    /// <para>All assemblies already loaded in the nant Application Domain are automatically added as references when assembly is built from sources.</para>
    /// <para>
    /// Task defintions are propagated to dependent packages like global properies.
    /// </para>
    /// <para>
    /// NAnt by default will scan any assemblies ending in *Task.dll in the 
    /// same directory as NAnt.  You can use this task to include assemblies 
    /// in different locations or which use a different file
    /// convention.  (Some .NET assemblies end will end in .net
    /// instead of .dll)
    /// </para>
    /// <para>
    /// NAnt can only use .NET assemblies; other types of files which
    /// end in .dll won't work.
    /// </para>
    /// </remarks>
    /// <include file='Examples/TaskDef/TaskDef.example' path='example'/>
    /// <include file='Examples/TaskDef/TaskDefBuild.example' path='example'/>
    [TaskName("taskdef")]
    public class TaskDefTask : Task
    {
        private string	_assemblyFileName	= null;
        private bool _override = false;
        private bool _failOnErrorTaskDef = false; //Affects TaskFactory.AddTasks

        /// <summary>File name of the assembly containing the NAnt task.</summary>
        [TaskAttribute("assembly", Required = true)]
        public string AssemblyFileName
        {
            get 
            { 
                return _assemblyFileName; 
            }
            set { _assemblyFileName = PathString.MakeNormalized(value, PathString.PathParam.NormalizeOnly).Path; }
        }

        /// <summary>
        /// Override task with the same name if it already loaded into the Project. Default is "false"</b>
        /// </summary>
        [TaskAttribute("override", Required = false)]
        public bool Override
        {
            get { return _override; }
            set { _override = value; }
        }

        [TaskAttribute("failonerror")]
        public override bool FailOnError
        {
            get { return base.FailOnError; }
            set 
            {
                base.FailOnError = value;
                _failOnErrorTaskDef = value; 
            }
        }

        /// <summary>If defined, Tasks dll will be built using these source files.</summary>
        [FileSet("sources", Required = false)]
        public FileSet Sources
        {
            get { return _sources; }
        } private FileSet _sources = new FileSet();

        /// <summary>Reference assembles. NAll assemblies already loaded in the nant Application Domain are automatically added as references.</summary>
        [FileSet("references", Required = false)]
        public FileSet References
        {
            get { return _references; }
        } private FileSet _references = new FileSet();

        /// <summary>Generate debug info. Default is "true"</summary>
        [TaskAttribute("debugbuild", Required = false)]
        public bool DebugBuild
        {
            get { return _debugbuild; }
            set { _debugbuild = value; }
        } private bool _debugbuild = true;

        /// <summary>
        /// Version of .Net compiler to use when building tasks. Format is: 'v3.5', or 'v4.0', or 'v4.5', ... . Default is "v4.0"
        /// </summary>
        [TaskAttribute("compiler", Required = false)]
        public string Compiler
        {
            get { return _compiler; }
            set 
            {
                if (!String.IsNullOrEmpty(value) && value.StartsWith("v"))
                {
                    _compiler = value;
                }
                else
                {
                    Log.Warning.WriteLine("'taskdef' compiler parameter accepts compiler version values in format 'v3.5', or 'v4.0', or 'v4.5', etc. Input value '{0}' is ignored. Will use default value '{1}", value ?? String.Empty, _compiler);
                }
            }
        } private string _compiler = "v4.0";


#if BPROFILE
		/// <summary>Return additional info identifying the task to the BProfiler.</summary>
		public override string BProfileAdditionalInfo 
		{
			get { return AssemblyFileName; }
		}
#endif

        protected override void ExecuteTask()
        {
            int trycount = 0;
            bool forcebuild = false;
            Init();

            Assembly assembly = null;

            while (trycount < 2)
            {
                using (var buildState = PackageAutoBuilCleanMap.AssemblyAutoBuilCleanMap.StartBuild(AssemblyFileName, "dotnet"))
                {
                    if (buildState.IsDone())
                    {
                        Log.Debug.WriteLine("Task definition {0} already built.", AssemblyFileName);

                    }
                    else
                    {
                        if (!AssemblyLoader.TryGetCached(AssemblyFileName, out assembly))
                        {
                            assembly = BuildAssemblyIfOutOfDate(forcebuild);
                        }
                    }

                }
                try
                {
                    assembly = assembly ?? AssemblyLoader.Get(AssemblyFileName, fromMemory: PathUtil.IsPathInBuildRoot(Project, AssemblyFileName));

                    Project.TaskFactory.AddTasks(assembly, Override, _failOnErrorTaskDef, AssemblyFileName);

                    /* IM TODO - just register callback                
                                    StringCollection taskNames = Project.TaskFactory.GetTaskNames(assembly);
                                    foreach (string taskName in taskNames) {
                                        Log.WriteLineIf(Verbose, LogPrefix + "Added task <{0}>", taskName);
                                    }
                     */

                }
                catch (Exception e)
                {
                    if (trycount < 1 && Sources.FileItems.Count > 0)
                    {
                        // Try to force build. Sometimes timestamps may not be correct.
                        forcebuild = true;
                    }
                    else
                    {
                        string msg = String.Format("Could not add tasks from '{0}'.", AssemblyFileName);

                        try
                        {
                            if (assembly != null && assembly.ImageRuntimeVersion != null && (assembly.ImageRuntimeVersion.StartsWith("v2.") || assembly.ImageRuntimeVersion.StartsWith("v3.")))
                            {
                                msg += Environment.NewLine;
                                msg += "------------------------------------------------------------------------------------------------------------------------" + Environment.NewLine;
                                msg += "-                                                                                                                      -" + Environment.NewLine;
                                msg += "- Looks like this task assembly was built against old version of Framework. Rebuid this assembly against Framework 3.  -" + Environment.NewLine;
                                msg += "-                                                                                                                      -" + Environment.NewLine;
                                msg += "------------------------------------------------------------------------------------------------------------------------" + Environment.NewLine;
                                msg += Environment.NewLine;
                            }
                        }
                        catch (Exception) { }
                        
                        throw new BuildException(msg, Location, ThreadUtil.ProcessAggregateException(e));
                    }
                }

                trycount++;
            }
        }

        private void Init()
        {
            if (!Path.IsPathRooted(AssemblyFileName))
            {
                if (Sources.FileItems.Count > 0)
                {
                    string buildpath = Project.Properties[Project.NANT_PROPERTY_PROJECT_TEMPROOT];
                    if (buildpath != null)
                    {
                        AssemblyFileName = PathString.MakeNormalized(Path.Combine(buildpath, "tasks", AssemblyFileName)).Path;
                    }
                }
                else
                {
                    AssemblyFileName = Project.GetFullPath(AssemblyFileName);
                }
            }
        }

        private Assembly BuildAssemblyIfOutOfDate(bool force)
        {
            Assembly assembly = null;

            if(Sources.FileItems.Count > 0)
            {
                using (var compiler = new ScriptCompiler(Project, DebugBuild, Location, version: _compiler))
                {
                    var references = compiler.GetReferenceAssemblies(References);

                    var keyfile = Project.Properties.GetBooleanProperty("nant.issigned") ? Project.NantLocation + @"\..\source\framework.snk" : String.Empty;

                    // To generate VisualStudio solution for tasks (slntaskdef target)
                    StoreTaskDefModule(references, keyfile);

                    var dependencyFile = AssemblyFileName + ".dep";

                    if (IsOutOfDate(force, dependencyFile, references))
                    {
                        compiler.CompileAssemblyFromFiles(Sources, AssemblyFileName);

                        assembly = AssemblyLoader.Get(AssemblyFileName, fromMemory: PathUtil.IsPathInBuildRoot(Project, AssemblyFileName));

                        WriteDependencyFile(dependencyFile, references, assembly != null ? assembly.GetReferencedAssemblies() : new AssemblyName[0]);
                    }
                }
            }
            return assembly;
        }

        private bool IsOutOfDate(bool force, string dependencyFile, List<string> references)
        {
            bool outofdate = false;

            IEnumerable<string> referencedLookup;
            var output_time = File.GetLastWriteTimeUtc(AssemblyFileName);

            StringBuilder reason = null;
            if (Log.Level >= Log.LogLevel.Diagnostic || Environment.GetEnvironmentVariable("FRAMEWORK_DEBUG_TASKDEF") != null)
            {
                reason = new StringBuilder();
            }

            if (force)
            {
                outofdate = true;
                if(reason != null)
                {
                reason.Append("Parameter 'force=true'");
                }
            }
            else if (!File.Exists(AssemblyFileName))
            {
                outofdate = true;
                if(reason != null)
                {
                    reason.AppendFormat("File '{0}'does not exist", AssemblyFileName);
                }
            }
            else if (!DependentsMatch(dependencyFile, references, reason, out referencedLookup))
            {
                outofdate = true;
            }
            else if (Sources.FileItems.Any(fi => output_time < File.GetLastWriteTimeUtc(fi.Path.Path)))
            {
                outofdate = true;
                if (reason != null)
                {
                    reason.AppendFormat("Source files ({0}) are newer than assembly file {1}[{2}]", 
                        Sources.FileItems.Where(fi => output_time < File.GetLastWriteTimeUtc(fi.Path.Path))
                            .ToString("; ", fi=>String.Format("{0}[{1}]", fi.Path.Path.Quote(), File.GetLastWriteTimeUtc(fi.Path.Path))),
                        AssemblyFileName.Quote(), output_time);
                }
            }
            else if (referencedLookup.Any(path => output_time < File.GetLastWriteTimeUtc(path)))
            {
                outofdate = true;

                if (reason != null)
                {
                    reason.AppendFormat("Referenced assembly files ({0}) are newer than assembly file {1}[{2}]",
                        referencedLookup.Where(f => output_time < File.GetLastWriteTimeUtc(f))
                            .ToString("; ", f => String.Format("{0}[{1}]", f.Quote(), File.GetLastWriteTimeUtc(f))),
                        AssemblyFileName.Quote(), output_time);

                }
            }

            if (reason != null)
            {
                if (outofdate)
                {

                    Log.Status.WriteLine(LogPrefix + "Build assembly {0} because{1}{2}", AssemblyFileName.Quote(), Environment.NewLine, reason.ToString());
                }
                else
                {
                    Log.Status.WriteLine(LogPrefix + "UpToDate assembly {0}", AssemblyFileName.Quote());
                }
            }

            return outofdate;
        }

        private void WriteDependencyFile(string dependencyFile, List<string> references, IEnumerable<AssemblyName> referencedAssemblies)
        {
            var gacAssemblies = new HashSet<string>(AppDomain.CurrentDomain.GetAssemblies().Where(a => !a.IsDynamic && !String.IsNullOrEmpty(a.Location) && a.GlobalAssemblyCache).Select(a => Path.GetFileNameWithoutExtension(a.Location)), StringComparer.OrdinalIgnoreCase);

            var lookup = new HashSet<string>(referencedAssemblies.Select(n=>n.Name).Where(n=>!gacAssemblies.Contains(n)), StringComparer.OrdinalIgnoreCase);

            var depRef = references.Where(l => lookup.Contains(Path.GetFileNameWithoutExtension(l)));

            using (StreamWriter writer = new StreamWriter(dependencyFile))
            {
                foreach (var src in this.Sources.FileItems)
                {
                    writer.WriteLine(src.Path.Path);
                }
                writer.WriteLine();
                foreach (var r in depRef)
                {
                    try
                    {
                        if (File.Exists(r))
                        {
                            writer.WriteLine(r);
                        }
                    }
                    catch(Exception){}
                }
            }

        }

        private bool DependentsMatch(string dependencyFile, List<string> references, StringBuilder reason, out IEnumerable<string> referenceLookup)
        {
            bool ret = false;

            referenceLookup = null;

            if (File.Exists(dependencyFile))
            {
                var depsrc = new HashSet<string>();
                var depref = new HashSet<string>(StringComparer.OrdinalIgnoreCase);

                var tofill = depsrc;
                using (StreamReader r = File.OpenText(dependencyFile))
                {
                    // each dependent file is stored on it's own line
                    string dependent = r.ReadLine();
                    while (dependent != null)
                    {
                        if (String.IsNullOrEmpty(dependent))
                        {
                            tofill = depref;
                        }
                        else
                        {
                            tofill.Add(dependent);
                        }
                        dependent = r.ReadLine();
                    }
                    r.Close();
                }

                var input = new List<string>(Sources.FileItems.Select(fi => fi.Path.Path));

                ret = depsrc.SetEquals(input);
                if (ret)
                {
                    var filtered = references.Where(r => depref.Contains(r)).ToList();
                    referenceLookup = filtered;
                }
                else
                {
                    if (reason != null)
                    {
                        var added = input.Where(f=>!depsrc.Contains(f));
                        var removed = depsrc.Where(f=>!input.Contains(f));

                        reason.Append("Source files list changed.");
                        if (added.Count() != 0)
                        {
                            reason.AppendFormat("  New files added: {0}.", added.ToString("; ", s=>s.Quote()));
                        }
                        if (removed.Count() != 0)
                        {
                            reason.AppendFormat("  Files removed: {0}.", removed.ToString("; ", s=>s.Quote()));
                        }
                    }
                }
            }
            else
            {
                if(reason != null)
                {
                    reason = reason.AppendFormat("Dependency file '{0}' does not exist.", dependencyFile);
                }
            }
            return ret;
        }


        private void StoreTaskDefModule(IEnumerable<string> references, string keyfile)
        {
            Project.TaskDefModules().TryAdd(AssemblyFileName, new TaskDefModule(
                Project.Properties["config"],
                Project.Properties["package.name"],
                references,
                Sources,
                AssemblyFileName,
                DebugBuild,
                keyfile,
                Project.CurrentScriptFile));
        }
    }

    // Data Model to generate csproj and sln file for tasks;
    public class TaskDefModule
    {
        public TaskDefModule(string config, string package, IEnumerable<string> references, FileSet sources, string assembly, bool includedebuginfo, string keyfile, string scriptfile)
        {
            Config = config;
            Package = package;
            References = references;
            Sources = sources;
            AssemblyFileName = assembly;
            IncludeDebugInfo = includedebuginfo;
            KeyFile = keyfile;
            ScriptFile = scriptfile;
        }

        public readonly string Config;
        public readonly string Package;
        public readonly IEnumerable<string> References;
        public readonly FileSet Sources;
        public readonly string AssemblyFileName;
        public readonly bool IncludeDebugInfo;
        public readonly string KeyFile;
        public readonly string ScriptFile;
    }

    public class TaskDefModules :  ConcurrentDictionary<string, TaskDefModule>
    {
    }

}
