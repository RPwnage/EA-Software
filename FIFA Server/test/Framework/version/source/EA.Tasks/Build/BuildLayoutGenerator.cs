using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;

using NAnt.Core;
using NAnt.Core.Logging;
using NAnt.Core.Util;
using NAnt.Core.Writers;
using NAnt.Core.Threading;
using NAnt.Core.Attributes;

using EA.Eaconfig.Core;
using EA.Eaconfig.Modules;
using EA.FrameworkTasks.Model;

using Newtonsoft.Json;
using System;

namespace EA.Eaconfig.Build
{
	[TaskName("GenerateBuildLayout")]
	internal class BuildLayoutGenerator : Task
	{
		private sealed class BuildLayout
		{
			[JsonProperty(PropertyName = "version", Order = 0)]
			internal int Version = 2;
			// version 1: first version
			// version 2: added additional files

			[JsonProperty(PropertyName = "platform", Order = 1)]
			internal string Platform; // config system unless overridden by eaconfig.build-layout-platform property

			[JsonProperty(PropertyName = "entrypoint", Order = 2)]
			internal Dictionary<string, string> EntryPoint; // key value pairs for all the information needed to run the build, keys vary depending on what data is needed to run the platform

			[JsonProperty(PropertyName = "additionalfiles", Order = 3)]
			internal List<string> AdditionalFiles; // list of loose files alongside the final output that are required for build to execute, on platforms with bundles (android, ios)
												   // this is typically empty

			[JsonProperty(PropertyName = "deployfilters", Order = 4)]
			internal List<string> DeployFilters; // list of filters to exclude from deployed builds, e.g. .ilk or .appxmanifest files,
			                                     // used when the deploy step is normally a whole directory, like Icepick's deploy step

			[JsonProperty(PropertyName = "tags", Order = 5)]
			internal Dictionary<string, string> Tags; // additional custom data
		}

		protected override void ExecuteTask()
		{
			try
			{
				// generate build layout input files
				NAntUtil.ParallelForEach(Project.BuildGraph().SortedActiveModules, (m) =>
				{
					if (!(m is Module_UseDependency))
					{
						ProcessableModule mod = (ProcessableModule)m;
						AddBuildLayoutFile(mod);
					}
				});
			} 
			catch (Exception e)
			{
				ThreadUtil.RethrowThreadingException(String.Format("Error in task '{0}'", Name), Location, e);
			}
		}

		internal static void AddBuildLayoutFile(IModule module)
		{
			if (!module.GetModuleBooleanPropertyValue("generate-buildlayout", false, failOnNonBoolValue: true))
			{
				return;
			}

			string configSystem = module.Package.Project.GetPropertyOrFail("config-system");
			string platform = module.Package.Project.Properties["eaconfig.build-layout-platform"] ?? configSystem;

			Dictionary<string, string> tags = new Dictionary<string, string>();
			string tagOptionsetName = module.PropGroupName("build-layout-tags");
			if (module.Package.Project.NamedOptionSets.TryGetValue(tagOptionsetName, out OptionSet tagOptionset))
			{
				tags = tagOptionset.Options;
			}

			string entryOptionsetName = module.PropGroupName("build-layout-entrypoint");
			OptionSet entryPointOptionset = OptionSetUtil.GetOptionSet(module.Package.Project, entryOptionsetName);
			Dictionary<string, string> entryPoint = null;
			if (entryPointOptionset != null)
			{
				entryPoint = entryPointOptionset.Options.ToDictionary
				(
					keySelector: kvp => kvp.Key,
					elementSelector: kvp => MacroMap.Replace(kvp.Value, "output", PathUtil.RelativePath(module.PackagingOutput(kvp.Key), module.OutputDir.Path))
				);
			}

			List<string> additionalFiles = null;
			if (module.Package.Project.Properties.GetBooleanPropertyOrDefault(module.Configuration.System+".build-layout-skip-additionalfiles",false))
			{
				additionalFiles = new List<string>();
			}
			else
			{
				additionalFiles = module.RuntimeDependencyFiles.OrderBy(s => s).ToList();
			}

			string deployFilters = module.PropGroupValue("layout-extension-filter");
			List<string> deployFiltersList = deployFilters.Split(';').ToList();

			BuildLayout layout = new BuildLayout
			{
				Platform = platform,
				Tags = tags,
				EntryPoint = entryPoint,
				AdditionalFiles = additionalFiles,
				DeployFilters = deployFiltersList
			};

			string buildLayoutInDir = module.Package.PackageConfigBuildDir.NormalizedPath;
			Directory.CreateDirectory(buildLayoutInDir);
			string buildLayoutInPath = Path.Combine(buildLayoutInDir, module.Name + ".buildlayout.in");

			JsonSerializer serializer = new JsonSerializer();
			using (TextWriterWithIndent writer = new TextWriterWithIndent { FileName = buildLayoutInPath })
			using (JsonWriter jsonWriter = new JsonTextWriter(new StreamWriter(writer.Data, Encoding.Default, 1024, leaveOpen: true)))
			{
				writer.CacheFlushed += (s, e) => OnBuildLayoutFlush(e.FileName, e.IsUpdatingFile, module.Package.Project.Log);
				serializer.Serialize(jsonWriter, layout, typeof(BuildLayout));
			}

			// add a copy entry for the .in file to the output path alongside the final output
			module.CopyLocalFiles.Add(new CopyLocalInfo(cached: new CachedCopyLocalInfo(buildLayoutInPath, module.Name + ".buildlayout", true, false, false, module), destinationModule: module));
		}

		private static void OnBuildLayoutFlush(string fileName, bool updating, Log log)
		{
			string message = string.Format("[buildlayout] {0} buildlayout file  '{1}'", updating ? "    Updating" : "NOT Updating", fileName);
			if (updating)
			{
				log.Minimal.WriteLine(message);
			}
			else
			{
				log.Status.WriteLine(message);
			}
		}
	}
}