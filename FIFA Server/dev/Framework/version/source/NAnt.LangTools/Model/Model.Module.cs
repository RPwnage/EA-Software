using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;

namespace NAnt.LangTools
{
	public sealed partial class Model
	{
		public sealed class Module // TODO: this is currently cpp specific, eventually need to move some of this to base class for utlity / c#
		{
			public readonly string Name;
			public readonly string Group; // todo: enum?

			public readonly ConditionValueSet Conditions;

			public readonly ReadOnlyDictionary<string, ConditionValueSet> PublicBuildType;
			public readonly ReadOnlyDictionary<string, ConditionValueSet> PrivateBuildType;

			public readonly ReadOnlyCollection<ConditionalEntry> PublicIncludeDirs;
			public readonly ReadOnlyCollection<ConditionalEntry> PrivateIncludeDirs;

			public readonly ReadOnlyCollection<ConditionalEntry> PublicDefines;
			public readonly ReadOnlyCollection<ConditionalEntry> PrivateDefines;

			internal Module(string name, string group, Parser parser, ConditionValueSet conditions)
			{
				Name = name;
				Group = group;

				Conditions = conditions;

				string packagePublicPrefix = $"package.{parser.PackageName}";
				string modulePublicPrefix = $"{packagePublicPrefix}.{Name}";
				string privateGroupPrefix = $"{group}.{name}";

				// public build type - this is mainly we used to automatically export the correct paths for (shared) libraries
				// in the public data task, but it's nice to have it here for verification
				if (parser.Properties.TryGetValue($"{modulePublicPrefix}.buildtype", out Property.Node publicBuildTypeNode)) // todo: parser doesn't set this yet
				{
					PublicBuildType = new ReadOnlyDictionary<string, ConditionValueSet>(publicBuildTypeNode.ReduceByConditions(conditions).Expand());
				}
				else
				{
					PublicBuildType = new ReadOnlyDictionary<string, ConditionValueSet>(new Dictionary<string, ConditionValueSet>());
				}

				if (parser.Properties.TryGetValue($"{privateGroupPrefix}.buildtype", out Property.Node privateBuildTypeNode)) // todo: parser doesn't set this yet
				{
					PrivateBuildType = new ReadOnlyDictionary<string, ConditionValueSet>(privateBuildTypeNode.ReduceByConditions(conditions).Expand());
				}
				else
				{
					throw new NotImplementedException(); // todo if module was determined to exist then this _should_ have a value
				}

				// public includes - fall backs to legacy whole package defines if module does explicitly set it
				if (parser.Properties.TryGetValue($"{modulePublicPrefix}.includedirs", out Property.Node publicIncludeDirsNode) ||
					parser.Properties.TryGetValue($"{packagePublicPrefix}.includedirs", out publicIncludeDirsNode))
				{
					PublicIncludeDirs = new ReadOnlyCollection<ConditionalEntry>(OrderedEntriesFromMultiLineProperty(publicIncludeDirsNode.ReduceByConditions(conditions)));
				}
				else
				{
					PublicIncludeDirs = new ReadOnlyCollection<ConditionalEntry>(Enumerable.Empty<ConditionalEntry>().ToList());
				}

				// private includes
				if (parser.Properties.TryGetValue($"{privateGroupPrefix}.includedirs", out Property.Node privateIncludeDirsNode))
				{
					PrivateIncludeDirs = new ReadOnlyCollection<ConditionalEntry>(OrderedEntriesFromMultiLineProperty(privateIncludeDirsNode.ReduceByConditions(conditions)));
				}
				else
				{
					PrivateIncludeDirs = new ReadOnlyCollection<ConditionalEntry>(Enumerable.Empty<ConditionalEntry>().ToList());
				}

				// public defines - fall backs to legacy whole package defines if module does explicitly set it
				if (parser.Properties.TryGetValue($"{modulePublicPrefix}.defines", out Property.Node publicDefinesNode) ||
					parser.Properties.TryGetValue($"{packagePublicPrefix}.defines", out publicDefinesNode))
				{
					PublicDefines = new ReadOnlyCollection<ConditionalEntry>(OrderedEntriesFromMultiLineProperty(publicDefinesNode.ReduceByConditions(conditions)));
				}
				else
				{
					PublicDefines = new ReadOnlyCollection<ConditionalEntry>(Enumerable.Empty<ConditionalEntry>().ToList());
				}

				// private defines
				if (parser.Properties.TryGetValue($"{privateGroupPrefix}.defines", out Property.Node privateDefinesNode))
				{
					PrivateDefines = new ReadOnlyCollection<ConditionalEntry>(OrderedEntriesFromMultiLineProperty(privateDefinesNode.ReduceByConditions(conditions)));
				}
				else
				{
					PrivateDefines = new ReadOnlyCollection<ConditionalEntry>(Enumerable.Empty<ConditionalEntry>().ToList());
				}
			}

			private List<ConditionalEntry> OrderedEntriesFromMultiLineProperty(Property.Node node)
			{
				string lastValue = null;
				ConditionValueSet lastConditions = null;
				List<ConditionalEntry> returnList = new List<ConditionalEntry>();

				foreach (Property.Node split in node.SplitLinesToArray())
				{
					Dictionary<string, ConditionValueSet> expandedSplit = split.Expand(throwOnMissing: false);
					foreach (KeyValuePair<string, ConditionValueSet> expansion in expandedSplit)
					{
						string trimmedkey = expansion.Key.Trim();
						if (trimmedkey == String.Empty)
						{
							continue;
						}

						if (trimmedkey == lastValue)
						{
							lastConditions = lastConditions.Or(expansion.Value);
						}
						else
						{
							if (lastValue != null)
							{
								returnList.Add(new ConditionalEntry(lastValue, lastConditions));
							}

							lastValue = trimmedkey;
							lastConditions = expansion.Value;
						}
					}
				}

				if (lastValue != null)
				{
					returnList.Add(new ConditionalEntry(lastValue, lastConditions));
				}

				return returnList.Distinct().ToList(); // Framework doesn't export these distinct - but they are distinct'd when added to depending modules. For us it's cleaner to do it here
			}
		}
	}
}
