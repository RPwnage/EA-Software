// (c) Electronic Arts. All rights reserved.

using System;
using System.Xml;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Attributes;
using NAnt.Core.Functions;
using NAnt.Core.Logging;
using EA.FrameworkTasks.Model;

using EA.Eaconfig;
using EA.Eaconfig.Core;
using EA.Eaconfig.Build;
using EA.Eaconfig.Modules;
using EA.Eaconfig.Modules.Tools;

namespace EA.KettleConfig
{
	[TaskName("kettle-clang-postprocess", XmlSchema = false)]
	class Kettle_Clang_Postprocess : AbstractModuleProcessorTask
	{
		public override void ProcessTool(CcCompiler cc)
		{
			AddSystemIncludesToOptions(ref cc);
			base.ProcessTool(cc);
		}
		
		public override void ProcessCustomTool(CcCompiler cc)
		{
			AddSystemIncludesToOptions(ref cc);
			base.ProcessCustomTool(cc);
		}

		public override void Process(Module_Native module)
		{
			AddAssetDeployment(module);
			CreateShaderTool(module);

			if (module.BuildType.IsProgram && (module.CopyLocal == CopyLocalType.True))
			{
				// libc.prx is a low level prx and required by all programs and need to be copied over.  Instead of having needing people declare this
				// for all their module build files, we have this post process task to automatically copy this for people. 
				// All other sce_module prx usage should have people explicitly specify them in their build file as needed.
				string libcprx = PathString.MakeNormalized(Path.Combine(module.Package.Project.Properties["package.kettlesdk.sdkdir"], "target", "sce_module", "libc.prx")).Path;
				if (File.Exists(libcprx))
				{
					string destfile = PathString.MakeNormalized(Path.Combine(module.OutputDir.Path, "sce_module", "libc.prx")).Path;
					module.CopyLocalFiles.Add(
						new CopyLocalInfo(cached: new CachedCopyLocalInfo(libcprx, destfile, true, false, true, module), destinationModule: module)
					);
					if (!module.RuntimeDependencyFiles.Contains(destfile))
					{
						module.RuntimeDependencyFiles.Add(destfile);
					}
				}
			}
		}

		public virtual void AddAssetDeployment(Module module)
		{
			if (module.DeployAssets && module.Assets != null && module.Assets.Includes.Count > 0)
			{
				var step = new BuildStep("postbuild", BuildStep.PostBuild);
				var targetInput = new TargetInput();
				step.TargetCommands.Add(new TargetCommand("copy-asset-files.kettle", targetInput, nativeNantOnly: false));
				module.BuildSteps.Add(step);
			}
		}
		
		private static void AddSystemIncludesToOptions(ref CcCompiler cc)
		{
			// specifically for visual studio build - add system include dir directly to options so it gets moved
			// to AdditionalOptions in VS, prodg vsi will pass system includes as -I rather than -isystem causing
			// warnings from sony headers to be thrown when they shouldn't be
			foreach (PathString systemInclude in cc.SystemIncludeDirs)
			{
				cc.Options.Add(String.Format("-isystem \"{0}\"", systemInclude.NormalizedPath)); // TODO should use system include dir template here
			}
		}

		private void CreateShaderTool(Module_Native module)
		{
			PathString outputdir = PathString.MakeNormalized(module.IntermediateDir.Path);

			module.ShaderFiles = AddModuleFiles(module, ".shaderfiles");
			if (module.ShaderFiles == null) return;

			BuildTool psslc = new BuildTool(Project, "psslc",
				PathString.MakeNormalized(Project.ExpandProperties("${package.kettlesdk.appdir}/orbis-wave-psslc.exe")),
				BuildTool.TypePreCompile, intermediatedir: outputdir, outputdir: outputdir,
				description: "Compiling shader files");

			BuildTool selectedTool = psslc;
			foreach (var shaderfile in module.ShaderFiles.FileItems)
			{
				string filename = Path.GetFileNameWithoutExtension(shaderfile.FileName);
				PathString outputfile = PathString.MakeCombinedAndNormalized(outputdir.Path, filename + ".sb");

				FileData fileData = new FileData(psslc);
				List<string> fileOptions = new List<string>();
				//fileOptions.Add("/nologo");
				fileData.Data = fileOptions;

				OptionSet customOptionset = null;
				if (Project.NamedOptionSets != null && shaderfile.OptionSetName != null)
				{
					Project.NamedOptionSets.TryGetValue(shaderfile.OptionSetName, out customOptionset);
				}

				if (customOptionset != null && customOptionset.Options != null)
				{
					if (customOptionset.Options.TryGetValue("psslc.embed", out string embedOption))
					{
						fileOptions.Add("/embed"); // not a really option, just used to pass to the solution generator
					}
					if (customOptionset.Options.TryGetValue("psslc.options", out string shaderOptions))
					{
						foreach (string option in shaderOptions.LinesToArray())
						{
							string replacedOption = option.Replace("%filename%", filename)
														.Replace("%fileext%", "sb")
														.Replace("%intermediatedir%", outputdir.ToString());
							fileOptions.Add(replacedOption);
						}
					}
				}

				psslc.Files.Include(shaderfile.Path.ToString(), failonmissing: false, data: fileData, optionSetName: shaderfile.OptionSetName);

				module.ShaderFiles.Exclude(shaderfile.Path.ToString());
			}

			module.SetTool(psslc);
		}
	}
}
