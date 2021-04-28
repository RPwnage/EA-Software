// (c) Electronic Arts. All rights reserved.
using System;
using System.Collections.Generic;
using System.IO;

using NAnt.Core;
using NAnt.Core.Util;

using EA.Eaconfig.Core;
using EA.Eaconfig.Modules;
using EA.FrameworkTasks.Model;

namespace EA.Eaconfig
{
	// GRADLE TODO: nuke this file and roll into the MS extension
	public abstract class VisualStudioExtensionBase : IMCPPVisualStudioExtension
	{
		private string mAntDir;
		public string AntDir
		{
			get
			{
				if (mAntDir == null)
				{
					if (!Module.Package.Project.Properties.Contains("package.ApacheAnt.dir"))
					{
						TaskUtil.Dependent(Project, "ApacheAnt");
					}
					mAntDir = PathNormalizer.Normalize(Module.Package.Project.GetPropertyValue("package.ApacheAnt.dir"));
				}
				return mAntDir;
			}
		}

		public string AndroidNdkDir { get { return AndroidBuildUtil.GetAndroidNdkDir(Module); } }
		public string AndroidSdkDir { get { return AndroidBuildUtil.GetAndroidSdkDir(Module); } }
		public string JavaHomeDir { get { return AndroidBuildUtil.GetJavaHome(Module); } }
		public string GradleRootDir { get { return AndroidBuildUtil.GetGradleRoot(Module); } }

		public bool IsPortable { get { return Module.Package.Project.Properties.GetBooleanPropertyOrDefault("eaconfig.generate-portable-solution", false); } }
		public bool DebugFlags { get { return Module.Package.Project.NamedOptionSets["config-options-common"].GetBooleanOptionOrDefault("debugflags", false); } }

		protected string GetModuleFileSetBaseDirectory(string filesetName)
		{
			string baseDir = String.Empty;
			FileSet fs = Module.Package.Project.GetFileSet(Module.PropGroupName(filesetName));
			if (fs != null && !String.IsNullOrEmpty(fs.BaseDirectory))
			{
				baseDir = PathNormalizer.Normalize(fs.BaseDirectory, false);
			}
			return baseDir;
		}

		protected IEnumerable<string> GetSoLibDirs()
		{
			List<string> solibdirs = new List<string>();
			Module_Native native = Module as Module_Native;

			if (native != null)
			{
				solibdirs.Add(native.Link.LinkOutputDir.Path);
			}

			solibdirs.Add(PathNormalizer.Normalize(Module.Package.Project.GetPropertyValue("package.AndroidNDK.libdir"), false));

			FileSet sharedFs = null;
			if (Module.Package.Project.NamedFileSets.TryGetValue(Module.PropGroupName("packagesharedlibs"), out sharedFs))
			{
				foreach (FileItem fi in sharedFs.FileItems)
				{
					solibdirs.Add(Path.GetDirectoryName(fi.Path.Path));
				}
			}
					  
			foreach (Dependency<IModule> dep in native.Dependents)
			{
				string moduleGroup = dep.Dependent.PropGroupName("packagesharedlibs");
				if (Module.Package.Project.NamedFileSets.TryGetValue(moduleGroup, out sharedFs))
				{
					foreach (FileItem fi in sharedFs.FileItems)
					{
						solibdirs.Add(Path.GetDirectoryName(fi.Path.Path));
					}
				}

				string packageGroup = "package." + dep.Dependent.Package.Name + ".packagesharedlibs";
				if (Module.Package.Project.NamedFileSets.TryGetValue(packageGroup, out sharedFs))
				{
					foreach (FileItem fi in sharedFs.FileItems)
					{
						solibdirs.Add(Path.GetDirectoryName(fi.Path.Path));
					}
				}
			}

			if (Module.Package.Project.Properties.GetBooleanPropertyOrDefault("android.use-gnu-stdc", false))
			{
				solibdirs.Add(PathNormalizer.Normalize(Module.Package.Project.GetPropertyValue("package.AndroidNDK.stdcdir.libs"), false));
			}

			return solibdirs.OrderedDistinct();
		}
	}
}
