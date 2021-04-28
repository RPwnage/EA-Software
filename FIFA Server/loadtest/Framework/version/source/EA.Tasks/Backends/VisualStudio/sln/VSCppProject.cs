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
using System.Xml;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
using System.IO;
using System.Globalization;
using System.Runtime.InteropServices;

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Writers;
using NAnt.Core.Events;
using NAnt.Core.Logging;
using EA.FrameworkTasks.Model;

using EA.Eaconfig.Core;
using EA.Eaconfig.Build;
using EA.Eaconfig.Modules;
using EA.Eaconfig.Modules.Tools;
using System.Collections.Concurrent;
using System.Runtime.InteropServices.ComTypes;

namespace EA.Eaconfig.Backends.VisualStudio
{
	internal abstract class VSCppProject : VSProjectBase
	{
		protected VSCppProject(VSSolutionBase solution, IEnumerable<IModule> modules)
			:
			base(solution, modules, VCPROJ_GUID)
		{
			InitFunctionTables();

			// Get project configuration that equals solution startup config:
			Configuration startupconfig;
			if (ConfigurationMap.TryGetValue(solution.StartupConfiguration, out startupconfig))
			{
				StartupModule = Modules.Single(m => m.Configuration == startupconfig) as Module;
			}
			else
			{
				StartupModule = Modules.First() as Module;
			}
		}

		protected abstract void InitFunctionTables();

		protected readonly Module StartupModule;

		#region Virtual Protected Methods

		#region Utility Methods

		internal bool ExplicitlyIncludeDependentLibraries
		{
			set { _explicitlyIncludeDependentLibraries = value; }
		}

		protected virtual string DefaultTargetFrameworkVersion
		{
			// Overridden by VSCppMsbuildProject, but I can't remove it.
			get { return null; }
		}

		protected virtual string TargetFrameworkVersion
		{
			// Overridden by VSCppMsbuildProject, but I can't remove it.
			get { return null; }
		}

		internal override string RootNamespace { get => (StartupModule is Module_Native) ? (StartupModule as Module_Native).RootNamespace : String.Empty; }

		protected virtual string Keyword
		{
			get
			{
				return null;
			}
		}

		protected virtual VSConfig.CharacterSetTypes CharacterSet(Module module)
		{
			VSConfig.CharacterSetTypes type = VSConfig.CharacterSetTypes.MultiByte;

			if (module.Tools != null)
			{
				CcCompiler compiler = module.Tools.SingleOrDefault(t => t.ToolName == "cc") as CcCompiler;

				if (compiler != null)
				{
					type = VSConfig.GetCharacterSetFromDefines(compiler.Defines);
				}
			}
			//IMTODO: Makestyle projects allow for set of defines

			return type;
		}

		protected virtual IEnumerable<string> GetAllDefines(CcCompiler compiler, Configuration configuration)
		{
			// this method is explicitly called by VSCppMsbuildProject
			var defines = new SortedSet<string>(compiler.Defines);
			defines.Add("EA_SLN_BUILD=1");
			return defines;
		}

		protected virtual string GetAllIncludeDirectories(CcCompiler compiler, bool addQuotes)
		{
			if (!addQuotes)
			{
				return compiler.IncludeDirs.ToString(";", p => GetProjectPath(p));
			}
			else
			{
				return compiler.IncludeDirs.ToString(";", p => GetProjectPath(p).Quote());
			}
		}

		protected virtual string GetAllUsingDirectories(CcCompiler compiler)
		{
			return compiler.UsingDirs.ToString(";", p => GetProjectPath(p).Quote());
		}

		protected virtual IEnumerable<PathString> GetAllLibraries(Linker link, Module module)
		{
			var dependents = (module.Package.Project.Properties.GetBooleanProperty(Project.NANT_PROPERTY_TRANSITIVE)
				? module.Dependents.Flatten(DependencyTypes.Build).Where(d => d.IsKindOf(DependencyTypes.Link))
				: module.Dependents.Where(d => d.IsKindOf(DependencyTypes.Build) && d.IsKindOf(DependencyTypes.Link))).Select(d => d.Dependent);

			if (BuildGenerator.IncludedModules != null)
				dependents = dependents.Intersect(BuildGenerator.IncludedModules);

			if (_explicitlyIncludeDependentLibraries)
			{
				return link.Libraries.FileItems.Select(item => item.Path);
			}


			// Note that usually Managed C++ do not export .lib library (just .dll assemblies).  So Visual Studio by default won't
			// link with any export .lib from any Managed C++ dependent project (even if Project Reference is properly set).  However,
			// we ran into a use case where some team actually export unmanaged code in their Managed C++ module and created a .lib
			// library that needs to be linked with.  So the following code allow the managed C++'s lib to be in the final libraries list.

			// These dependents are removed as they've already been setup as Project References. Visual Studio will automatically pass Project References into the linker command.
			var deplibraries = dependents.Where(d => !d.IsKindOf(Module.Managed)).Select(d => d.LibraryOutput()).Where(s => s != null);

			var libraryPaths = link.Libraries.FileItems.Select(item => item.Path).Except(deplibraries);

			return libraryPaths;
		}

