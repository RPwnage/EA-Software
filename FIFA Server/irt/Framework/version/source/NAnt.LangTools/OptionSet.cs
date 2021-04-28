using System.Collections;
using System.Collections.Generic;
using System.Linq;

namespace NAnt.LangTools
{
	internal class OptionSet : IReadOnlyDictionary<string, Property.Node>
	{
		internal sealed class Map : IReadOnlyDictionary<string, Map.Entry>
		{
			internal struct Entry
			{
				internal Entry(OptionSet optionSet, ConditionValueSet existenceConditions)
				{
					OptionSet = optionSet;
					Conditions = existenceConditions;
				}

				internal readonly OptionSet OptionSet;
				internal readonly ConditionValueSet Conditions;
			}

			private readonly Dictionary<string, Entry> m_optionSets = new Dictionary<string, Entry>();

			public IEnumerable<string> Keys => m_optionSets.Keys;
			public IEnumerable<Entry> Values => m_optionSets.Values;
			public int Count => m_optionSets.Count;

			public Entry this[string key] { get => m_optionSets[key]; }

			public bool ContainsKey(string key) => m_optionSets.ContainsKey(key);
			public bool TryGetValue(string key, out Entry value) => m_optionSets.TryGetValue(key, out value);

			internal void SetOptionSet(string name, OptionSet set, ConditionValueSet conditions)
			{
				set = new OptionSet(set.ToDictionary(kvp => kvp.Key, kvp => kvp.Value.AttachConditions(conditions)));

				if (m_optionSets.TryGetValue(name, out Entry entry))
				{
					entry = new Entry
					(
						entry.OptionSet.Update(set),
						entry.Conditions.Or(conditions)
					);
				}
				else
				{
					entry = new Entry(set, conditions);
				}
				m_optionSets[name] = entry;
			}

			internal bool TryGetValue(string name, out OptionSet set, ConditionValueSet conditions)
			{
				if (m_optionSets.TryGetValue(name, out Entry entry) && entry.Conditions.TrueWhen(conditions))
				{
					set = entry.OptionSet;
					return true;
				}
				set = null;
				return false;
			}

			public IEnumerator<KeyValuePair<string, Entry>> GetEnumerator() => m_optionSets.GetEnumerator();
			IEnumerator IEnumerable.GetEnumerator() => GetEnumerator();
		}

		private readonly Dictionary<string, Property.Node> m_options;

		public IEnumerable<string> Keys => m_options.Keys;
		public IEnumerable<Property.Node> Values => m_options.Values;
		public int Count => m_options.Count;

		internal OptionSet()
		{
			m_options = new Dictionary<string, Property.Node>();
		}

		private OptionSet(IDictionary<string, Property.Node> copy)
		{
			m_options = new Dictionary<string, Property.Node>(copy);
		}

		public Property.Node this[string key]
		{ 
			get => m_options[key];
			private set => m_options[key] = value;
		}

		public bool ContainsKey(string key) => m_options.ContainsKey(key);
		public bool TryGetValue(string key, out Property.Node value) => m_options.TryGetValue(key, out value);
		public IEnumerator<KeyValuePair<string, Property.Node>> GetEnumerator() => m_options.GetEnumerator();

		internal void SetOption(string name, Property.Node node, ConditionValueSet conditions)
		{
			node = node.AttachConditions(conditions);
			if (m_options.TryGetValue(name, out Property.Node existing))
			{
				node = existing.Update(node);
			}
			m_options[name] = node;
		}

		internal Property.Node GetOptionOrNullWithConditions(string name, ConditionValueSet conditions)
		{
			if (m_options.TryGetValue(name, out Property.Node existing))
			{
				return existing.ReduceByConditions(conditions);
			}
			return null;
		}

		internal OptionSet Update(OptionSet fromSet)
		{
			OptionSet newSet = new OptionSet(m_options);
			foreach (KeyValuePair<string, Property.Node> fromOption in fromSet)
			{
				if (newSet.TryGetValue(fromOption.Key, out Property.Node existingOption))
				{
					existingOption = existingOption.Update(fromOption.Value);
				}
				else
				{
					existingOption = fromOption.Value;
				}
				newSet[fromOption.Key] = existingOption;
			}
			return newSet;
		}

		IEnumerator IEnumerable.GetEnumerator() => GetEnumerator();
	}
}