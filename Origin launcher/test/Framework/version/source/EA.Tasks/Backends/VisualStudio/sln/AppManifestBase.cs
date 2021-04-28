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
using System.Collections.ObjectModel;
using System.IO;
using System.Linq;
using System.Reflection;

using NAnt.Core;
using NAnt.Core.Events;
using NAnt.Core.Logging;
using NAnt.Core.Util;
using NAnt.Core.Writers;

using EA.Eaconfig.Modules.Tools;
using EA.Eaconfig.Modules;
using EA.FrameworkTasks.Model;
using EA.Razor;

// DAVE-FUTURE-REFACTOR-TODO - this class exists because we used to have two manifest generations classes,
// appxmanifest for xb1 and wmappmanifest for winrt/windowsphone. the winrt windowsphone path is gone so
// this could be folded into AppXManifest class

namespace EA.Eaconfig.Backends.VisualStudio
{
	abstract class AppManifestBase
	{
		protected static readonly string _dotNetImageToolOptionSetName = "dotnet-image-visual-studio-tool";
		protected static readonly string _imageToolOptionSetName = "image-visual-studio-tool";
		protected static readonly string ReswToolOptionSetName = "resw-visual-studio-tool";

		protected readonly string LogPrefix;
		protected readonly Log Log;
		protected readonly IModule Module;
		protected readonly Project Project;
		protected readonly PathString VsProjectDir;
		protected readonly string VsProjectName;
		protected readonly string VsProjectFileNameWithoutExtension;
		protected readonly List<string> ImageFilesInternal;

		protected readonly string UserOptionSetName;
		protected readonly string DefaultOptionSetName;

		public PathString ManifestFilePath { get; protected set; }
		public readonly ReadOnlyCollection<string> ImageFiles;

		protected abstract PathString CreateImages(OptionSet manifestOptions, PathString imageFolder);

		protected AppManifestBase(Log log, IModule module, string userOptionsetName, string defaultOptionsetName, 
								  PathString vsProjectDir, string vsProjectName, string vsProjectFileNameWithoutExtension)
		{
			Log = log;
			LogPrefix = "[visualstudio] ";
			Module = module;
			Project = module.Package.Project;
			UserOptionSetName = userOptionsetName;
			DefaultOptionSetName = defaultOptionsetName;
			ImageFilesInternal = new List<string>();
			ImageFiles = new ReadOnlyCollection<string>(ImageFilesInternal);

			VsProjectDir = vsProjectDir;
			VsProjectName = vsProjectName;
			VsProjectFileNameWithoutExtension = vsProjectFileNameWithoutExtension;

			if (module.IsKindOf(FrameworkTasks.Model.Module.DotNet))
			{
				AddVisualStudioToolOptionset(_dotNetImageToolOptionSetName, "Content", new Dictionary<string, string>() {{"CopyToOutputDirectory", "PreserveNewest"}});
			}
			else
			{
				AddVisualStudioToolOptionset(_imageToolOptionSetName, "Image", excludedFromBuildRespected:false);
			}

			AddVisualStudioToolOptionset(ReswToolOptionSetName, "PRIResource", excludedFromBuildRespected: false);
		}

		protected string ImageToolOptionSetName
		{
			get
			{
				if (Module.IsKindOf(FrameworkTasks.Model.Module.DotNet))
				{
					return _dotNetImageToolOptionSetName;
				}
				else
				{
					return _imageToolOptionSetName;
				}

			}
		}

		protected void WriteAppManifestFile<TModel>(PathString templateFile, PathString appManifestFolder) where TModel : RazorModel_Manifest
		{
			var manifestOptions = CreateManifestOptions(appManifestFolder);
			var optionsetname = Project.NamedOptionSets.Contains(UserOptionSetName) ? UserOptionSetName : DefaultOptionSetName;

			var imageDir = CreateImages(manifestOptions, VsProjectDir);

			var model = CreateRazorModel<TModel>(optionsetname, manifestOptions, imageDir);

			var engine = new Razor<RazorTemplate<TModel>, TModel>(Project);

			using (var writer = new MakeWriter())
			{
				Directory.CreateDirectory(appManifestFolder.Path);

				ManifestFilePath = PathString.MakeCombinedAndNormalized(appManifestFolder.Path, manifestOptions.Options["manifest.filename"]);

				writer.FileName = ManifestFilePath.Path;
				writer.CacheFlushed += new CachedWriterEventHandler(OnManifestFileUpdate);

				writer.Write(engine.RenderTemplate(templateFile, model, Module));
			}

		}

