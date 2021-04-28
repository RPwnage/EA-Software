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
using System.Linq;
using System.Text;
using System.Xml;

using NAnt.Core.Logging;
using NAnt.Core.Util;

namespace NAnt.Core.PackageCore
{
	public sealed partial class MasterConfig
	{
		private sealed class MasterConfigLoader
		{
			private MasterConfig m_masterConfig = null;
			private readonly Log m_log;
			private readonly Dictionary<string, NuGetPackage> m_masterVersionsToNugetAutoConversions = new Dictionary<string, NuGetPackage>(StringComparer.OrdinalIgnoreCase);
			private readonly List<string> m_automaticNugetSourceConversions = new List<string>();

			internal MasterConfigLoader(Log log)
			{
				m_log = log;
			}

			internal MasterConfig Load(string file)
			{
				if (String.IsNullOrWhiteSpace(file))
				{
					return UndefinedMasterconfig;
				}

				LineInfoDocument doc = LineInfoDocument.Load(file);
				if (doc.DocumentElement.Name != "project")
				{
					XmlUtil.Warning(m_log, doc.DocumentElement, String.Format("Error in masterconfig file: Expected root element is 'project', found root '{0}'.", doc.DocumentElement.Name));
				}

				m_masterConfig = new MasterConfig(file);

				foreach (XmlNode childNode in doc.DocumentElement.ChildNodes)
				{
					if (childNode.NodeType == XmlNodeType.Element && childNode.NamespaceURI.Equals(doc.DocumentElement.NamespaceURI))
					{
						switch (childNode.Name)
						{
							case "masterversions":
								ParseMasterVersions(childNode, file);
								break;
							case "packageroots":
								ParsePackageRoots(childNode);
								break;
							case "config":
								ParseConfig(childNode);
								break;
							case "buildroot":
								ParseBuildRoots(childNode);
								break;
							case "ondemand":
								if (m_masterConfig.m_onDemand != null)
								{
									XmlUtil.Error(childNode, "Error in masterconfig file: Element 'ondemand' is defined more than once in masterconfig file.");
								}
								m_masterConfig.m_onDemand = ConvertUtil.ToNullableBoolean(StringUtil.Trim(childNode.GetXmlNodeText()));
								break;
							case "packageserverondemand":
								if (m_masterConfig.m_packageServerOnDemand != null)
								{
									XmlUtil.Error(childNode, "Error in masterconfig file: Element 'packageserverondemand' is defined more than once in masterconfig file.");
								}
								m_masterConfig.m_packageServerOnDemand = ConvertUtil.ToNullableBoolean(StringUtil.Trim(childNode.GetXmlNodeText()));
								break;
							case "localondemand":
								if (m_masterConfig.m_localOnDemand != null)
								{
									XmlUtil.Error(childNode, "Error in masterconfig file: Element 'localondemand' is defined more than once in masterconfig file.");
								}
								m_masterConfig.m_localOnDemand = ConvertUtil.ToNullableBoolean(StringUtil.Trim(childNode.GetXmlNodeText()));
								break;
							case "ondemandroot":
								ParseSpecialRoot(childNode, ref m_masterConfig.m_onDemandRoots);
								break;
							case "developmentroot":
								ParseSpecialRoot(childNode, ref m_masterConfig.m_developmentRoots);
								break;
							case "localondemandroot":
								ParseSpecialRoot(childNode, ref m_masterConfig.m_localOnDemandRoots);
								break;
							case "globalproperties":
								ParseGlobalProperties(childNode, m_masterConfig.GlobalProperties);
								break;
							case "fragments":
								ParseFragments(childNode);
								break;
							case "globaldefines":
								ParseGlobalDefines(childNode, m_masterConfig.GlobalDefines, false);
								break;
							case "globaldotnetdefines":
								ParseGlobalDefines(childNode, m_masterConfig.GlobalDefines, true);
								break;
							case "warnings":
								ParseWarnings(childNode, m_masterConfig.Warnings);
								break;
							case "deprecations":
								ParseDeprecations(childNode, m_masterConfig.Deprecations);
								break;
							case "nugetpackages":
								ParseNuGetPackages(childNode, m_masterConfig);
								break;
						default:
								// allow extra elements in masterconfig file
								break;
						}
					}
				}

				ProcessNugetVersions();

				return m_masterConfig;
			}

