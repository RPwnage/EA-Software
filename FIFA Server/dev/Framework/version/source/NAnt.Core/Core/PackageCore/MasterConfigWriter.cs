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
using System.IO;
using System.Linq;

using NAnt.Core.Logging;
using NAnt.Core.Util;

namespace NAnt.Core.PackageCore
{
	public class MasterConfigWriter
	{
		private readonly IXmlWriter m_Writer;
		private readonly MasterConfig m_Masterconfig;
		private readonly Project m_project;
		private readonly List<string> m_packagesFilter;

		// Default values are set to match what we want for -showmasterconfig
		// This class is also used by test_nant for merging masterconfigs and needs to be able to produce a valid masterconfig
		/// <summary>whether to evaluate grouptype exceptions</summary>
		public bool EvaluateGroupTypes = true;
		/// <summary>whether to evaluate exceptions and replace the default with the evaluated version</summary>
		public bool EvaluateMasterVersions = true;
		/// <summary>skips writing the fragments block</summary>
		public bool HideFragments = false;
		/// <summary>whether to evaluate global property conditions</summary>
		public bool ResolveGlobalProperties = true;
		/// <summary>resolves the normal and special roots by evaluating properties and converting to absolute paths</summary>
		public bool ResolveRoots = true;
		/// <summary>whether to show which fragment a particular part of the masterconfig file is coming from</summary>
		public bool ShowFragmentSources = true;
		/// <summary>An option to remove any metapackage attributes from packages</summary>
		public bool RemoveMetaPackages = false;

		public MasterConfigWriter(IXmlWriter writer, MasterConfig masterconfig, Project project, List<string> packagesFilter = null)
		{
			m_Writer = writer;
			m_Masterconfig = masterconfig;
			m_project = project;
			m_packagesFilter = packagesFilter?.Select(packageName => packageName.ToLowerInvariant()).ToList();
		}

		public void Write()
		{
			m_Writer.WriteStartDocument();
			m_Writer.WriteStartElement("project");

			WriteMasterVersions();
			WriteGlobalDefines();
			WriteGlobalProperties();
			WriteWarnings();
			WriteOnDemandRoot();
			WriteDevelopmentRoot();
			WriteLocalOnDemandRoot();
			WriteBuildRoot();
			WritePackageRoots();
			WriteNugetPackages();
			if (!HideFragments)
			{
				WriteFragments();
			}
			WriteConfig();

			m_Writer.WriteEndElement();
		}

		private void WriteMasterVersions()
		{
			if (m_Masterconfig.Packages.Count == 0)
			{
				return;
			}

			m_Writer.WriteStartElement("masterversions");

			List<string> groupTypesAdded = new List<string>();

			IOrderedEnumerable<IGrouping<string, MasterConfig.Package>> packagesByGroup = m_Masterconfig.Packages.Values
				.GroupBy(p => EvaluateGroupTypes ? p.EvaluateGroupType(m_project).Name : p.GroupType.Name)
				.OrderBy(g => g.Key == "masterversions" ? "_" : g.Key);
			foreach (IGrouping<string, MasterConfig.Package> group in packagesByGroup)
			{
				bool isNotMasterVersionGroup = !String.IsNullOrEmpty(group.Key) && group.Key != MasterConfig.GroupType.DefaultMasterVersionsGroupName;
				if (isNotMasterVersionGroup)
				{
					WriteGroupType(group.Key);
					groupTypesAdded.Add(group.Key);
				}

				System.Threading.Tasks.Parallel.ForEach(group, (package) =>
				{
					WritePackage(package);
				});

				if (isNotMasterVersionGroup)
				{
					m_Writer.WriteEndElement();
				}
			}

			// write any empty group types
			foreach (MasterConfig.GroupType group in m_Masterconfig.PackageGroups)
			{
				if (group.Name != "masterversions" && !groupTypesAdded.Contains(group.Name))
				{
					WriteGroupType(group.Name);
					m_Writer.WriteEndElement();
				}
			}

			m_Writer.WriteEndElement();
		}

