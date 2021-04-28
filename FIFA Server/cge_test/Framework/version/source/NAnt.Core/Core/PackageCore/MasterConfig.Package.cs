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
using System.Linq;

using NAnt.Core.Logging;

namespace NAnt.Core.PackageCore
{
	// DAVE-FUTURE-REFACTOR-TODO how many of the details here really need to be public? need to check how this class is used in and out of Framework
	// and see if we can't start hiding some implementation details
	public sealed partial class MasterConfig
	{
		// packagespecs represent a set of package metadata from the masterconfig or constructed objects used to drive package downloads
		// it contains all the information needed to:
		//  * find / download a package
		//  * decide if group is auto build clean
		//  * get the masterconfig group
		//  * apply warning / deprecation based upon package
		public interface IPackage
		{
			string Name { get; }
			string Version { get; }

			string XmlPath { get;  }

			ReadOnlyDictionary<string, string> AdditionalAttributes { get; }

			bool? AutoBuildClean { get; }
			string AutoBuildTarget { get; }
			string AutoCleanTarget { get; }

			bool? LocalOnDemand { get; }

			bool IsAutoVerison { get; }

			Dictionary<Log.WarningId, WarningDefinition> Warnings { get; } // DAVE-FUTURE-REFACTOR-TODO: readonly dictionary
			Dictionary<Log.DeprecationId, DeprecationDefinition> Deprecations { get; }

			GroupType EvaluateGroupType(Project project);
		}

		// a <condition> in an <exception> under a <package> in masterconfig
		public sealed class PackageCondition : Condition<IPackage>, IPackage
		{
			public string Name => m_parentMasterConfigPackage.Name;
			public string XmlPath => m_parentMasterConfigPackage.XmlPath;

			public string Version { get; private set; }
			public bool? AutoBuildClean { get; private set; }
			public ReadOnlyDictionary<string, string> AdditionalAttributes { get; private set; }

			// inherit from parent, exception can't have different values
			public bool IsAutoVerison => m_parentMasterConfigPackage.IsAutoVerison;
			public Dictionary<Log.WarningId, WarningDefinition> Warnings => m_parentMasterConfigPackage.Warnings;
			public Dictionary<Log.DeprecationId, DeprecationDefinition> Deprecations => m_parentMasterConfigPackage.Deprecations;

			public string AutoBuildTarget => AdditionalAttributes.TryGetValue("autobuildtarget", out string autoBuildTarget) ? autoBuildTarget : m_parentMasterConfigPackage.AutoBuildTarget;
			public string AutoCleanTarget => AdditionalAttributes.TryGetValue("autocleantarget", out string autoCleanTarget) ? autoCleanTarget : m_parentMasterConfigPackage.AutoCleanTarget;

			public bool? LocalOnDemand { get; private set; } // we could pull this direct from attributes, but since we need to translate to bool it's better to do that at initialization time
															 // when we still have xml line / file info for better errors

			public override IPackage Value => this;

			private readonly Package m_parentMasterConfigPackage;

			public PackageCondition(Package parent, string compareValue, string version, IDictionary<string, string> attributes, bool? autoBuildClean = null, bool? localOnDemand = null)
				: base(compareValue)
			{
				Version = version;

				// we DO NOT inherit custom attributes from parent, only special attributes (autobuildclean, autobuildtarget, 
				//autocleantarget, localondemand), others (uri, switch-from-version) are not inherited
				AdditionalAttributes = new ReadOnlyDictionary<string, string>(new Dictionary<string, string>(attributes));

				AutoBuildClean = autoBuildClean ?? parent.AutoBuildClean;
				LocalOnDemand = localOnDemand ?? parent.LocalOnDemand;

				m_parentMasterConfigPackage = parent;
			}

			public GroupType EvaluateGroupType(Project project)
			{
				// group cannot be overridden in exceptions so return parent's
				return m_parentMasterConfigPackage.EvaluateGroupType(project);
			}
		}

		// a <package>
		public sealed class Package : IPackage
		{
			public string Name { get; private set; }
			public string Version { get; private set; }

			public string XmlPath { get; private set; }