		protected virtual IEnumerable<PathString> GetAllLibraryDirectories(Linker link)

		{
			return link.LibraryDirs;

		}

		protected virtual void InitCompilerToolProperties(Module module, Configuration configuration, string includedirs, string usingdirs, string defines, out IDictionary<string, string> nameToXMLValue, IDictionary<string, string> nameToDefaultXMLValue)
		{
			// called by VSCppMsbuildProject
			nameToXMLValue = new SortedDictionary<string, string>();



			//**********************************
			//* Set defaults that should exist before the switch parsing
			//* These are either something that a switch can add to (if switch setOnce==false)
			//* or something that ever can't be changed (if switch setOnce==true)
			//**********************************

			//Turn the include dirs from '\n' separated to ; separated
			if (!String.IsNullOrEmpty(includedirs))
				nameToXMLValue.Add("AdditionalIncludeDirectories", includedirs);

			//Turn the include dirs from '\n' separated to ; separated
			if (!String.IsNullOrEmpty(usingdirs))
				nameToXMLValue.Add("AdditionalUsingDirectories", usingdirs);

			//Turn the defines from '\n' separated to ; separated
			if (!String.IsNullOrEmpty(defines))
			{
				nameToXMLValue.Add("PreprocessorDefinitions", defines);
			}

			//*****************************
			//* Set the defaults that are used if a particular attribute has no value after the parsing
			//*****************************

			// DAVE-FUTURE-REFACTOR-TODO: would be better if this initial pre-state was data driven from options file
			// than being hardcoded here

			nameToDefaultXMLValue.Add("MinimalRebuild", "FALSE");
			nameToDefaultXMLValue.Add("BufferSecurityCheck", "FALSE");

			if (configuration.Compiler == "vc") // msvc platforms default to exceptions on, we default to off unless explicity enabled
			{
				nameToDefaultXMLValue.Add("ExceptionHandling", "FALSE");
			}
		}

		protected abstract void InitLinkerToolProperties(Configuration configuration, Linker linker, IEnumerable<PathString> libraries, IEnumerable<PathString> libraryDirs, IEnumerable<PathString> objects, string defaultOutput, out IDictionary<string, string> nameToXMLValue, IDictionary<string, string> nameToDefaultXMLValue);

		protected abstract void InitLibrarianToolProperties(Configuration configuration, IEnumerable<PathString> objects, string defaultOutput, out IDictionary<string, string> nameToXMLValue, IDictionary<string, string> nameToDefaultXMLValue);

		private string GetToolPathFromProject(IModule module, string toolName, string toolOptionSet, bool useProperty = true)
		{
			string toolpath = module.Package.Project.Properties[toolName];

			if (!useProperty || String.IsNullOrEmpty(toolpath))
			{
				OptionSet programOptionSet;
				if (module.Package.Project.NamedOptionSets.TryGetValue(toolOptionSet, out programOptionSet))
				{
					toolpath = programOptionSet.Options[toolName];
				}
			}
			return toolpath;
		}

		protected override IEnumerable<KeyValuePair<string, IEnumerable<KeyValuePair<string, string>>>> GetUserData(ProcessableModule module)
		{
			var userdata = new List<KeyValuePair<string, IEnumerable<KeyValuePair<string, string>>>>();

			if (module != null && module.AdditionalData.DebugData != null)
			{
				var debugSettings = new List<KeyValuePair<string, string>>();

				userdata.Add(new KeyValuePair<string, IEnumerable<KeyValuePair<string, string>>>("DebugSettings", debugSettings));

				if (!String.IsNullOrEmpty(module.AdditionalData.DebugData.Command.Executable.Path))
				{
					debugSettings.Add(new KeyValuePair<string, string>("Command", module.AdditionalData.DebugData.Command.Executable.Path));
				}

				if (!String.IsNullOrEmpty(module.AdditionalData.DebugData.Command.WorkingDir.Path))
				{
					debugSettings.Add(new KeyValuePair<string, string>("WorkingDirectory", module.AdditionalData.DebugData.Command.WorkingDir.Path));
				}

				string commandargs = module.AdditionalData.DebugData.Command.Options.ToString(" ");
				if (!String.IsNullOrEmpty(commandargs))
				{
					debugSettings.Add(new KeyValuePair<string, string>("CommandArguments", commandargs));
				}

				if (!String.IsNullOrEmpty(module.AdditionalData.DebugData.RemoteMachine))
				{
					debugSettings.Add(new KeyValuePair<string, string>("RemoteMachine", module.AdditionalData.DebugData.RemoteMachine));
				}
			}

			return userdata;
		}

		#endregion Utility Methods

		#region Write Methods

		protected abstract void WriteVisualStudioProject(IXmlWriter writer);

		protected abstract void WritePlatforms(IXmlWriter writer);

		protected abstract void WriteConfigurations(IXmlWriter writer);

