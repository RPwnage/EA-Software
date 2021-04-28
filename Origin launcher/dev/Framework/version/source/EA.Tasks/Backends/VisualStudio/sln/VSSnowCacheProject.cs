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

using System.IO;
using System.Linq;

using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Writers;

using EA.Eaconfig.Core;
using EA.Eaconfig.Modules;
using EA.FrameworkTasks.Model;
using System;
using System.Text;

namespace EA.Eaconfig.Backends.VisualStudio
{
	static class SnowCacheUtils
	{
		internal static string GetServerExe(string executablePath)
		{
			if (String.IsNullOrEmpty(executablePath))
			{
				// unique, per-branch key, not frostbite specific
				string tntRoot = System.Environment.GetEnvironmentVariable("TNT_ROOT") ?? "";
				executablePath = System.IO.Path.Combine(tntRoot, "bin\\SnowCache\\SnowCacheServer.exe");
			}
			return executablePath;
		}

		internal static void WriteServerExe(IXmlWriter writer, string executablePath)
		{
			writer.WriteStartElement("PropertyGroup");
			writer.WriteElementString("SnowCache", GetServerExe(executablePath));
			writer.WriteEndElement(); // PropertyGroup
		}

		internal static void WriteLocalDir(IXmlWriter writer)
		{
			// unique, per-branch key, not frostbite specific
			string localRoot = System.Environment.GetEnvironmentVariable("LOCAL_ROOT") ?? "";
			if (string.IsNullOrEmpty(localRoot))
			{
				string tntRoot = System.Environment.GetEnvironmentVariable("TNT_ROOT") ?? "";
				localRoot = Path.Combine(tntRoot, "Local");
			}

			writer.WriteStartElement("PropertyGroup");
			writer.WriteElementString("LocalDir", localRoot);
			writer.WriteEndElement(); // PropertyGroup
		}

		internal static void WriteUsingTask(IXmlWriter writer, Generator aBuildGenerator)
		{
			string filePath = (aBuildGenerator.IsPortable) ? Path.Combine(aBuildGenerator.SlnFrameworkFilesRoot, "bin") : Path.Combine(NAnt.Core.PackageCore.PackageMap.Instance.GetFrameworkRelease().Path, "bin");
			var msbuild_assembly = Path.GetFullPath(Path.Combine(filePath, "SnowCache.dll"));

			writer.WriteStartElement("UsingTask");
			writer.WriteAttributeString("TaskName", "EA.Framework.MsBuild.SnowCache");
			writer.WriteAttributeString("AssemblyFile", msbuild_assembly);
			writer.WriteEndElement();
		}
	}

	public partial class VSCppMsbuildProject
	{
		protected void WriteTargetInvocation(IXmlWriter writer, string aCondition, params string[] aTargets)
		{
			foreach (var target in aTargets)
			{
				writer.WriteStartElement("CallTarget");
				writer.WriteAttributeString("Targets", target);
				if (!string.IsNullOrEmpty(aCondition))
				{
					writer.WriteAttributeString("Condition", aCondition);
				}
				writer.WriteEndElement();
			}
		}

		bool CanUseSnowCache()
		{
			bool canUseSnowCache = false;
			foreach (Module module in Modules)
			{
				bool isSharedPchModule = module.Package.Project.Properties.GetBooleanPropertyOrDefault(
					"package." + module.Package.Name + "." + module.Name + ".issharedpchmodule", false);
				string configType = GetConfigurationType(module);
				if (configType == "Utility" || configType == "Makefile" || isSharedPchModule)
				{
					continue;
				}
				else
				{
					if (module.Tools.Count() <= 3)
					{
						var native = module as Module_Native;
						if ((native.Cc != null && native.Cc.SourceFiles.Includes.Count > 0)
						  || (native.Asm != null && native.Asm.SourceFiles.Includes.Count > 0)
						  || (native.Lib != null && native.Lib.ObjectFiles.Includes.Count > 0))
						{
							canUseSnowCache = true;
						}
					}
					else
					{
						canUseSnowCache = true;
					}
				}
			}

			return canUseSnowCache;
		}