			private void ParseWarnings(XmlNode el, Dictionary<Log.WarningId, WarningDefinition> warnings)
			{
				foreach (XmlNode childNode in el.ChildNodes)
				{
					if (childNode.NodeType == XmlNodeType.Element && childNode.NamespaceURI.Equals(el.NamespaceURI))
					{
						switch (childNode.Name)
						{
							case "warning":
								XmlUtil.ValidateAttributes(childNode, "id", "enabled", "as-error");
								string id = XmlUtil.GetRequiredAttribute(childNode, "id");
								if (Enum.TryParse(id, out Log.WarningId warningId))
								{
									bool? enabled = null;
									{
										string enabledStr = XmlUtil.GetOptionalAttribute(childNode, "enabled");
										if (enabledStr != null)
										{
											if (Boolean.TryParse(enabledStr, out bool isEnabled))
											{
												enabled = isEnabled;
											}
											else
											{
												XmlUtil.Error(childNode, $"Could not interpret {enabledStr} as a boolean value.");
											}
										}
									}
									bool? asError = null;
									{
										string asErrorStr = XmlUtil.GetOptionalAttribute(childNode, "as-error");
										if (asErrorStr != null)
										{
											if (Boolean.TryParse(asErrorStr, out bool isAsError))
											{
												asError = isAsError;
											}
											else
											{
												XmlUtil.Error(childNode, $"Could not interpret {asErrorStr} as a boolean value.");
											}
										}
									}
									Location location = Location.GetLocationFromNode(el);
									if (enabled == null && asError == null)
									{
										XmlUtil.Warning(m_log, childNode, "Nothing set for warning, setting ignored.");
									}
									else
									{
										WarningDefinition warningDefinition = new WarningDefinition(enabled, asError, location);
										AddOrMergeWarning(m_log, warnings, warningId, warningDefinition);
									}
								}
								else
								{
									XmlUtil.Warning(m_log, childNode, $"Unrecognised warning code {id} - this warning setting will be ignored.");
								}
								break;
							default:
								XmlUtil.Error(el, "Error in masterconfig file: 'warnings' element can only have 'warning' child nodes, found: '" + childNode.Name + "'.");
								break;
						}
					}
				}
			}

			private void ParseDeprecations(XmlNode el, Dictionary<Log.DeprecationId, DeprecationDefinition> deprecations)
			{
				foreach (XmlNode childNode in el.ChildNodes)
				{
					if (childNode.NodeType == XmlNodeType.Element && childNode.NamespaceURI.Equals(el.NamespaceURI))
					{
						switch (childNode.Name)
						{
							case "deprecation":
								XmlUtil.ValidateAttributes(childNode, "id", "enabled");
								string id = XmlUtil.GetRequiredAttribute(childNode, "id");
								if (!Enum.TryParse(id, out Log.DeprecationId deprecationId))
								{
									XmlUtil.Warning(m_log, childNode, $"Unrecognised deprecation code {id} - this deprecation setting will be ignored.");
								}
								string enabledStr = XmlUtil.GetRequiredAttribute(childNode, "enabled");
								bool enabled = true;    // Enable is set to true by default.
								if (Boolean.TryParse(enabledStr, out bool isEnabled))
								{
									enabled = isEnabled;
								}
								else
								{
									XmlUtil.Error(childNode, $"Could not interpret {enabledStr} as a boolean value.");
								}
								Location location = Location.GetLocationFromNode(el);
								DeprecationDefinition deprecationDefinition = new DeprecationDefinition(enabled, location);
								AddOrMergeDeprecation(m_log, deprecations, deprecationId, deprecationDefinition);
								break;
							default:
								XmlUtil.Error(el, "Error in masterconfig file: 'deprecations' element can only have 'deprecation' child nodes, found: '" + childNode.Name + "'.");
								break;
						}
					}
				}
			}

			private void ParseSpecialRoot(XmlNode el, ref List<SpecialRootInfo> rootList)
			{
				string ifClause = XmlUtil.GetOptionalAttribute(el, "if", null);
				Dictionary<string, string> extraAttributes = new Dictionary<string, string>();
				foreach (XmlAttribute attr in el.Attributes)
				{
					if (attr.Name == "if")
						continue;
					extraAttributes.Add(attr.Name, attr.Value);
				}
				IEnumerable<SpecialRootInfo> defaultAdded = rootList.Where(r => String.IsNullOrEmpty(r.IfClause));
				if (ifClause == null && defaultAdded.Any())
				{
					XmlUtil.Error(el,
						String.Format("Error in masterconfig file: Element '{0}' (with no condition) is defined more than once in masterconfig file.  We found:\n  {1}\n  {2}",
							el.Name,
							defaultAdded.First().Path,
							StringUtil.Trim(el.GetXmlNodeText()))
					);
				}
				rootList.Add(new SpecialRootInfo(ifClause, StringUtil.Trim(el.GetXmlNodeText()), extraAttributes));
			}

