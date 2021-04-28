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
using System.Linq;
using System.IO;

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Logging;

using EA.Eaconfig.Modules;

namespace EA.Eaconfig.Backends.VisualStudio
{
	internal class GameConfig : AppManifestBase
	{
		internal static GameConfig Generate(Log log, Module_Native module, PathString vsProjectDir, string vsProjectName, string vsProjectFileNameWithoutExtension)
		{
			if (module != null)
			{
				var manifest = new GameConfig(log, module, vsProjectDir, vsProjectName, vsProjectFileNameWithoutExtension);

				var templateFile = manifest.GetTemplateFilePath("gameconfig.template", "gameconfig.template.file");

				manifest.WriteAppManifestFile<RazorModel_Manifest>(templateFile, module.IntermediateDir);

				return manifest;
			}

			return null;
		}

		private GameConfig(Log log, Module_Native module, PathString vsProjectDir, string vsProjectName, string vsProjectFileNameWithoutExtension)
			: base(log, module, module.GroupName + ".gameconfigoptions", "config-options-gameconfigoptions", vsProjectDir, vsProjectName, vsProjectFileNameWithoutExtension)
		{
		}

		protected override PathString CreateImages(OptionSet manifestOptions, PathString appManifestFolder)
		{
			var resourcefiles = GetModuleResourceFiles();
			var languages = manifestOptions.Options["resource.languages"].ToArray().OrderedDistinct();

			if (resourcefiles != null)
			{
				PathString imageDestFolder;
				string image_directory = manifestOptions.Options["resource.image-directory"];
				if (image_directory is null)
				{
					imageDestFolder = PathString.MakeCombinedAndNormalized(Module.Package.PackageConfigBuildDir.Path, Module.Name);
				}
				else
				{
					imageDestFolder = PathString.MakeCombinedAndNormalized(appManifestFolder.Path, image_directory);
				}

				AddImageAsResource(resourcefiles, ImageToolOptionSetName, manifestOptions.Options["shellvisuals.storelogo"], languages, imageDestFolder, 100, 100);
				AddImageAsResource(resourcefiles, ImageToolOptionSetName, manifestOptions.Options["shellvisuals.square150x150logo"], languages, imageDestFolder, 150, 150);
				AddImageAsResource(resourcefiles, ImageToolOptionSetName, manifestOptions.Options["shellvisuals.square44x44logo"], languages, imageDestFolder, 44, 44);
				AddImageAsResource(resourcefiles, ImageToolOptionSetName, manifestOptions.Options["shellvisuals.splashscreen"], languages, imageDestFolder, 1920, 1080);
				
				// Add/Copy resw files:
				var fileItems = resourcefiles.FileItems; // to prevent file set re-expanding after each include below.
				foreach (var fi in fileItems)
				{
					if (String.Equals(Path.GetExtension(fi.Path.Path), ".resw", StringComparison.OrdinalIgnoreCase))
					{
						var basedir = fi.BaseDirectory ?? resourcefiles.BaseDirectory;
						if (String.IsNullOrEmpty(basedir))
						{
							Log.Error.WriteLine("BaseDirectory for '.resw' file '{0}' is empty, file will not be added to manifest resources. To be included in the manifest resources file path relative to BaseDirectory must be empty or match one of the language folders: '{1}'",
								fi.Path.Path, languages.ToString("; ", s => s.Quote()));
							continue;
						}
						var relPath = PathUtil.RelativePath(fi.Path.Path, basedir);
						var relDir = Path.GetDirectoryName(relPath);

						if (!(String.IsNullOrEmpty(relDir) || languages.Contains(relDir, StringComparer.OrdinalIgnoreCase)))
						{
							Log.Error.WriteLine("Path of '.resw' file '{0}' relative to BaseDirectory must be empty or match one of the language folders: '{1}', the value of relative path is '{2}', this file will not be included to manifest resources",
								fi.Path.Path, languages.ToString("; ", s => s.Quote()), relDir);
							continue;
						}

						var dest = PathString.MakeCombinedAndNormalized(appManifestFolder.Path, relPath);

						if (CopyImageFile(fi.Path, dest))
						{
							//Include copied resource as PRIResource
							resourcefiles.Include(dest.Path, optionSetName: ReswToolOptionSetName, baseDir: appManifestFolder.Path);
						}
						else
						{
							// Source file is already at the destination. Assign optionset
							fi.OptionSetName = ReswToolOptionSetName;
						}
					}
				}
			}

			return appManifestFolder;
		}
	}
}
