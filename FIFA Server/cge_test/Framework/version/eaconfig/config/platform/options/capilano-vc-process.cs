// (c) Electronic Arts. All rights reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using System.Xml.Linq;

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Attributes;
using NAnt.Core.Functions;
using NAnt.Core.Logging;
using NAnt.Core.Writers;
using NAnt.Core.Events;
using EA.FrameworkTasks.Model;

using EA.Eaconfig;
using EA.Eaconfig.Core;
using EA.Eaconfig.Build;
using EA.Eaconfig.Modules;
using EA.Eaconfig.Modules.Tools;

namespace EA.Eaconfig
{
	[TaskName("capilano-vc-preprocess", XmlSchema = false)]
	public class Capilano_VC_Preprocess : AbstractModuleProcessorTask
	{
		protected override void ExecuteTask()
		{
			base.ExecuteTask();
			SetAssetFilesDeploymentOptions();
		}

		protected virtual void SetAssetFilesDeploymentOptions()
		{
			if (Module.DeployAssets)
			{
				var custombuildfiles = PropGroupFileSet(".custombuildfiles." + Module.Configuration.System) ?? PropGroupFileSet(".custombuildfiles");
				if (custombuildfiles == null)
				{
					custombuildfiles = new FileSet();
					custombuildfiles.BaseDirectory = Module.Assets.BaseDirectory;
					Project.NamedFileSets.Add(PropGroupName(".custombuildfiles"), custombuildfiles);
				}

				string deployOptionSetName = DeploymentActionOptionsetName;
				string assetTargetPath = Project.GetPropertyValue(PropGroupName(".asset-configbuilddir"));
				string assetTargetRelPath = null;
				if (!String.IsNullOrEmpty(assetTargetPath))
				{
					// We want to normalize the path first (but not turn it into a full path).
					assetTargetPath = PathString.MakeNormalized(assetTargetPath, PathString.PathParam.NormalizeOnly).Path;
					if (Path.IsPathRooted(assetTargetPath))
					{
						PathString moduleOutDir = PathString.MakeNormalized(BuildFunctions.GetModuleOutputDir(Project, "bin", Project.GetPropertyValue("package.name")));
						assetTargetRelPath = PathUtil.RelativePath(assetTargetPath, moduleOutDir.Path);
					}
					else
					{
						assetTargetRelPath = assetTargetPath;
					}
				}
				OptionSet fileOptions = null;
				if (!String.IsNullOrEmpty(assetTargetRelPath) && Project.NamedOptionSets.TryGetValue(DeploymentActionOptionsetName, out fileOptions))
				{
					if (fileOptions.Options.ContainsKey("copy.command") && fileOptions.Options.ContainsKey("outputs"))
					{
						string adjustedRelPath = String.Format("{0}\\%filereldir%",assetTargetRelPath);
						string newOptionSetName = PropGroupName(".custombuildfiles.targetreldir-fixed-deploycontent-option");
						deployOptionSetName = newOptionSetName;
						OptionSet newOptionset = new OptionSet(fileOptions);
						newOptionset.Options["copy.command"] = fileOptions.Options["copy.command"].ReplaceDir("%filereldir%", adjustedRelPath);
						newOptionset.Options["outputs"] = fileOptions.Options["outputs"].ReplaceDir("%filereldir%", adjustedRelPath);
						Project.NamedOptionSets.Add(newOptionSetName, newOptionset);
					}
				}

				custombuildfiles.Include(Module.Assets, deployOptionSetName);
			}
		}

		protected virtual string DeploymentActionOptionsetName
		{
			get
			{
				return "config-options-xboxdeploymentcontent";
			}
		}
	}

	[TaskName("capilano-vc-postprocess", XmlSchema = false)]
	public class Capilano_VC_Postprocess : VC_Common_PostprocessOptions
	{
		protected override void ExecuteTask()
		{
			base.ExecuteTask();

			AddExtensionSDKPaths(Module as Module_Native);
		}