			internal MasterConfig[] LoadFragments(Project project, string includePattern, string baseDir)
			{
				// convert file path to full path using fileset for validation
				FileSet fragmentFileSet = new FileSet();
				fragmentFileSet.BaseDirectory = baseDir;
				fragmentFileSet.Include(includePattern);
				FileItemList items = fragmentFileSet.FileItems;

				List<MasterConfig> masterConfigs = new List<MasterConfig>();
				foreach (FileItem item in items)
				{
					string file = item.FullPath;
					if (String.IsNullOrWhiteSpace(file))
					{
						masterConfigs.Add(UndefinedMasterconfig);
					}
					LineInfoDocument doc = LineInfoDocument.Load(file);

					if (doc.DocumentElement.Name != "project")
					{
						XmlUtil.Warning(project.Log, doc.DocumentElement, String.Format("Error in masterconfig fragment file: Expected root element is 'project', found root '{0}'.", doc.DocumentElement.Name));
					}

					m_masterConfig = new MasterConfig(file);

					foreach (XmlNode childNode in doc.DocumentElement.ChildNodes)
					{
						if (childNode.NodeType == XmlNodeType.Element && childNode.NamespaceURI.Equals(doc.DocumentElement.NamespaceURI))
						{
							switch (childNode.Name)
							{
								case "masterversions":
									ParseMasterVersions(childNode, file);
									break;
								case "packageroots":
									ParsePackageRoots(childNode);
									break;
								case "fragments":
									ParseFragments(childNode);
									break;
								case "globaldefines":
									ParseGlobalDefines(childNode, m_masterConfig.GlobalDefines, dotnet: false);
									break;
								case "globaldotnetdefines":
									ParseGlobalDefines(childNode, m_masterConfig.GlobalDefines, dotnet: true);
									break;
								case "globalproperties":
									ParseGlobalProperties(childNode, m_masterConfig.GlobalProperties);
									break;
								case "gameconfig":
									XmlUtil.Warning(project.Log, doc.DocumentElement, String.Format("Element 'gameconfig' is no longer valid and will be ignored. Use configuration extensions to add additional configuration." ));
									break;
								case "warnings":
									ParseWarnings(childNode, m_masterConfig.Warnings);
									break;
								case "deprecations":
									ParseDeprecations(childNode, m_masterConfig.Deprecations);
									break;
								case "config":
									ParseFragmentConfigElement(childNode, m_masterConfig);
									break;
								default:
									XmlUtil.Error(childNode, "Error in masterconfig fragment file '" + file + "': Unknown element <" + childNode.Name + ">.");
									break;
							}
						}
					}

					ProcessNugetVersions();

					// Calling MergeFragments (which will call this LoadFragments again) will modify the class data member 'masterconfig' 
					// and create a new object (see beginning of this function call).  So we need to create a copy of it first.
					MasterConfig localMasterconfigCopy = m_masterConfig;
					localMasterconfigCopy.MergeFragments(project, this);

					masterConfigs.Add(localMasterconfigCopy);
				}

				return masterConfigs.ToArray();
			}

			private void ParseFragments(XmlNode el)
			{
				foreach (XmlNode childNode in el.ChildNodes)
				{
					if (childNode.NodeType == XmlNodeType.Element && childNode.NamespaceURI.Equals(el.NamespaceURI))
					{
						switch (childNode.Name)
						{
							case "include":
								m_masterConfig.FragmentDefinitions.Add(ParseFragment(childNode));
								break;
							default:
								XmlUtil.Error(el, "Error in masterconfig file: 'fragments' element can only have 'include' child nodes, found: '" + childNode.Name + "'.");
								break;
						}
					}
				}
			}

			private FragmentDefinition ParseFragment(XmlNode el)
			{
				FragmentDefinition fragmentDefinition = new FragmentDefinition
				{
					LoadCondition = el.GetAttributeValue("if", null)
				};

				foreach (XmlNode childNode in el.ChildNodes)
				{
					if (childNode.NodeType == XmlNodeType.Element && childNode.NamespaceURI.Equals(el.NamespaceURI))
					{
						switch (childNode.Name)
						{
							case "exception":
								fragmentDefinition.Exceptions.Add(ParseFragmentException(childNode));
								break;
							default:
								XmlUtil.Error(el, "Error in masterconfig file: 'include' element can only have 'exception' child nodes, found: '" + childNode.Name + "'.");
								break;
						}
					}
				}

				// if no name AND no exceptions with conditions this is invalid
				if (!fragmentDefinition.Exceptions.SelectMany(ex => ex).Any())
				{
					fragmentDefinition.MasterConfigPath = XmlUtil.GetRequiredAttribute(el, "name"); // check exists and value not empty
				}
				else
				{
					fragmentDefinition.MasterConfigPath = XmlUtil.GetOptionalAttribute(el, "name"); // returns null if attribute is not set or String.Empty if value is only whitespace
				}

				return fragmentDefinition;
			}

			private Exception<string> ParseFragmentException(XmlNode el)
			{
				return ParseException(el, childNode => new StringCondition(XmlUtil.GetRequiredAttribute(childNode, "value"), XmlUtil.GetRequiredAttribute(childNode, "name")));
			}

			private void ParseMasterVersions(XmlNode el, string xmlPath)
			{
				m_masterConfig.GroupMap[GroupType.DefaultMasterVersionsGroupName] = m_masterConfig.MasterGroup;

				foreach (XmlNode childNode in el.ChildNodes)
				{
					if (childNode.NodeType == XmlNodeType.Element && childNode.NamespaceURI.Equals(el.NamespaceURI))
					{
						// Can be a package or grouptype:
						switch (childNode.Name)
						{
							case "package":
								ParsePackage(childNode, m_masterConfig.MasterGroup, xmlPath);
								break;
							case "metapackage":
								ParsePackage(childNode, m_masterConfig.MasterGroup, xmlPath, metaPackage: true);
								break;
							case "grouptype":
								ParseGroupType(childNode, xmlPath);
								break;
							default:
								XmlUtil.Error(el, "Error in masterconfig file: 'masterversions' element can only have 'package', 'metapackage' or 'grouptype' child nodes, found: '" + childNode.Name + "'.");
								break;
						}
					}
				}
			}

