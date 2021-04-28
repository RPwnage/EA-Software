// (c) Electronic Arts. All rights reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Logging;
using Model = EA.FrameworkTasks.Model;

using EA.Eaconfig.Core;
using EA.Eaconfig.Modules;
using EA.Eaconfig.Modules.Tools;

namespace EA.Eaconfig
{

	public class VC_Common_PostprocessOptions : AbstractModuleProcessorTask
	{
		public override void Process(Module_Native module)
		{
			OptionSet buildOptionSet = OptionSetUtil.GetOptionSetOrFail(Project, module.BuildType.Name);

			CreateShaderTool(module);

			CreateMidlTool(module, buildOptionSet);

			CreateResourceTool(module);

			CreateManifestTool(module, buildOptionSet);
		}

		public override void ProcessTool(Linker linker)
		{
			UpdateLinkOptionsWithBaseAddAndDefFile(linker);

			if (linker.ImportLibFullPath != null)
			{
				linker.Options.Add(String.Format("-implib:\"{0}\"", linker.ImportLibFullPath.Path));
				linker.CreateDirectories.Add(new PathString(Path.GetDirectoryName(linker.ImportLibFullPath.Path), PathString.PathState.Directory|PathString.PathState.Normalized));
			}
		}

		public override void ProcessTool(CcCompiler cc)
		{
			ProcessDependents(cc);
			base.ProcessTool(cc);
		}

		public override void ProcessCustomTool(CcCompiler cc)
		{
			ProcessDependents(cc);
			base.ProcessCustomTool(cc);
		}

		private void UpdateLinkOptionsWithBaseAddAndDefFile(Linker linker)
		{
			string defFile = PropGroupValue(".definition-file." + Module.Configuration.System, null) ?? PropGroupValue(".definition-file");

			if (!String.IsNullOrEmpty(defFile))
			{
				linker.Options.Add(String.Format("-DEF:{0}", PathNormalizer.Normalize(defFile, false)));
			}

			string baseAddr = PropGroupValue(".base-address." + Module.Configuration.System, null) ?? PropGroupValue(".base-address.");

			if (!String.IsNullOrEmpty(baseAddr))
			{
				linker.Options.Add(String.Format("-BASE:{0}", baseAddr));
			}
		}

		private void ProcessDependents(CcCompiler cc)
		{
			if (Module.IsKindOf(Model.Module.Managed))
			{
				// We use this hack to disable warning 4945 because managed modules sometimes load the same assembly twice. 
				// This is introduced by project references support. We need to remove this when the double loading problem is resolved-->

				if (!Module.Package.Project.Properties.Contains(PropGroupName("enablewarning4945")))
				{
					cc.Options.Add("-wd4945");
				}

				foreach (var assembly in cc.Assemblies.FileItems.Select(item => item.Path))
				{
					// Add assemblies as input dependencies for the dependency check tasks
					cc.InputDependencies.Include(assembly.Path, failonmissing: false);
				}
			}
		}