		private void WriteGroupType(string groupTypeName)
		{
			MasterConfig.GroupType group = null;
			if (EvaluateGroupTypes)
			{
				group = m_Masterconfig.PackageGroups.FirstOrDefault(gt => gt.EvaluateGroupType(m_project)?.Name == groupTypeName);
			}
			else
			{
				group = m_Masterconfig.PackageGroups.FirstOrDefault(gt => gt.Name == groupTypeName);
			}

			if (group == null)
			{
				m_Writer.WriteStartElement("grouptype");
				m_Writer.WriteAttributeString("name", groupTypeName);
			}
			else
			{
				m_Writer.WriteStartElement("grouptype");
				m_Writer.WriteAttributeString("name", group.Name);
				if (group.AutoBuildClean.HasValue)
				{
					m_Writer.WriteAttributeString("autobuildclean", group.AutoBuildClean.ToString().ToLower());
				}
				if (group.BuildRoot != null)
				{
					m_Writer.WriteAttributeString("buildroot", group.BuildRoot);
				}
				if (!String.IsNullOrEmpty(group.OutputMapOptionSet))
				{
					m_Writer.WriteAttributeString("outputname-map-options", group.OutputMapOptionSet);
				}
				foreach (KeyValuePair<string, string> additionalAttribute in group.AdditionalAttributes)
				{
					m_Writer.WriteAttributeString(additionalAttribute.Key, additionalAttribute.Value);
				}
				foreach (MasterConfig.Exception<MasterConfig.GroupType> exception in group.Exceptions)
				{
					WriteGroupException(exception);
				}
			}
		}

		private void WriteGroupException(MasterConfig.Exception<MasterConfig.GroupType> exception)
		{
			m_Writer.WriteStartElement("exception");
			m_Writer.WriteAttributeString("propertyname", exception.PropertyName);
			foreach (MasterConfig.Condition<MasterConfig.GroupType> condition in exception)
			{
				m_Writer.WriteStartElement("condition");
				m_Writer.WriteAttributeString("value", condition.PropertyCompareValue);
				m_Writer.WriteAttributeString("name", condition.Value.Name);
				if (condition.Value.AutoBuildClean.HasValue)
				{
					m_Writer.WriteAttributeString("autobuildclean", condition.Value.AutoBuildClean.ToString().ToLower());
				}

				m_Writer.WriteEndElement();
			}
			m_Writer.WriteEndElement();
		}

		private void WritePackage(MasterConfig.Package package)
		{
			if (!(m_packagesFilter?.Contains(package.Name.ToLowerInvariant()) ?? true))
			{
				return; // This package has been chosen to be skipped based on the packagesFilter list passed in.
			}

			MasterConfig.IPackage packageSpec = EvaluateMasterVersions ? package.Exceptions.ResolveException(m_project, package) : package;
			lock (m_Writer)
			{
				m_Writer.WriteStartElement("package");
				m_Writer.WriteAttributeString("name", packageSpec.Name);
				m_Writer.WriteAttributeString("version", packageSpec.Version);

				if (EvaluateMasterVersions)
				{
					Release release = PackageMap.Instance.FindInstalledRelease(packageSpec);
					if (release != null)
					{
						m_Writer.WriteAttributeString("path", release.Path);
					}
				}

				if (ShowFragmentSources)
				{
					if (package.XmlPath != null)
					{
						m_Writer.WriteAttributeString("fragment", package.XmlPath);
					}
				}

				if (packageSpec.AutoBuildClean.HasValue)
				{
					m_Writer.WriteAttributeString("autobuildclean", packageSpec.AutoBuildClean.Value);
				}

				if (packageSpec.LocalOnDemand.HasValue)
				{
					m_Writer.WriteAttributeString("localondemand", package.LocalOnDemand.Value);
				}

				foreach (KeyValuePair<string, string> attr in packageSpec.AdditionalAttributes)
				{
					if (RemoveMetaPackages && attr.Key == "metapackage")
					{
						continue;
					}

					if (attr.Key != "path")
					{
						m_Writer.WriteAttributeString(attr.Key, attr.Value);
					}
				}

				foreach (MasterConfig.Exception<MasterConfig.IPackage> packageException in package.Exceptions)
				{
					WritePackageException(packageException);
				}

				m_Writer.WriteEndElement();
			}
		}