		protected virtual void AddExtensionSDKPaths(Module_Native module)
		{
			if (module != null && module.Package.Project != null)
			{
				var references = GetSdkReferences(module);
				if (references != null)
				{
					// Starting with November 2015 XDK, Extensions files are no longer in the XDK install folder.  It is now located in
					// C:\Program Files (x86)\Microsoft SDKs\Durango.<xdk_edition>\v8.0
					// The old and new non-proxy has the property package.CapilanoSDK.extensionsdkdir to point to the extension path.
					// The new November 2015 proxy package has been updated to have that property as well. But the old proxy
					// package doesn't.  So we need to test for that.
					string extensionpath = "";
					if (Project.Properties.Contains("package.GDK.extensionsdkdir"))
					{
						extensionpath = PathNormalizer.Normalize(Project.Properties["package.GDK.extensionsdkdir"], false);
						if (!Directory.Exists(extensionpath))
						{
							Log.Warning.WriteLine("Module '{0}' contains Extension SDK References but extension directory '{1}' specified in property package.GDK.extensionsdkdir does not exist", Module.ModuleIdentityString(), extensionpath);
							return;
						}
					}
					else if (Project.Properties.Contains("package.CapilanoSDK.extensionsdkdir"))
					{
						extensionpath = PathNormalizer.Normalize(Project.Properties["package.CapilanoSDK.extensionsdkdir"], false);
						if (!Directory.Exists(extensionpath))
						{
							Log.Warning.WriteLine("Module '{0}' contains Extension SDK References but extension directory '{1}' specified in property package.CapilanoSDK.extensionsdkdir does not exist", Module.ModuleIdentityString(), extensionpath);
							return;
						}
					}
					else
					{
						// For old behaviour for older SDK package.
						string extensionRoot = PathNormalizer.Normalize(Project.Properties["package.CapilanoSDK.kitdir"], false).TrimRightSlash();
						extensionpath = Path.Combine(extensionRoot, "Xbox.xdk", "v8.0", "ExtensionSDKs");
						if (!Directory.Exists(extensionpath))
						{
							extensionpath = Path.Combine(extensionRoot, "Extensions");

							if (!Directory.Exists(extensionpath))
							{
								Log.Warning.WriteLine("Module '{0}' contains Extension SDK References but directory with extensions does not exist", Module.ModuleIdentityString());
								return;
							}
						}
					}

					foreach (var sdkref in references)
					{
						string sdkname;
						string sdkversion;

						bool res = GetSdkNameAndVersion(sdkref, out sdkname, out sdkversion);

						if (sdkname == "XboxDevKitExtensions")
						{
							var includedir = String.Empty;
							var libdir = String.Empty;
							if (res)
							{
								includedir = Path.Combine(extensionpath, String.Format(@"XboxDevKitExtensions\{0}\DesignTime\CommonConfiguration\Neutral\Include", sdkversion));
								libdir = Path.Combine(extensionpath, String.Format(@"XboxDevKitExtensions\{0}\DesignTime\CommonConfiguration\Neutral\Lib", sdkversion));
							}
							else
							{
								includedir = Path.Combine(extensionpath, @"XboxDevKitExtensions\8.0\DesignTime\CommonConfiguration\Neutral\Include");
								libdir = Path.Combine(extensionpath, @"XboxDevKitExtensions\8.0\DesignTime\CommonConfiguration\Neutral\Lib");
							}
							if (module.Cc != null)
							{
								module.Cc.IncludeDirs.Add(PathString.MakeNormalized(includedir));
								module.Cc.UsingDirs.Add(PathString.MakeNormalized(libdir));

								foreach (var fileitem in module.Cc.SourceFiles.FileItems)
								{
									var compiler = fileitem.GetTool() as CcCompiler;
									if (compiler != null)
									{
										compiler.IncludeDirs.Add(PathString.MakeNormalized(includedir));
										compiler.UsingDirs.Add(PathString.MakeNormalized(libdir));
									}
								}
							}
							
							if (module.Link != null)
							{
								module.Link.LibraryDirs.Add(PathString.MakeNormalized(libdir));
								module.Link.Libraries.Include("devkitextensions.lib", asIs: true);
							}
						}

						if (sdkname == "Xbox.Services.API.Cpp")
						{
							var includedir = String.Empty;
							var libdir = String.Empty;
							var libMsvcVersion = "141";
							if (Project.Properties["config-vs-version"].StrCompareVersions("15.0") < 0)
								libMsvcVersion = "140";

							var libVariant = "Release";
							if (Project.NamedOptionSets.ContainsKey("config-options-common"))
							{
								OptionSet cmnOptSet = Project.NamedOptionSets["config-options-common"];
								string debugFlagOpt = cmnOptSet.GetOptionOrDefault("debugflags", "on").ToLowerInvariant();
								if (debugFlagOpt == "on")
								{
									libVariant = "Debug";
								}
							}

							if (res)
							{
								includedir = Path.Combine(extensionpath, String.Format(@"Xbox.Services.API.Cpp\{0}\DesignTime\CommonConfiguration\Neutral\Include", sdkversion));
								libdir = Path.Combine(extensionpath, String.Format(@"Xbox.Services.API.Cpp\{0}\DesignTime\CommonConfiguration\Neutral\Lib\{1}\v{2}", sdkversion, libVariant, libMsvcVersion));
							}
							else
							{
								includedir = Path.Combine(extensionpath, @"Xbox.Services.API.Cpp\8.0\DesignTime\CommonConfiguration\Neutral\Include");
								libdir = Path.Combine(extensionpath, @"Xbox.Services.API.Cpp\8.0\DesignTime\CommonConfiguration\Neutral\Lib\{0}\v{1}", libVariant, libMsvcVersion);
							}
							if (module.Cc != null)
							{
								string [] defines = { "XSAPI_CPP=1", "_NO_ASYNCRTIMP", "_NO_PPLXIMP", "_NO_XSAPIIMP", "XBL_API_EXPORT" };
								module.Cc.IncludeDirs.Add(PathString.MakeNormalized(includedir));
								module.Cc.UsingDirs.Add(PathString.MakeNormalized(libdir));
								foreach (string def in defines)
									module.Cc.Defines.Add(def);

								foreach (var fileitem in module.Cc.SourceFiles.FileItems)
								{
									var compiler = fileitem.GetTool() as CcCompiler;
									if (compiler != null)
									{
										compiler.IncludeDirs.Add(PathString.MakeNormalized(includedir));
										compiler.UsingDirs.Add(PathString.MakeNormalized(libdir));
										foreach (string def in defines)
											compiler.Defines.Add(def);
									}
								}
							}

							if (module.Link != null)
							{
								module.Link.LibraryDirs.Add(PathString.MakeNormalized(libdir));
								module.Link.Libraries.Include(String.Format("libHttpClient.{0}.XDK.C.lib", libMsvcVersion), asIs: true);
								module.Link.Libraries.Include(String.Format("casablanca.{0}.xbox.lib", libMsvcVersion), asIs: true);
								module.Link.Libraries.Include(String.Format("Microsoft.Xbox.Services.{0}.XDK.Ship.Cpp.lib", libMsvcVersion), asIs: true);
							}
						}

						// extensions reference folders in the SDK extension dir, each extension contains an SDKManifest.xml that describes the assembly references
						// required for that extension - these files always seems to be located at References\CommonConfiguration\Neutral within each SDK folder
						string extensionSDKDir = Path.Combine(extensionpath, sdkname, sdkversion);
						string extensionSDKManifestPath = Path.Combine(extensionSDKDir, "SDKManifest.xml");
						if (!File.Exists(extensionSDKManifestPath))
						{
							throw new BuildException(String.Format("Cannot find requested SDK extension '{0}' at expected path: {1}. This error occured when processing module '{2}' in package '{3}' ({4}).", sdkref, extensionSDKManifestPath, module.Name, module.Package.Name, module.Package.Project.CurrentScriptFile));
						}

						XDocument manifestDoc = XDocument.Load(extensionSDKManifestPath);
						foreach (XElement fileElement in manifestDoc.Root.Descendants("File"))
						{
							string referencePath = Path.Combine(extensionSDKDir, "References", "CommonConfiguration", "Neutral", fileElement.Attribute("Reference").Value);
							if (module.Cc != null)
							{
								module.Cc.Assemblies.Include(referencePath, optionSetName: "sdk-reference");

								foreach (FileItem compileItem in module.Cc.SourceFiles.FileItems)
								{
									CcCompiler compiler = compileItem.GetTool() as CcCompiler;
									if (compiler != null)
									{
										compiler.Assemblies.Include(referencePath, optionSetName: "sdk-reference");
									}
								}
							}
						}
					}
				}
			}
		}