		protected abstract void WriteOneTool(IXmlWriter writer, string vcToolName, Tool tool, Module module, IDictionary<string, string> nameToDefaultXMLValue = null, FileItem file = null, OptionSet customtoolproperties = null);

		private enum RegKind
		{
			RegKind_Default = 0,
			RegKind_Register = 1,
			RegKind_None = 2
		}

		[DllImport("oleaut32.dll", CharSet = CharSet.Unicode, PreserveSig = false)]
		private static extern void LoadTypeLibEx(string strTypeLibName, RegKind regKind, out System.Runtime.InteropServices.ComTypes.ITypeLib typeLib);

		protected bool GenerateInteropAssembly(PathString comLib, out Guid libGuid, out System.Runtime.InteropServices.ComTypes.TYPELIBATTR typeLibAttr)
		{
			if (!File.Exists(comLib.Path))
			{
				string msg = String.Format("Unable to generate interop assembly for COM reference, file '{0}' not found", comLib.Path);
				throw new BuildException(msg);
			}

			System.Runtime.InteropServices.ComTypes.ITypeLib typeLib = null;
			try
			{
				LoadTypeLibEx(comLib.Path, RegKind.RegKind_None, out typeLib);
			}
			catch (Exception ex)
			{
				string msg = String.Format("Unable to generate interop assembly for COM reference '{0}'.", comLib.Path);
				new BuildException(msg, ex);
			}
			if (typeLib == null)
			{
				string msg = String.Format("Unable to generate interop assembly for COM reference '{0}'.", comLib.Path);
				new BuildException(msg);
			}

			IntPtr pTypeLibAttr = IntPtr.Zero;
			typeLib.GetLibAttr(out pTypeLibAttr);

			typeLibAttr = (System.Runtime.InteropServices.ComTypes.TYPELIBATTR)Marshal.PtrToStructure(pTypeLibAttr, typeof(System.Runtime.InteropServices.ComTypes.TYPELIBATTR));

#if NETFRAMEWORK
			libGuid = Marshal.GetTypeLibGuid(typeLib);
#else
			libGuid = typeLibAttr.guid;
#endif

			return true;
		}

		protected abstract void WriteComReferences(IXmlWriter writer, ICollection<FileEntry> comreferences);

		protected virtual void WriteFiles(IXmlWriter writer)
		{
			var files = GetAllFiles(tool =>
			{
				List<Tuple<FileSet, uint, Tool>> filesets = new List<Tuple<FileSet, uint, Tool>>();
				CcCompiler compilerTool = tool as CcCompiler;
				if (compilerTool != null)
				{
					filesets.Add(Tuple.Create(compilerTool.SourceFiles, FileEntry.Buildable, tool));
				}
				AsmCompiler asmTool = tool as AsmCompiler;
				if (asmTool != null)
				{
					filesets.Add(Tuple.Create(asmTool.SourceFiles, FileEntry.Buildable, tool));
				}
				BuildTool buildTool = tool as BuildTool;
				if (buildTool != null)
				{
					filesets.Add(Tuple.Create(buildTool.Files, FileEntry.Buildable, tool));
					if (buildTool.ToolName == "rc")
					{
						filesets.Add(Tuple.Create(buildTool.InputDependencies, FileEntry.Buildable | FileEntry.Resources, null as Tool));
					}
					else if (buildTool.ToolName == "resx")
					{
						filesets.Add(Tuple.Create(buildTool.InputDependencies, FileEntry.Buildable | FileEntry.Resources, null as Tool));
					}
				}

				return filesets;
			}).Select(e => e.Value).OrderBy(e => e.Path.Path);

			WriteFilesWithFilters(writer, FileFilters.ComputeFilters(files, AllConfigurations, Modules));
		}

		protected abstract void WriteFilesWithFilters(IXmlWriter writer, FileFilters filters);

		#region Write Tools

		protected virtual void WriteCustomBuildRuleTools(IXmlWriter writer, Module_Native module)
		{
			if (module != null && module.CustomBuildRuleOptions != null)
			{
				foreach (var o in module.CustomBuildRuleOptions.Options)
				{
					WriteOneCustomBuildRuleTool(writer, o.Key, o.Value, module);
				}
			}
		}

		protected abstract void WriteOneCustomBuildRuleTool(IXmlWriter writer, string name, string toolOptionsName, IModule module);

		protected abstract void WriteBuildEvents(IXmlWriter writer, string name, string toolname, uint type, Module module);

		protected abstract void WriteXMLDataGeneratorTool(IXmlWriter writer, string name, Tool tool, Configuration config, Module module, IDictionary<string, string> nameToDefaultXMLValue = null, FileItem file = null);

		protected abstract void WriteWebServiceProxyGeneratorTool(IXmlWriter writer, string name, Tool tool, Configuration config, Module module, IDictionary<string, string> nameToDefaultXMLValue = null, FileItem file = null);

		protected abstract void WriteFxCompileTool(IXmlWriter writer, string name, Tool tool, Configuration config, Module module, IDictionary<string, string> nameToDefaultXMLValue = null, FileItem file = null);

