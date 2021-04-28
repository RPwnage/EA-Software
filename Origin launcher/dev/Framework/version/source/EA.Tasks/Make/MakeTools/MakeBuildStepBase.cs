// Originally based on NAnt - A .NET build tool
// Copyright (C) 2003-2018 Electronic Arts Inc.
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
// Electronic Arts (Frostbite.Team.CM@ea.com)

using System;
using System.Collections.Generic;
using System.Linq;

using NAnt.Core;
using NAnt.Core.Util;

using EA.Eaconfig.Modules.Tools;
using EA.Eaconfig.Build;
using EA.FrameworkTasks.Model;

using EA.Make.MakeItems;


namespace EA.Make.MakeTools
{
	class MakeBuildStepBase<TType> : MakeToolBase
	{
		public List<MakeTarget> StepTargets = new List<MakeTarget>();

		protected readonly TType BuildStepTool;
		protected readonly IModule Module;

		private readonly EA.FrameworkTasks.Model.ProjectExtensions.BuildStepOrder Order = EA.FrameworkTasks.Model.ProjectExtensions.BuildStepOrder.TargetFirst;


		public MakeBuildStepBase(MakeModuleBase makeModule, IModule module, TType tool)
			: base(makeModule, module.Package.Project, tool as Tool)
		{
			BuildStepTool = tool;
			Module = module;

		}

		protected int ConstructToolTarget(IEnumerable<Command> commands, IEnumerable<TargetCommand> targetCommands, List<PathString> inputDependencies)
		{
			int nonEmptyCommands = 0;

			if (inputDependencies != null)
			{
				foreach (MakeTarget stepTargets in StepTargets)
				{
					stepTargets.Prerequisites += inputDependencies.Select(p => MakeModule.ToMakeFilePath(p.Path));
				}
			}

			switch (Order)
			{
				case EA.FrameworkTasks.Model.ProjectExtensions.BuildStepOrder.CommandFirst:
					if (!commands.IsNullOrEmpty())
					{
						nonEmptyCommands += MakeCommands(commands);
					}
					else
					{
						nonEmptyCommands += MakeCommandTargets(targetCommands);
					}
					break;				
				case EA.FrameworkTasks.Model.ProjectExtensions.BuildStepOrder.TargetFirst:
				default:
					if (!targetCommands.IsNullOrEmpty())
					{
						nonEmptyCommands += MakeCommandTargets(targetCommands);
					}
					else
					{
						nonEmptyCommands += MakeCommands(commands);
					}
					break;
			}
			return nonEmptyCommands;
		}


		protected int MakeCommands(IEnumerable<Command> commands, string scriptFilename = null)
		{
			int nonEmptyCommands = 0;
			if (commands != null)
			{
				foreach (var command in commands)
				{
					nonEmptyCommands += MakeCommand(command, scriptFilename);
				}
			}
			return nonEmptyCommands;
		}

