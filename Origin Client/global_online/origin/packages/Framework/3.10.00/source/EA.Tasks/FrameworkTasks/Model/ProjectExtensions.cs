using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Writers;
using NAnt.Core.Logging;
using NAnt.Core.Tasks;
using NAnt.Shared.Properties;
using EA.FrameworkTasks.Model;
using EA.Eaconfig.Modules.Tools;

namespace EA.FrameworkTasks.Model
{
    public static class ProjectExtensions
    {
        private static readonly Guid PackageObjectId = new Guid("4A8147EA-E566-4FC4-A7ED-EFAEF0FFB689");
        private static readonly Guid BuildGraphObjectId = new Guid("336FC227-2728-4E35-B642-F57EF8CF1D7F");
        private static readonly Guid RootTargetStyleId = new Guid("EC19A8E2-2415-43DF-A2DA-CC45CC77526D");
        private static readonly Guid ModuleDependencyConstaintsId = new Guid("A7512C09-DEE6-4BA9-A336-07F6B0FD8C7C");

        public static bool TryGetPackage(this Project project, out IPackage package)
        {
            object obj;

            if (project.NamedObjects.TryGetValue(PackageObjectId, out obj))
            {
                package = (IPackage)obj;
            }
            else
            {
                package = null;
            }
            return package != null;
        }

        public static bool TrySetPackage(this Project project, string name, string version, PathString dir, string config, FrameworkVersions frameworkversion, out IPackage package)
        {
            package = project.BuildGraph().GetOrAddPackage(name, version, dir, config, frameworkversion);

            if (!project.BuildGraph().TryGetPackage(name, version, dir, config, out package))
            {
                throw new BuildException("INTENAL ERROR. Package does not exist");
            }

            if (package.PackageBuildDir == null && project.Properties.Contains(PackageProperties.PackageBuildDirectoryPropertyName))
            {
                package.PackageBuildDir = PathString.MakeNormalized(project.Properties[PackageProperties.PackageBuildDirectoryPropertyName]);
            }

            return project.NamedObjects.TryAdd(PackageObjectId, package);
        }

        public static Project.TargetStyleType GetRootTargetStyle(this Project project)
        {
            return (Project.TargetStyleType)project.NamedObjects.GetOrAdd(RootTargetStyleId, (key) => 
                    { 
                        lock (Project.GlobalNamedObjects)
                        {
                            Project.GlobalNamedObjects.Add(RootTargetStyleId);
                        }
                        return (object)project.TargetStyle;
                    } 
                    );
        }

        public static void ResetBuildGraph(this Project project)
        {
            project.NamedObjects.Remove(BuildGraphObjectId);
            project.NamedObjects.Remove(PackageObjectId);
            EA.FrameworkTasks.Model.BuildGraph.WasReset = true;
        }

        public static bool HasBuildGraph(this Project project)
        {
            return project.NamedObjects.ContainsKey(BuildGraphObjectId);
        }

        public static BuildGraph BuildGraph(this Project project)
        {
            return project.NamedObjects.GetOrAdd(BuildGraphObjectId, (key) =>
            {
                lock(Project.GlobalNamedObjects)
                {
                    Project.GlobalNamedObjects.Add(BuildGraphObjectId);
                }

                return new BuildGraph();
            }) as BuildGraph;
        }

        public static List<ModuleDependencyConstraints> GetConstraints(this Project project)
        {
            Object constraints;

            if (!project.NamedObjects.TryGetValue(ModuleDependencyConstaintsId, out constraints))
            {
                constraints = null;
            }

            return constraints as List<ModuleDependencyConstraints>;
        }

        public static bool SetConstraints(this Project project, List<ModuleDependencyConstraints> constraints)
        {
            return project.NamedObjects.TryAdd(ModuleDependencyConstaintsId, constraints);
        }

        public static bool RemoveConstraints(this Project project)
        {
            object constraints;
            return project.NamedObjects.TryRemove(ModuleDependencyConstaintsId, out constraints);
        }


        public static void ExecuteTargetCommands(this Project project, IEnumerable<TargetCommand> targets)
        {
            if (targets != null)
            {
                foreach(var command in targets)
                {
                    project.ExecuteTargetIfExists(command.Target, true);
                }
            }
        }

        public static void ExecuteCommands(this Project project, IEnumerable<Command> commands, bool failonerror = true, string scriptFilename = null)
        {
            if (commands != null)
            {
                foreach (var command in commands)
                {
                    ExecuteCommand(project, command, failonerror, scriptFilename);
                }
            }
        }