		partial void WriteSnowCacheWrite(IXmlWriter writer)
		{
			string mode = Modules[0].Package.Project.Properties["package.SnowCache.mode"].TrimWhiteSpace();
			bool isUpload = mode == "upload";
			bool isForceUpload = mode == "forceupload";

			if (isUpload || isForceUpload)
			{
				if (!CanUseSnowCache())
				{
					return;
				}

				string log = Modules[0].Package.Project.Properties["package.SnowCache.log"].TrimWhiteSpace();
				bool logEnabled = (string.Compare(log, "true", true) == 0);

				string diagnostics = Modules[0].Package.Project.Properties["package.SnowCache.diagnostics"].TrimWhiteSpace();
				bool diagnosticsEnabled = (string.Compare(diagnostics, "true", true) == 0);

				// defaults to "eav-fbvs-sc01.eac.ad.ea.com" which is EAV's snowcache avalanche host. Proper usage should never use the default.
				string avalancheHost = Modules[0].Package.Project.Properties.GetPropertyOrDefault("package.SnowCache.avalancheHost", "eav-fbvs-sc01.eac.ad.ea.com").TrimWhiteSpace();

				string syncedcl = Modules[0].Package.Project.Properties["package.SnowCache.synced_changelist"].TrimWhiteSpace();
				string executablePath = Modules[0].Package.Project.Properties.GetPropertyOrDefault("package.SnowCache.executablePath", "").TrimWhiteSpace();

				SnowCacheUtils.WriteUsingTask(writer, BuildGenerator);
				SnowCacheUtils.WriteServerExe(writer, executablePath);
				SnowCacheUtils.WriteLocalDir(writer);

				writer.WriteStartElement("Target");
				writer.WriteAttributeString("Name", "SnowCacheWriteTarget");
				writer.WriteStartElement("SnowCache");
				writer.WriteAttributeString("ExecutableFile", "$(SnowCache)");
				writer.WriteAttributeString("Mode", isForceUpload ? "forceupload" : "upload");
				writer.WriteAttributeString("Platform", "$(Platform)");
				writer.WriteAttributeString("Configuration", "$(Configuration)");
				writer.WriteAttributeString("ProjectFile", "$(ProjectDir)$(ProjectFileName)");
				writer.WriteAttributeString("SolutionFile", "$(SolutionPath)");
				writer.WriteAttributeString("IntermediateDir", "$(IntDir)");
				writer.WriteAttributeString("LocalDir", "$(LocalDir)");
				writer.WriteAttributeString("AvalancheHost", avalancheHost);
				writer.WriteAttributeString("LoggingEnabled", logEnabled ? "true" : "false");
				writer.WriteAttributeString("DiagnosticsEnabled", diagnosticsEnabled ? "true" : "false");
				writer.WriteAttributeString("SyncedChangelist", syncedcl);
				writer.WriteEndElement(); // SnowCache
				writer.WriteEndElement(); // SnowCacheWriteTarget

				StartPropertyGroup(writer);
				writer.WriteStartElement("BuildDependsOn");
				writer.WriteString("$(BuildDependsOn);SnowCacheWriteTarget");
				writer.WriteEndElement(); // BuildSteps
				writer.WriteEndElement(); // PropertyGroup
			}
		}

