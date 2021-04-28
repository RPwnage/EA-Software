using System;
using System.Collections;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;

namespace NAnt.LangTools
{
	public sealed class Condition
	{
		internal readonly string ConditionaName;
		internal readonly ReadOnlyCollection<string> Values;

		internal Condition(string name, IEnumerable<string> values)
		{
			ConditionaName = name;
			Values = new ReadOnlyCollection<string>(values.ToArray());
		}

		public override string ToString()
		{
			return ConditionaName;
		}

		public override bool Equals(object obj)
		{
			return base.Equals(obj); // relying on isntances of Conditions being created only once, so override not needed
		}

		public override int GetHashCode()
		{
			return base.GetHashCode();  // relying on isntances of Conditions being created only once, so override not needed
		}
	}

	// TODO: lots of places where it would be more efficient to update these in place - implement AndWith, OrWith, etc...
	public sealed class ConditionValueSet : IEnumerable<ReadOnlyDictionary<Condition, string>>
	{
		private sealed class DictComparer : IEqualityComparer<Dictionary<Condition, string>>
		{
			internal static readonly DictComparer Instance = new DictComparer();

			private DictComparer()
			{
			}

			public bool Equals(Dictionary<Condition, string> x, Dictionary<Condition, string> y)
			{
				return x.Count == y.Count && !x.Except(y).Any(); // assume sizes are already the same from GetHashCode, relying on instance equality of Condition
			}

			public int GetHashCode(Dictionary<Condition, string> obj)
			{
				return obj.Count(); // just use the size as a hashcode, we don't want to waste time calculating a proper hash and go straight to Equals check
			}

		}

		/*internal - public for demo! undo!*/ public readonly static ConditionValueSet True = new ConditionValueSet();
		internal readonly static ConditionValueSet False = new ConditionValueSet();

		private readonly HashSet<Dictionary<Condition, string>> m_conditionValueSets;

		private ConditionValueSet()
		{
			m_conditionValueSets = new HashSet<Dictionary<Condition, string>>(DictComparer.Instance);
		}

		private ConditionValueSet(ConditionValueSet copy)
		{
			m_conditionValueSets = new HashSet<Dictionary<Condition, string>>(copy.m_conditionValueSets.Select(dict => new Dictionary<Condition, string>(dict)), DictComparer.Instance);
		}

		private ConditionValueSet(HashSet<Dictionary<Condition, string>> conditionValueSets)
		{
			m_conditionValueSets = conditionValueSets;
		}

		public override bool Equals(object obj)
		{
			if (ReferenceEquals(this, True))
			{
				return ReferenceEquals(obj, True);
			}

			if (ReferenceEquals(this, False))
			{
				return ReferenceEquals(obj, False);
			}

			return obj is ConditionValueSet set &&
				m_conditionValueSets.Count == set.m_conditionValueSets.Count &&
				!m_conditionValueSets.Except(set.m_conditionValueSets, DictComparer.Instance).Any();
		}

		public override int GetHashCode()
		{
			return 1962948306 + m_conditionValueSets.Count; // just use count and fall back to equals
		}

		public override string ToString()
		{
			if (this == True)
			{
				return "true";
			}
			else if (this == False)
			{
				return "false";
			}
			else if (m_conditionValueSets.Count == 1)
			{
				return String.Join(" and ", m_conditionValueSets.First().Select(cv => cv.ToString()));
			}
			else
			{
				return String.Join(" or ", m_conditionValueSets.Select(set => $"({String.Join(" and ", set.Select(cv => cv.ToString()))})"));
			}
		}

		public static bool operator ==(ConditionValueSet lhs, ConditionValueSet rhs)
		{
			if (lhs is null)
			{
				if (rhs is null)
				{
					return true;
				}
				return false;
			}
			return lhs.Equals(rhs);
		}

		public static bool operator !=(ConditionValueSet lhs, ConditionValueSet rhs)
		{
			return !(lhs == rhs);
		}

		public IEnumerator<ReadOnlyDictionary<Condition, string>> GetEnumerator()
		{
			return m_conditionValueSets.Select(set => new ReadOnlyDictionary<Condition, string>(set)).GetEnumerator();
		}

