// (c) Electronic Arts. All rights reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using System.IO;

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Attributes;

using EA.Eaconfig;
using EA.Eaconfig.Backends.VisualStudio;
using EA.Eaconfig.Build;
using EA.Eaconfig.Core;
using EA.Eaconfig.Modules;
using EA.Eaconfig.Modules.Tools;
using Model=EA.FrameworkTasks.Model;

namespace EA.NXConfig
{
	[TaskName("nx-clang-visualstudio-extension")]
	public class nxVisualStudioExtension : IMCPPVisualStudioExtension
	{
		// If build script did not set platforms/nx/application-data-dir field, default would be [module's output directory]\assets
		private string ApplicationDataDir 
		{
			get
			{
				// Make sure you have eaconfig (or nx_config implemented property 'nx-default-application-data-dir-template' as '%outputbindir%\assets' or you will get
				// null object reference exception.
				return (Module.Package.Project.Properties[Module.BuildGroup + "." + Module.Name + ".application-data-dir"] ??
							Module.Package.Project.Properties["nx-default-application-data-dir-template"])
					.Replace("%outputbindir%", Module.OutputDir.Path).Replace("%moduleoutputname%", Module.OutputName);
			}
		}

		// If build script did not set platforms/nx/nrr-file-name field, default would be [application-data-dir]/.nrr/Application.nrr
		private string NrrFileName
		{
			get
			{
				// Make sure you have eaconfig (or nx_config implemented property 'nx-default-nrr-file-name-template' as '%outputbindir%\assets' or you will get
				// null object reference exception.
				return (Module.Package.Project.Properties[Module.BuildGroup + "." + Module.Name + ".nrr-file-name"] ??
							Module.Package.Project.Properties["nx-default-nrr-file-name-template"])
					.Replace("%outputbindir%", Module.OutputDir.Path).Replace("%moduleoutputname%", Module.OutputName);
			}
		}

		private string NrrFolder
		{
			get
			{
				return Path.GetDirectoryName(NrrFileName);
			}
		}

		// If build script did not set platforms/nx/nro-deploy-path field, default would be [application-data-dir]
		private string NroDeployPath
		{
			get
			{
				return (Module.Package.Project.Properties[Module.BuildGroup + "." + Module.Name + ".nro-deploy-path"] ??
							Module.Package.Project.Properties["nx-default-nro-deploy-path-template"])
					.Replace("%outputbindir%", Module.OutputDir.Path);
			}
		}

		// If build script did not setup platforms/nx/authoring-user-command, this field will not be set and VSI default will be used.
		private string AuthoringUserCommand
		{
			get
			{
				return Module.Package.Project.Properties.GetPropertyOrDefault(Module.BuildGroup + "." + Module.Name + ".authoring-user-command", string.Empty);
			}
		}

		public override void UserData(IDictionary<string, string> userData)
		{
			if (Module.IsKindOf(Model.Module.Program))
			{
				userData["DebuggerFlavor"] = "OasisNXDebugger";

				// For NX, the VSI expects the "Remote" prefix
				string value;
				if(userData.TryGetValue("LocalDebuggerCommandArguments", out value))
					userData["RemoteDebuggerCommandArguments"] = value;
				if (userData.TryGetValue("LocalDebuggerWorkingDirectory", out value))
					userData["RemoteDebuggerWorkingDirectory"] = value;
			}
		}

		private bool Is64bit()
		{
			return Module.Package.Project.Properties.GetPropertyOrDefault("platform-ptrsize", "64bit") == "64bit";
		}

		public override void ProjectGlobals(IDictionary<string, string> projectGlobals)
		{
			TaskUtil.Dependent(Project, "nxsdk", Project.TargetStyleType.Use);
			projectGlobals["NintendoSdkRoot"] = String.Format("{0}\\", PathNormalizer.Normalize(Project.Properties["package.nxsdk.appdir"]));
			projectGlobals["NintendoSdkSpec"] = "NX";
		}