		private void CreateShaderTool(Module_Native module)
		{
			PathString outputdir = PathString.MakeNormalized(module.IntermediateDir.Path);

			module.ShaderFiles = AddModuleFiles(module, ".shaderfiles");
			if (module.ShaderFiles == null) return;

			foreach (var shaderfile in module.ShaderFiles.FileItems)
			{
				// TODO: dxc is used when the shader model is >= 6, so should probably adjust automatically like visual studio
				BuildTool fxcompile = new BuildTool(Project, "fxc",
					PathString.MakeNormalized(Project.ExpandProperties("${package.WindowsSDK.tools.dxc??${package.WindowsSDK.kitbin.dir}/dxc.exe}")),
					BuildTool.TypePreCompile, intermediatedir: outputdir, outputdir: outputdir,
					description: "Compiling hlsl");

				string filename = Path.GetFileNameWithoutExtension(shaderfile.FileName);

				FileData fileData = new FileData(fxcompile);
				List<string> fileOptions = new List<string> { "/nologo" };
				fileData.Data = fileOptions;
				
				OptionSet customOptionset = null;
				if (Project.NamedOptionSets != null && shaderfile.OptionSetName != null)
				{
					Project.NamedOptionSets.TryGetValue(shaderfile.OptionSetName, out customOptionset);
				}

				if (customOptionset != null && customOptionset.Options != null && customOptionset.Options.TryGetValue("fxcompile.options", out string shaderOptions))
				{
					foreach (string option in shaderOptions.LinesToArray())
					{
						string replacedOption = option.Replace("%filename%", filename)
													.Replace("%fileext%", "cso")
													.Replace("%intermediatedir%", outputdir.ToString());
						fileOptions.Add(replacedOption);
						fxcompile.Options.Add(replacedOption);

						// The directory of the header file needs to be created ahead of time or else dxc will fail
						string trimmedOption = replacedOption.Trim();
						if (trimmedOption.StartsWith("-Fh"))
						{
							Directory.CreateDirectory(Path.GetDirectoryName(trimmedOption.Substring("-Fh".Length).Replace("\"", "").Trim()));
						}
					}
				}

				fxcompile.Files.Include(shaderfile.Path.ToString(), failonmissing: false, data: fileData, optionSetName: shaderfile.OptionSetName);

				fxcompile.Options.Add(shaderfile.Path.ToString());

				module.AddTool(fxcompile);
			}
		}

		private void CreateMidlTool(Module_Native module, OptionSet buildOptionSet)
		{
			string midleoptions;
			if (buildOptionSet.Options.TryGetValue("midl.options", out midleoptions))
			{
				BuildTool midl = new BuildTool(Project, "midl", PathString.MakeNormalized(Project.ExpandProperties(@"???????????????")), BuildTool.TypePreCompile, 
					outputdir: module.OutputDir, intermediatedir: module.IntermediateDir);

				foreach (var option in midleoptions.LinesToArray())
				{
					midl.Options.Add(option);
				}
				module.SetTool(midl);
			}
		}

		private void CreateManifestTool(Module_Native module, OptionSet buildOptionSet)
		{
			if (module.Link != null && Project.Properties.Contains("package.WindowsSDK.appdir"))
			{
				var manifest = new BuildTool(Project, "vcmanifest", PathString.MakeNormalized(Project.ExpandProperties("${build.mt.program}")), BuildTool.None,
					outputdir: module.OutputDir, intermediatedir: module.IntermediateDir, description: "compiling manifest");

				var additionalManifestFiles = AddModuleFiles(module, ".additional-manifest-files");
				var inputResourceManifests = AddModuleFiles(module, ".input-resource-manifests");
				manifest.Files.Append(additionalManifestFiles);
				manifest.InputDependencies.Append(inputResourceManifests);
				manifest.OutputDependencies.Include(module.Link.OutputName + module.Link.OutputExtension + ".embed.manifest", failonmissing: false, baseDir: module.IntermediateDir.Path);
				manifest.OutputDependencies.Include(module.Link.OutputName + module.Link.OutputExtension + ".embed.manifest.res", failonmissing: false, baseDir: module.IntermediateDir.Path);

				module.SetTool(manifest);

				// In native nant build we use postlink command to build manifest. When the above "manifest" tool (for Visual Studio only) is created, 
				// the PostLink tool is already created.  So we'll just update the command line of the created PostLink took for this module.
				// NOTE that the manifest filename between Visual Studio and native nant build is different.  That's why we kept these two as separate tools.
				if (module.PostLink != null)
				{
					if (module.PostLink.Options != null)
					{
						for (int ii = 0; ii < module.PostLink.Options.Count; ++ii)
						{
							if (module.PostLink.Options[ii].Contains("-manifest "))
							{
								if (additionalManifestFiles != null)
								{ 
									StringBuilder extraFiles = new StringBuilder();
									foreach (FileItem fi in additionalManifestFiles.FileItems)
									{
										extraFiles.Append(string.Format(" \"{0}\"",fi.FileName));
									}
									module.PostLink.Options[ii] = module.PostLink.Options[ii].Insert(
										module.PostLink.Options[ii].IndexOf("-manifest") + "-manifest".Length,
										extraFiles.ToString());
								}
							}
						}
						if (inputResourceManifests != null)
						{
							foreach (FileItem fi in inputResourceManifests.FileItems)
							{
								module.PostLink.Options.Add(string.Format("-inputresource:\"{0}\"",fi.FileName));
							}
						}
					}
				}
			}
		}