			private void ParseGroupType(XmlNode el, string fragmentPath)
			{
				string name = XmlUtil.GetRequiredAttribute(el, "name");

				if (!NAntUtil.IsMasterconfigGroupTypeStringValid(name, out string groupNameInvalidReason))
				{
					XmlUtil.Error(el, String.Format("Error in masterconfig file: Group type name '{0}' is invalid. {1}", name, groupNameInvalidReason));
				}

				bool? autoBuildClean = XmlUtil.ToNullableBoolean(XmlUtil.GetOptionalAttribute(el, "autobuildclean", null), el);
				string outputMapOptionSet = XmlUtil.GetOptionalAttribute(el, "outputname-map-options", null);
				string buildRoot = XmlUtil.GetOptionalAttribute(el, "buildroot");

				Dictionary<string, string> additionalAttributes = GetAddtionalElementAttributes(el, "name", "autobuildclean", "outputname-map-options", "buildroot");

				if (!m_masterConfig.GroupMap.TryGetValue(name, out GroupType groupType))
				{
					groupType = new GroupType(name, buildRoot, autoBuildClean, outputMapOptionSet, additionalAttributes);
					m_masterConfig.GroupMap[name] = groupType;
				}
				else
				{
					groupType.BuildRoot = buildRoot;
					groupType.AutoBuildClean = autoBuildClean;
					groupType.OutputMapOptionSet = outputMapOptionSet;
				}

				// parse exceptions and properties
				foreach (XmlNode childNode in el.ChildNodes)
				{
					if (childNode.NodeType == XmlNodeType.Element && childNode.NamespaceURI.Equals(el.NamespaceURI))
					{
						switch (childNode.Name)
						{
							case "package":
								ParsePackage(childNode, groupType, fragmentPath);
								break;
							case "metapackage":
								ParsePackage(childNode, groupType, fragmentPath, metaPackage: true);
								break;
							case "exception":
								groupType.Exceptions.Add(ParseGroupException(childNode, name));
								break;
							default:
								XmlUtil.Error(childNode, "Error in masterconfig file: 'grouptype' element can only have 'package', 'metapackage' or 'exception' child nodes, found: '" + childNode.Name + "'.");
								break;
						}
					}
				}
			}

			private Exception<GroupType> ParseGroupException(XmlNode el, string parentGroup)
			{
				return ParseException
				(
					el,
					childNode =>
					{
						string name = XmlUtil.GetRequiredAttribute(childNode, "name");
						bool? autoBuildClean = XmlUtil.ToNullableBoolean(XmlUtil.GetOptionalAttribute(childNode, "autobuildclean", null), childNode);

						// group type already exists, autobuildclean is ignored even if set,
						// if not then it inherits from parent unless the condition uses the same
						// name but different settings, then it is simply a setting override
						bool isOverride = name == parentGroup;
						if (!m_masterConfig.GroupMap.TryGetValue(name, out GroupType groupType) || isOverride)
						{
							groupType = new GroupType(name, autobuildclean: autoBuildClean);
							if (!isOverride)
							{
								m_masterConfig.GroupMap[name] = groupType;
							}
						}

						return new GroupCondition
						(
							groupType: groupType,
							compareValue: XmlUtil.GetRequiredAttribute(childNode, "value")
						);
					}
				);
			}

			private void ParsePackage(XmlNode el, GroupType grouptype, string xmlPath, bool isFragment = false, bool metaPackage = false)
			{
				// package name
				string name = XmlUtil.GetRequiredAttribute(el, "name");
				if (!NAntUtil.IsPackageNameValid(name, out string invalidPackageNameReason))
				{
					XmlUtil.Error(el, String.Format("Error in masterconfig file: package  name '{0}' is invalid. {1}", name, invalidPackageNameReason));
				}

				// package version
				string version = XmlUtil.GetRequiredAttribute(el, "version");
				if (!NAntUtil.IsPackageVersionStringValid(version, out string invalidVersionNameReason))
				{
					XmlUtil.Error(el, String.Format("Error in masterconfig file: package '{0}' version name '{1}' is invalid. {2}", name, version, invalidVersionNameReason));
				}

				// special attributes
				bool? localOnDemand = XmlUtil.ToNullableBoolean(XmlUtil.GetOptionalAttribute(el, "localondemand", null), el); // parse this here so we can give line number context if format is incorrect
				bool? autobuildClean = XmlUtil.ToNullableBoolean(XmlUtil.GetOptionalAttribute(el, "autobuildclean", null), el); // same

				Dictionary<string, string> additionalAttributes = GetAddtionalElementAttributes(el, "name", "version", "localondemand", "autobuildclean"); // TODO we didn't use to exclude localondemand and autobuildclean here - check no code is going straight to attributes for them

				// if using new nuget rsolution, short circuit adding the package to masterversions if it has a nuget uri - auto convert to nugetpackages
				if (additionalAttributes.TryGetValue("uri", out string uriAttr) && !String.IsNullOrWhiteSpace(uriAttr))
				{
					Uri uri = new Uri(uriAttr);
					if (uri.Scheme == "nuget-http" || uri.Scheme == "nuget-https")
					{
						// also register the uri as a nuget source
						string nugetSource = uri.AbsoluteUri.Substring("nuget-".Length);
						if (!m_automaticNugetSourceConversions.Contains(nugetSource)) // NUGET-TODO: comparison
						{
							m_automaticNugetSourceConversions.Add(nugetSource);
						}

						// if this is a duplicate we will error below when trying to add it as a regular package
						m_masterVersionsToNugetAutoConversions[name] = new NuGetPackage(name, version);

						// we don't early out here - we still want to register this as a regular package
						// as we haven't checked if we are using nuget resolution or not
					}
				}

				Package package = new Package
				(
					grouptype,
					name,
					version,
					additionalAttributes,
					autobuildClean,
					localOnDemand,
					xmlPath: xmlPath,
					isFromFragment: isFragment,
					isMetaPackage: metaPackage
				);

				//Parse exceptions and properties
				foreach (XmlNode childNode in el.ChildNodes)
				{
					if (childNode.NodeType == XmlNodeType.Element && childNode.NamespaceURI.Equals(el.NamespaceURI))
					{
						switch (childNode.Name)
						{
							case "exception":
								package.Exceptions.Add(ParsePackageException(childNode, package));
								break;
							case "warnings":
								ParseWarnings(childNode, package.Warnings);
								break;
							case "deprecations":
								ParseDeprecations(childNode, package.Deprecations);
								break;
							default:
								XmlUtil.Error(el, "Error in masterconfig file: 'package' element can only have 'exception' or 'properties' child nodes, found: '" + childNode.Name + "'.");
								break;
						}
					}
				}

				if (m_masterConfig.Packages.ContainsKey(package.Name))
				{
					XmlUtil.Error(el, "Error in masterconfig file: more than one definition of package'" + package.Name + "' found.");
				}

				m_masterConfig.Packages.Add(package.Name, package);
			}