		public override void ProjectConfiguration(IDictionary<string, string> configurationElements)
		{
			configurationElements["NintendoSdkBuildType"] = Project.Properties["NintendoBuildType"];
			if (Module.IsKindOf(Model.Module.Program))
			{
				string descFile = GenerateNmeta.GetUniqueFilesetFile(Module, "descFile");
				configurationElements["ApplicationDesc"] = File.Exists(descFile) ? descFile : "$(NintendoSdkRoot)Resources\\SpecFiles\\Application.desc";
				if(Is64bit())
				{
					string manifest64File = GenerateNmeta.GetUniqueFilesetFile(Module, "nmeta64file");
					configurationElements["ApplicationArm64Nmeta"] = File.Exists(manifest64File) ? manifest64File : "$(NintendoSdkRoot)Resources\\SpecFiles\\Application.aarch64.lp64.nmeta";
				}
				else
				{
					string manifest32File = GenerateNmeta.GetUniqueFilesetFile(Module, "nmeta32file");
					configurationElements["ApplicationArmNmeta"] = File.Exists(manifest32File) ? manifest32File : "$(NintendoSdkRoot)Resources\\SpecFiles\\Application.arm.ilp32.nmeta";
				}

				bool hasNroFiles = Module.CopyLocalFiles.Where(file => file.AbsoluteSourcePath.EndsWith(".nro")).Any();
				if (hasNroFiles)
				{
					configurationElements["NRODeployPath"] = NroDeployPath;
					// We need to remove all dll copy local because when we set the AdditionalNRODependencies field, NX VSI will do the copying as well.  And we have to set
					// this AdditionalNRODependencies field so that NRR file can be constructed properly (without the NRS file being mixed in) or you will have trouble
					// in running the Authoring Tool (bad nro hash in nrr file) when you are building for NSP ApplicationProgramFormat instead of RAW format.
					Module.CopyLocalFiles.RemoveAll(file => file.AbsoluteSourcePath.EndsWith(".nro"));
				}
				if (
				   hasNroFiles 
				|| Module.Package.Project.Properties.Contains(Module.BuildGroup + "." + Module.Name + ".application-data-dir")
				|| (Module.DeployAssets && Module.Assets != null && Module.Assets.Includes.Count > 0)
				)
				{
					configurationElements["ApplicationDataDir"] = ApplicationDataDir;
				}

				string appProgramFormatOverride = Module.Package.Project.Properties.GetPropertyOrDefault("nx.application_program_format", null);
				if (!String.IsNullOrEmpty(appProgramFormatOverride))
				{
					switch (appProgramFormatOverride)
					{
						case "nsp":
							configurationElements["ApplicationProgramFormat"] = "Nsp";
							break;
						case "raw":
							configurationElements["ApplicationProgramFormat"] = "Directory";
							break;
						default:
							throw new BuildException(String.Format("Unrecognized global property setting for 'nx.application_program_format'.  It is currently set to '{0}'.  Accepted values are 'nsp' or 'raw'!",appProgramFormatOverride));
					}
				}
			}

			// Make sure we are doing this for Program module and not a dll module
			if (Module.IsKindOf(ProcessableModule.Program) && Module.Tools.Where(t => t is Linker).Any())
			{
				foreach (var dependent in Module.Dependents)
				{
					if (dependent.IsKindOf(DependencyTypes.Link) & dependent.Dependent.IsKindOf(ProcessableModule.DynamicLibrary))
					{
						// We need to remove the "link" part so that project reference can be setup without this setting.
						// We need this removed because we are explicitly adding the built NRO to AdditionalNRODependencies field.
						// If we don't do this, the MakeNrr will have the .nsr being added to the list and then when we run
						// the Authoring Tool with 'creatensp' instead of 'createnspd' (ie NSP ApplicationProgramFormat isntead
						// of RAW), we will get build error about the bad nro hash in nrr file.
						dependent.ClearType(DependencyTypes.Link);
					}
				}
			}

		}