		private void WritePackageException(MasterConfig.Exception<MasterConfig.IPackage> exception)
		{
			m_Writer.WriteStartElement("exception");
			m_Writer.WriteAttributeString("propertyname", exception.PropertyName);
			foreach (MasterConfig.PackageCondition condition in exception)
			{
				m_Writer.WriteStartElement("condition");
				m_Writer.WriteAttributeString("value", condition.PropertyCompareValue);
				m_Writer.WriteAttributeString("version", condition.Version);

				if (condition.AutoBuildClean.HasValue)
				{
					m_Writer.WriteAttributeString("autobuildclean", condition.AutoBuildClean.Value);
				}

				if (condition.LocalOnDemand.HasValue)
				{
					m_Writer.WriteAttributeString("localondemand", condition.LocalOnDemand.Value);
				}

				foreach (KeyValuePair<string, string> attribute in condition.AdditionalAttributes)
				{
					if (RemoveMetaPackages && attribute.Key == "metapackage")
					{
						continue;
					}
					if (attribute.Key == "value" || 
						attribute.Key == "version" || 
						attribute.Key == "autobuildclean" ||
						attribute.Key == "localondemand")
					{
						// Already dealt with above.
						continue;
					}
					m_Writer.WriteAttributeString(attribute.Key, attribute.Value);
				}
				m_Writer.WriteEndElement();
			}
			m_Writer.WriteEndElement();
		}

		private void WriteOnDemandRoot()
		{
			m_Writer.WriteElementString("ondemand", m_Masterconfig.OnDemand.ToString().ToLower());
			m_Writer.WriteElementString("packageserverondemand", m_Masterconfig.PackageServerOnDemand.ToString().ToLower());
			string onDemandRoot = m_Masterconfig.GetOnDemandRoot();
			if (!String.IsNullOrEmpty(onDemandRoot))
			{
				if (ResolveRoots)
				{
					onDemandRoot = MakePathStringFromProperty(onDemandRoot);
				}
				m_Writer.WriteElementString("ondemandroot", onDemandRoot);
			}
		}

		private void WriteDevelopmentRoot()
		{
			string developmentRoot = m_Masterconfig.GetDevelopmentRoot();
			if (!String.IsNullOrEmpty(developmentRoot))
			{
				if (ResolveRoots)
				{
					string devRootEnv = Environment.GetEnvironmentVariable("FRAMEWORK_DEVELOPMENT_ROOT");
					if (devRootEnv.IsNullOrEmpty())
					{
						developmentRoot = MakePathStringFromProperty(developmentRoot);
					}
					else
					{
						developmentRoot = devRootEnv;
					}
				}
				m_Writer.WriteElementString("developmentroot", developmentRoot);
			}
		}

		private void WriteLocalOnDemandRoot()
		{
			MasterConfig.SpecialRootInfo localOndemandRoot = m_Masterconfig.GetLocalOnDemandRoot();
			if (localOndemandRoot != null)
			{
				if (ResolveRoots)
				{
					localOndemandRoot.Path = MakePathStringFromProperty(localOndemandRoot.Path);
				}
				m_Writer.WriteStartElement("localondemandroot");
				if (localOndemandRoot.ExtraAttributes != null)
				{
					foreach (KeyValuePair<string,string> kv in localOndemandRoot.ExtraAttributes)
					{
						m_Writer.WriteAttributeString(kv.Key, kv.Value);
					}
				}
				m_Writer.WriteString(localOndemandRoot.Path);
				m_Writer.WriteEndElement();
			}
		}

