// (c) Electronic Arts. All rights reserved.

using System.Collections.Generic;
using System.IO;
using System.Linq;

using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Util;

using EA.Eaconfig.Build;
using EA.Eaconfig.Core;
using EA.Eaconfig.Modules;
using EA.Eaconfig.Modules.Tools;

using EA.FrameworkTasks.Model;

namespace EA.nxConfig
{
	[TaskName("nx-arm-clang-postprocess")]
	class NxArmClangPostprocess : AbstractModuleProcessorTask
	{
		public override void ProcessTool(CcCompiler cc)
		{
			AddSystemIncludesToCcOptions(ref cc);
			base.ProcessTool(cc);
		}

		public override void ProcessTool(Linker link)
		{
			base.ProcessTool(link);
		}

		public override void ProcessCustomTool(CcCompiler cc)
		{
			AddSystemIncludesToCcOptions(ref cc);
			base.ProcessCustomTool(cc);
		}

		public override void Process(Module_Native module)
		{
			// Need to fix up copylocal's destination.  By default Framework's module setup will put copylocal files to be side by
			// side the exe.  But on NX, this need to be under the ApplicationDataDir, NRODeployPath, and [ApplicationDataDir]/.nrr folders.
			if (module.IsKindOf(ProcessableModule.DynamicLibrary) && module.CopyLocalFiles.Any())
			{
				// We have to rely on the Program module's to properly setup the AdditionalNRODependency field.  That setting will automatically do copylocal
				// so we don't want to do the copying ourselves or we can have collision.
				// Usually dll build don't do copylocal.  But if "nant.postbuildcopylocal" is turned on, Framework may add copylocal instruction to copy to
				// parent module's destination.  So we need to test for that and remove it.
				List<CopyLocalInfo> copyTasksToRemove = new List<CopyLocalInfo>();
				foreach (CopyLocalInfo copylocal in module.CopyLocalFiles)
				{
					if (copylocal.DestinationModule.Name != module.Name) // Signature that this copylocal came from "nant.postbuildcopylocal" usage!
					{
						copyTasksToRemove.Add(copylocal);
						if (module.RuntimeDependencyFiles.Contains(copylocal.AbsoluteDestPath))
						{
							module.RuntimeDependencyFiles.Remove(copylocal.AbsoluteDestPath);
						}
					}
				}
				foreach (CopyLocalInfo copylocal in copyTasksToRemove)
				{
					module.CopyLocalFiles.Remove(copylocal);
				}
			}
			else if (module.IsKindOf(ProcessableModule.Program) && module.CopyLocal != CopyLocalType.False)
			{
				bool hasNroFiles = false;
				string moduleOutputBinDir = module.OutputDir.Path;
				string appDataDir = module.PropGroupValue("application-data-dir",module.Package.Project.Properties["nx-default-application-data-dir-template"])
						.Replace("%outputbindir%", moduleOutputBinDir).Replace("%moduleoutputname%", module.OutputName);
				string nroDeployPath = module.PropGroupValue("nro-deploy-path",module.Package.Project.Properties["nx-default-nro-deploy-path-template"])
						.Replace("%outputbindir%", moduleOutputBinDir);
				string nrrFileName = module.PropGroupValue("nrr-file-name",module.Package.Project.Properties["nx-default-nrr-file-name-template"])
						.Replace("%outputbindir%", moduleOutputBinDir).Replace("%moduleoutputname%",module.OutputName);
				bool disableNrsCopy = module.Package.Project.Properties.GetBooleanPropertyOrDefault("nx.disable-nrs-copy", false);
				List<CopyLocalInfo> nrsCopyLocalFiles = new List<CopyLocalInfo>();
				for (int idx=0; idx<module.CopyLocalFiles.Count; ++idx)
				{
					if (module.CopyLocalFiles[idx].AbsoluteDestPath.EndsWith(".nro"))
					{
						hasNroFiles = true;

						CopyLocalInfo oldCopylocal = module.CopyLocalFiles[idx];

						string nroFileName = Path.GetFileName(oldCopylocal.AbsoluteDestPath);
						string newDestinationPath = Path.Combine(nroDeployPath, nroFileName);

						module.CopyLocalFiles[idx] = new CopyLocalInfo(
							new CachedCopyLocalInfo(
								oldCopylocal.AbsoluteSourcePath,
								newDestinationPath,
								oldCopylocal.SkipUnchanged,
								oldCopylocal.NonCritical,
								oldCopylocal.AllowLinking,
								oldCopylocal.SourceModule),
							oldCopylocal.DestinationModule);

						if (module.RuntimeDependencyFiles.Contains(oldCopylocal.AbsoluteDestPath))
						{
							module.RuntimeDependencyFiles.Remove(oldCopylocal.AbsoluteDestPath);
							module.RuntimeDependencyFiles.Add(newDestinationPath);
						}

						// The standard NX VSI's AdditionalNRODependencies setup only copy the .nro, it won't copy the .nrs files
						// But icepick need those .nrs in order to do stack trace.  So we are adding the .nrs as well.
						if (!disableNrsCopy)
						{
							CopyLocalInfo nrsCopyInfo = new CopyLocalInfo(
								new CachedCopyLocalInfo(
									Path.ChangeExtension(module.CopyLocalFiles[idx].AbsoluteSourcePath, ".nrs"),
									Path.ChangeExtension(module.CopyLocalFiles[idx].AbsoluteDestPath, ".nrs"),
									module.CopyLocalFiles[idx].SkipUnchanged,
									module.CopyLocalFiles[idx].NonCritical,
									module.CopyLocalFiles[idx].AllowLinking,
									module.CopyLocalFiles[idx].SourceModule),
								oldCopylocal.DestinationModule);
							nrsCopyLocalFiles.Add(nrsCopyInfo);
							module.RuntimeDependencyFiles.Add(nrsCopyInfo.AbsoluteDestPath);
						}
					}
					else if (module.CopyLocalFiles[idx].AbsoluteDestPath.StartsWith(module.OutputDir.Path) && 
							!module.CopyLocalFiles[idx].AbsoluteDestPath.StartsWith(appDataDir))
					{
						// We need to correct any other asset files as well.  They actually all need to be moved over to application data dir.
						CopyLocalInfo oldCopylocal = module.CopyLocalFiles[idx];

						string oldDestDataFile = Path.GetFileName(oldCopylocal.AbsoluteDestPath);
						string newDestinationPath = Path.Combine(appDataDir, oldDestDataFile);

						module.CopyLocalFiles[idx] = new CopyLocalInfo(
							new CachedCopyLocalInfo(
								oldCopylocal.AbsoluteSourcePath,
								newDestinationPath,
								oldCopylocal.SkipUnchanged,
								oldCopylocal.NonCritical,
								oldCopylocal.AllowLinking,
								oldCopylocal.SourceModule),
							oldCopylocal.DestinationModule);

						if (module.RuntimeDependencyFiles.Contains(oldCopylocal.AbsoluteDestPath))
						{
							module.RuntimeDependencyFiles.Remove(oldCopylocal.AbsoluteDestPath);
							module.RuntimeDependencyFiles.Add(newDestinationPath);
						}
					}
				}
				if (nrsCopyLocalFiles.Any())
				{
					module.CopyLocalFiles.AddRange(nrsCopyLocalFiles);
				}
				if (hasNroFiles)
				{
					// Need to add the .nrr file to the RuntimeDependencyFiles list.
					module.RuntimeDependencyFiles.Add(nrrFileName);
				}
			}
			AddAssetDeployment(module);
		}