		private IList<string> GetSdkReferences(Module_Native module)
		{
			List<string> references = null;
			if (module != null && module.Package.Project != null)
			{
				var sdkrefprop = module.PropGroupValue("sdkreferences." + module.Configuration.System, module.PropGroupValue("sdkreferences")).TrimWhiteSpace();

				var globalsdkrefprop = (module.Package.Project.GetPropertyValue("package.sdkreferences." + module.Configuration.System) ?? module.Package.Project.GetPropertyValue("package.sdkreferences")).TrimWhiteSpace();

				if (!(String.IsNullOrEmpty(sdkrefprop) && String.IsNullOrEmpty(globalsdkrefprop)))
				{

					references = new List<string>(sdkrefprop.LinesToArray());
					references.AddRange(globalsdkrefprop.LinesToArray());
				}
			}

			return references;
		}

		private bool GetSdkNameAndVersion(string sdkref, out string sdkname, out string sdkversion)
		{
			bool ret = false;
			sdkname = sdkref;
			sdkversion = String.Empty;

			var tmp = sdkref.ToArray(",");
			if (tmp.Count > 0)
			{
				sdkname = tmp[0];
			}
			if (tmp.Count > 1)
			{
				tmp = tmp[1].ToArray("=");
				if (tmp.Count > 1)
				{
					sdkversion = tmp[1];
					ret = true;
				}
			}
			return ret;
		}
	}

	
	[TaskName("capilano-vc-set-pull-deployment", XmlSchema = false)]
	public class CapilanoVcSetPullDeployment : AbstractModuleProcessorTask
	{
		protected override void ExecuteTask()
		{
			base.ExecuteTask();
			SetPullMappingOptions();
		}