		public override void WriteExtensionItems(IXmlWriter writer)
		{
			writer.WriteStartElement("ItemGroup");
			writer.WriteStartElement("CustomBuildStep");
			writer.WriteAttributeString("Include", "$(ProjectName)");
			writer.WriteEndElement();
			writer.WriteEndElement();
			
			if (Module.IsKindOf(Model.Module.Program))
			{
				writer.WriteStartElement("ItemDefinitionGroup");
				writer.WriteAttributeString("Condition", Generator.GetConfigCondition(Module.Configuration));
				writer.WriteStartElement("Link");
				writer.WriteElementString("FinalizeDescSource", "$(ApplicationDesc)");
				writer.WriteElementString("FinalizeMetaSource", Is64bit() ? "$(ApplicationArm64Nmeta)" : "$(ApplicationArmNmeta)");

				// Any "dlls" fileset (specified in .build file) that has a .nso extension, we need to Add them to AdditionalNSODependencies
				// so that they get copied to the "subsdk[?]" location.
				HashSet<string> nsoFileList = new HashSet<string>();  // Using a HashSet so that we won't be adding duplicates.
 				foreach (Linker linkerTool in Module.Tools.Where(tl => tl is Linker))
				{
					if (linkerTool.DynamicLibraries != null)
					{
						foreach (FileItem nso in linkerTool.DynamicLibraries.FileItems.Where(f => f.FullPath.EndsWith(".nso")))
						{
							nsoFileList.Add(nso.FullPath);
						}
					}
					if (linkerTool.Libraries != null)
					{
						foreach (FileItem nso in linkerTool.Libraries.FileItems.Where(f => f.FullPath.EndsWith(".nso")))
						{
							nsoFileList.Add(nso.FullPath);
						}
					}
				}

				HashSet<string> pubDataNroFileList = new HashSet<string>();
				foreach (Model.Dependency<Model.IModule> dep in Module.Dependents.Flatten())
				{
					Model.IPublicData pub = dep.Dependent.Public(Module);
					if (pub != null && pub.DynamicLibraries != null && pub.DynamicLibraries.FileItems.Any())
					{
						foreach (FileItem dll in pub.DynamicLibraries.FileItems)
						{
							if (dll.FullPath.EndsWith(".nro"))
							{
								pubDataNroFileList.Add(dll.FullPath);
							}
							else if (dll.FullPath.EndsWith(".nso"))
							{
								nsoFileList.Add(dll.FullPath);
							}
						}
					}

					if (dep.Dependent.Package != null && dep.Dependent.Package.Project != null)
					{
						FileSet externalDlls = Model.ModuleExtensions.GetPublicFileSet(dep.Dependent, dep.Dependent, "dlls.external");
						if (externalDlls != null && externalDlls.FileItems.Any())
						{
							foreach (FileItem dll in externalDlls.FileItems)
							{
								if (dll.FullPath.EndsWith(".nso"))
								{
									nsoFileList.Add(dll.FullPath);
								}
							}
						}
					}
				}

				if (nsoFileList.Any())
				{
					writer.WriteElementString("AdditionalNSODependencies", String.Join(";",nsoFileList) + ";%(AdditionalNSODependencies)");
				}

				// Get the source nro files (note that we removed the nro file CopyLocalFiles list at this point already in above ProjectConfiguration() function).  
				// So we need to get that from actual dependency info.
				HashSet<string> allNroList = new HashSet<string>(Module.Dependents.FlattenAll(DependencyTypes.Build | DependencyTypes.Link).Where(d => d.Dependent.IsKindOf(ProcessableModule.DynamicLibrary)).Select(d => d.Dependent.PrimaryOutput()).Where(f=>f.EndsWith(".nro")));
				foreach (string nro in pubDataNroFileList)
				{
					if (!allNroList.Any(f => f.EndsWith(Path.GetFileName(nro))))	// Just in case we have duplicate and people setup Initialize.xml wrong, take the buildable module output info.
					{
						allNroList.Add(nro);
					}
				}
				if (allNroList.Any())
				{
					writer.WriteElementString("AdditionalNRODependencies", String.Join(";", allNroList) + ";%(AdditionalNRODependencies)");
					writer.WriteElementString("NRRFileName", NrrFileName);
				}

				string authoringUserCommand = AuthoringUserCommand;
				if (!String.IsNullOrEmpty(authoringUserCommand))
				{
					writer.WriteElementString("AuthoringUserCommand", authoringUserCommand);
				}

				writer.WriteEndElement();
				writer.WriteEndElement();
			}
		}

		public override void WriteExtensionLinkerToolProperties(IDictionary<string, string> toolProperties)
		{
			// Strip .nso files from the additional dependencies, they will injected
			// into AdditionalNSODependencies from WriteExtensionItems.
			if (toolProperties.ContainsKey("AdditionalDependencies"))
			{
				if (toolProperties["AdditionalDependencies"] != null)
				{
					string[] add_deps = toolProperties["AdditionalDependencies"].Split(new char[] { ';' });
					toolProperties["AdditionalDependencies"] = string.Join(";", add_deps.Where(file => !file.EndsWith(".nso\"")));
				}
			}
		}