		protected abstract void WriteWavePsslcTool(IXmlWriter writer, string name, Tool tool, Configuration config, Module module, IDictionary<string, string> nameToDefaultXMLValue = null, FileItem file = null);

		protected abstract void WriteMIDLTool(IXmlWriter writer, string name, Tool tool, Configuration config, Module module, IDictionary<string, string> nameToDefaultXMLValue = null, FileItem file = null);

		protected abstract void WriteAsmCompilerTool(IXmlWriter writer, string name, Tool tool, Configuration config, Module module, IDictionary<string, string> nameToDefaultXMLValue = null, FileItem file = null);

		protected abstract void WriteCompilerTool(IXmlWriter writer, string name, Tool tool, Configuration config, Module module, IDictionary<string, string> nameToDefaultXMLValue = null, FileItem file = null);

		protected abstract void WriteLibrarianTool(IXmlWriter writer, string name, Tool tool, Configuration config, Module module, IDictionary<string, string> nameToDefaultXMLValue = null, FileItem file = null);

		protected abstract void WriteManagedResourceCompilerTool(IXmlWriter writer, string name, Tool tool, Configuration config, Module module, IDictionary<string, string> nameToDefaultXMLValue = null, FileItem file = null);

		protected abstract void WriteResourceCompilerTool(IXmlWriter writer, string name, Tool tool, Configuration config, Module module, IDictionary<string, string> nameToDefaultXMLValue = null, FileItem file = null);

		protected abstract void WriteLinkerTool(IXmlWriter writer, string name, Tool tool, Configuration config, Module module, IDictionary<string, string> nameToDefaultXMLValue = null, FileItem file = null);

		protected abstract void WriteALinkTool(IXmlWriter writer, string name, Tool tool, Configuration config, Module module, IDictionary<string, string> nameToDefaultXMLValue = null, FileItem file = null);

		protected abstract void WriteManifestTool(IXmlWriter writer, string name, Tool tool, Configuration config, Module module, IDictionary<string, string> nameToDefaultXMLValue = null, FileItem file = null);

		protected abstract void WriteXDCMakeTool(IXmlWriter writer, string name, Tool tool, Configuration config, Module module, IDictionary<string, string> nameToDefaultXMLValue = null, FileItem file = null);

		protected abstract void WriteBscMakeTool(IXmlWriter writer, string name, Tool tool, Configuration config, Module module, IDictionary<string, string> nameToDefaultXMLValue = null, FileItem file = null);

		protected abstract void WriteFxCopTool(IXmlWriter writer, string name, Tool tool, Configuration config, Module module, IDictionary<string, string> nameToDefaultXMLValue = null, FileItem file = null);

		protected abstract void WriteMakeTool(IXmlWriter writer, string name, Configuration config, Module module, IDictionary<string, string> nameToDefaultXMLValue = null, FileItem file = null);

		protected virtual void GetMakeToolCommands(Module module, out string build, out string rebuild, out string clean, out string output)
		{
			var buildSb = new StringBuilder();
			var rebuildSb = new StringBuilder();
			var cleanSb = new StringBuilder();

			foreach (var step in module.BuildSteps)
			{
				if (step.IsKindOf(BuildStep.Build))
				{
					foreach (var command in step.Commands)
					{
						buildSb.AppendLine(command.CommandLine);
					}
				}
				else if (step.IsKindOf(BuildStep.Clean))
				{
					foreach (var command in step.Commands)
					{
						cleanSb.AppendLine(command.CommandLine);
					}
				}
				else if (step.IsKindOf(BuildStep.ReBuild))
				{
					foreach (var command in step.Commands)
					{
						rebuildSb.AppendLine(command.CommandLine);
					}
				}
			}

			build = buildSb.ToString().TrimWhiteSpace();
			rebuild = rebuildSb.ToString().TrimWhiteSpace();
			clean = cleanSb.ToString().TrimWhiteSpace();
			output = module.PropGroupValue("MakeOutput").TrimWhiteSpace();

			if (BuildGenerator.IsPortable)
			{
				build = BuildGenerator.PortableData.NormalizeCommandLineWithPathStrings(build, OutputDir.Path);
				rebuild = BuildGenerator.PortableData.NormalizeCommandLineWithPathStrings(rebuild, OutputDir.Path);
				clean = BuildGenerator.PortableData.NormalizeCommandLineWithPathStrings(clean, OutputDir.Path);
				output = BuildGenerator.PortableData.NormalizeCommandLineWithPathStrings(output, OutputDir.Path);
			}
		}


		protected abstract void WriteAppVerifierTool(IXmlWriter writer, string name, Tool tool, Configuration config, Module module, IDictionary<string, string> nameToDefaultXMLValue = null, FileItem file = null);

		protected abstract void WriteWebDeploymentTool(IXmlWriter writer, string name, Tool tool, Configuration config, Module module, IDictionary<string, string> nameToDefaultXMLValue = null, FileItem file = null);

