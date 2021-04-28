using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;

namespace NAnt.LangTools
{
	public sealed partial class Model
	{
		public readonly ReadOnlyCollection<Module> Modules;

		public readonly ReadOnlyDictionary<string, Module> RuntimeModules;
		public readonly ReadOnlyDictionary<string, Module> TestModules;
		public readonly ReadOnlyDictionary<string, Module> ExampleModules;
		public readonly ReadOnlyDictionary<string, Module> ToolModules;

		private Model(Parser parser)
		{
			RuntimeModules = InitializeGroupModules(parser, "runtime");
			TestModules = InitializeGroupModules(parser, "test");
			ExampleModules = InitializeGroupModules(parser, "example");
			ToolModules = InitializeGroupModules(parser, "tool");

			Modules = new ReadOnlyCollection<Module>(RuntimeModules.Values.Concat(TestModules.Values.Concat(ExampleModules.Values.Concat(ToolModules.Values))).ToList());
		}

		public static Model CreateFromPackageFile(string packageName, string buildFile, params Feature[] features)
		{
			Parser parser = Parser.Parse(packageName, buildFile, features);
			return new Model(parser);
		}

		private static ReadOnlyDictionary<string, Module> InitializeGroupModules(Parser parser, string group)
		{
			Dictionary<string, Module> modules = new Dictionary<string, Module>();
			if (parser.Properties.TryGetValue($"{group}.buildmodules", out Property.Node buildModulesNode))
			{
				Dictionary<string, ConditionValueSet> moduleNamesToConditions = new Dictionary<string, ConditionValueSet>();
				foreach (Property.Node split in buildModulesNode.SplitToArray())
				{
					Dictionary<string, ConditionValueSet> expandedSplit = split.Expand(throwOnMissing: false);
					foreach (KeyValuePair<string, ConditionValueSet> expansion in expandedSplit)
					{
						if (moduleNamesToConditions.TryGetValue(expansion.Key, out ConditionValueSet conditions))
						{
							conditions = conditions.Or(expansion.Value);
						}
						else
						{
							conditions = expansion.Value;
						}
						moduleNamesToConditions[expansion.Key] = conditions;
					}
				}

				foreach (KeyValuePair<string, ConditionValueSet> moduleNameToCondition in moduleNamesToConditions)
				{
					modules.Add(moduleNameToCondition.Key, new Module(moduleNameToCondition.Key, group, parser, moduleNameToCondition.Value));
				}
			}

			return new ReadOnlyDictionary<string, Module>(modules);
		}
	}
}