		partial void WriteSnowCacheRead(IXmlWriter writer)
		{
			string mode = Modules[0].Package.Project.Properties["package.SnowCache.mode"].TrimWhiteSpace();
			bool isDownload = mode == "download";
			bool isUploadAndDownload = mode == "uploadanddownload";
			if (isDownload || isUploadAndDownload)
			{
				if (!CanUseSnowCache())
				{
					return;
				}

				string log = Modules[0].Package.Project.Properties["package.SnowCache.log"].TrimWhiteSpace();
				bool logEnabled = (string.Compare(log, "true", true) == 0);

				string diagnostics = Modules[0].Package.Project.Properties["package.SnowCache.diagnostics"].TrimWhiteSpace();
				bool diagnosticsEnabled = (string.Compare(diagnostics, "true", true) == 0);

				// defaults to "eav-fbvs-sc01.eac.ad.ea.com" which is EAV's snowcache avalanche host. Proper usage should never use the default.
				string avalancheHost = Modules[0].Package.Project.Properties.GetPropertyOrDefault("package.SnowCache.avalancheHost", "eav-fbvs-sc01.eac.ad.ea.com").TrimWhiteSpace();

				string syncedcl = Modules[0].Package.Project.Properties["package.SnowCache.synced_changelist"].TrimWhiteSpace();
				string executablePath = Modules[0].Package.Project.Properties.GetPropertyOrDefault("package.SnowCache.executablePath", "").TrimWhiteSpace();

				SnowCacheUtils.WriteUsingTask(writer, BuildGenerator);
				SnowCacheUtils.WriteServerExe(writer, executablePath);
				SnowCacheUtils.WriteLocalDir(writer);

				writer.WriteStartElement("Target");
				writer.WriteAttributeString("Name", "SnowCacheInitialTargets");
				writer.WriteAttributeString("DependsOnTargets", "ResolveReferences;PrepareForBuild;InitializeBuildStatus;BuildGenerateSources");
				writer.WriteEndElement(); // Target SnowCacheInitialTargets

				writer.WriteStartElement("Target");
				writer.WriteAttributeString("Name", "SnowCachePostBuildEvent");
				writer.WriteAttributeString("DependsOnTargets", "PostBuildEvent");
				writer.WriteAttributeString("Condition", "'$(SnowCacheSuccess)' == 'true' and ( '$(RunPostBuildEvent)'=='Always' or '$(RunPostBuildEvent)'=='OnOutputUpdated' )");
				writer.WriteEndElement(); // Target SnowCachePostBuildEvent

				writer.WriteStartElement("Target");
				writer.WriteAttributeString("Name", "SnowCacheBuildTargets");
				writer.WriteAttributeString("Condition", "$(SnowCacheSuccess) == 'false'");
				writer.WriteAttributeString("DependsOnTargets", "BuildCompile;BuildLink");
				writer.WriteStartElement("SnowCache");
				writer.WriteAttributeString("ExecutableFile", "$(SnowCache)");

				if (isUploadAndDownload)
					writer.WriteAttributeString("Mode", "uploadandupdatelocal");
				else
					writer.WriteAttributeString("Mode", "updatelocal");

				writer.WriteAttributeString("Platform", "$(Platform)");
				writer.WriteAttributeString("Configuration", "$(Configuration)");
				writer.WriteAttributeString("ProjectFile", "$(ProjectDir)$(ProjectFileName)");
				writer.WriteAttributeString("SolutionFile", "$(SolutionPath)");
				writer.WriteAttributeString("IntermediateDir", "$(IntDir)");
				writer.WriteAttributeString("LocalDir", "$(LocalDir)");
				writer.WriteAttributeString("AvalancheHost", avalancheHost);
				writer.WriteAttributeString("LoggingEnabled", logEnabled ? "true" : "false");
				writer.WriteAttributeString("DiagnosticsEnabled", diagnosticsEnabled ? "true" : "false");
				writer.WriteAttributeString("SyncedChangelist", syncedcl);
				writer.WriteEndElement(); // SnowCache
				writer.WriteEndElement(); // Target SnowCacheBuildTargets

				writer.WriteStartElement("Target");
				writer.WriteAttributeString("Name", "SnowCachePostBuildCopyTargets");
				writer.WriteAttributeString("Condition", "$(SnowCacheSuccess) == 'true'");
				writer.WriteAttributeString("DependsOnTargets", "CopyFilesToOutputDirectory;FrameworkExecutePostBuildCopyLocal;FrameworkExecuteTimestampUpdate");
				writer.WriteEndElement(); // Target SnowCachePostBuildTargets

				writer.WriteStartElement("Target");
				writer.WriteAttributeString("Name", "SnowCachePostBuildTargets");
				writer.WriteAttributeString("DependsOnTargets", "SnowCachePostBuildEvent;AfterBuild;SnowCachePostBuildCopyTargets;FinalizeBuildStatus");
				writer.WriteEndElement(); // Target SnowCachePostBuildTargets

				writer.WriteStartElement("Target");
				writer.WriteAttributeString("Name", "SnowCacheReadTarget");
				writer.WriteStartElement("SnowCache");
				writer.WriteAttributeString("ExecutableFile", "$(SnowCache)");
				writer.WriteAttributeString("Mode", "download");
				writer.WriteAttributeString("Platform", "$(Platform)");
				writer.WriteAttributeString("Configuration", "$(Configuration)");
				writer.WriteAttributeString("ProjectFile", "$(ProjectDir)$(ProjectFileName)");
				writer.WriteAttributeString("SolutionFile", "$(SolutionPath)");
				writer.WriteAttributeString("IntermediateDir", "$(IntDir)");
				writer.WriteAttributeString("LocalDir", "$(LocalDir)");
				writer.WriteAttributeString("AvalancheHost", avalancheHost);
				writer.WriteAttributeString("LoggingEnabled", logEnabled ? "true" : "false");
				writer.WriteAttributeString("DiagnosticsEnabled", diagnosticsEnabled ? "true" : "false");
				writer.WriteAttributeString("SyncedChangelist", syncedcl);
				writer.WriteStartElement("Output");
				writer.WriteAttributeString("TaskParameter", "Success");
				writer.WriteAttributeString("PropertyName", "SnowCacheSuccess");
				writer.WriteEndElement(); // Output
				writer.WriteEndElement(); // SnowCache
				writer.WriteEndElement(); // SnowCache


				writer.WriteStartElement("Target");
				writer.WriteAttributeString("Name", "SnowCache");
				writer.WriteAttributeString("DependsOnTargets", "SnowCacheInitialTargets;SnowCacheReadTarget;SnowCacheBuildTargets;SnowCachePostBuildTargets");
				writer.WriteEndElement(); // SnowCache

				StartPropertyGroup(writer);
				writer.WriteStartElement("BuildSteps");
				writer.WriteString("SnowCache");
				writer.WriteEndElement(); // BuildSteps
				writer.WriteEndElement(); // PropertyGroup
			}
		}
	}