		protected abstract void WriteVCCustomBuildTool(IXmlWriter writer, string name, Tool tool, Configuration config, Module module, IDictionary<string, string> nameToDefaultXMLValue = null, FileItem file = null);

		#endregion Write Tools

		protected abstract int SetUserFileConfiguration(XmlNode configurationsEl, ProcessableModule module, Func<ProcessableModule, IEnumerable<KeyValuePair<string, string>>> getGeneratedUserData = null);

		#endregion Write Methods

		#endregion Virtual Protected Methods

		#region Protected Methods

		struct OptionAndDirectives : IEquatable<OptionAndDirectives>
		{
			public readonly string Option;
			public readonly object Directives;

			public OptionAndDirectives(string opt, object di)
			{
				Option = opt;
				Directives = di;
				if (opt == null && di == null)
				{
					throw new BuildException("FRAMEWORK ERROR: Attempt to setup 'OptionAndDirectives' object with null input for both Option and Directives!");
				}
				if (opt == null)
				{
					throw new BuildException("FRAMEWORK ERROR: Attempt to setup 'OptionAndDirectives' object with null Option input!");
				}
				else if (di == null)
				{
					throw new BuildException(string.Format("FRAMEWORK ERROR: Attempt to setup 'OptionAndDirectives' object with null Directives input for option value {0}", opt));
				}
			}

			public override int GetHashCode()
			{
				// This function assumes that input Option AND Directives data member cannot be null!
				return Option.GetHashCode() ^ Directives.GetHashCode();
			}

			public bool Equals(OptionAndDirectives other)
			{
				return String.Equals(Option, other.Option, StringComparison.Ordinal) && ReferenceEquals(Directives, other.Directives);
			}
		}

		static ConcurrentDictionary<OptionAndDirectives, List<KeyValuePair<VSConfig.SwitchInfo, string[]>>> s_validParseDirectives = new ConcurrentDictionary<OptionAndDirectives, List<KeyValuePair<VSConfig.SwitchInfo, string[]>>>();