		public virtual void AddAssetDeployment(Module module)
		{
			if (module.DeployAssets && module.Assets != null && module.Assets.Includes.Count > 0)
			{
				var step = new BuildStep("prelink", BuildStep.PreLink);
				var targetInput = new TargetInput();
				step.TargetCommands.Add(new TargetCommand("copy-asset-files.nx", targetInput, nativeNantOnly: false));
				module.BuildSteps.Add(step);
			}
		}

		private void AddSystemIncludesToCcOptions(ref CcCompiler cc)
		{
			// specifically for visual studio build - add system include dir directly to options so it gets moved
			// to AdditionalOptions in VS, nx vsi will pass system includes as -I rather than -isystem.
			// This can potentially cause problems with clang warning (and clang normally has ability to auto suppress 
			// warnings from headers coming from -isystem).

			string includeSwitchTemplate = Project.Properties.GetPropertyOrDefault("cc.template.system-includedir", null);
			if (!string.IsNullOrEmpty(includeSwitchTemplate))
			{
				List<string> sysIncOptions = new List<string>();
				foreach (PathString systemInclude in cc.SystemIncludeDirs)
				{
					sysIncOptions.Add(includeSwitchTemplate.Replace("%system-includedir%", systemInclude.NormalizedPath));
				}
				cc.Options.InsertRange(0, sysIncOptions);
			}
		}
	}
}
