// (c) Electronic Arts. All rights reserved.

using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.IO;
using System.Linq;
using System.Text;
using System.Xml;

using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Events;
using NAnt.Core.Logging;
using NAnt.Core.Util;
using NAnt.Core.Writers;
using NAnt.Core.Functions;

using EA.Eaconfig.Build;
using EA.Eaconfig.Core;
using EA.Eaconfig.Modules.Tools;
using EA.Eaconfig.Modules;
using EA.Eaconfig.Structured;
using EA.Eaconfig;
using EA.FrameworkTasks.Model;
using NAnt.Core.PackageCore;

namespace EA.NXConfig
{
	// this task is called as a postprocess task for every nx program
	[TaskName("nx-generate-nmeta")]
	internal class GenerateNmeta : Task
	{
		private static readonly IXmlWriterFormat s_nxXmlWriterFormat = new XmlFormat
		(
			identChar: ' ',
			identation: 2,
			newLineOnAttributes: false,
			encoding: new UTF8Encoding(true) // no byte order mark!
		);

		internal static IEnumerable<Module_Native> GetAllNativeModules(Project project)
		{
			return project.BuildGraph().SortedActiveModules.Where(m => m.Configuration.System == "nx").Where(mod => mod.IsKindOf(Module.Program)).Cast<Module_Native>();
		}
		
		internal static string GetUniqueFilesetFile(IModule module, string fileSetName, bool failOnMissing = false)
		{
			// search for a matching file set
			FileSet fileSet = module.GetPlatformModuleFileSet(fileSetName);
			if (fileSet != null)
			{
				FileItemList fileSetItems = fileSet.FileItems;
				if (!fileSetItems.Any())
				{
					throw new BuildException(String.Format("Nx fileset '{0}' is empty. If you were expecting this file to be auto-generated do not declare this fileset.", fileSetName));
				}
				else if (fileSetItems.Count() > 1)
				{
					throw new BuildException(String.Format("Nx fileset '{0}' contains more than one file: {1}{2}{1}", fileSetName, Environment.NewLine, FileSetFunctions.FileSetToString(module.Package.Project, fileSet.Name, Environment.NewLine)));
				}
				return PathNormalizer.Normalize(fileSetItems.First().Path.Path, getFullPath: true);
			}
			else if (failOnMissing)
			{
				throw new BuildException(String.Format("Critical Nmeta file missing! Define fileset {0} or {1}.", module.PropGroupName(fileSetName), module.GetPlatformModulePropGroupName(fileSetName)));
			}
			else
			{
				return null;
			}
		}

		internal static void CreateUniqueFilesetFile(IModule module, string fileSetNamePropertyName, string baseDir, string fileInclude)
		{
			FileSet fileset = new FileSet(module.Package.Project);
			fileset.BaseDirectory = baseDir;
			fileset.Include(fileInclude);
			module.Package.Project.NamedFileSets[module.PropGroupName(fileSetNamePropertyName + "." + module.Configuration.System)] = fileset;
		}

		internal static string GetFilesetName(Module module)
		{
			return module.Package.Project.Properties.GetPropertyOrDefault("platform-ptrsize", "64bit") == "64bit" ? "nmeta64file" : "nmeta32file";
		}
		
		protected override void ExecuteTask()
		{
			foreach (Module_Native nativeModule in GetAllNativeModules(Project))
			{
				ProcessModule(nativeModule);
			}
		}
		
		private void ProcessModule(Module_Native module)
		{
			string nmetaFileName = GetFilesetName(module);
			string manifestFile = GetUniqueFilesetFile(module, nmetaFileName);
			if (manifestFile == null)
			{
				// write nmeta and strings file
				var nmetaFile = WriteDefaultNmetaFile(module, module.IntermediateDir.Path);
				CreateUniqueFilesetFile(module, nmetaFileName, module.IntermediateDir.Path, nmetaFile);
			}
		}

