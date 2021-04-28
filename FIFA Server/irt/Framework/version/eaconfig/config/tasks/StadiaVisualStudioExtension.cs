// (c) Electronic Arts. All rights reserved.

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;

using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Util;

using EA.Eaconfig.Core;
using EA.Eaconfig.Modules;
using EA.Eaconfig.Modules.Tools;
using EA.FrameworkTasks.Model;

using Model = EA.FrameworkTasks.Model;

namespace EA.Eaconfig
{
	[TaskName("stadia-clang-visualstudio-extension")]
	public class StadiaVisualStudioExtension : IMCPPVisualStudioExtension
	{
		public override void UserData(IDictionary<string, string> userData)
		{
			if (Module.IsKindOf(Model.Module.Program))
			{
				userData["DebuggerFlavor"] = "GgpGameletDebugger";

				// TODO:
				/*<GgpDeployOnLaunch>false</GgpDeployOnLaunch>
				<GgpApplication>Frostbite</GgpApplication>*/

				if (userData.TryGetValue("LocalDebuggerCommandArguments", out string value))
					userData["GgpGameletLaunchArguments"] = value;

				userData["GgpCustomDeployOnLaunch"] = String.Format("\"{0}\" \"$(OutDir).\" -s -g \"{1}\" -x \"$(OutDir)$(TargetFileName)\"", PathString.MakeNormalized(Project.ExpandProperties("${nant.location}\\stadia_copy.exe")).ToString(), Path.GetDirectoryName(Project.Properties["package.StadiaSDK.sdkdir"]));
			}
		}

		public override void ProjectGlobals(IDictionary<string, string> projectGlobals)
		{
			// The environment override needs to be in Global scope.  There seems to be tasks needing
			// that info outside the config/platform scope.
			if (!projectGlobals.ContainsKey("GGP_SDK_PATH"))
			{
				bool isPortable = Module.Package.Project.Properties.GetBooleanPropertyOrDefault("eaconfig.generate-portable-solution", false);
				if (!isPortable)
				{
					// Make sure we set up these properties override path with backslash ending that is 
					// consistent with how Stadia's MSBuild files uses them.

					string sdkdir = Module.Package.Project.Properties["package.StadiaSDK.sdkdir"].TrimWhiteSpace();
					if (!String.IsNullOrEmpty(sdkdir))
					{
						sdkdir = sdkdir.Replace("/", "\\");
						if (!sdkdir.EndsWith("\\"))
							sdkdir = sdkdir + "\\";
						projectGlobals.Add("GGP_SDK_PATH", sdkdir);
					}

					string toolchaindir = Module.Package.Project.Properties["package.StadiaSDK.toolchain"];
					if (!String.IsNullOrEmpty(toolchaindir))
					{
						toolchaindir = toolchaindir.Replace("/", "\\");
						if (!toolchaindir.EndsWith("\\"))
							toolchaindir = toolchaindir + "\\";
						projectGlobals["GgpToolchainPath"] = toolchaindir;
					}

					string headerpath = Module.Package.Project.Properties["package.StadiaSDK.includedir"];
					if (!string.IsNullOrEmpty(headerpath))
					{
						headerpath = headerpath.Replace("/", "\\");
						if (headerpath.EndsWith("\\"))
							headerpath = toolchaindir.TrimEnd(new char[] { '\\' });
						projectGlobals["GgpHeaderPath"] = headerpath;
					}
				}
			}
		}

		public override void WriteExtensionItems(IXmlWriter writer)
		{
			writer.WriteStartElement("PropertyGroup");
			writer.WriteAttributeString("Condition", Generator.GetConfigCondition(Module.Configuration));

			// VSI will technically add $(GgpBinDir) already.  We are explicitly adding it as well.
			// By default, VSI will assign GgpBinDir as $(GgpToolchainDir)bin\  (note that it is expecting GgpToolchainDir to end with backslash)
			// VSI by default will create GgpToolchainDir based on GGP_SDK_PATH.  But we explicitly set it up in the global block as well.
			writer.WriteElementString("ExecutablePath", "$(GgpBinDir);$(ExecutablePath)");

			writer.WriteEndElement();
		}