			private Exception<IPackage> ParsePackageException(XmlNode el, Package parentPackage)
			{
				return ParseException
				(
					el, childNode =>
					{
						return new PackageCondition
						(
							parent: parentPackage,
							compareValue: XmlUtil.GetRequiredAttribute(childNode, "value"),
							version: XmlUtil.GetRequiredAttribute(childNode, "version"),
							attributes: GetAddtionalElementAttributes(childNode, "name", "version", "localondemand", "autobuildclean"),
							autoBuildClean: XmlUtil.ToNullableBoolean(XmlUtil.GetOptionalAttribute(childNode, "autobuildclean", null), childNode),
							localOnDemand: XmlUtil.ToNullableBoolean(XmlUtil.GetOptionalAttribute(childNode, "localondemand", null), childNode)
						);
					}
				);
			}

			private static Dictionary<string, string> GetAddtionalElementAttributes(XmlNode el, params string[] ignore)
			{
				HashSet<string> ignoreSet = new HashSet<string>(ignore);
				return el.Attributes
					.OfType<XmlAttribute>()
					.Where(attr => !ignoreSet.Contains(attr.Name))
					.ToDictionary(attr => attr.Name, attr => attr.Value.TrimWhiteSpace());
			}

			private void ParseGlobalDefines(XmlNode el, List<GlobalDefine> globaldefines, bool dotnet, string packages = "", string condition = null)
			{
				var text = new StringBuilder();
				if (el != null)
				{
					foreach (XmlNode child in el)
					{
						if (child.NodeType == XmlNodeType.Text || child.NodeType == XmlNodeType.CDATA)
						{
							text.AppendLine(child.InnerText.TrimWhiteSpace());
						}
						else if (child.Name == "if")
						{
							if (text.Length > 0)
							{
								ParseGlobalDefineDefinitions(globaldefines, text.ToString().TrimWhiteSpace(), dotnet, packages, condition);
								text.Clear();
							}
							ParseGlobalDefines(child, globaldefines, dotnet, XmlUtil.GetOptionalAttribute(child, "packages", String.Empty), GetCondition(child, condition));
						}
						else if (child.Name == "globaldefine")
						{
							if (text.Length > 0)
							{
								ParseGlobalDefineDefinitions(globaldefines, text.ToString().TrimWhiteSpace(), dotnet, packages, condition);
								text.Clear();
							}
							globaldefines.Add(new GlobalDefine(XmlUtil.GetRequiredAttribute(child, "name"),
								XmlUtil.GetOptionalAttribute(child, "value", ""), GetCondition(child, condition),
								dotnet, packages));
						}
					}
					if (text.Length > 0)
					{
						ParseGlobalDefineDefinitions(globaldefines, text.ToString().TrimWhiteSpace(), dotnet, packages, condition);
					}
				}
			}

			private void ProcessNugetVersions()
			{
				// make sure there are no duplicate names between packages and nuget packages
				// this can cause confusion
				foreach (string packageName in m_masterConfig.Packages.Keys)
				{
					if (m_masterConfig.NuGetPackages.ContainsKey(packageName) && !m_masterVersionsToNugetAutoConversions.ContainsKey(packageName))
					{
						throw new BuildException($"Masterconfig package '{packageName}' was specified as both a regular <package> and a <nugetpackage>. " +
							$"This is disallowed to avoid confusion and potential errors with conflicts between Framework and NuGet packages.");
					}
				}

				// process masterversion than were automatically converted to nuget references
				foreach (KeyValuePair<string, NuGetPackage> autoConversion in m_masterVersionsToNugetAutoConversions)
				{
					// only update nuget packages from auto converted masterversions if there is no explicit
					// nugetpackage specified
					if (!m_masterConfig.NuGetPackages.ContainsKey(autoConversion.Key))
					{
						m_masterConfig.NuGetPackages[autoConversion.Key] = autoConversion.Value;
					}
					// NUGET-TODO: warning
				}
				m_masterVersionsToNugetAutoConversions.Clear(); // loader gets recycled for each fragment so need to reset each time

				// process nuget sources from autoconversion
				foreach (string autoConversion in m_automaticNugetSourceConversions)
				{
					if (!m_masterConfig.NuGetSources.Contains(autoConversion)) // NUGET-TODO: how should we compare nuget sources? can we normalize addresses?
					{
						// add implict sources to the end
						m_masterConfig.NuGetSources.Add(autoConversion);
					}
					// NUGET-TODO: warning
				}
			}