	public partial class VSDotNetProject
	{
		partial void WriteSnowCache(IXmlWriter writer)
		{
			string mode = Modules[0].Package.Project.Properties["package.SnowCache.mode"].TrimWhiteSpace();
			string log = Modules[0].Package.Project.Properties["package.SnowCache.log"].TrimWhiteSpace();
			bool isDownload = mode == "download";
			bool isUpload = mode == "upload";
			bool isForceUpload = mode == "forceupload";
			bool isUploadAndDownload = mode == "uploadanddownload";
			bool logEnabled = (string.Compare(log, "true", true) == 0);
			string avalancheHost = Modules[0].Package.Project.Properties.GetPropertyOrDefault("package.SnowCache.avalancheHost", "eav-fbvs-sc01.eac.ad.ea.com").TrimWhiteSpace();
			string executablePath = Modules[0].Package.Project.Properties.GetPropertyOrDefault("package.SnowCache.executablePath", "").TrimWhiteSpace();

			if (isDownload || isUpload || isUploadAndDownload || isForceUpload)
			{
				SnowCacheUtils.WriteUsingTask(writer, BuildGenerator);
				SnowCacheUtils.WriteServerExe(writer, executablePath);
				SnowCacheUtils.WriteLocalDir(writer);

				if (isDownload || isUploadAndDownload)
				{
					writer.WriteStartElement("ItemGroup");
					writer.WriteStartElement("CustomAdditionalCompileInputs");
					writer.WriteAttributeString("Include", "$(SnowCache)");
					writer.WriteAttributeString("Visible", "false");
					writer.WriteEndElement();
					writer.WriteEndElement();

					writer.WriteStartElement("Target");
					writer.WriteAttributeString("Name", "Set_AssemblyTimestampBeforeCompile");
					writer.WriteStartElement("PropertyGroup");
					writer.WriteElementString("_AssemblyTimestampBeforeCompile", "%(IntermediateAssembly.ModifiedTime)");
					writer.WriteEndElement();
					writer.WriteEndElement();

					writer.WriteStartElement("Target");
					writer.WriteAttributeString("Name", "Set_AssemblyTimestampAfterCompile");
					writer.WriteAttributeString("Condition", "$(SnowCacheSuccess) == 'true'");
					writer.WriteStartElement("PropertyGroup");
					writer.WriteElementString("_AssemblyTimestampAfterCompile", "%(IntermediateAssembly.ModifiedTime)");
					writer.WriteEndElement();
					writer.WriteEndElement();

					writer.WriteStartElement("Target");
					writer.WriteAttributeString("Name", "QuerySnowCache");
					writer.WriteStartElement("SnowCache");
					{
						writer.WriteAttributeString("ExecutableFile", "$(SnowCache)");
						writer.WriteAttributeString("Mode", "download");
						writer.WriteAttributeString("Platform", "$(Platform)");
						writer.WriteAttributeString("Configuration", "$(Configuration)");
						writer.WriteAttributeString("ProjectFile", "$(ProjectDir)$(ProjectFileName)");
						writer.WriteAttributeString("SolutionFile", "$(SolutionPath)");
						writer.WriteAttributeString("IntermediateDir", "$(IntermediateOutputPath)");
						writer.WriteAttributeString("LocalDir", "$(LocalDir)");
						writer.WriteAttributeString("AvalancheHost", avalancheHost);
						writer.WriteAttributeString("LoggingEnabled", logEnabled ? "true" : "false");
						writer.WriteStartElement("Output");
						{
							writer.WriteAttributeString("TaskParameter", "Success");
							writer.WriteAttributeString("PropertyName", "SnowCacheSuccess");
						}
						writer.WriteEndElement(); // Output
					}
					writer.WriteEndElement(); // SnowCache
					writer.WriteEndElement();

					writer.WriteStartElement("Target");
					writer.WriteAttributeString("Name", "BuildDependsOnWithoutBeforeBuild");
					writer.WriteAttributeString("Condition", "$(SnowCacheSuccess) == 'false'");
					writer.WriteStartElement("ItemGroup");
					writer.WriteStartElement("BuildDependsOnWithoutBeforeBuildList");
					writer.WriteAttributeString("Include", "$(BuildDependsOn)");
					writer.WriteAttributeString("Exclude", "BeforeBuild");
					writer.WriteElementString("Visible", "false");
					writer.WriteEndElement();
					writer.WriteEndElement();
					writer.WriteStartElement("CallTarget");
					writer.WriteAttributeString("Targets", "@(BuildDependsOnWithoutBeforeBuildList)");
					writer.WriteEndElement();

					writer.WriteStartElement("SnowCache");
					writer.WriteAttributeString("ExecutableFile", "$(SnowCache)");
					if (isUploadAndDownload)
						writer.WriteAttributeString("Mode", "uploadandupdatelocal");
					else
						writer.WriteAttributeString("Mode", "updatelocal");
					writer.WriteAttributeString("Platform", "$(Platform)");
					writer.WriteAttributeString("Configuration", "$(Configuration)");
					writer.WriteAttributeString("ProjectFile", "$(ProjectDir)$(ProjectFileName)");
					writer.WriteAttributeString("SolutionFile", "$(SolutionPath)");
					writer.WriteAttributeString("IntermediateDir", "$(IntermediateOutputPath)");
					writer.WriteAttributeString("LocalDir", "$(LocalDir)");
					writer.WriteAttributeString("AvalancheHost", avalancheHost);
					writer.WriteAttributeString("LoggingEnabled", logEnabled ? "true" : "false");
					writer.WriteEndElement();

					writer.WriteEndElement();

					writer.WriteStartElement("Target");
					writer.WriteAttributeString("Name", "CacheCopyResults");
					writer.WriteAttributeString("Condition", "'$(SnowCacheSuccess)' == 'true'");
					writer.WriteAttributeString("DependsOnTargets", "CopyFilesToOutputDirectory;FrameworkExecutePostBuildCopyLocal;FrameworkExecuteTimestampUpdate;GetTargetPath");
					writer.WriteEndElement();

					writer.WriteStartElement("Target");
					writer.WriteAttributeString("Name", "CachePostBuildEvent");
					writer.WriteAttributeString("Condition", "'$(SnowCacheSuccess)' == 'true' and ( '$(RunPostBuildEvent)'=='Always' or '$(RunPostBuildEvent)'=='OnOutputUpdated' )");
					writer.WriteAttributeString("DependsOnTargets", "PostBuildEvent");
					writer.WriteEndElement();

					// command line solution dependency traversal works differently from in-IDE builds, we need to do a little more work in this case
					writer.WriteStartElement("Target");
					writer.WriteAttributeString("Name", "FirstPartOfBuildRequiredByCmdLineBuilds");
					writer.WriteAttributeString("Condition", "'$(BuildingInsideVisualStudio)'!='true'");
					writer.WriteAttributeString("DependsOnTargets", "BuildOnlySettings;PrepareForBuild;PreBuildEvent;ResolveReferences");
					writer.WriteEndElement();

					writer.WriteStartElement("Target");
					writer.WriteAttributeString("Name", "Build");
					writer.WriteAttributeString("Returns", "@(TargetPathWithTargetPlatformMoniker)");
					writer.WriteAttributeString("DependsOnTargets",
						"Set_AssemblyTimestampBeforeCompile" + ";" +
						"_TimeStampBeforeCompile" + ";" +
						"BeforeBuild" + ";" +
						"FirstPartOfBuildRequiredByCmdLineBuilds" + ";" +
						"QuerySnowCache" + ";" +
						"BuildDependsOnWithoutBeforeBuild" + ";" +
						"Set_AssemblyTimestampAfterCompile" + ";" +
						"CacheCopyResults" + ";" +
						"CachePostBuildEvent");

					writer.WriteEndElement();
				}
				else
				{
					writer.WriteStartElement("Target");
					writer.WriteAttributeString("Name", "SnowCacheBuild");
					writer.WriteStartElement("SnowCache");
					writer.WriteAttributeString("ExecutableFile", "$(SnowCache)");
					writer.WriteAttributeString("Mode", isForceUpload ? "forceupload" : "upload");
					writer.WriteAttributeString("Platform", "$(Platform)");
					writer.WriteAttributeString("Configuration", "$(Configuration)");
					writer.WriteAttributeString("ProjectFile", "$(ProjectDir)$(ProjectFileName)");
					writer.WriteAttributeString("SolutionFile", "$(SolutionPath)");
					writer.WriteAttributeString("IntermediateDir", "$(IntermediateOutputPath)");
					writer.WriteAttributeString("LocalDir", "$(LocalDir)");
					writer.WriteAttributeString("AvalancheHost", avalancheHost);
					writer.WriteAttributeString("LoggingEnabled", logEnabled ? "true" : "false");
					writer.WriteEndElement();
					writer.WriteEndElement();

					writer.WriteStartElement("PropertyGroup");
					writer.WriteElementString("BuildDependsOn", "$(BuildDependsOn);SnowCacheBuild");
					writer.WriteEndElement();
				}
			}
		}
	}