		protected virtual void SetPullMappingOptions()
		{
			if (Properties.GetBooleanPropertyOrDefault("package.consoledeployment", true))
			{
				var deploymodeStr = PropGroupValue("deploy-mode");

				if (!String.IsNullOrEmpty(deploymodeStr))
				{
					if (DeployModeType.Pull == ConvertUtil.ToEnum<DeployModeType>(deploymodeStr))
					{
						OptionSet pullmapping;

						if (Project.NamedOptionSets.TryGetValue(PropGroupName("pullmapping"), out pullmapping))
						{
							var pullmappingfile = PropGroupValue("pullmappingfile").TrimWhiteSpace();

							if (pullmapping != null && pullmapping.Options.Count > 0)
							{
								if (String.IsNullOrEmpty(pullmappingfile))
								{
									// Add default mapping file
									pullmappingfile = Path.Combine(Module.IntermediateDir.Path, "capilano-pullmappings.xml");
									Project.Properties[PropGroupName("pullmappingfile")] = pullmappingfile;
								}

								GeneratePullMappingFile(PathString.MakeNormalized(pullmappingfile, PathString.PathParam.NormalizeOnly), pullmapping);
							}
						}
					}
				}
			}
		}


		protected virtual void GeneratePullMappingFile(PathString mappingfile, OptionSet filemappings)
		{
			using (var writer = new XmlWriter(new XmlFormat(identChar: ' ', identation: 2, newLineOnAttributes: false, encoding: new UTF8Encoding(false))))
			{
				writer.FileName = mappingfile.Path;

				writer.WriteStartDocument();
				writer.WriteStartElement("mappings");
				foreach (var pair in filemappings.Options)
				{
					WriteOneFileMapping(writer, pair.Key.TrimWhiteSpace(), pair.Value.TrimWhiteSpace());
				}
				writer.WriteEndElement();
			}

		}

		protected virtual void WriteOneFileMapping(IXmlWriter writer, string type, string mapping)
		{
			if (type.StartsWith("p", StringComparison.OrdinalIgnoreCase))
			{
				var parsed = ParseMapingValue(mapping);
				writer.WriteStartElement("path");
				foreach (var pair in parsed)
				{
					writer.WriteNonEmptyAttributeString(pair.Key, pair.Value);
				}
				writer.WriteEndElement();
			}
			else if (type.StartsWith("e", StringComparison.OrdinalIgnoreCase))
			{
				var parsed = ParseMapingValue(mapping);
				writer.WriteStartElement("excludepath");
				foreach (var pair in parsed)
				{
					if (pair.Key == "source")
					{
						writer.WriteNonEmptyAttributeString(pair.Key, pair.Value);
					}
					else
					{
						Log.Warning.WriteLine("Capilano Deployment mapping data:  \"excludepath\" only supports attribute \"source\". Attribute \"{0}\" ignored.", pair.Key);
					}
				}
				writer.WriteEndElement();
			}
			else
			{
				var msg = String.Format("Capilano Deployment mapping data contains invalid option: '{0}'. Valid options are \"path\" and \"excludepath\".", type);
				throw new BuildException(msg);
			}
		}

		private IEnumerable<KeyValuePair<string,string>> ParseMapingValue(string val)
		{
			var parsed = new List<KeyValuePair<string,string>>();
		   foreach(var line in val.LinesToArray())
		   {
			   bool found = false;
			   foreach (var attr in attributes)
			   {
				   if (line.StartsWith(attr, StringComparison.OrdinalIgnoreCase))
				   {
					   parsed.Add(new KeyValuePair<string, string>(attr.Substring(0, attr.Length - 1), line.Substring(attr.Length).TrimWhiteSpace()));
					   found = true;
					   break;
				   }
			   }
			   if(!found)
			   {
				   var msg = String.Format("Capilano Deployment mapping data contain invalid line: '{0}'. Each line must start with one of the following {1}.", line, attributes.ToString(", ", a => a.Quote()));
				   throw new BuildException(msg);
			   }
		   }
		   return parsed;
		}

		protected virtual string DeploymentActionOptionsetName
		{
			get
			{
				return "config-options-xboxdeploymentcontent";
			}
		}

		private static readonly string[] attributes = {"source:",  "target:", "include:", "exclude:"};
		private enum DeployModeType { Default, Push, Pull, External, RegisterNetworkShare };
	}
	

}