			public ReadOnlyDictionary<string, string> AdditionalAttributes { get; private set; }
			public GroupType GroupType { get; set; } // publically writable because of the way we do metapackage merging // DAVE-FUTURE-REFACTOR-TODO - can we avoid this?
			public bool IsAutoVerison { get; private set; }    // This value should be false all the times and only get set to true
															   // during NuGet download for package that is part of a NuGet package dependency
															   // but package is not listed in masterconfig.

			public Dictionary<Log.WarningId, WarningDefinition> Warnings { get; private set; } = new Dictionary<Log.WarningId, WarningDefinition>();
			public Dictionary<Log.DeprecationId, DeprecationDefinition> Deprecations { get; private set; } = new Dictionary<Log.DeprecationId, DeprecationDefinition>();

			public bool? AutoBuildClean { get; private set; } = null;
			public string AutoBuildTarget => AdditionalAttributes.TryGetValue("autobuildtarget", out string autoBuildTarget) ? autoBuildTarget : null;
			public string AutoCleanTarget => AdditionalAttributes.TryGetValue("autocleantarget", out string autoCleanTarget) ? autoCleanTarget : null;

			public bool? LocalOnDemand { get; set; } // writable because of the way we do metapackage merging // DAVE-FUTURE-REFACTOR-TODO - can we avoid this?

			public readonly ExceptionCollection<IPackage> Exceptions = new ExceptionCollection<IPackage>();

			public readonly bool IsMetaPackage;
			public readonly bool IsFromFragment;

			internal Package(GroupType grouptype, string name, string version, IDictionary<string, string> attributes = null, bool? autoBuildClean = null, bool? localOndemand = null, bool isAutoVersion = false, string xmlPath = null, bool isFromFragment = false, bool isMetaPackage = false, Dictionary<Log.WarningId, WarningDefinition> warnings = null, Dictionary<Log.DeprecationId, DeprecationDefinition> deprecations = null)
			{
				GroupType = grouptype ?? throw new ArgumentNullException(nameof(grouptype));

				Name = name;
				Version = version;

				XmlPath = xmlPath ?? "<special>";

				AdditionalAttributes = new ReadOnlyDictionary<string, string>(attributes != null ? new Dictionary<string, string>(attributes) : new Dictionary<string, string>());

				AutoBuildClean = autoBuildClean;
				LocalOnDemand = localOndemand;

				IsAutoVerison = isAutoVersion;
				IsMetaPackage = isMetaPackage;
				IsFromFragment = isFromFragment;

				Warnings = warnings ?? Warnings;
				Deprecations = deprecations ?? Deprecations;
			}

			public GroupType EvaluateGroupType(Project project)
			{
				return GroupType.EvaluateGroupType(project);
			}

			[Obsolete("Please use EvaluateGroupType instead (note the casing!).")]
			public GroupType EvaluateGrouptype(Project project) => EvaluateGroupType(project);
		}

		// a <grouptype>
		public class GroupType // DAVE-FUTURE-REFACTOR-TODO: merging is ugly, but this is a side effect of masterconfig merging in general being ugly
		{
			public string Name { get; private set; }
			public bool? AutoBuildClean { get; internal set; }

			public string BuildRoot { get; internal set; }
			public string OutputMapOptionSet { get; internal set; }

			public ExceptionCollection<GroupType> Exceptions { get; private set; } = new ExceptionCollection<GroupType>();

			public const string DefaultMasterVersionsGroupName = "masterversions";

			public Dictionary<string, string> AdditionalAttributes { get; private set; }

			internal GroupType(string name, string buildroot = null, bool? autobuildclean = null, string outputMapOptionSet = null, Dictionary<string, string> attributes = null)
			{
				Name = name;

				// TODO: validate this outside of construction when we have xml location context:
				/*if (name.Trim() == String.Empty)
				{
					throw new BuildException(String.Format("GroupType name '{0}' cannot be empty", name));
				}
				else if (name.Contains('/') || name.Contains('\\'))
				{
					throw new BuildException(String.Format("GroupType name '{0}' contains Illegal characters", name));
				}
				else if (name[name.Length - 1] == '.' || name[0] == '.')
				{
					throw new BuildException(String.Format("GroupType name '{0}' cannot start or end with a '.'", name));
				}*/

				AutoBuildClean = autobuildclean;
				BuildRoot = buildroot;
				OutputMapOptionSet = outputMapOptionSet;

				if (attributes == null)
				{
					AdditionalAttributes = new Dictionary<string, string>();
				}
				else
				{
					AdditionalAttributes = attributes;
				}
			}