	[TaskName("snowcache-solution-extension")]
	class SnowcacheSolutionExtension : IVisualStudioSolutionExtension
	{
		public override void BeforeSolutionBuildTargets(System.Text.StringBuilder builder)
		{
			base.BeforeSolutionBuildTargets(builder);

			string executablePath = Project.Properties.GetPropertyOrDefault("package.SnowCache.executablePath", "").TrimWhiteSpace();
			string mode = Project.Properties["package.SnowCache.mode"].TrimWhiteSpace();

			builder.AppendLine(String.Format("  <UsingTask TaskName=\"EA.Framework.MsBuild.SnowCacheSlnSummary\" AssemblyFile=\"{0}\"/>", Path.Combine(Project.NantLocation, "SnowCache.dll")));
			builder.AppendLine(String.Format("  <Target Name=\"SnowCacheSlnSummaryInitialize\" BeforeTargets=\"Build\">"));
			builder.AppendLine(String.Format("    <SnowCacheSlnSummary"));
			builder.AppendLine(String.Format("      ExecutableFile=\"{0}\"", SnowCacheUtils.GetServerExe(executablePath)));
			builder.AppendLine(String.Format("      Action=\"start\""));
			builder.AppendLine(String.Format("      Mode=\"{0}\"", mode));
			builder.AppendLine(String.Format("      Platform=\"$(Platform)\""));
			builder.AppendLine(String.Format("      Configuration=\"$(Configuration)\""));
			builder.AppendLine(String.Format("      SolutionFile=\"$(SolutionPath)\""));
			builder.AppendLine(String.Format("    />"));
			builder.AppendLine(String.Format("  </Target>"));
		}