			private static void ParseGlobalProperties(XmlNode el, List<GlobalProperty> globalproperties, string condition = null)
			{
				var text = new StringBuilder();
				if (el != null)
				{
					foreach (XmlNode child in el)
					{
						if (child.NodeType == XmlNodeType.Text || child.NodeType == XmlNodeType.CDATA)
						{
							text.AppendLine(child.InnerText.TrimWhiteSpace());
						}
						else if (child.Name == "if")
						{
							if (text.Length > 0)
							{
								ParseGlobalPropertyDefinitions(globalproperties, text.ToString().TrimWhiteSpace(), condition);
								text.Clear();
							}
							ParseGlobalProperties(child, globalproperties, GetCondition(child, condition));
						}
						else if (child.Name == "globalproperty")
						{
							if (text.Length > 0)
							{
								ParseGlobalPropertyDefinitions(globalproperties, text.ToString().TrimWhiteSpace(), condition);
								text.Clear();
							}

							globalproperties.Add(new GlobalProperty(XmlUtil.GetRequiredAttribute(child, "name"), child.GetXmlNodeText(TrimWhiteSpace: false), GetCondition(child, condition), useXmlElement: true));
						}

					}
					if (text.Length > 0)
					{
						ParseGlobalPropertyDefinitions(globalproperties, text.ToString().TrimWhiteSpace(), condition);
					}
				}
			}

			private static string GetCondition(XmlNode el, string condition)
			{
				string val = null;
				var attr = el.Attributes["condition"];
				if (attr != null && !String.IsNullOrEmpty(attr.Value.TrimWhiteSpace()))
				{
					val = attr.Value.TrimWhiteSpace();

					if (!String.IsNullOrEmpty(condition))
					{
						val = String.Format("({0}) AND ({1})", val, condition);
					}
				}
				else if (!String.IsNullOrEmpty(condition))
				{
					val = condition;
				}
				return val;
			}

			private static void ParseGlobalPropertyDefinitions(List<GlobalProperty> globalProperties, string text, string condition)
			{
				foreach (string prop in StringUtil.QuotedToArray(text))
				{
					string propName = prop;
					string propValue = null;
					int splitPos = prop.IndexOf("=");
					bool quoted = false;
					if (splitPos != -1)
					{
						propName = prop.Substring(0, splitPos);
						propValue = splitPos < prop.Length - 1 ? prop.Substring(splitPos + 1) : String.Empty;
						if (propValue.StartsWith("\"") && propValue.EndsWith("\""))
						{
							quoted = true;
							propValue = StringUtil.TrimQuotes(propValue);
						}
					}
					if (String.IsNullOrEmpty(propName))
					{
						Console.WriteLine("There is likely an error in the Masterconfig.xml file, in <globalproperties>, spaces around assignment operator are not allowed, property '{0}'", prop);
					}
					else
					{
						globalProperties.Add(new GlobalProperty(propName, propValue, condition, quoted: quoted));
					}
				}
			}

			private static void ParseGlobalDefineDefinitions(List<GlobalDefine> globalDefines, string text, bool dotnet, string packages, string condition)
			{
				foreach (string prop in StringUtil.QuotedToArray(text))
				{
					string propName = prop;
					string propValue = null;
					int splitPos = prop.IndexOf("=");
					if (splitPos != -1)
					{
						propName = prop.Substring(0, splitPos);
						propValue = splitPos < prop.Length - 1 ? prop.Substring(splitPos + 1) : String.Empty;
						propValue = StringUtil.TrimQuotes(propValue);
					}
					if (String.IsNullOrEmpty(propName))
					{
						Console.WriteLine("There is likely an error in the Masterconfig.xml file, in <globaldefines>, spaces around assignment operator are not allowed, define '{0}'", prop);
					}
					else
					{
						globalDefines.Add(new GlobalDefine(propName, propValue, condition, dotnet, packages));
					}
				}
			}

			private void ParseConfig(XmlNode el)
			{
				if (m_masterConfig.Config != ConfigInfo.DefaultConfigInfo)
				{
					XmlUtil.Error(el, "Error in masterconfig file: Element 'config' is defined more than once in masterconfig file.");
				}

				List<ConfigExtension> extensions = new List<ConfigExtension>();
				foreach (XmlNode childNode in el.ChildNodes)
				{
					if (childNode.NodeType == XmlNodeType.Element && childNode.NamespaceURI.Equals(el.NamespaceURI))
					{
						if (childNode.Name == "extension")
						{
							extensions.Add(new ConfigExtension(childNode.InnerText.Trim(), XmlUtil.GetOptionalAttribute(childNode, "if", null)));
						}
						else
						{
							XmlUtil.Error(el, String.Format("Error in masterconfig file: Element 'config' has invalid child element '{0}'.", childNode.Name));
						}
					}
				}

				XmlUtil.ValidateAttributes(el, new string[] { "includes", "excludes", "package", "default", "extra-config-dir" });
				if (XmlUtil.GetOptionalAttribute(el, "extra-config-dir") != null)
				{
					XmlUtil.Warning(m_log, el,
						"Element 'config' in masterconfig specifies attributes 'extra-config-dir'. " + 
						"This attribute is no longer supported. Additional configurations can be added using config extension packages."				
					);
				}

				m_masterConfig.Config = new ConfigInfo
				(
					XmlUtil.GetOptionalAttribute(el, "package"),
					XmlUtil.GetOptionalAttribute(el, "default"),
					XmlUtil.GetOptionalAttribute(el, "includes"),
					XmlUtil.GetOptionalAttribute(el, "excludes"),
					extensions
				);
			}