		protected virtual TModel CreateRazorModel<TModel>(string optiosetname, OptionSet options, PathString imageDir) where TModel : RazorModel_Manifest
		{
			return new RazorModel_Manifest(optiosetname, options, imageDir, VsProjectDir, VsProjectFileNameWithoutExtension, VsProjectName) as TModel;
		}

		protected virtual PathString GetTemplateFilePath(string defaultTemplateFile, string templateFileProperty)
		{
			string templatePath = Project.GetPropertyValue(CombinePropNames(Module.GroupName, templateFileProperty));

			if (String.IsNullOrEmpty(templatePath))
			{
				templatePath = Project.GetPropertyValue(CombinePropNames("package", templateFileProperty));
			}

			if (String.IsNullOrEmpty(templatePath))
			{
				templatePath = Path.GetFullPath(Path.Combine(Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location), "Data", defaultTemplateFile));
			}

			return PathString.MakeNormalized(templatePath);
		}

		protected virtual FileSet GetModuleResourceFiles()
		{
			FileSet resourcefiles = null;

			var native_module = Module as Module_Native;

			if (native_module != null)
			{
				if (native_module.ResourceFiles == null)
				{
					native_module.ResourceFiles = new FileSet();
				}
				resourcefiles = native_module.ResourceFiles;
			}
			else
			{
				var dotnet_module = Module as Module_DotNet;

				if (dotnet_module != null)
				{
					resourcefiles = dotnet_module.Compiler.Resources;
				}
			}
			return resourcefiles;
		}

		protected string CombinePropNames(string prefix, string name)
		{
			if (name.StartsWith("."))
			{
				return prefix + name;
			}
			return prefix + "." + name;
		}

		protected OptionSet CreateManifestOptions(PathString appManifestFolder)
		{
			var defaultOptions = Project.NamedOptionSets[DefaultOptionSetName];
			var manifestOptions = defaultOptions != null ? new OptionSet(defaultOptions) : new OptionSet();

			if (manifestOptions.Options.Count == 0)
			{
				Log.Warning.WriteLine("Did not find named optionset {0}", DefaultOptionSetName);
			}

			var useroptions = Project.NamedOptionSets[UserOptionSetName];
			if(useroptions != null)
			{
				manifestOptions.Append(useroptions);
			}

			return manifestOptions;
		}

		protected PathString CopyOrCreateImage(string src, PathString destfolder, int width, int height, out PathString srcpath)
		{
			srcpath = null;
			if (string.IsNullOrEmpty(src))
			{
				Log.Error.WriteLine("CopyOrCreateImage() - src path is null or empty");
				return new PathString(String.Empty);
			}

			var destpng = Path.IsPathRooted(src) ? 
				PathString.MakeCombinedAndNormalized(destfolder.Path, Path.GetFileName(src)) :
				PathString.MakeCombinedAndNormalized(destfolder.Path, src);

			if (!Directory.Exists(Path.GetDirectoryName(destpng.Path)))
			{
				Directory.CreateDirectory(Path.GetDirectoryName(destpng.Path));
			}

			if (Path.IsPathRooted(src))
			{
				PathString normalizedSrcPath = PathString.MakeNormalized(src);
				srcpath = normalizedSrcPath;
				bool copySuccessful = true;
				// Should use a blocking do once lock with file path being the key.  Shouldn't allow different images being copied to
				// same destination.
				NAnt.Core.Tasks.DoOnce.Execute(Project, destpng.Path, () =>
				{
					copySuccessful = CopyImageFile(normalizedSrcPath, destpng);
				},
				isblocking: true);
				if (!copySuccessful)
				{
					srcpath = null;
				}
			}
			else
			{
				// Should use a blocking do once lock with file path being the key.  Shouldn't allow different images being created at
				// same destination multiple times.
				NAnt.Core.Tasks.DoOnce.Execute(Project, destpng.Path, () =>
				{
					if (File.Exists(destpng.Path))
					{
						File.SetAttributes(destpng.Path, FileAttributes.Normal);
					}
					System.Drawing.Bitmap logo = new System.Drawing.Bitmap(width, height);
					logo.Save(destpng.Path, System.Drawing.Imaging.ImageFormat.Png);
				},
				isblocking: true);
			}
			return destpng;
		}