		private BuildTool CreateResourceCompilerBuildTool(Module_Native module, PathString resourcefile, FileSet resourcefiles)
		{
			string resorcefilename = Path.GetFileNameWithoutExtension(resourcefile.Path);
			PathString outputdir = PathString.MakeNormalized(PropGroupValue(".res.outputdir", null) ?? module.IntermediateDir.Path);
			PathString outputfile = PathString.MakeCombinedAndNormalized(outputdir.Path, PropGroupValue(".res.name", null) ?? resorcefilename + ".res");

			List<string> includeDirs = new List<string>()
					{
						PathNormalizer.Normalize(Project.Properties["package.eaconfig.vcdir"] + "/atlmfc/include", false),
						PathNormalizer.Normalize(Project.Properties["eaconfig.PlatformSDK.dir"] + "/Include", false),
						PathNormalizer.Normalize(Project.Properties["package.eaconfig.vcdir"] + "/Include")
					};
			List<string> defines = new List<string>() { "_DEBUG" };

			if (Properties.Contains("package.WindowsSDK.MajorVersion"))
			{
				int WinSdkMajorVer;
				Int32.TryParse(Properties["package.WindowsSDK.MajorVersion"], out WinSdkMajorVer);

				if (WinSdkMajorVer == 0)
				{
					Log.Warning.WriteLine("property package.WindowsSDK.MajorVersion from the WindowsSDK is not in integer format.");
				}

				if (WinSdkMajorVer >= 10 && Project.Properties.Contains("eaconfig.PlatformSDK.includedirs"))
				{
					// Starting with Windows 10 SDK, we should let that package setup the list and we just grab that list instead
					// of hard coding the path ourselves.
					string platformIncDirs = Project.Properties["eaconfig.PlatformSDK.includedirs"];
					foreach (string incdir in platformIncDirs.Split(new char[] { '\n' }))
					{
						string trimedIncDir = PathNormalizer.Normalize(incdir.Trim());
						if (string.IsNullOrEmpty(trimedIncDir))
						{
							continue;
						}
						includeDirs.Add(trimedIncDir);
					}
				}
				else if (WinSdkMajorVer >= 8)
				{
					includeDirs.Add(PathNormalizer.Normalize(Project.Properties["eaconfig.PlatformSDK.dir"] + "/Include/um"));
					includeDirs.Add(PathNormalizer.Normalize(Project.Properties["eaconfig.PlatformSDK.dir"] + "/Include/shared"));
				}
			}

			// Add ModuleDefined include dirs and defines:
			string ressourceIncludeDirs = PropGroupValue(".res.includedirs", null);
			if (ressourceIncludeDirs != null)
			{
				// Technically, all includedirs properties are expected to be setup with each path on a new line.  
				// But FBConfig's FrostbiteMain partial module is using quoted string with semi-colon to separate paths.
				// So we test for ';' as well and strip out the quote.
				includeDirs.AddRange(
					ressourceIncludeDirs.Split(new char[] { ';', '\r', '\n' })
						.Select(p => PathString.MakeNormalized(p.Trim(new char[] { ' ', '\t', '"' })).Path)
						.Where(p => !String.IsNullOrEmpty(p))
				);
			}
			defines.AddRange(PropGroupValue(".res.defines", null).LinesToArray());

			var rcexe = PathString.MakeNormalized(Project.Properties.GetPropertyOrDefault("build.rc.program", ""));

			BuildTool rc = new BuildTool(Project, "rc", rcexe, BuildTool.TypePreCompile,
				String.Format("compile resource '{0}'", Path.GetFileName(resourcefile.Path)),
				intermediatedir: module.IntermediateDir, outputdir: outputdir);
			rc.Files.Include(resourcefile.Path, failonmissing: false, data: new FileData(rc));
			rc.OutputDependencies.Include(outputfile.Path, failonmissing: false);
			foreach (var define in defines)
			{
				rc.Options.Add("/d " + define);
			}
			rc.Options.Add("/l 0x409"); // Specify the English codepage 
			foreach (var dir in includeDirs)
			{
				rc.Options.Add("/i " + dir.Quote());
			}
			rc.Options.Add("/fo" + outputfile.Path.Quote());
			rc.Options.Add(resourcefile.Path.Quote());

			resourcefiles.Exclude(resourcefile.Path);
			rc.CreateDirectories.Add(outputdir);

			module.Link.ObjectFiles.Include(outputfile.Path, failonmissing: false);

			return rc;
		}