		private string WriteDefaultNmetaFile(IModule module, string outputDirectory)
		{
			if (!Directory.Exists(outputDirectory))
			{
				Directory.CreateDirectory(outputDirectory);
			}

			bool is64bit = module.Package.Project.Properties.GetPropertyOrDefault("platform-ptrsize", "64bit") == "64bit";
			string suffix = is64bit ? "aarch64.lp64" : "arm.ilp32";
			string dataDir = Path.Combine(Project.Properties["package.nx_config.dir"], "config", "data");
			string moduleName = module.PropGroupValue("packagename", module.Name).Replace('-', 'X').Replace('.', '_');
			string fileName = Path.Combine(outputDirectory, moduleName + "." + suffix + ".nmeta");

			OptionSet overrides = null;
			string optionSetName = module.GroupName + ".nmetaoptions";
			module.Package.Project.NamedOptionSets.TryGetValue(optionSetName, out overrides);

			if (overrides == null)
				overrides = new OptionSet();

			{
				using (IXmlWriter writer = new NAnt.Core.Writers.XmlWriter(s_nxXmlWriterFormat))
				{
					writer.FileName = fileName;

					writer.WriteStartElement("NintendoSdkMeta");

					writer.WriteStartElement("Core");
					writer.WriteElementString("Name",                           overrides.Options["core.name"] ?? "Application");
					writer.WriteElementString("ApplicationId",                  overrides.Options["core.applicationId"] ?? "0x01004b9000490000");
					writer.WriteElementString("Is64BitInstruction",             is64bit ? "True" : "False");
					writer.WriteElementString("ProcessAddressSpace",            is64bit ? "AddressSpace64Bit" : "AddressSpace32Bit");
					if (is64bit)
						writer.WriteElementString("SystemResourceSize",         overrides.Options["core.systemResourceSize"] ?? "0x01000000");
					
					if (overrides.Options["core.SaveDataOwnerIds_" + 0] != null)
					{
						writer.WriteStartElement("FsAccessControlData");
						writer.WriteElementString("FlagPresets", "Debug");
						int i = 0;
						while (overrides.Options["core.SaveDataOwnerIds_" + i] != null)
						{
							writer.WriteStartElement("SaveDataOwnerIds");
							writer.WriteElementString("Accessibility", "Read");
							writer.WriteElementString("Id", overrides.Options["core.SaveDataOwnerIds_" + i]);
							writer.WriteEndElement();
							++i;
						}
							
						writer.WriteEndElement();
					}
					
					writer.WriteEndElement();

					writer.WriteStartElement("Application");

					writer.WriteStartElement("Title");
					writer.WriteElementString("Language",                       overrides.Options["application.title.language"] ?? "AmericanEnglish");
					writer.WriteElementString("Name",                           moduleName);
					writer.WriteElementString("Publisher",                      overrides.Options["application.title.publisher"] ?? "Electronic Arts");
					writer.WriteEndElement();

					writer.WriteStartElement("Icon");
					writer.WriteElementString("Language",                       overrides.Options["application.icon.language"] ?? "AmericanEnglish");
					writer.WriteElementString("IconPath",                       overrides.Options["application.icon.iconPath"] ?? dataDir + "\\nx_ea_logo.bmp");
					writer.WriteEndElement();

					writer.WriteElementString("DisplayVersion",                 overrides.Options["application.displayVersion"] ?? "1.0.0");
					
					if (!overrides.Options["application.supportedLanguage"].IsNullOrEmpty())
					{
						foreach (var supportedLanguage in overrides.Options["application.supportedLanguage"].Split(','))
							writer.WriteElementString("SupportedLanguage", supportedLanguage.Trim());
					}
					else
					{
						writer.WriteElementString("SupportedLanguage", "AmericanEnglish");
					}

					writer.WriteElementString("CrashReport",                    overrides.Options["application.crashreport"] ?? "Allow");

					writer.WriteStartElement("Rating");
					writer.WriteElementString("Organization",                   overrides.Options["application.rating.organization"] ?? "ESRB");
					writer.WriteElementString("Age",                            overrides.Options["application.rating.age"] ?? "0");
					writer.WriteEndElement();

					if (overrides.Options["application.accessibleUrlsFilePath"] != null)
						writer.WriteElementString("AccessibleUrlsFilePath", overrides.Options["application.accessibleUrlsFilePath"]); 

					writer.WriteElementString("CacheStorageSize",               overrides.Options["application.cacheStorageSize"] ?? "0x0000000000400000");
					writer.WriteElementString("CacheStorageJournalSize",        overrides.Options["application.cacheStorageJournalSize"] ?? "0x0000000000100000");
					writer.WriteElementString("UserAccountSaveDataSize",        overrides.Options["application.userAccountSaveDataSize"] ?? "0x0000000000400000");
					writer.WriteElementString("UserAccountSaveDataJournalSize", overrides.Options["application.userAccountSaveDataJournalSize"] ?? "0x0000000000100000");
					writer.WriteElementString("TemporaryStorageSize",           overrides.Options["application.temporaryStorageSize"] ?? "0x0000000004000000");
					writer.WriteElementString("LogoType",                       overrides.Options["application.logoType"] ?? "LicensedByNintendo");
					writer.WriteElementString("ReleaseVersion",                 overrides.Options["application.releaseVersion"] ?? "0");
					writer.WriteElementString("StartupUserAccount",             overrides.Options["application.startupUserAccount"] ?? "None");

					writer.WriteEndElement();

					writer.WriteEndElement();
				}
			}

			return fileName;
		}
	}
}