			private void ParseFragmentConfigElement(XmlNode configElement, MasterConfig masterconfig)
			{
				masterconfig.Config = new ConfigInfo();
				foreach (XmlNode childNode in configElement.ChildNodes)
				{
					if (childNode.NodeType == XmlNodeType.Element && childNode.NamespaceURI.Equals(configElement.OwnerDocument.DocumentElement.NamespaceURI))
					{
						if (childNode.Name == "extension")
						{
							masterconfig.Config.Extensions.Add(new ConfigExtension(childNode.InnerText.Trim(), XmlUtil.GetOptionalAttribute(childNode, "if", null)));
						}
						else
						{
							XmlUtil.Error(childNode, string.Format("Error in masterconfig fragment file: Element 'config' has invalid child element '{0}'.", childNode.Name));
						}
					}
				}
			}

			private void ParseNuGetPackages(XmlNode mappingElement, MasterConfig masterconfig)
			{
				foreach (XmlNode childNode in mappingElement.ChildNodes)
				{
					if (childNode.NodeType == XmlNodeType.Element && childNode.NamespaceURI.Equals(mappingElement.NamespaceURI))
					{
						switch (childNode.Name)
						{
							case "sources":
								foreach (XmlNode sourceChildNode in childNode.ChildNodes)
								{
									if (sourceChildNode.NodeType == XmlNodeType.Element && sourceChildNode.NamespaceURI.Equals(childNode.NamespaceURI))
									{
										switch (sourceChildNode.Name)
										{
											case "source":
												string sourceText = sourceChildNode.GetXmlNodeText().TrimWhiteSpace();
												if (String.IsNullOrWhiteSpace(sourceText))
												{
													XmlUtil.Error(sourceChildNode, "Invalid nuget source.'");
												}
												masterconfig.NuGetSources.Add(sourceChildNode.GetXmlNodeText().TrimWhiteSpace());
												break;
											default:
												XmlUtil.Error(childNode, "Error in masterconfig file: 'sources' element can only have 'source' child nodes, found: '" + sourceChildNode.Name + "'.");
												break;
										}
									}
								}
								break;
							case "nugetpackage":
								ParseNugetPackage(childNode, masterconfig.NuGetPackages);
								break;
							case "core":
								ParseNugetPackageGroup(childNode, masterconfig.NetCoreNuGetPackages);
								break;
							case "framework":
								ParseNugetPackageGroup(childNode, masterconfig.NetFrameworkNuGetPackages);
								break;
							case "standard":
								ParseNugetPackageGroup(childNode, masterconfig.NetStandardNuGetPackages);
								break;
							default:
								XmlUtil.Error(mappingElement, "Error in masterconfig file: 'nugetpackages' element can only have 'nugetpackage', 'core', 'framework', 'standard' or 'sources' child nodes, found: '" + childNode.Name + "'.");
								break;
						}
					}
				}
			}

			private static void ParseNugetPackageGroup(XmlNode childNode, Dictionary<string, NuGetPackage> updateDict)
			{
				foreach (XmlNode packageNode in childNode.ChildNodes)
				{
					if (packageNode.NodeType == XmlNodeType.Element && packageNode.NamespaceURI.Equals(childNode.NamespaceURI))
					{
						switch (packageNode.Name)
						{
							case "nugetpackage":
								ParseNugetPackage(packageNode, updateDict, childNode.Name);
								break;
							default:
								XmlUtil.Error(childNode, $"Error in masterconfig file: '{childNode.Name}' element can only have 'nugetpackage' child nodes, found: '" + packageNode.Name + "'.");
								break;
						}
					}
				}
			}

			private static void ParseNugetPackage(XmlNode childNode, Dictionary<string, NuGetPackage> updateDictionary, string groupName = null)
			{
				NuGetPackage nugetPackage = new NuGetPackage(XmlUtil.GetRequiredAttribute(childNode, "name"), XmlUtil.GetRequiredAttribute(childNode, "version"));
				if (nugetPackage.Name == null)
				{
					XmlUtil.Error(childNode, String.Format("Error in masterconfig file: Element 'nugetpackage' is missing required attribute 'name'"));
				}

				if (updateDictionary.ContainsKey(nugetPackage.Name))
				{
					XmlUtil.Error(childNode, $"Error in masterconfig file: more than one definition for nuget package '{nugetPackage.Name}' found{(groupName != null ? $" under group '{groupName}'" : String.Empty)}.");
				}

				updateDictionary.Add(nugetPackage.Name, nugetPackage);
			}