		private BuildTool CreateResxBuildTool(Module_Native module, IEnumerable<PathString> resxfiles, FileSet resourcefiles)
		{
			string rootnamespace = PropGroupValue(".rootnamespace", null) ?? module.Name;
			PathString outputdir = PathString.MakeCombinedAndNormalized(module.IntermediateDir.Path, "forms");

			BuildTool resx = new BuildTool(Project, "resx", PathString.MakeNormalized(Project.ExpandProperties(Properties["build.resgen.program"])), BuildTool.TypePreCompile, "compile managed resources.",
				intermediatedir: outputdir, outputdir: outputdir);
			resx.Options.Add("/useSourcePath");
			resx.Options.Add("/compile");
			foreach (var file in resxfiles)
			{
				string filename = Path.GetFileNameWithoutExtension(file.Path);
				string outresource = Path.Combine(resx.OutputDir.Path, rootnamespace + "." + filename + ".resources");

				resx.Options.Add(String.Format("{0},{1}", file.Path.Quote(), outresource.Quote()));
				resx.OutputDependencies.Include(outresource, failonmissing: false);

				resourcefiles.Exclude(file.Path);
			}

			resx.CreateDirectories.Add(outputdir);

			return resx;
		}

		// Add resource tool.
		public void CreateResourceTool(Module_Native module)
		{
			if (module.Link == null)
			{
				return;
			}

			FileSet resourcefiles = module.ResourceFiles;

			if (resourcefiles == null || resourcefiles.FileItems.Count <= 0)
			{
				return;
			}

			BuildTool rc = null;
			BuildTool resx = null;

			var rcfiles = resourcefiles.FileItems.Where(item => Path.GetExtension(item.Path.Path) == ".rc").Select(item => item.Path);

			if (rcfiles.Count() > 0 && Properties.Contains("eaconfig.PlatformSDK.dir"))
			{
				foreach (var resourcefile in rcfiles)
				{
					rc = CreateResourceCompilerBuildTool(module, resourcefile, resourcefiles);

					module.AddTool(rc);
				}
			}
			else if (!module.IsKindOf(Model.Module.Managed))
			{
				Log.Error.WriteLine("package '{0}' module '{1}' resource fileset is defined, but contains no files", module.Package.Name, module.Name);
			}

			if (module.IsKindOf(Model.Module.Managed))
			{
				var resxfiles = resourcefiles.FileItems.Where(item => Path.GetExtension(item.Path.Path) == ".resx").Select(item => item.Path);

				if (resxfiles.Count() > 0)
				{
					resx = CreateResxBuildTool(module, resxfiles, resourcefiles);

					module.SetTool(resx);
				}
				foreach (var res in resourcefiles.FileItems)
				{
					module.Link.Options.Add(String.Format("-ASSEMBLYRESOURCE:{0}", res.Path.Path.Quote()));
				}
			}

			if (rc != null)
			{
				rc.InputDependencies.AppendWithBaseDir(resourcefiles);
			}
			if (resx != null)
			{
				resx.InputDependencies.AppendWithBaseDir(resourcefiles);
			}
		}

	}

}