		private void WriteBuildRoot()
		{
			if (ResolveRoots && PackageMap.Instance.BuildRootWasInitialized)
			{
				m_Writer.WriteElementString("buildroot", PackageMap.Instance.BuildRoot);
			}
			else
			{
				string buildRoot = m_Masterconfig.BuildRoot.Path;
				if (!buildRoot.IsNullOrEmpty())
				{
					m_Writer.WriteStartElement("buildroot");
					if (m_Masterconfig.BuildRoot.AllowDefault == false)
					{
						m_Writer.WriteAttributeString("allow-default", "false");
					}
					m_Writer.WriteString(buildRoot);
					m_Writer.WriteEndElement();
				}
			}
		}

		private void WriteNugetPackages()
		{
			m_Writer.WriteStartElement("nugetpackages");
			if (m_Masterconfig.NuGetSources.Any())
			{
				m_Writer.WriteStartElement("sources");
				foreach (string source in m_Masterconfig.NuGetSources)
				{
					m_Writer.WriteElementString("source", source);
				}
				m_Writer.WriteEndElement();
			}
			foreach (MasterConfig.NuGetPackage nugetPackage in m_Masterconfig.NuGetPackages.Values)
			{
				m_Writer.WriteStartElement("nugetpackage");
				m_Writer.WriteAttributeString("name", nugetPackage.Name);
				m_Writer.WriteAttributeString("version", nugetPackage.Version);
				m_Writer.WriteEndElement();

				// NUGET-TODO: groups
			}
			m_Writer.WriteEndElement();
		}

		/// <summary>Writes a single global define</summary>
		private void WriteGlobalDefine(IGrouping<string, MasterConfig.GlobalDefine> gdef_group)
		{
			if (!string.IsNullOrEmpty(gdef_group.Key))
			{
				m_Writer.WriteStartElement("if");
				m_Writer.WriteAttributeString("packages", gdef_group.Key);
			}
			foreach (var gdef in gdef_group)
			{
				if (!String.IsNullOrEmpty(gdef.Condition))
				{
					m_Writer.WriteStartElement("if");
					m_Writer.WriteAttributeString("condition", gdef.Condition);
				}
				m_Writer.WriteStartElement("globaldefine");
				m_Writer.WriteAttributeString("name", gdef.Name);
				if (gdef.Value != null)
				{
					m_Writer.WriteAttributeString("value", gdef.Value);
				}
				m_Writer.WriteEndElement();
				if (!String.IsNullOrEmpty(gdef.Condition))
				{
					m_Writer.WriteEndElement();
				}
			}

			if (!string.IsNullOrEmpty(gdef_group.Key))
			{
				m_Writer.WriteEndElement();
			}
		}

		private void WriteGlobalDefines()
		{
			if (m_Masterconfig.GlobalDefines.Any())
			{
				var globalNativeDefines = m_Masterconfig.GlobalDefines.Where(x => !x.DotNet).GroupBy(gd => gd.Packages ?? String.Empty);
				if (globalNativeDefines.Count() > 0)
				{
					m_Writer.WriteStartElement("globaldefines");
					foreach (var gdef_group in globalNativeDefines)
					{
						WriteGlobalDefine(gdef_group);
					}
					m_Writer.WriteEndElement();
				}

				var globalDotNetDefines = m_Masterconfig.GlobalDefines.Where(x => x.DotNet).GroupBy(gd => gd.Packages ?? String.Empty);
				if (globalNativeDefines.Count() > 0)
				{
					m_Writer.WriteStartElement("globaldotnetdefines");
					foreach (var gdef_group in globalDotNetDefines)
					{
						WriteGlobalDefine(gdef_group);
					}
					m_Writer.WriteEndElement();
				}
			}
		}

		private static MasterConfig FindGlobalPropertyFragment(MasterConfig mc, MasterConfig.GlobalProperty prop)
		{
			foreach (MasterConfig f in mc.Fragments)
			{
				MasterConfig mm = FindGlobalPropertyFragment(f, prop);
				if (mm != null)
					return mm;
			}

			if (mc.GlobalProperties.Contains(prop))
				return mc;

			return null;
		}

