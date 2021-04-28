using System;
using System.IO;
using System.Xml;

using Microsoft.VisualStudio.TestTools.UnitTesting;

using EA.Eaconfig.Build;
using EA.FrameworkTasks.Model;

using NAnt.Core;
using NAnt.Core.PackageCore;
using NAnt.Core.Tasks;
using System.Collections.Generic;

namespace NAnt.Tests.Util
{
	[TestClass] // this attribute needs to be here for the initialize and clean up to work
	public abstract class NAntTestClassBase
	{
		// IMPORTANT: Tests that needs to use this base class to setup package map probably should not be run
		// in parallel with another tests as it would need to clear and reset package map singleton instance.
		// So running the tests in parallel could interfere each other.

		[TestInitialize]
		public virtual void TestSetup()
		{
		}

		[TestCleanup]
		public virtual void TestCleanUp()
		{
			// reset a bunch of global systems in nant
			LineInfoDocument.ClearCache();
			PackageMap.Reset();
			DoOnce.Reset();
		}

		protected virtual void InitializePackageMapMasterConfig(Project project, params string[] masterConfigContents)
		{
			// Can't initialize package map if there is a previous instance. Run the reset first
			// in case we still have an instance dangling from some unit test.
			PackageMap.Reset();

			string masterConfigPath = Path.Combine(Environment.CurrentDirectory, "masterconfig.xml");
			SeedXmlDocumentCache
			(
				masterConfigPath,
				masterConfigContents
			);
			MasterConfig masterConfig = MasterConfig.Load(project, masterConfigPath);
			PackageMap.Init(project, masterConfig);
		}

		// can be used to put a file into the document cache without touching the disk, useful for creating mock files
		// for tests
		protected static void SeedXmlDocumentCache(string filePath, string fileContents)
		{
			if (LineInfoDocument.IsInCache(filePath))
			{
				throw new ArgumentException($"Already a cache entry for '{filePath}'.!");
			}
			using (XmlTextReader reader = new XmlTextReader(filePath, new StringReader(fileContents)))
			{
				LineInfoDocument.UpdateCache(filePath, reader);
			}
		}

		protected static void SeedXmlDocumentCache(string filePath, params string[] fileContents)
		{
			SeedXmlDocumentCache(filePath, String.Join(Environment.NewLine, fileContents));
		}

		protected static void UpdateXmlDocumentCache(string filePath, string fileContents)
		{
			if (!LineInfoDocument.IsInCache(filePath))
			{
				throw new ArgumentException($"No cache entry for '{filePath}'.!");
			}
			using (XmlTextReader reader = new XmlTextReader(filePath, new StringReader(fileContents)))
			{
				LineInfoDocument.UpdateCache(filePath, reader);
			}
		}

		protected static void UpdateXmlDocumentCache(string filePath, params string[] fileContents)
		{
			UpdateXmlDocumentCache(filePath, String.Join(Environment.NewLine, fileContents));
		}

		protected static int RunNant(params string[] args)
		{
			using (Console.ConsoleRunner runner = new Console.ConsoleRunner())
			{
				return runner.Start(args);
			}
		}

		protected static BuildGraph CreateBuildGraph(Project project, string[] buildConfigurations, string[] buildGroupNames)
		{
			// 
			// TODO?: We really should have a real function in Framework itself that does this rather than
			// having this test case "duplicate" the work (and not really complete). 
			// We aren't really testing the true build graph creation process here!  Unfornately this 
			// create build graph process is setup in eaconfig.  So would need some refactoring.
			//
			// Need to follow the process in eaconfig-build-graph-all target (in eaconfig package) if we allow
			// multiple configs as input!
			//
			string buildConfigurationsString = String.Join(" ", buildConfigurations);
			string buildGroupNamesString = String.Join(" ", buildGroupNames);
			string currConfig = project.Properties["config"];

			// Setup this property mainly for the "load-package" target
			project.Properties["eaconfig.build.group.names"] = buildGroupNamesString;

			new Task_InitBuildGraph()
			{
				Project = project,
				BuildConfigurations = buildConfigurationsString,
				BuildGroupNames = buildGroupNamesString,
				ProcessGenerationData = true
			}
			.Execute();

			// eaconfig execute the following loop as parallel foreach.  Should test the same try to force disable parallel if we need to debug.
			Core.Util.NAntUtil.ParallelForEach
			(
				buildConfigurations, config =>
				{
					// Do the load package for each config and need a new nested "Project" object for config that is 
					// not the current config. (That's how eaconfig's target is setup).
					if (config == currConfig)
					{
						new LoadPackageTask()
						{
							Project = project,
							// "AutobuildTarget" get executed if we have package dependency and 
							// need config package to setup this target!
							AutobuildTarget = "load-package",
							BuildGroupNames = buildGroupNamesString,
							ProcessGenerationData = true
						}
						.Execute();
					}
					else
					{
						// In eaconfig, a new <nant> task was executed for non-current configs and 
						// the followings is just the absolute basic in that the nant tasks was
						// doing (ignoring the command line) with global properties set as initialized
						// and start new build set to false!
						Project nestedProject = new Project
						(
							log: project.Log,
							buildFile: project.BuildFileLocalName,
							buildTargets: project.BuildTargetNames,
							parent: project,
							commandLineProperties: new Dictionary<string, string>()
							{
								{ "config", config },
								{ "package.configs", buildConfigurationsString },
								{ "eaconfig.build.group.names", buildGroupNamesString }
							}
						);

						// Not doing new build, sp passing down the Global Named Objects to nested project.
						lock (Project.GlobalNamedObjects)
						{
							foreach (Guid id in Project.GlobalNamedObjects)
							{
								if (project.NamedObjects.TryGetValue(id, out object obj))
								{
									nestedProject.NamedObjects[id] = obj;
								}
							}
						}

						nestedProject.Run();

						new LoadPackageTask()
						{
							Project = nestedProject,
							// "AutobuildTarget" get executed if we have package dependency and 
							// need config package to setup this target!
							AutobuildTarget = "load-package",
							BuildGroupNames = buildGroupNamesString,
							ProcessGenerationData = true
						}
						.Execute();
					}
				}
			);	// End of NAntUtil.ParallelForEach
			new Task_CreateBuildGraph()
			{
				Project = project,
				BuildConfigurations = buildConfigurationsString,
				BuildGroupNames = buildGroupNamesString,
				ProcessGenerationData = true
			}
			.Execute();
			BuildGraph buildGraph = project.BuildGraph();
			return buildGraph;
		}
	}
}