		public override void WriteExtensionToolProperties(IXmlWriter writer)
		{
			CcCompiler cc = null;
			if (Module.Tools != null)
			{
				IEnumerable<CcCompiler> ccTools = Module.Tools.Where(t => t is CcCompiler).Select(t => t as CcCompiler);
				if (ccTools.Any())
					cc = ccTools.First();
			}
			if (cc != null)
			{
				IEnumerable<string> cppVers = cc.Options.Where(op => op.StartsWith("-std="));
				if (cppVers.Any())
				{
					string cppLangStd = null;
					string cppVerOption = cppVers.Last();
					if (cppVerOption.StartsWith("-std=c++"))
						cppLangStd = cppVerOption.Substring("-std=c++".Length);
					else if (cppVerOption.StartsWith("-std=gnu++"))
						cppLangStd = cppVerOption.Substring("-std=gnu++".Length);

					string nxVsiVersion = Properties.GetPropertyOrDefault("package.nx_vsi.InstalledVersion", "0");
					if (nxVsiVersion.StrCompareVersions("10.4.3.29683") >= 0)
					{
						if (!String.IsNullOrEmpty(cppLangStd))
						{
							writer.WriteElementString("CppLanguageStandard", "Gnu++" + cppLangStd);
						}
					}
					else
						writer.WriteNonEmptyElementString("CppLanguageStandard", cppLangStd);
				}
			}
		}

		public override void SetVCPPDirectories(VCPPDirectories directories)
		{
			// NX VSI intellisense seems to have a problem if a vcxproj provided both <IncludePath> (global configuration properties) and
			// <AdditionalIncludeDirectories> (C/C++ General option) and <IncludePath> do not end with $(IncludePath), the very first include 
			// line in the AdditionalIncludeDirectories will be ignored by intellisense (You will see this problem even with a project 
			// generated by Nintendo's project wizard).  To hack around this, we make sure that our <IncludePath> setup will inherit include path.
			directories.InheritIncludePath = true;
		}

		public override void AddPropertySheetsImportList(List<Tuple<string,string,string>> propSheetList)
		{
			propSheetList.Add(new Tuple<string,string,string>(@"$(NintendoSdkRoot)\Build\Vc\NintendoSdkVcProjectSettings.props", @"exists('$(NintendoSdkRoot)\Build\Vc\NintendoSdkVcProjectSettings.props')",null));
		}