		/// <summary>
		/// ProcessSwitches -	Takes a newline separated collection of command line switches; parses them according to
		///						the parsing directives and then puts the translated result (XML attributes) into a hash
		///						table.
		/// </summary>
		/// <param name="project"></param>
		/// <param name="parseDirectives">An List of SwitchInfo's dictating the translation from command line to XML</param>
		/// <param name="nameToXMLValue">The hash table that contains values of the XML attributes (string->string map) that were obtained by the parsing</param>
		/// <param name="options"></param>
		/// <param name="taskName"></param>
		/// <param name="errorIfUnrecognized"></param>
		/// <param name="nameToDefaultXMLValue"></param>
		protected void ProcessSwitches(Project project, List<VSConfig.SwitchInfo> parseDirectives, IDictionary<string, string> nameToXMLValue, List<string> options, string taskName, bool errorIfUnrecognized = true, IDictionary<string, string> nameToDefaultXMLValue = null)
		{
			Dictionary<string, string> conditionTokens = new Dictionary<string, string>();
			// Even though the config-vs-version property is created (through set-config-vs-version task) 
			// before the call to "eaconfig-build-graph" and "backend-generator" tasks (in eaconfig's target-vcproj.xml), 
			// this property still doesn't exist under the "module" scope for the non-pc configs.  So we have to 
			// explicity convert this property ourselves.
			conditionTokens.Add("${config-vs-version}", SolutionGenerator.VSVersion);
			// This is temporary placeholder.  Will be updated for each parse directive loop and for each option.
			conditionTokens.Add("%previous_match_exists%", "false");

			//Go through all the switches
			foreach (string option in options.ToList())
			{
				var key = new OptionAndDirectives(option, parseDirectives);
				var optimizedParseDirectives = s_validParseDirectives.GetOrAdd(key, (k) =>
				{
					var dirs = new List<KeyValuePair<VSConfig.SwitchInfo, string[]>>();
					List<VSConfig.SwitchInfo> directives = k.Directives as List<VSConfig.SwitchInfo>;
					if (directives == null)
						return dirs;
					foreach (VSConfig.SwitchInfo si in directives)
					{
						Regex curRegEx = si.Switch;                         //Get the current reg exp of the switch
						Match match = curRegEx.Match(k.Option);             //see if it matches

						//if it doesn't, go on
						if (match.Success)
						{
							var groups = new string[match.Groups.Count];
							int index = 0;
							foreach (var g in match.Groups)
								groups[index++] = g.ToString();
							dirs.Add(new KeyValuePair<VSConfig.SwitchInfo, string[]>(si, groups));
						}
					}
					return dirs;
				});


				//Got through all the switchInfo's that we have and see if one matches
				bool matchFound = false; //marks if we've found a match for the current switch
				conditionTokens["%previous_match_exists%"] = "false";
				foreach (var pair in optimizedParseDirectives)
				{
					VSConfig.SwitchInfo si = pair.Key;
					// Go to next directive if condition doesn't pass.
					if (!EvaluateSwitchCondition(project, si, conditionTokens))
					{
						continue;
					}

					//if the attribute name corresponding the switch is "", we don't need to do anything else
					if (si.XMLName.Length == 0)
					{
						matchFound = true;
						conditionTokens["%previous_match_exists%"] = "true";
						break;
					}

					bool checkForSetOnce = false;                           //Check if setOnce condition has been violated
					if (nameToXMLValue.ContainsKey(si.XMLName) == false)
					{
						nameToXMLValue.Add(si.XMLName, string.Empty);  //The attribute is not set yet, add it
					}
					else
					{
						//the attribute is already set, see if this switch can only be set once
						if (si.SetOnce)
							checkForSetOnce = true;
					}

					//Prepare the type specs (split the string along ','; kill whitespace)
					string[] typeSpecs = si.TypeSpec.Split(new char[] { ',' });
					for (int cnt = 0; cnt < typeSpecs.Length; cnt++)
					{
						typeSpecs[cnt] = typeSpecs[cnt].TrimWhiteSpace();
					}

					var groups = pair.Value;

					string lastValue = nameToXMLValue[si.XMLName];                          //record the previous setting
					Array myArray = Array.CreateInstance(typeof(Object), groups.Length + 1);    //Make an array for passing to String.Format

					//{0} is the previous attribute setting
					//{1} and up are the regexp matches corresponding to $1...$n
					myArray.SetValue(lastValue, 0);
					for (int cnt = 1; cnt < groups.Length; cnt++)
					{
						//If we have a corresponding type specifier, parse it; otherwise use the string
						if (cnt - 1 >= typeSpecs.Length)
						{
							myArray.SetValue(GetPortableOption(si.XMLName, groups[cnt]), cnt);
						}
						else
						{
							switch (typeSpecs[cnt - 1])
							{
								case "": //The type is empty
									if (typeSpecs.Length > 1 || cnt - 1 != 0) //type is empty because type specifier string is malformed
									{
										//IMTODO
										//Error.SetFormat(Log, ErrorCodes.INTERNAL_ERROR, "{0}: Empty format specifier found.", taskName);
									}
									else //type is empty because the type specifier string is empty
										myArray.SetValue(GetPortableOption(si.XMLName, groups[cnt]), cnt);
									break;

								case "string": //it's a string
									myArray.SetValue(GetPortableOption(si.XMLName, groups[cnt]), cnt);
									break;

								case "int": //it's an int
									myArray.SetValue(int.Parse(groups[cnt]), cnt);
									break;

								case "hex": //it's a hex number
									string number = groups[cnt];

									if (number.StartsWith("0x"))
										number = number.Substring(2, number.Length - 2);

									myArray.SetValue(int.Parse(number, NumberStyles.HexNumber), cnt);
									break;

								case "int-hex": //it's a hex number if "0x", integer otherwise
									string ihnumber = groups[cnt];

									if (ihnumber.StartsWith("0x"))
									{
										ihnumber = ihnumber.Substring(2, ihnumber.Length - 2);
										myArray.SetValue(int.Parse(ihnumber, NumberStyles.HexNumber), cnt);
									}
									else
									{
										myArray.SetValue(int.Parse(ihnumber), cnt);
									}
									break;

								default:
									//IMTODO
									//Error.SetFormat(Log, ErrorCodes.INTERNAL_ERROR, "{0}: Unknown format type", typeSpecs[cnt - 1], taskName);
									break;
							}
						}
					}

					//Make the new value by evaluating the format string with the parameter array
					String newValue = String.Format(si.XMLResultString, (Object[])myArray);

					if (BuildGenerator.IsPortable)
					{
						newValue = BuildGenerator.PortableData.NormalizeOptionsWithPathStrings(newValue, OutputDir.Path, "\\s*(\"(?:[^\"]*)\"|(?:[^\\s]+))\\s*").Trim();
					}

					//If we're checking for set once and it's been violated, complain
					if (checkForSetOnce && newValue != lastValue)
					{
						//Error.SetFormat(Log, ErrorCodes.INTERNAL_ERROR, "{3}: {0}: Compiler XML name {1} already has a conflicting setting {2}.", CMLSwitches[index], si.XMLName, lastValue, taskName);
					}

					nameToXMLValue[si.XMLName] = newValue; //set the new value
					matchFound = true; //got our match
					conditionTokens["%previous_match_exists%"] = "true";

					if (si.MatchOnce)
						break;

					//go on to the next parsing spec, because there may be more than 1 spec for 1 switch
				}

				//Unrecognized switch; complain
				if (errorIfUnrecognized && !matchFound)
				{
					//Error.SetFormat(Log, ErrorCodes.INTERNAL_ERROR, "{0}: option {1} has not been recognized.", taskName, CMLSwitches[index]);
				}
			}

			string extranAdditionalOptions = project.Properties.GetPropertyOrDefault("backend.VisualStudio." + taskName + ".additional-options", null);
			if (!String.IsNullOrEmpty(extranAdditionalOptions))
			{
				string additionalOptions;
				if (nameToXMLValue.TryGetValue("AdditionalOptions", out additionalOptions))
				{
					nameToXMLValue["AdditionalOptions"] = additionalOptions + " " + extranAdditionalOptions;
				}
			}

			if (nameToDefaultXMLValue != null)
			{
				//Apply default attributes
				foreach (KeyValuePair<string, string> item in nameToDefaultXMLValue)
				{
					//If the attribute is not set yet, set it to the default
					if (nameToXMLValue.ContainsKey(item.Key) == false)
					{
						nameToXMLValue.Add(item.Key, item.Value);
					}
				}
			}

			foreach (var entry in _cleanupParameters)
			{
				string value;
				if (nameToXMLValue.TryGetValue(entry.Key, out value))
				{
					if ((entry.Value & ParameterCleanupFlags.RemoveQuotes) == ParameterCleanupFlags.RemoveQuotes)
					{
						value = value.TrimQuotes();

						if ((entry.Value & ParameterCleanupFlags.EnsureTrailingPathSeparator) == ParameterCleanupFlags.EnsureTrailingPathSeparator)
						{
							value = value.EnsureTrailingSlash();
						}
					}
					else if ((entry.Value & ParameterCleanupFlags.AddQuotes) == ParameterCleanupFlags.AddQuotes)
					{
						if ((entry.Value & ParameterCleanupFlags.EnsureTrailingPathSeparator) == ParameterCleanupFlags.EnsureTrailingPathSeparator)
						{
							value = value.TrimQuotes();
							value = value.EnsureTrailingSlash();
						}
						value = value.Quote();
					}

					nameToXMLValue[entry.Key] = value;
				}
			}

			if (BuildGenerator.IsPortable)
			{
				string additionalOptions;

				if (nameToXMLValue.TryGetValue("AdditionalOptions", out additionalOptions))
				{
					nameToXMLValue["AdditionalOptions"] = BuildGenerator.PortableData.NormalizeOptionsWithPathStrings(additionalOptions, OutputDir.Path, "\\s*(\"(?:[^\"]*)\"|(?:[^\\s]+))\\s*");
				}
			}
		}