		private void WriteGlobalProperties()
		{
			if (m_Masterconfig.GlobalProperties.Any())
			{
				m_Writer.WriteStartElement("globalproperties");
				if (ResolveGlobalProperties)
				{
					foreach (var gprop in Project.GlobalProperties.EvaluateExceptions(m_project))
					{
						MasterConfig.GlobalProperty prop = m_Masterconfig.GlobalProperties.Find(gp => gp.Name == gprop.Name);
						if (prop != null)
						{
							m_Writer.WriteStartElement("globalproperty");
							m_Writer.WriteAttributeString("name", gprop.Name);

							if (ShowFragmentSources)
							{
								MasterConfig fragment = FindGlobalPropertyFragment(m_Masterconfig, prop);
								if (fragment != null)
								{
									m_Writer.WriteAttributeString("fragment", fragment.MasterconfigFile);
								}
							}

							m_Writer.WriteString(m_project.Properties[gprop.Name]);
							m_Writer.WriteEndElement();
						}
					}
				}
				else
				{
					foreach (var gprop in m_Masterconfig.GlobalProperties)
					{
						if (gprop.UseXmlElement)
						{
							m_Writer.WriteStartElement("globalproperty");
							m_Writer.WriteAttributeString("name", gprop.Name);
							if (!string.IsNullOrEmpty(gprop.Condition))
							{
								m_Writer.WriteAttributeString("condition", gprop.Condition);
							}
							m_Writer.WriteString(gprop.Value);
							m_Writer.WriteEndElement();
							m_Writer.WriteString(Environment.NewLine);
						}
						else
						{
							if (gprop.Condition != null)
							{
								m_Writer.WriteStartElement("if");
								m_Writer.WriteAttributeString("condition", gprop.Condition);
							}
							if (gprop.Value != null)
							{
								string value = gprop.Value;
								if (gprop.Quoted)
								{
									value = value.Quote();
								}
								m_Writer.WriteString(gprop.Name + "=" + value);
							}
							else
							{
								m_Writer.WriteString(gprop.Name);
							}
							if (gprop.Condition != null)
							{
								m_Writer.WriteEndElement();
							}
							m_Writer.WriteString(Environment.NewLine);
						}
					}
				}
				m_Writer.WriteEndElement();
			}
		}

		private void WriteWarnings()
		{
			if (m_Masterconfig.Warnings.Any())
			{
				m_Writer.WriteStartElement("warnings");
				foreach (KeyValuePair<Log.WarningId, MasterConfig.WarningDefinition> warning in m_Masterconfig.Warnings.OrderBy(kvp => kvp.Key))
				{
					m_Writer.WriteStartElement("warning");
					m_Writer.WriteAttributeString("id", warning.Key.ToString("d"));
					if (warning.Value.Enabled.HasValue)
					{
						m_Writer.WriteAttributeString("enabled", warning.Value.Enabled.Value.ToString());
					}
					if (warning.Value.AsError.HasValue)
					{
						m_Writer.WriteAttributeString("as-error", warning.Value.AsError.Value.ToString());
					}
					m_Writer.WriteEndElement();
				}
				m_Writer.WriteEndElement();
			}
		}

		private void WriteFragments()
		{
			if (!m_Masterconfig.FragmentDefinitions.IsNullOrEmpty())
			{
				m_Writer.WriteStartElement("fragments");

				foreach (var fragment in m_Masterconfig.FragmentDefinitions)
				{
					m_Writer.WriteStartElement("include");
					if (!fragment.MasterConfigPath.IsNullOrEmpty())
					{
						m_Writer.WriteAttributeString("name", fragment.MasterConfigPath);
					}
					if (!fragment.LoadCondition.IsNullOrEmpty())
					{
						m_Writer.WriteAttributeString("if", fragment.LoadCondition);
					}
					foreach (MasterConfig.Exception<string> exception in fragment.Exceptions)
					{
						m_Writer.WriteStartElement("exception");
						m_Writer.WriteAttributeString("propertyname", exception.PropertyName);
						foreach (MasterConfig.Condition<string> condition in exception)
						{
							m_Writer.WriteStartElement("condition");
							m_Writer.WriteAttributeString("value", condition.PropertyCompareValue);
							m_Writer.WriteAttributeString("name", condition.Value);
							m_Writer.WriteEndElement(); 
						}
						m_Writer.WriteEndElement();
					}
					m_Writer.WriteEndElement();
				}
				m_Writer.WriteEndElement();
			}
		}

