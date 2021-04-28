// Originally based on NAnt - A .NET build tool
// Copyright (C) 2003-2015 Electronic Arts Inc.
// Copyright (C) 2001-2003 Gerry Shaw
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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
// 
// Gerry Shaw (gerry_shaw@yahoo.com)
// Ian MacLean (ian_maclean@another.com)
// Electronic Arts (Frostbite.Team.CM@ea.com)

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;

using NAnt.Core;
using NAnt.Core.PackageCore;
using NAnt.Core.Tasks;
using NAnt.Core.Util;
using NAnt.Core.Writers;
using NAnt.Shared.Properties;

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
			if (project.NamedObjects.TryGetValue(PackageObjectId, out object obj))
			{
				package = (IPackage)obj;
			}
			else
			{
				package = null;
			}
			return package != null;
		}

		public static bool TrySetPackage(this Project project, Release release, string config, out IPackage package)
		{
			package = project.BuildGraph().GetOrAddPackage(release, config);
			package.Project = project;

			if (package.PackageBuildDir == null && project.Properties.Contains(PackageProperties.PackageBuildDirectoryPropertyName))
			{
				package.PackageBuildDir = PathString.MakeNormalized(project.Properties[PackageProperties.PackageBuildDirectoryPropertyName]);
			}
			if (package.PackageGenDir == null && project.Properties.Contains(PackageProperties.PackageGenDirectoryPropertyName))
			{
				package.PackageGenDir = PathString.MakeNormalized(project.Properties[PackageProperties.PackageGenDirectoryPropertyName]);
			}

			return project.NamedObjects.TryAdd(PackageObjectId, package);
		}

		public static Project.TargetStyleType GetRootTargetStyle(this Project project)
		{
			return 
			(
				Project.TargetStyleType)project.NamedObjects.GetOrAdd(RootTargetStyleId, (key) => 
				{ 
					lock (Project.GlobalNamedObjects)
					{
						Project.GlobalNamedObjects.Add(RootTargetStyleId);
					}
					return project.TargetStyle;
				} 
			);
		}

		public static void ResetBuildGraph(this Project project)
		{
			project.NamedObjects.Remove(BuildGraphObjectId);
			project.NamedObjects.Remove(PackageObjectId);
			Model.BuildGraph.WasReset = true;
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
			if (!project.NamedObjects.TryGetValue(ModuleDependencyConstaintsId, out object constraints))
			{
				return null;
			}

			return constraints as List<ModuleDependencyConstraints>;
		}

		public static bool SetConstraints(this Project project, List<ModuleDependencyConstraints> constraints)
		{
			return project.NamedObjects.TryAdd(ModuleDependencyConstaintsId, constraints);
		}

		public static void AddConstraint(this Project project, BuildGroups group, string modulename = "", uint type = 0)
		{
			project.NamedObjects.AddOrUpdate(ModuleDependencyConstaintsId, (k) =>
			{
				var list = new List<ModuleDependencyConstraints>();
				list.Add(new ModuleDependencyConstraints(group, modulename, type));
				return list;
			}, (k, v) =>
			{
				var list = v as List<ModuleDependencyConstraints>;
				if (list == null)
					list = new List<ModuleDependencyConstraints>();
				list.Add(new ModuleDependencyConstraints(group, modulename, type));
				return list;
			});
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
				int cmdIdx = 0;
				foreach (var command in commands)
				{
					ExecuteCommand(project, command, failonerror, (scriptFilename == null ? null : scriptFilename + "_cmd" + cmdIdx.ToString() + "_"));
					++cmdIdx;
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

						// script filename is appended with a GUID probably because we cannot guarantee filename uniqueness especially when the same module for different configs can be executed
						// concurrently by multiple threads and we can't rely on input "scriptFilename" being unique for all situation.
						var scriptFileName = (scriptFilename ?? "command") + Hash.MakeGUIDfromString(project.ScriptInitLock.GetHashCode()+command.CommandLine) + ".bat";
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
							File.Delete(scriptfile.Path);
						}

					}
				}

				if (failonerror && exitcode != 0)
				{
					throw new BuildException(ProcessUtil.DecodeProcessExitCode(executed, exitcode));
				}
			}
			return exitcode;
		}

		public static int ExecuteCommand(this Project project, CommandWithDependencies command, bool failonerror = true, string scriptFilename = null)
		{
			int exitcode = 0;
			if (command != null)
			{
				if (command.NeedsRunning(project))
				{
					project.ExecuteCommand(command as Command, failonerror, scriptFilename);
				}
				else
				{
					project.Log.Info.WriteLine("      {0} is up-to-date.", command.Message);
				}
			}
			return exitcode;
		}


		public enum BuildStepOrder { TargetFirst, CommandFirst }

		public static void ExecuteBuildStep(this Project project, BuildStep step, BuildStepOrder order = BuildStepOrder.TargetFirst, string moduleName = null)
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
							project.ExecuteCommands(step.Commands, scriptFilename: (moduleName == null ? null : moduleName + "_" + step.Name));
						}
						break;

					case BuildStepOrder.CommandFirst:
						if (step.Commands.Count > 0)
						{
							project.ExecuteCommands(step.Commands, scriptFilename: (moduleName == null ? null : moduleName + "_" + step.Name));
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