		public override void WriteExtensionCompilerToolProperties(CcCompiler compiler, FileItem file, IDictionary<string, string> toolProperties)
		{
			// NX VSI has a hard coded way of setting up PrecompiledHeaderOutputFile (ie the .pch output).  We cannot explicitly set it.
			// It has to be based on the header file (PrecompiledHeaderFile) that create this .pch output (and the .pch output extension is 
			// also forced on you).  The specification of this PrecomopiledHeaderFile has further criteria that it has to be a relative path 
			// to the source cpp file that initiated the pch creation.  To simply things, we set PrecompiledHeaderFile field as the 
			// source cpp file instead of the usual header file.  It would do the trick.
			
			Module_Native nativeModule = Module as Module_Native;
			if (nativeModule == null)
				return;

			IEnumerable<string> cppVers = compiler.Options.Where(op => op.StartsWith("-std="));
			if (cppVers.Any())
			{
				// Framework would have already translated the compiler setting into XML fields through the options_msbuild.xml
				// parsing at this point. We need to touch up the CppLanguageStandard field because NX's VSI keep changing it's
				// value format with different release and we don't have ability to provide NX VSI version condition in the
				// options_msbuild.xml.  So we'll touch it up here.
				string nxVsiVersion = Properties.GetPropertyOrDefault("package.nx_vsi.InstalledVersion", "0");
				if (nxVsiVersion.StrCompareVersions("10.4.3.29683") >= 0)
				{
					string cppLangStd = null;
					string cppVerOption = cppVers.Last();
					if (cppVerOption.StartsWith("-std=c++"))
						cppLangStd = cppVerOption.Substring("-std=c++".Length);
					else if (cppVerOption.StartsWith("-std=gnu++"))
						cppLangStd = cppVerOption.Substring("-std=gnu++".Length);
					if (!String.IsNullOrEmpty(cppLangStd))
					{
						toolProperties["CppLanguageStandard"] = "Gnu++" + cppLangStd;
					}
					else if (toolProperties.ContainsKey("CppLanguageStandard"))
					{
						// Something went wrong.  Should remove the old setting and use the default.
						toolProperties.Remove("CppLanguageStandard");
					}
				}
			}

			// Make sure we uses the input "compiler" isntead of nativeModule.Cc because the input "compiler" could be file specific and
			// "PrecompiledHeaders" action setting is definitely file specific.
			if (compiler.PrecompiledHeaders != CcCompiler.PrecompiledHeadersAction.NoPch && !String.IsNullOrEmpty(nativeModule.PrecompiledHeaderFile))
			{
				// Setting this field has no purpose because NX VSI will set it's own path anyways.  Leaving it in there won't hurt, but it would
				// confuses people keep thinking why it keeps outputing to wrong path.  So may as well remove it if this got added.
				if (toolProperties.ContainsKey("PrecompiledHeaderOutputFile"))
				{
					toolProperties.Remove("PrecompiledHeaderOutputFile");
				}

				string pchSrcFile = null;
				if (compiler.PrecompiledHeaders == CcCompiler.PrecompiledHeadersAction.CreatePch && file != null)
				{
					// The input 'file' shouldn't be null if action is CreatePch
					pchSrcFile = file.FullPath;

					// Stupid NX VSI have some hard coded nonsensical logic that doesn't allow this output file to end with a .obj, 
					// so change it to a .o extension.
					if (file.Data is FileData)
					{
						FileData filedata = file.Data as FileData;
						if (filedata.ObjectFile != null && filedata.ObjectFile.Path.EndsWith(".obj"))
						{
							filedata.ObjectFile = new PathString(Path.ChangeExtension(filedata.ObjectFile.Path, ".o"));
						}
					}
				}
				else
				{
					// First time this function get run, it will be for setting up the tool default (ie input "file" variable will be null),
					// so we find the source files fileset to see which one has the option set that has the option value create-pch.  That will
					// be our source file.
					IEnumerable<FileItem> pchSrcFiles = compiler.SourceFiles.FileItems
						.Where(
								fitm => !String.IsNullOrEmpty(fitm.OptionSetName) && 
								Project.NamedOptionSets.Contains(fitm.OptionSetName) && 
								Project.NamedOptionSets[fitm.OptionSetName].Options.Contains("create-pch") &&
								Project.NamedOptionSets[fitm.OptionSetName].Options["create-pch"] == "on");
					if (pchSrcFiles.Count() == 1)
					{
						// There can be only one. Highlander!
						pchSrcFile = pchSrcFiles.First().FullPath;
					}
				}
				if (!String.IsNullOrEmpty(pchSrcFile))
				{
					// We typically set "PrecompiledHeaderFile" to the input nativeModule.PrecompiledHeaderFile.  HOWEVER, NX VSI has some stupid
					// hard coded logics to always assume this .h header file sit side by side the source file and this setting must be set
					// as relative path to source file.  But our nativeModule.PrecompiledHeaderFile cannot contain path info (standard for all other
					// platforms).  So instead of setting this as the header file, NX VSI actually allow us to set this as the source file.  
					// So we set this PrecompiledHeaderFile field as the source file.  (Still need to be expressed as relative path to source file).
					// In other words, we just need to set the filename.
					toolProperties["PrecompiledHeaderFile"] = Path.GetFileName(pchSrcFile);
				}

				// We shouldn't need the following block unless someone set options_msbuild.xml incorrectly and have these leaked to AdditionalOptions.
				// If -include-pch is in there, we absolutely have to remove it because NS VSI hard coded it's setup to use a different filename/path.
				// Leaving this block here as commented for now in case people messed up the options_msbuild.xml file and we have to remove these info.
				// 
				//if (toolProperties.ContainsKey("AdditionalOptions"))
				//{
				//	string currOpts = toolProperties["AdditionalOptions"];
				//	toolProperties["AdditionalOptions"] = currOpts.Replace("-x c++-header", "").Replace("-x c-header", "");

				//	// Need to remove that option.
				//	System.Text.RegularExpressions.Regex regex = new System.Text.RegularExpressions.Regex(@"-include-pch\s*""[^""]+""?");
				//	System.Text.RegularExpressions.Match mt = regex.Match(currOpts);
				//	if (mt.Success)
				//	{
				//		toolProperties["AdditionalOptions"] = currOpts.Replace(mt.Value, "");
				//	}
				//}
			}
		}
	}
}