		protected int MakeCommand(Command command, string scriptFilename = null)
		{
			int nonEmptyCommands = 0;

			if (command != null)
			{
				var comment = String.IsNullOrEmpty(command.Description) ? command.Message : command.Description;
				if (!String.IsNullOrEmpty(comment))
				{
					StepTargets.First().AddRuleCommentLine(comment);
				}

				AddCreateDitectoriesRule(StepTargets.First(), command.CreateDirectories);

				if (command.IsKindOf(Command.Program))
				{
					StepTargets.First().AddRuleLine(@"@{0} {1}", command.Executable.Path.Quote(), command.Options.ToString(" ", o=>o.TrimWhiteSpace()));
					nonEmptyCommands++;
				}
				else
				{
					// Makefile requires each "rule" line being a separate command.  If we have "if block" spreading over
					// multiple lines, we need to add continuation and command separator/terminator symbol to allow them to be
					// spread over to multiple lines.
					List<string> ifBlockRules = new List<string>();
					string cmdContinuation = " \\";
					string ifStatementStartBracket = "(";
					string ifStatementEndBracket = ")";
					string cmdSeparator = "&";
					if (!PlatformUtil.IsWindows)
					{
						ifStatementStartBracket = "then";
						ifStatementEndBracket = "fi";
						cmdSeparator = ";";
					}
					foreach (var line in command.CommandLine.LinesToArray())
					{
						if (line.Equals(BuildTool.SCRIPT_ERROR_CHECK, StringComparison.OrdinalIgnoreCase))
						{
							continue;
						}
						if (ifBlockRules.Any())
						{
							// If we are inside an if-block, just collect those lines for now and then we output them
							// all at once when we reached the end of the if-block.
							string trimmedLine = line.Trim();
							if (!trimmedLine.EndsWith(ifStatementEndBracket))
							{
								ifBlockRules.Add(line);
							}
							else
							{
								// We reached the end of if-block, now re-format and output the rules all at once.

								// Under linux/osx bash shell, this is actually a command "terminator" not separator,
								// so we need to add this command terminator symbol for every line!
								int cmdSeparatorCount = PlatformUtil.IsWindows ? ifBlockRules.Count - 1 : ifBlockRules.Count;
								for (int lineIdx=0; lineIdx < ifBlockRules.Count; ++lineIdx)
								{
									string newLine = ifBlockRules[lineIdx];
									if (!newLine.EndsWith(cmdContinuation))
									{
										if (lineIdx > 0 && lineIdx != cmdSeparatorCount)
										{
											// On windows, we only add this if we're not on the last line.
											// On osx/linux bash, we need this for everyline.
											newLine += cmdSeparator;
										}
										newLine += cmdContinuation;
									}
									if (lineIdx == 0 && !newLine.StartsWith("@"))
									{
										// Only add '@' on the very first "if" statement line.
										newLine = "@" + newLine;
									}
									StepTargets.First().AddRuleLine(newLine);
								}
								ifBlockRules.Clear();
								// Now add in the current if-statement end line.
								StepTargets.First().AddRuleLine(line);
							}
						}
						else
						{
							if ((line.StartsWith("@if ") || line.StartsWith("if ")) && line.Contains(ifStatementStartBracket) && !line.EndsWith(ifStatementEndBracket))
							{
								// Starting a new "if block"
								ifBlockRules.Add(line);
							}
							else
							{
								// Normal command line.  Only add "@" if it isn't already on command line.
								if (line.StartsWith("@"))
									StepTargets.First().AddRuleLine(line);
								else
									StepTargets.First().AddRuleLine("@" + line);
							}
						}
						nonEmptyCommands++;
					}
				}
			}
			return nonEmptyCommands;
		}

		protected int MakeCommandTargets(IEnumerable<TargetCommand> commands)
		{
			int nonEmptyCommands = 0;
			if (commands != null)
			{
				foreach (var command in commands)
				{
					nonEmptyCommands += MakeCommandTarget(command);
				}
			}
			return nonEmptyCommands;
		}


		public int MakeCommandTarget(TargetCommand targetCommand)
		{
			int nonEmptyCommands = 0;
			if (targetCommand != null)
			{
				StepTargets.First().AddRuleCommentLine("Executing NAnt target '{0}'.", targetCommand.Target);

				//Func<string, string> normalizeFunc = (X) => MakeModule.ToMakeFilePath(X);

				Func<string, string> normalizeFunc = null;

				//if (MakeModule.IsPortable)
				//{
				//    normalizeFunc = (X) => MakeModule.PortableData.NormalizeIfPathString(X, OutputDir.Path);
				//}

				var nant_cmd = NantInvocationProperties.TargetToNantCommand(Module, targetCommand, addGlobal: true, normalizePathString: normalizeFunc);

				StepTargets.First().AddRuleLine("@" + nant_cmd);
				nonEmptyCommands++;
			}
			return nonEmptyCommands;
		}

		protected void AddSerializationTargets(IEnumerable<string> outputDependenciesCollection = null, IEnumerable<string> inputDependenciesCollection = null)
		{
			// create serialization targets - in order for parallel make to handle multiple output properly each output must depend on first target
			// in order to serialize dependencies (otherwise target will be run multiple times)
			if (outputDependenciesCollection != null)
			{
				foreach (string multiOutputPath in outputDependenciesCollection.Skip(1))
				{
					string serializationTargetName = MakeModule.ToMakeFilePath(multiOutputPath);
					MakeTarget serializationTarget = MakeModule.MakeFile.Target(serializationTargetName);
					if (inputDependenciesCollection != null)
					{
						serializationTarget.Prerequisites += inputDependenciesCollection;
					}
					serializationTarget.Prerequisites += StepTargets.First().Label;
					StepTargets.Add(serializationTarget);

					MakeModule.AddFileToClean(multiOutputPath);
				}
			}
		}

		protected string GetPrimaryTargetName(IEnumerable<string> outputDependenciesCollection, string defaultName, out bool isPhony)
		{
			if (outputDependenciesCollection != null && outputDependenciesCollection.Any())
			{
				isPhony = false;
				return MakeModule.ToMakeFilePath(outputDependenciesCollection.First());
			}
			isPhony = true;
			return defaultName;
		}
	}
}