		public override void WriteExtensionCompilerToolProperties(CcCompiler compiler, FileItem file, IDictionary<string, string> toolProperties)
		{
			Module_Native nativeModule = Module as Module_Native;
			if (nativeModule == null)
				return;

			// Make sure we uses the input "compiler" isntead of nativeModule.Cc because the input "compiler" could be file specific and
			// "PrecompiledHeaders" action setting is definitely file specific.
			if (compiler.PrecompiledHeaders != CcCompiler.PrecompiledHeadersAction.NoPch)
			{
				if (!String.IsNullOrEmpty(nativeModule.PrecompiledHeaderFile))
				{
					string pchHeaderDir = nativeModule.PrecompiledHeaderDir.Path;
					if (String.IsNullOrEmpty(pchHeaderDir))
					{
						// Try and see if we can find where the header is located by checking the module's includedir setup.
						// Ideally, it would be better performance if the build script provide this information in the first place.
						foreach (PathString incDir in nativeModule.Cc.IncludeDirs)
						{
							if (File.Exists(Path.Combine(incDir.Path,nativeModule.PrecompiledHeaderFile)))
							{
								pchHeaderDir = incDir.Path;
								break;
							}
						}
						if (String.IsNullOrEmpty(pchHeaderDir))
							throw new BuildException(String.Format("Using precompiled header in stadia config for module {0} requires you to provide 'pchheaderdir' attribute in <pch> element (or the property [group].[module].pch-header-file-dir).", nativeModule.Name));
					}
					string pchfile = Path.Combine(pchHeaderDir, nativeModule.PrecompiledHeaderFile).Replace("\\","/");

					toolProperties["PrecompiledHeaderFile"] = pchfile;

					if (toolProperties.ContainsKey("AdditionalOptions"))
					{
						// Remove all the -include "pch.h" and -include-pch "[path]\pch.h.pch"  They will cause
						// conflict to the VSI.
						string forceIncludes = "-include " + nativeModule.PrecompiledHeaderFile.Quote();
						toolProperties["AdditionalOptions"] = toolProperties["AdditionalOptions"].Replace(forceIncludes, "");
						string includepch = "-include-pch " + nativeModule.PrecompiledFile.Path.Quote();
						toolProperties["AdditionalOptions"] = toolProperties["AdditionalOptions"].Replace(includepch, "");
					}

					if (file != null)
					{
						if (!String.IsNullOrEmpty(file.OptionSetName) &&
							Project.NamedOptionSets.ContainsKey(file.OptionSetName) &&
							Project.NamedOptionSets[file.OptionSetName].Options.Contains("create-pch") &&
							(Project.NamedOptionSets[file.OptionSetName].Options["create-pch"].ToLower() == "on" ||
							Project.NamedOptionSets[file.OptionSetName].Options["create-pch"].ToLower() == "true"))
						{
							// We actually need to exclude this file for build or we can get link error.
							toolProperties["ExcludedFromBuild"] = "true";
						}
					}
					else
					{
						// First time this function get run, it will be for setting up the tool default (ie input "file" variable will be null),
						// so we find the source files fileset to see which one has the option set that has the option value create-pch.  That will
						// be our source file. We need to find that file to check the optionset to see if we 
						// are using -x c-header or -x c++-header and we actually need to set PrecompiledHeaderCompileAs
						// at the tool level, not file specific.
						OptionSet createPchOpt = compiler.SourceFiles.FileItems
							.Where(
									fitm => !String.IsNullOrEmpty(fitm.OptionSetName) &&
									Project.NamedOptionSets.ContainsKey(fitm.OptionSetName) &&
									Project.NamedOptionSets[fitm.OptionSetName].Options.Contains("create-pch") &&
									(Project.NamedOptionSets[fitm.OptionSetName].Options["create-pch"].ToLower() == "on" ||
									Project.NamedOptionSets[fitm.OptionSetName].Options["create-pch"].ToLower() == "true"))
							.Select(fitm => Project.NamedOptionSets[fitm.OptionSetName])
							.FirstOrDefault();
						if (createPchOpt != null)
						{
							if (createPchOpt.Options.ContainsKey("cc.options"))
							{
								if (createPchOpt.Options["cc.options"].Contains("-x c-header"))
								{
									toolProperties["PrecompiledHeaderCompileAs"] = "CompileAsC";
								}
								else if (createPchOpt.Options["cc.options"].Contains("-x c++-header"))
								{
									toolProperties["PrecompiledHeaderCompileAs"] = "CompileAsCpp";
								}
							}
						}
					}
				}
			}
		}