        public static int ExecuteCommand(this Project project, Command command, bool failonerror = true, string scriptFilename = null)
        {
            int exitcode = 0;
            if (command != null)
            {
                if (!String.IsNullOrEmpty(command.Message))
                {
                    project.Log.Status.WriteLine("      {0}", command.Message);
                }

                if(command.CreateDirectories != null)
                {
                    foreach (var dir in command.CreateDirectories)
                    {
                        if (dir != null && !String.IsNullOrEmpty(dir.Path))
                        {
                            MkDirTask.CreateDir(project, dir.Path);
                        }
                    }
                }

                var executed = String.Empty; 

                if (command.IsKindOf(Command.Program))
                {
                    exitcode = Program.Execute(project, command.Executable, command.Options, command.WorkingDir, command.Env);

                    executed = command.Executable.Path + ((command.Options != null && command.Options.Count() > 0) ? " ...." : "");
                }
                else
                {
                    // Create a script file:
                    string script = command.CommandLine.TrimWhiteSpace();
                    if (!String.IsNullOrEmpty(script))
                    {
                        PathString program = null;
                        IEnumerable<string> args = null;

                        PathString scriptfile = null;

                        var scriptFileName = (scriptFilename ?? "command") + Hash.MakeGUIDfromString(project.SyncRoot.GetHashCode()+command.CommandLine) + ".bat";
                        if (command.WorkingDir != null && !String.IsNullOrEmpty(command.WorkingDir.Path))
                        {
                            scriptfile = new PathString(Path.Combine(command.WorkingDir.Path, scriptFileName), PathString.PathState.FullPath);
                        }
                        else
                        {
                            scriptfile = PathString.MakeNormalized(Path.Combine(project.Properties["package.configbuilddir"] ?? Environment.CurrentDirectory, scriptFileName));
                        }

                        using (var writer = new MakeWriter(writeBOM:false))
                        {
                            writer.FileName = scriptfile.Path;

                            switch (PlatformUtil.Platform)
                            {
                                case PlatformUtil.Windows:
                                default:
                                    {
                                        //writer.WriteLine("@echo off");
                                        writer.Write(script);
                                        program = scriptfile;
                                    }
                                    break;
                                case PlatformUtil.Unix:
                                case PlatformUtil.OSX:
                                    {
                                        writer.WriteLine("#!/bin/bash");
                                        writer.Write(script.Replace("\r\n", "\n"));

                                        program = new PathString("bash", PathString.PathState.File);
                                        args = new string[] { scriptfile.Path };
                                    }
                                    break;
                            }
                        }

                        switch (PlatformUtil.Platform)
                        {
                            case PlatformUtil.Unix:
                            case PlatformUtil.OSX:
                                exitcode = Program.Execute(project, new PathString("chmod"), new string[] { "777", scriptfile.Path });
                                executed = "chmod 777";
                                break;
                            default:
                                break;
                        }

                        if (exitcode == 0)
                        {
                            exitcode = Program.Execute(project, program, args, workingdir: command.WorkingDir, env: command.Env);
                            executed = program.Path + ((args != null && args.Count() > 0) ? args.ToString(" ") : String.Empty);
                        }

                    }
                }

                if (failonerror && exitcode != 0)
                {
                    throw new BuildException(String.Format("External program {0} exit code '{1}'.", executed, exitcode));
                }
            }
            return exitcode;
        }

        public static int ExecuteCommand(this Project project, CommandWithDependencies command, bool failonerror = true)
        {
            int exitcode = 0;
            if (command != null)
            {
                if (command.NeedsRunning(project))
                {
                    project.ExecuteCommand(command as Command, failonerror);
                }
                else
                {
                    project.Log.Info.WriteLine("      {0} is up-to-date.", command.Message);
                }
            }
            return exitcode;
        }


        public enum BuildStepOrder { TargetFirst, CommandFirst }

        public static void ExecuteBuildStep(this Project project, BuildStep step, BuildStepOrder order = BuildStepOrder.TargetFirst)
        {
            var depResult = CheckDependency.Execute(project, step.InputDependencies, step.OutputDependencies);//, PathString dependencyFile = null, PathString dummyOutputFile = null)

            if (!depResult.IsUpToDate)
            {
                switch (order)
                {

                    case BuildStepOrder.TargetFirst:
                    default:
                        if (step.TargetCommands.Count > 0)
                        {
                            project.ExecuteTargetCommands(step.TargetCommands);
                        }
                        else
                        {
                            project.ExecuteCommands(step.Commands);
                        }
                        break;

                    case BuildStepOrder.CommandFirst:
                        if (step.Commands.Count > 0)
                        {
                            project.ExecuteCommands(step.Commands);
                        }
                        else
                        {
                            project.ExecuteTargetCommands(step.TargetCommands);
                        }
                        break;
                }
            }

            depResult.Update();
        }

    }

}