		protected bool CopyImageFile(PathString src, PathString dst)
		{
			bool ret = false;
			if (dst == src)
			{
				Log.Error.WriteLine("CopyImageFile() - src path equals dst path: {0}", dst.Path);
			}
			else
			{
				// if we have a rooted path, then copy that file to the dest folder
				if (File.Exists(dst.Path))
				{
					File.SetAttributes(dst.Path, FileAttributes.Normal);
				}
				else
				{
					Directory.CreateDirectory(Path.GetDirectoryName(dst.Path));
				}
				File.Copy(src.Path, dst.Path, true);
				ret = true;
			}
			return ret;
		}

		protected void AddImageAsResource(FileSet fs, string optionsetname, string src, IEnumerable<string> languages, PathString destfolder, int width, int height)
		{
			if (fs != null)
			{
				PathString srcImage;
				var image = CopyOrCreateImage(src, destfolder, width, height, out srcImage);
				if (!image.IsNullOrEmpty())
				{
					ImageFilesInternal.Add(image.Path);
					fs.Include(image.Path, optionSetName: optionsetname, baseDir: destfolder.Path);

					if (!srcImage.IsNullOrEmpty())
					{
						// Include localized images in case image was not auto created (in later case srcImage == null):
						var srcDir = Path.GetDirectoryName(srcImage.Path);
						var srcFile = Path.GetFileName(srcImage.Path);

						//Include source image as regular resource (action None)
						fs.Include(srcImage.Path, baseDir: srcDir);

						foreach (var language in languages)
						{
							var srcLangFile = PathString.MakeCombinedAndNormalized(Path.Combine(srcDir, language), srcFile);
							
							if (File.Exists(srcLangFile.Path))
							{
								var dstLangFile = PathString.MakeCombinedAndNormalized(Path.Combine(destfolder.Path, language), srcFile);
								if (CopyImageFile(srcLangFile, dstLangFile))
								{
									//Include source image as regular resource (action None)
									fs.Include(srcLangFile.Path, baseDir: srcDir);
								}
								fs.Include(dstLangFile.Path, optionSetName: optionsetname, baseDir: destfolder.Path);
							}
						}
					}
				}
			}
		}

		protected virtual string AppExecutableName(IModule imodule)
		{
			string appExecutableName = String.Empty;
			if (imodule is FrameworkTasks.Model.Module module && module.Tools != null)
			{
				if (module.Tools.SingleOrDefault(t => t.ToolName == "link") is Linker link)
				{
					appExecutableName = link.OutputName + link.OutputExtension;
				}
			}

			return appExecutableName;
		}

		protected void OnManifestFileUpdate(object sender, CachedWriterEventArgs e)
		{
			if (e != null)
			{
				string message = string.Format("{0}{1} Manifest file '{2}'", LogPrefix, e.IsUpdatingFile ? "    Updating" : "NOT Updating", e.FileName);
				if (e.IsUpdatingFile)
					Log.Minimal.WriteLine(message);
				else
					Log.Status.WriteLine(message);
			}
		}

		protected bool AddVisualStudioToolOptionset(string setname, string toolname, IDictionary<string, string> toolProperties = null, bool excludedFromBuildRespected=true)
		{
			bool ret = false;
			if (Module != null && Module.Package.Project != null)
			{
				var optionset = new OptionSet();
				optionset.Options["visual-studio-tool"] = toolname;
				if (toolProperties != null)
				{
					optionset.Options["visual-studio-tool-properties"] = toolProperties.ToString(Environment.NewLine, e => String.Format("{0}={1}", e.Key, e.Value));
				}
				if (!excludedFromBuildRespected)
				{
					optionset.Options["visual-studio-tool-excludedfrombuild-respected"] = "false";
				}

				ret = Module.Package.Project.NamedOptionSets.TryAdd(setname, optionset);
			}
			return ret;
		}

	}
}