		public override void WriteExtensionLinkerToolProperties(IDictionary<string, string> toolProperties)
		{
			// Map AdditionalDependencies element to CircularDependencies Element
			// This is because GGP assumes you will sort your archives in proper link order which we never do
			// So using the CircularDependencies element will ensure that the libs/archives get included in the linker --start-group --end-group block
			if (toolProperties.ContainsKey("AdditionalDependencies"))
			{
				var add_deps = toolProperties["AdditionalDependencies"];
				toolProperties.Remove("AdditionalDependencies");
				// apparently quotes are illegal characters so they need to be removed.
				toolProperties["CircularDependencies"] = add_deps.Replace("\"", "");
			}

			// this is based off of FlagEquals behaviour
			bool stripDebugSymbols = false;
			string stripOption = Module.Package.Project.Properties["eaconfig.stripallsymbols"] ?? Module.GetModuleBuildOptionSet().Options["stripallsymbols"];
			if (!String.IsNullOrEmpty(stripOption))
			{
				if (stripOption.Equals("true", StringComparison.OrdinalIgnoreCase) || stripOption.Equals("on", StringComparison.OrdinalIgnoreCase))
				{
					stripDebugSymbols = true;
				}
				else if (stripOption.Equals("false", StringComparison.OrdinalIgnoreCase) || stripOption.Equals("off", StringComparison.OrdinalIgnoreCase))
				{
					stripDebugSymbols = false;
				}
			}

			toolProperties["GgpShouldStripDebugSymbols"] = (!stripDebugSymbols).ToString().ToLowerInvariant();
		}
		
		public override void WriteMsBuildOverrides(IXmlWriter writer)
		{
			base.WriteMsBuildOverrides(writer);

			// TODO: this needs re-doing, currently cause too much incremental rebuild
			if (Module.IsKindOf(EA.FrameworkTasks.Model.Module.Native) && Module.Tools.Any(tool => tool is Librarian))
            {
				// Linux style archiver just appends to the .a file so you can end up  with duplicate symbols when code is moved between libraries, so delete the .a file before the librarian/archiver (ar.exe) runs.
				writer.WriteStartElement("Target");
				writer.WriteAttributeString("Name", "_DeleteOldLib" + Generator.GetVSProjTargetConfiguration(Module.Configuration));
				writer.WriteAttributeString("BeforeTargets", "Lib");
				writer.WriteAttributeString("Condition", Generator.GetConfigCondition(Module.Configuration));
				{
					writer.WriteStartElement("Message");
					writer.WriteAttributeString("Text", "Deleting %(Lib.OutputFile).  This will ensure the archiver does not end up with duplicate symbols when code is moved between libraries.");
					writer.WriteAttributeString("Importance", "low");
					writer.WriteEndElement(); // Message
					writer.WriteStartElement("Delete");
					writer.WriteAttributeString("Files", "%(Lib.OutputFile)");
					writer.WriteEndElement(); // Delete
				}
				writer.WriteEndElement();
			}
		}
	}
}