		private bool EvaluateSwitchCondition(Project project, VSConfig.SwitchInfo switchInfo, Dictionary<string, string> tokenReplacement)
		{
			if (String.IsNullOrEmpty(switchInfo.Condition))
			{
				return true;    // Default true if there are no special condition
			}

			if (project == null)
			{
				// This shouldn't happen.  Maybe Framework got restructured and have a new use case that we
				// didn't expect.
				throw new BuildException(
					string.Format("ERROR:  Attempting to process MSBuild option regex pattern '{0}' with condition '{1}'.  But 'project' information is null.  Need to have project information to process condition information!",
						switchInfo.Switch.ToString(), switchInfo.Condition)
					);
			}

			string expandedCondition = switchInfo.Condition;
			foreach (var token in tokenReplacement)
			{
				expandedCondition = expandedCondition.Replace(token.Key, token.Value);
			}
			// Expand any other properties.
			expandedCondition = project.ExpandProperties(expandedCondition);

			// Return the evaluation
			return Expression.Evaluate(expandedCondition);
		}

		private string GetPortableOption(string name, string value)
		{
			if (BuildGenerator.IsPortable && PARAMETERS_WITH_PATH.Contains(name))
			{
				value = GetProjectPath(value);
			}
			return value;
		}

		protected abstract string[,] GetTools(Configuration config);

		// If rule file has optionset attached to it, it is considered a template.
		// Apply Regex.Replace() and put resulting file into build directory:
		protected string CreateRuleFileFromTemplate(FileItem rule, IModule module)
		{
			string inputFileName = rule.FileName.Trim();
			string outputFileName = inputFileName;

			if (String.IsNullOrEmpty(rule.OptionSetName))
			{
				return outputFileName;
			}

			// The rule file is a template and we need to run Regex.Replace() using specified optionset as parameters.
			OptionSet templateOptions;

			if (module.Package.Project.NamedOptionSets.TryGetValue(rule.OptionSetName, out templateOptions))
			{
				// Read file, 
				if (File.Exists(inputFileName))
				{
					string fullIntermediateDir = Path.Combine(OutputDir.Path, module.IntermediateDir.Path);
					Directory.CreateDirectory(fullIntermediateDir);

					outputFileName = Path.Combine(fullIntermediateDir, Path.GetFileName(inputFileName));

					var templateFileContent = File.ReadAllText(inputFileName);
					foreach (var option in templateOptions.Options)
					{
						templateFileContent = Regex.Replace(templateFileContent, option.Key, option.Value);
					}

					using (IMakeWriter writer = new MakeWriter(writeBOM: false))
					{
						writer.FileName = Path.Combine(fullIntermediateDir, Path.GetFileName(inputFileName));

						writer.CacheFlushed += new NAnt.Core.Events.CachedWriterEventHandler(OnCustomRuleFileUpdate);

						writer.Write(templateFileContent);


						//Handle vs2010 rules as well...
						if (outputFileName.EndsWith(".props"))
						{
							//ensure requisite build rules files are also there.
							string inputTargetsFile = Path.ChangeExtension(inputFileName, ".targets");
							string inputSchemaFile = Path.ChangeExtension(inputFileName, ".xml");

							var inputTargetsFI = new FileItem(PathString.MakeNormalized(inputTargetsFile), rule.OptionSetName);
							var inputSchemaFI = new FileItem(PathString.MakeNormalized(inputSchemaFile), rule.OptionSetName);

							CreateRuleFileFromTemplate(inputTargetsFI, module);
							CreateRuleFileFromTemplate(inputSchemaFI, module);
						}
					}
				}
				else
				{
					String msg = String.Format("Custom rules template file '{0}' does not exist", inputFileName);
					throw new BuildException(msg);
				}
			}
			else
			{
				String msg = String.Format("Rules template file Optionset '{0}' with replacement patterns does not exist. Can not apply Regex.Replace()", rule.OptionSetName);
				throw new BuildException(msg);
			}
			return outputFileName;
		}