		private void WritePackageRoots()
		{
			if (!m_Masterconfig.PackageRoots.IsNullOrEmpty())
			{
				m_Writer.WriteStartElement("packageroots");

				foreach (var root in m_Masterconfig.PackageRoots)
				{
					string path = root.EvaluatePackageRoot(m_project);
					if (ResolveRoots)
					{
						path = MakePathStringFromProperty(path);
					}

					m_Writer.WriteStartElement("packageroot");
					if (root.MinLevel != null)
					{
						m_Writer.WriteAttributeString("minlevel", root.MinLevel.ToString());
					}
					if (root.MaxLevel != null)
					{
						m_Writer.WriteAttributeString("maxlevel", root.MaxLevel.ToString());
					}

					m_Writer.WriteString(path);
					m_Writer.WriteEndElement();
				}
				m_Writer.WriteEndElement();
			}
		}

		private void WriteConfig()
		{
			if (m_Masterconfig.Config == MasterConfig.ConfigInfo.DefaultConfigInfo)
			{
				return;
			}

			m_Writer.WriteStartElement("config");
			{
				if (m_Masterconfig.Config.Package.IsNullOrEmpty())
				{
					m_Writer.WriteAttributeString("package", "eaconfig");
				}
				else
				{
					m_Writer.WriteAttributeString("package", m_Masterconfig.Config.Package);
				}
				if (m_Masterconfig.Config != MasterConfig.ConfigInfo.DefaultConfigInfo)
				{
					foreach (string cfgHost in MasterConfig.ConfigInfo.SupportedConfigHosts)
					{
						string attribPosfix = "";
						switch (cfgHost)
                        {
							case "osx":
								attribPosfix = "-osx";
								break;
							case "unix":
								attribPosfix = "-unix";
								break;
							default:
								break;
                        }

						if (!String.IsNullOrEmpty(m_Masterconfig.Config.DefaultByHost[cfgHost]))
						{
							m_Writer.WriteAttributeString("default" + attribPosfix, m_Masterconfig.Config.DefaultByHost[cfgHost]);
						}
						if (!m_Masterconfig.Config.IncludesByHost[cfgHost].IsNullOrEmpty())
						{
							m_Writer.WriteAttributeString("includes" + attribPosfix, m_Masterconfig.Config.IncludesByHost[cfgHost].ToString(" "));
						}
						if (!m_Masterconfig.Config.ExcludesByHost[cfgHost].IsNullOrEmpty())
						{
							m_Writer.WriteAttributeString("excludes" + attribPosfix, m_Masterconfig.Config.ExcludesByHost[cfgHost].ToString(" "));
						}
					}
					foreach (MasterConfig.ConfigExtension extension in m_Masterconfig.Config.Extensions)
					{
						m_Writer.WriteStartElement("extension");
						if (!extension.IfClause.IsNullOrEmpty())
						{
							m_Writer.WriteAttributeString("if", extension.IfClause);
						}
						m_Writer.WriteString(extension.Name);
						m_Writer.WriteEndElement();
					}
				}
			}
			m_Writer.WriteEndElement();
		}

		private string MakePathStringFromProperty(string dir)
		{
			string path = m_project.ExpandProperties(dir);
			if (!String.IsNullOrWhiteSpace(m_Masterconfig.MasterconfigFile))
			{
				return PathString.MakeCombinedAndNormalized(Path.GetFullPath(Path.GetDirectoryName(m_Masterconfig.MasterconfigFile)), path).Path;
			}
			return PathString.MakeNormalized(dir).Path;
		}
	}
}