			public GroupType EvaluateGroupType(Project project)
			{
				Stack<GroupType> groupStack = null;
				GroupType group = this;
				while (true)
				{
					GroupType resolvedGroup = group.Exceptions.ResolveException(project, group);
					if (resolvedGroup == group)
					{
						return resolvedGroup;
					}

					groupStack = groupStack ?? new Stack<GroupType>();
					if (groupStack.Contains(group))
					{
						throw new BuildException($"Invalid circular grouptype exception {String.Join(" => ", groupStack.Reverse().Select(g => g.Name))} => {group.Name}");
					}
					groupStack.Push(group);

					group = resolvedGroup;
				}
			}

			internal void Merge(GroupType value)
			{
				if (AutoBuildClean != null && value.AutoBuildClean != null && AutoBuildClean != value.AutoBuildClean)
				{
					throw new BuildException($"The masterconfig group '{value.Name}' is defined in multiple fragments with conflicting values of the AutoBuildClean property.");
				}
				if (BuildRoot != null && value.BuildRoot != null && BuildRoot != value.BuildRoot)
				{
					throw new BuildException($"The masterconfig group '{value.Name}' is defined in multiple fragments with conflicting values of the BuildRoot property.");
				}
				if (OutputMapOptionSet != null && value.OutputMapOptionSet != null && OutputMapOptionSet != value.OutputMapOptionSet)
				{
					throw new BuildException($"The masterconfig group '{value.Name}' is defined in multiple fragments with conflicting values of the OutputMapOptionSet property.");
				}

				AutoBuildClean = value.AutoBuildClean ?? AutoBuildClean;
				BuildRoot = value.BuildRoot ?? BuildRoot;
				OutputMapOptionSet = value.OutputMapOptionSet ?? OutputMapOptionSet;

				foreach (string key in value.AdditionalAttributes.Keys)
				{
					if (AdditionalAttributes.ContainsKey(key))
					{
						if (AdditionalAttributes[key] != value.AdditionalAttributes[key])
						{
							throw new BuildException($"The masterconfig group '{value.Name}' is defined in multiple fragments with conflicting values of the '{key}' property.");
						}
					}
					else
					{
						AdditionalAttributes.Add(key, value.AdditionalAttributes[key]);
					}
				}
				foreach (string key in AdditionalAttributes.Keys)
				{
					if (!value.AdditionalAttributes.ContainsKey(key))
					{
						value.AdditionalAttributes.Add(key, AdditionalAttributes[key]);
					}
				}

				ExceptionCollection<GroupType> newExceptions = new ExceptionCollection<GroupType>();
				foreach (Exception<GroupType> exception in value.Exceptions.Concat(Exceptions))
				{
					newExceptions.Add(exception);
				}
				Exceptions = newExceptions;
			}

			internal void UpdateRefs(Dictionary<string, GroupType> groupMap)
			{
				foreach (Exception<GroupType> exception in Exceptions)
				{
					foreach (GroupCondition condition in exception.OfType<GroupCondition>())
					{
						if (condition.Value.Name != Name)
						{
							condition.UpdateRef(groupMap);
						}
					}
				}
			}
		}

		// a <condition> under an <exception> under a <grouptype>
		public class GroupCondition : Condition<GroupType>
		{
			public override GroupType Value => m_groupType;

			private GroupType m_groupType;

			internal GroupCondition(GroupType groupType, string compareValue)
				: base(compareValue)
			{
				m_groupType = groupType;
			}

			internal void UpdateRef(Dictionary<string, GroupType> groupMap)
			{
				m_groupType = groupMap[m_groupType.Name];
			}
		}
	}
}