		public override void AfterSolutionBuildTargets(System.Text.StringBuilder builder)
		{
			base.AfterSolutionBuildTargets(builder);

			string executablePath = Project.Properties.GetPropertyOrDefault("package.SnowCache.executablePath", "").TrimWhiteSpace();
			string mode = Project.Properties["package.SnowCache.mode"].TrimWhiteSpace();
			string syncedCL = Project.Properties["package.SnowCache.synced_changelist"].TrimWhiteSpace();

			builder.AppendLine(String.Format("  <UsingTask TaskName=\"EA.Framework.MsBuild.SnowCacheSlnSummary\" AssemblyFile=\"{0}\"/>", Path.Combine(Project.NantLocation, "SnowCache.dll")));
			builder.AppendLine(String.Format("  <Target Name=\"SnowCacheSlnSummaryOutput\" AfterTargets=\"Build\">"));
			builder.AppendLine(String.Format("    <SnowCacheSlnSummary"));
			builder.AppendLine(String.Format("      ExecutableFile=\"{0}\"", SnowCacheUtils.GetServerExe(executablePath)));
			builder.AppendLine(String.Format("      Action=\"stop\""));
			builder.AppendLine(String.Format("      Mode=\"{0}\"", mode));
			builder.AppendLine(String.Format("      SyncedChangelist=\"{0}\"", syncedCL));			
			builder.AppendLine(String.Format("      Platform=\"$(Platform)\""));
			builder.AppendLine(String.Format("      Configuration=\"$(Configuration)\""));
			builder.AppendLine(String.Format("      SolutionFile=\"$(SolutionPath)\""));
			builder.AppendLine(String.Format("    />"));
			builder.AppendLine(String.Format("  </Target>"));
		}
	}

}