			private void ParsePackageRoots(XmlNode el)
			{
				if (m_masterConfig.PackageRoots.Count > 0)
				{
					XmlUtil.Error(el, "Error in masterconfig file: Element 'packageroots' is defined more than once in masterconfig file.");
				}

				int? minlevel = ConvertUtil.ToNullableInt(XmlUtil.GetOptionalAttribute(el, "minlevel", null));
				int? maxlevel = ConvertUtil.ToNullableInt(XmlUtil.GetOptionalAttribute(el, "maxlevel", null));

				if (minlevel < 0 || maxlevel < 0)
				{
					throw new BuildException("Error: <packageroots> element in masterconfig file cannot have negative 'minlevel' or 'maxlevel' attribute values.");
				}
				if (minlevel > maxlevel)
				{
					throw new BuildException("Error: <packageroots> element in masterconfig file cannot have 'minlevel' greater than 'maxlevel' attribute value.");
				}

				//Parse exceptions and properties
				foreach (XmlNode childNode in el.ChildNodes)
				{
					if (childNode.NodeType == XmlNodeType.Element && childNode.NamespaceURI.Equals(el.NamespaceURI))
					{
						switch (childNode.Name)
						{
							case "packageroot":
								{
									PackageRoot packageroot = new PackageRoot
									(
										childNode.GetXmlNodeText().TrimWhiteSpace(),
										ConvertUtil.ToNullableInt(XmlUtil.GetOptionalAttribute(childNode, "minlevel", null)) ?? minlevel,
										ConvertUtil.ToNullableInt(XmlUtil.GetOptionalAttribute(childNode, "maxlevel", null)) ?? maxlevel
									);
									m_masterConfig.PackageRoots.Add(packageroot);

									//Parse exceptions 
									foreach (XmlNode exceptionNode in childNode.ChildNodes)
									{
										if (exceptionNode.NodeType == XmlNodeType.Element && exceptionNode.NamespaceURI.Equals(childNode.NamespaceURI))
										{
											switch (exceptionNode.Name)
											{
												case "exception":
													packageroot.Exceptions.Add(ParsePackageRootException(exceptionNode));
													break;
												default:
													XmlUtil.Error(childNode, "Error in masterconfig file: 'packageroot' element can only have 'exception'  child nodes, found: '" + exceptionNode.Name + "'.");
													break;
											}
										}
									}
								}
								break;
							default:
								XmlUtil.Error(el, "Error in masterconfig file: 'packageroots' element can only have 'packageroot' child nodes, found: '" + childNode.Name + "'.");
								break;
						}
					}
				}
			}

			private Exception<string> ParsePackageRootException(XmlNode el)
			{
				return ParseException(el, childNode => new StringCondition(XmlUtil.GetRequiredAttribute(childNode, "value"), XmlUtil.GetRequiredAttribute(childNode, "dir")));
			}

			private void ParseBuildRoots(XmlNode node)
			{
				if (m_masterConfig.BuildRoot.Path != null)
				{
					XmlUtil.Error(node, "Error in masterconfig file: Element 'buildroot' is defined more than once in masterconfig file.");
				}

				// the allow-default=false flag indicates that the default and masterconfig build root values should be ignored
				if (node.GetAttributeValue("allow-default", "true").ToLowerInvariant() == "false")
				{
					m_masterConfig.BuildRoot.AllowDefault = false;
				}
				else
				{
					// the build root has no XML child nodes (exceptions) so simply read the xmltext as the build root.
					var splitResult = node.GetXmlNodeText().LinesToArray();
					int splitResultLen = splitResult.Count;
					if (splitResultLen <= 0)
					{
						m_masterConfig.BuildRoot.Path = node.GetXmlNodeText().TrimWhiteSpace();
					}
					else
					{
						// grab the last string in the array.
						m_masterConfig.BuildRoot.Path = splitResult[splitResultLen - 1];
					}

					if (String.IsNullOrEmpty(m_masterConfig.BuildRoot.Path))
					{
						XmlUtil.Error(node, "Error in masterconfig file: Element 'buildroot' has no text data.");
					}

					// If exceptions exist, parse and store them in the BuildRootInfo struct
					if (node.HasChildNodes)
					{
						foreach (XmlNode childNode in node.ChildNodes)
						{
							if (childNode.NodeType == XmlNodeType.Element)
							{
								m_masterConfig.BuildRoot.Exceptions.Add(ParseBuildRootException(childNode));
							}
						}
					}
				}
			}

			private Exception<string> ParseBuildRootException(XmlNode el)
			{
				return ParseException(el, childNode => new StringCondition(XmlUtil.GetRequiredAttribute(childNode, "value"), XmlUtil.GetRequiredAttribute(childNode, "buildroot")));
			}

			private static Exception<TSelectType> ParseException<TSelectType>(XmlNode el, Func<XmlNode, Condition<TSelectType>> createCondition)
			{
				Exception<TSelectType> exception = new Exception<TSelectType>(XmlUtil.GetRequiredAttribute(el, "propertyname"));

				// Parse conditions:
				foreach (XmlNode childNode in el.ChildNodes)
				{
					if (childNode.NodeType == XmlNodeType.Element && childNode.NamespaceURI.Equals(el.NamespaceURI))
					{
						switch (childNode.Name)
						{
							case "condition":
								exception.Add(createCondition(childNode));
								break;
							default:
								XmlUtil.Error(childNode, "Error in masterconfig file: 'exception' element can only have 'condition' child nodes, found: '" + childNode.Name + "'.");
								break;
						}
					}
				}
				return exception;
			}
		}
	}
}