		protected string GetCreateDirectoriesCommand(IEnumerable<Command> commands)
		{
			if (commands == null || !commands.Any())
				return "";

			var commandstring = new StringBuilder();
			string format = "@if not exist \"{0}\" mkdir \"{0}\" & SET ERRORLEVEL=0";

			switch (Environment.OSVersion.Platform)
			{
				case PlatformID.MacOSX:
				case PlatformID.Unix:
					format = "NOT IMPLEMENTED";
					break;
			}

			var unique = new HashSet<PathString>();

			foreach (var command in commands)
			{
				if (command.CreateDirectories == null)
					continue;

				foreach (var dir in command.CreateDirectories)
				{
					var normPath = dir.Normalize(PathString.PathParam.NormalizeOnly);
					if (unique.Add(normPath))
					{
						commandstring.AppendFormat(format, GetProjectPath(normPath));
						commandstring.AppendLine();
					}
				}
			}
			return commandstring.ToString();

		}

		protected void OnCustomRuleFileUpdate(object sender, CachedWriterEventArgs e)
		{
			if (e != null)
			{
				string message = string.Format("{0}{1} Custom rule file  '{2}'", LogPrefix, e.IsUpdatingFile ? "    Updating" : "NOT Updating", Path.GetFileName(e.FileName));
				if (e.IsUpdatingFile)
					Log.Minimal.WriteLine(message);
				else
					Log.Status.WriteLine(message);
			}
		}

		#endregion

		#region Private Fields

		private bool _explicitlyIncludeDependentLibraries = false;

		protected delegate void WriteToolDelegate(IXmlWriter writer, string name, Tool tool, Configuration config, Module module, IDictionary<string, string> nameToDefaultXMLValue = null, FileItem file = null);

		#endregion Private Fields

		#region Static Const Data

		#region tool to VCTools mapping 

		protected readonly IDictionary<string, WriteToolDelegate> _toolMethods = new Dictionary<string, WriteToolDelegate>();

		#endregion VCTools to Configuration Tools Mapping

		protected enum ParameterCleanupFlags { RemoveQuotes = 1, AddQuotes = 2, EnsureTrailingPathSeparator = 4 }

		protected readonly IDictionary<string, ParameterCleanupFlags> _cleanupParameters = new Dictionary<string, ParameterCleanupFlags>()
		{
			  {"OutputDirectory",               ParameterCleanupFlags.RemoveQuotes|ParameterCleanupFlags.EnsureTrailingPathSeparator}
			 ,{"IntermediateDirectory",         ParameterCleanupFlags.RemoveQuotes|ParameterCleanupFlags.EnsureTrailingPathSeparator}
			 ,{"OutputManifestFile",            ParameterCleanupFlags.RemoveQuotes}
			 ,{"OutputFile",                    ParameterCleanupFlags.RemoveQuotes}
			 ,{"MapFileName",                   ParameterCleanupFlags.RemoveQuotes}
			 ,{"ModuleDefinitionFile",          ParameterCleanupFlags.RemoveQuotes}
			 ,{"ImportLibrary",                 ParameterCleanupFlags.RemoveQuotes}
			 ,{"ProgramDatabaseFile",           ParameterCleanupFlags.RemoveQuotes}
			 ,{"ProgramDatabaseFileName",       ParameterCleanupFlags.RemoveQuotes}
		};

		private static readonly HashSet<string> PARAMETERS_WITH_PATH = new HashSet<string> {
			"OutputDirectory",
			"IntermediateDirectory",
			"AdditionalIncludeDirectories",
			"AdditionalLibraryDirectories",
			"EmbedManagedResourceFile",
			"AdditionalDependencies",
			"OutputManifestFile",
			"OutputFile",
			"MapFileName",
			"ModuleDefinitionFile",
			"ImportLibrary",
			"ProgramDatabaseFile",
			"ProgramDataBaseFileName",
			"VCPostBuildEventTool",
			"OutputFileName",
			"GenerateMapFile"
		};

		#endregion Static Const Data
	}
}