		internal ConditionValueSet Or(ConditionValueSet orSet)
		{
			// handle special cases
			if (this == True || orSet == True)
			{
				return True;
			}
			else if (this == False)
			{
				return new ConditionValueSet(orSet); // TODO do we really need to copy here? in current implementtion these are immutable
			}
			else if (orSet == False)
			{
				return new ConditionValueSet(this); // TODO do we really need to copy here? in current implementtion these are immutable
			}

			// add sets together. If the orSet contains a less specific condition set then remove the more specific set
			// e.g if we have (A, B, C) and other set has (A, B) then ((A and B and C) or (A and B)) reduces to (A and B)
			HashSet<Dictionary<Condition, string>> newDicts = new HashSet<Dictionary<Condition, string>>(m_conditionValueSets.Select(dict => new Dictionary<Condition, string>(dict)), DictComparer.Instance);
			foreach (Dictionary<Condition, string> otherDict in orSet.m_conditionValueSets)
			{
				newDicts.RemoveWhere(dict => DictIsSubsetOf(otherDict, dict));
				newDicts.Add(otherDict);
			}

			// now eliminate cases where all option of a condition are accepted for example if we have ((A = true, B = true, C = true)
			// or (A = true, B = true, C= false) and the only possible values for C are true and false then it reduces to (A = true, B = true)
			Condition[] uniqueConditions = newDicts.SelectMany(newDict => newDict.Select(kvp => kvp.Key)).Distinct().ToArray();
			foreach (Condition uniqueCondition in uniqueConditions)
			{
				// create a tuple for (condition value, full dictionary of conditions, dictionary minus the current condition) for
				// entry that references this condition
				List<Tuple<string, Dictionary<Condition, string>, Dictionary<Condition, string>>> valueDictReducedDict = new List<Tuple<string, Dictionary<Condition, string>, Dictionary<Condition, string>>>();
				foreach (Dictionary<Condition, string> newDict in newDicts)
				{
					if (newDict.TryGetValue(uniqueCondition, out string matchingValue))
					{
						Dictionary<Condition, string> reducedDict = new Dictionary<Condition, string>(newDict);
						reducedDict.Remove(uniqueCondition);
						valueDictReducedDict.Add(Tuple.Create(matchingValue, newDict, reducedDict));
					}
				}

				// group the entries by the reduced dictionary - if we have more than one entry in a grouping then it may contain all possible values for a single
				// condition and the rest of the conditions are the same
				IEnumerable<IGrouping<Dictionary<Condition, string>, Tuple<string, Dictionary<Condition, string>, Dictionary<Condition, string>>>> byReducedDicts = valueDictReducedDict.GroupBy(tpl => tpl.Item3, DictComparer.Instance);
				
				// if the grouping contains every possible value remove the full sets and just used the reduced value
				foreach (IGrouping<Dictionary<Condition, string>, Tuple<string, Dictionary<Condition, string>, Dictionary<Condition, string>>> byReducedDict in byReducedDicts)
				{
					if (uniqueCondition.Values.All(v => byReducedDict.Any(tpl => tpl.Item1 == v)))
					{
						newDicts.RemoveWhere(n => byReducedDict.Any(tpl => DictComparer.Instance.Equals(n, tpl.Item2)));

						// if the reduce set contains no keys then it means no conditions are required so return true
						if (!byReducedDict.Key.Any())
						{
							return True;
						}

						newDicts.Add(byReducedDict.Key);
					}
				}
			}

			return new ConditionValueSet(newDicts);
		}

		internal ConditionValueSet Or(Condition condition, string key)
		{
			return Or(True.And(condition, key));
		}

		internal ConditionValueSet And(ConditionValueSet andSet)
		{
			if (this == False || andSet == False)
			{
				return False;
			}
			else if (this == True && andSet == True)
			{
				return True;;
			}
			else if (this == True)
			{
				return new ConditionValueSet(andSet);
			}
			else if (andSet == True)
			{
				return new ConditionValueSet(this);
			}

			HashSet<Dictionary<Condition, string>> newSets = new HashSet<Dictionary<Condition, string>>(DictComparer.Instance);
			foreach (Dictionary<Condition, string> thisSet in m_conditionValueSets)
			{
				foreach (Dictionary<Condition, string> otherSet in andSet.m_conditionValueSets)
				{
					if (!thisSet.Any(thisKvp => otherSet.TryGetValue(thisKvp.Key, out string otherValue) && thisKvp.Value != otherValue))
					{
						newSets.Add(DictUnion(thisSet, otherSet));
					}
				}
			}

			if (!newSets.Any())
			{
				return False;
			}

			return new ConditionValueSet(newSets);
		}

		internal ConditionValueSet And(Condition condition, string value)
		{
			if (!condition.Values.Contains(value))
			{
				throw new ArgumentException(); // TODO
			}

			if (this == True)
			{
				return new ConditionValueSet(new HashSet<Dictionary<Condition, string>>() { new Dictionary<Condition, string>() { { condition, value } } });
			}

			HashSet<Dictionary<Condition, string>> newDicts = new HashSet<Dictionary<Condition, string>>(DictComparer.Instance);
			foreach (Dictionary<Condition, string> dict in m_conditionValueSets)
			{		
				if (!dict.TryGetValue(condition, out string existingValue))
				{
					Dictionary<Condition, string> newDict = new Dictionary<Condition, string>(dict)
					{
						{ condition, value }
					};
					newDicts.Add(newDict);
				}
				else if (existingValue == value)
				{
					newDicts.Add(new Dictionary<Condition, string>(dict));
				}
			}

			if (!newDicts.Any())
			{
				return False;
			}

			return new ConditionValueSet(newDicts);
		}

		internal bool TrueWhen(ConditionValueSet conditions)
		{
			if (this == True)
			{
				return true;
			}

			foreach (Dictionary<Condition, string> valueSet in m_conditionValueSets)
			{
				foreach (Dictionary<Condition, string> otherValueSet in conditions.m_conditionValueSets)
				{
					if (DictIsSubsetOf(valueSet, otherValueSet))
					{
						return true;
					}
				}
			}
			return false;
		}

		private static Dictionary<Condition, string> DictUnion(Dictionary<Condition, string> thisSet, Dictionary<Condition, string> otherDicts)
		{
			Dictionary<Condition, string> newDict = new Dictionary<Condition, string>(thisSet);
			foreach (KeyValuePair<Condition, string> kvp in otherDicts)
			{
				newDict[kvp.Key] = kvp.Value;
			}
			return newDict;
		}

		private static bool DictIsSubsetOf(Dictionary<Condition, string> thisDict, Dictionary<Condition, string> otherDict)
		{
			foreach (KeyValuePair<Condition, string> kvp in thisDict)
			{
				if (!otherDict.TryGetValue(kvp.Key, out string otherValue) || kvp.Value != otherValue)
				{
					return false;
				}
			}
			return true;
		}

		IEnumerator IEnumerable.GetEnumerator()
		{
			return GetEnumerator();
		}
	}
}