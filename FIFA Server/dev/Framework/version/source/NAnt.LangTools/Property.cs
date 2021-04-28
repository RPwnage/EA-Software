using System;
using System.Collections;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.Linq;

namespace NAnt.LangTools
{
	internal static class Property
	{
		internal sealed class Map : IReadOnlyDictionary<string, Node>
		{
			private readonly Dictionary<string, Node> m_properties = new Dictionary<string, Node>();

			public IEnumerable<string> Keys => m_properties.Keys;
			public IEnumerable<Node> Values => m_properties.Values;
			public int Count => m_properties.Count;

			public Node this[string key] => m_properties[key];

			public bool ContainsKey(string key) => m_properties.ContainsKey(key);
			public bool TryGetValue(string key, out Node value) => m_properties.TryGetValue(key, out value);

			public IEnumerator<KeyValuePair<string, Node>> GetEnumerator()
			{
				return m_properties.GetEnumerator();
			}

			internal void SetProperty(string name, Node property)
			{
				property = property ?? throw new ArgumentNullException(nameof(property));
				if (m_properties.TryGetValue(name, out Node existing))
				{
					m_properties[name] = existing.Update(property);
				}
				else
				{
					m_properties[name] = property.Simplify();
				}
			}

			internal Node GetPropertyOrNull(string name)
			{
				if (m_properties.TryGetValue(name, out Node property))
				{
					return property;
				}
				return null;
			}

			internal void Remove(string property)
			{
				m_properties.Remove(property);
			}

			IEnumerator IEnumerable.GetEnumerator() => GetEnumerator();
		}

		internal abstract class Node
		{
			public static Node operator +(Node left, Node right) => left.Append(right);
			public static implicit operator Node(string str) => new String(str);

			public abstract override bool Equals(object obj);
			public abstract override int GetHashCode();

			internal abstract Node Simplify(Dictionary<Condition, HashSet<string>> accountedForValues = null);
			internal abstract ConditionValueSet GetValueConditions();
			internal abstract Node ReduceByConditions(ConditionValueSet conditionValueSet);
			internal abstract Node ExcludeConditions(ConditionValueSet conditionValueSet); // TODO: need more testing - we might need to invert the conditions set then reduce by it, rather than current approach

			internal abstract Dictionary<string, ConditionValueSet> Expand(ConditionValueSet forConditions = null, bool throwOnMissing = true);

			internal Node[] SplitLinesToArray() => SplitToArray(Constants.LinesDelimiters);

			internal Node Append(Node appendNode) => Concatenate(this, appendNode);
			internal Node AppendLine(Node appendNode) => Concatenate(this, Constants.Newline, appendNode);
   
			internal Node[] SplitToArray(char[] delimArray = null)
			{
				List<Node> _ = null;
				return InternalSplitToListWithTrailing(this, delimArray ?? Constants.DefaultDelimiters, ref _, separateTrailing: false).ToArray();
			}

			internal Node Update(Node property)
			{
				Debug.Assert(property != null);

				if (property is Conditional conditional)
				{
					return conditional.MergeUnresolvedConditions(this);
				}
				return property;
			}

			internal Node AttachConditions(ConditionValueSet conditionValueSet)
			{
				if (conditionValueSet == ConditionValueSet.False)
				{
					throw new InvalidOperationException(); // todo: cannot expand when there are no conditions
				}

				Node currentNode = this;
				foreach (IGrouping<Condition, string> valuesByCondition in conditionValueSet.SelectMany(set => set).GroupBy(kvp => kvp.Key, kvp => kvp.Value))
				{
					currentNode = MergeConditionalValues(valuesByCondition.Key, valuesByCondition.Distinct().ToDictionary(str => str, str => currentNode));
				}
				return currentNode;
			}

			internal Node Transform(Func<string, string> transform)
			{
				if (this is String thisString)
				{
					return transform(thisString.Value);
				}
				else
				{
					Node returnNode = null;
					foreach (KeyValuePair<string, ConditionValueSet> expansion in Expand(throwOnMissing: false))
					{
						string transformedExpansion = transform(expansion.Key);
						if (transformedExpansion != null)
						{
							Node newNode = new String(transform(expansion.Key)).AttachConditions(expansion.Value);
							returnNode = returnNode?.Update(newNode) ?? newNode;
						}
					}
					return returnNode?.Simplify();
				}
			}
		}

		internal sealed class String : Node
		{
			internal readonly string Value;

			internal String(string value)
			{
				Value = value;
			}

			public override bool Equals(object obj)
			{
				return obj is String other &&
					Value == other.Value;
			}

			public override int GetHashCode()
			{
				return -1937169414 + EqualityComparer<string>.Default.GetHashCode(Value);
			}

			public override string ToString()
			{
				return Value;
			}

			public static implicit operator String(string str) => new String(str);

			internal override Node Simplify(Dictionary<Condition, HashSet<string>> accountedForValues = null)
			{
				return this;
			}

			internal override ConditionValueSet GetValueConditions()
			{
				return ConditionValueSet.True;
			}

			internal override Node ReduceByConditions(ConditionValueSet conditionValueSet)
			{
				return this;
			}

			internal override Node ExcludeConditions(ConditionValueSet conditionValueSet)
			{
				if (conditionValueSet == ConditionValueSet.True)
				{
					return null;
				}
				return this;
			}

			internal override Dictionary<string, ConditionValueSet> Expand(ConditionValueSet forConditions = null, bool throwOnMissing = true)
			{
				return new Dictionary<string, ConditionValueSet> { { Value, ConditionValueSet.True } };
			}
		}

		internal sealed class Conditional : Node
		{
			internal readonly Condition Condition;
			internal readonly Dictionary<string, Node> SubNodes;

			internal Conditional(Condition condition, Dictionary<string, Node> subNodes)
			{
				Debug.Assert(condition != null, "Cannot create Conditional with null Condition!");
				Debug.Assert(subNodes != null, "Cannot create Conditional with null sub nodes dictionary!");
				Debug.Assert(subNodes.Any(), "Cannot create Conditional with zero sub node values!");
				Debug.Assert(subNodes.All(valueToNode => valueToNode.Value != null), "Composite node cannot have null conditional value"); // TODO: or can it? we might consider a "unset" case for certain feature
#if DEBUG
				foreach (KeyValuePair<string, Node> subNode in subNodes)
				{
					if (!condition.Values.Contains(subNode.Key))
					{
						Debug.Fail($"Sub node key {subNode.Key} not valid for Condition '{condition}'. Valid values are {string.Join(", ", condition.Values.Select(value => "'{value}'"))}.");
					}
					Node simplified = subNode.Value.Simplify(new Dictionary<Condition, HashSet<string>> { { condition, condition.Values.Where(key => key != subNode.Key).ToHashSet() } });
					if (!simplified.Equals(subNode.Value))
					{
						Debug.Fail($"Sub node can be simplifed.");
					}

					if (
						condition.Values.Count == subNodes.Keys.Count && !condition.Values.Except(subNodes.Keys).Any() &&
						subNodes.Values.Skip(1).All(value => value == subNodes.Values.First())
					)
					{
						Debug.Fail("Conditional node not required - can be simplified.");
					}
				}
#endif

				Condition = condition;
				SubNodes = new Dictionary<string, Node>(subNodes);
			}

			public override bool Equals(object obj)
			{
				if (!(obj is Conditional conditional))
				{
					return false;
				}

				if (Condition != conditional.Condition)
				{
					return false;
				}

				return SubNodes.Count == conditional.SubNodes.Count && !SubNodes.Except(conditional.SubNodes).Any();
			}

			public override int GetHashCode()
			{
				int hashCode = 1515073342;
				hashCode = hashCode * -1521134295 + Condition.GetHashCode();
				hashCode = hashCode * -1521134295 + SubNodes.Count(); // ignore subnodes contents - fall back to .Equals
				return hashCode;
			}

			public override string ToString()
			{
				if (SubNodes.Count <= 3)
				{
					return $"{Condition} ({string.Join(", ", SubNodes.OrderBy(subNode => subNode.Key).Select(subNode => $"{subNode.Key}: {subNode.Value}"))})";
				}
				else
				{
					return $"{Condition} ({string.Join(", ", SubNodes.Take(2).OrderBy(subNode => subNode.Key).Select(subNode => $"{subNode.Key}: {subNode.Value}").Append("..."))})";
				}
			}

			internal override Node Simplify(Dictionary<Condition, HashSet<string>> accountedForValues = null)
			{
				return MergeConditionalValues(Condition, SubNodes, accountedForValues);
			}

			internal override ConditionValueSet GetValueConditions()
			{
				ConditionValueSet combinedConditions = ConditionValueSet.False;
				foreach (KeyValuePair<string, Node> subNode in SubNodes)
				{
					ConditionValueSet subNodeValues = subNode.Value.GetValueConditions();
					if (subNodeValues == ConditionValueSet.True)
					{
						combinedConditions = combinedConditions.Or(Condition, subNode.Key);
					}
					else
					{
						combinedConditions = combinedConditions.Or(subNodeValues.And(Condition, subNode.Key));
					}
				}
				return combinedConditions;
			}

			internal override Node ReduceByConditions(ConditionValueSet conditionValueSet)
			{
				if (conditionValueSet == ConditionValueSet.False)
				{
					return null;
				}

				if (conditionValueSet == ConditionValueSet.True)
				{
					return this;
				}

				Dictionary<string, Node> validSubNodes = new Dictionary<string, Node>();
				foreach (ReadOnlyDictionary<Condition, string> conditionSet in conditionValueSet)
				{
					if (!conditionSet.TryGetValue(Condition, out string value))
					{
						return new Conditional(Condition, SubNodes.ToDictionary(kvp => kvp.Key, kvp => kvp.Value.ReduceByConditions(conditionValueSet)));
					}
					else if (SubNodes.TryGetValue(value, out Node subNode))
					{
						validSubNodes[value] = subNode;
					}
				}

				if (!validSubNodes.Any())
				{
					return null;
				}
				else if (validSubNodes.Count == 1)
				{
					return validSubNodes.Single().Value.ReduceByConditions(conditionValueSet);
				}
				else
				{
					return new Conditional(Condition, validSubNodes.ToDictionary(kvp => kvp.Key, kvp => kvp.Value.ReduceByConditions(conditionValueSet)));
				}
			}

			internal override Node ExcludeConditions(ConditionValueSet conditionValueSet)
			{
				// todo: this probably needs to be smarter
				Dictionary<string, Node> newSubNodes = null;
				foreach (KeyValuePair<string, Node> subNode in SubNodes)
				{
					bool include = true;
					foreach (ReadOnlyDictionary<Condition, string> set in conditionValueSet)
					{
						if (set.TryGetValue(Condition, out string value) && value == subNode.Key)
						{
							include = false;
							break;
						}
					}

					if (!include)
					{
						continue;
					}

					Node reducedSubNode = subNode.Value.ExcludeConditions(conditionValueSet);
					if (reducedSubNode != null)
					{
						newSubNodes = newSubNodes ?? new Dictionary<string, Node>();
						newSubNodes.Add(subNode.Key, reducedSubNode);
					}
				}
				if (newSubNodes == null)
				{
					return null;
				}
				return MergeConditionalValues(Condition, newSubNodes);
			}

			internal override Dictionary<string, ConditionValueSet> Expand(ConditionValueSet forConditions = null, bool throwOnMissing = true)
			{
				forConditions = forConditions ?? ConditionValueSet.True;

				Dictionary<string, ConditionValueSet> validValues;
				if (forConditions != ConditionValueSet.True)
				{
					validValues = new Dictionary<string, ConditionValueSet>();
					foreach (ReadOnlyDictionary<Condition, string> conditionSet in forConditions)
					{
						if (conditionSet.TryGetValue(Condition, out string value))
						{
							// TODO: ugly / inefficient - we're creating an object we already effectively have - need a better API
							ConditionValueSet conditions = ConditionValueSet.True;
							foreach (KeyValuePair<Condition, string> conditionValue in conditionSet)
							{
								conditions.And(conditionValue.Key, conditionValue.Value);
							}

							if (validValues.TryGetValue(value, out ConditionValueSet existingValidConditions))
							{
								conditions = conditions.Or(existingValidConditions);
							}
							validValues[value] = conditions;
						}
					}
				}
				else
				{
					validValues = Condition.Values.ToDictionary(value => value, value => ConditionValueSet.True);
				}

				Dictionary<string, ConditionValueSet> expansion = new Dictionary<string, ConditionValueSet>();
				foreach (KeyValuePair<string, ConditionValueSet> validToConditions in validValues)
				{
					if (!SubNodes.TryGetValue(validToConditions.Key, out Node node))
					{
						if (throwOnMissing)
						{
							throw new Exception(); // TODO
						}
					}
					else
					{
						foreach (KeyValuePair<string, ConditionValueSet> subExpand in node.Expand(validToConditions.Value, throwOnMissing))
						{
							ConditionValueSet combinedConditions = subExpand.Value.And(Condition, validToConditions.Key);
							if (expansion.TryGetValue(subExpand.Key, out ConditionValueSet expansionConditions))
							{
								expansion[subExpand.Key] = expansionConditions.Or(combinedConditions);
							}
							else
							{
								expansion.Add(subExpand.Key, combinedConditions);
							}
						}
					}
				}
				return expansion;
			}

			internal Node MergeUnresolvedConditions(Node node) // prefers this objects values over "node" values if there is an overlap
			{
				if (node is Conditional conditionalNode && conditionalNode.Condition.Equals(Condition))
				{
					Dictionary<string, Node> newSubNodes = new Dictionary<string, Node>();
					foreach (string validValue in Condition.Values)
					{

						if (SubNodes.TryGetValue(validValue, out Node existingNode))
						{
							if (conditionalNode.SubNodes.TryGetValue(validValue, out Node mergeNode))
							{
								newSubNodes.Add(validValue, mergeNode.Update(existingNode).Simplify());
							}
							else
							{
								newSubNodes.Add(validValue, existingNode);
							}
						}
						else if (conditionalNode.SubNodes.TryGetValue(validValue, out existingNode))
						{
							newSubNodes.Add(validValue, existingNode);
						}
					}
					return MergeConditionalValues(Condition, newSubNodes); // TODO - could this be more efficient by simplifying with accounted for nodes in the loop?
				}
				else
				{
					Dictionary<string, Node> newSubNodes = new Dictionary<string, Node>();
					foreach (string validValue in Condition.Values)
					{
						if (SubNodes.TryGetValue(validValue, out Node existingNode))
						{
							newSubNodes.Add(validValue, node.Update(existingNode));
						}
						else
						{
							newSubNodes.Add(validValue, node.Simplify(new Dictionary<Condition, HashSet<string>> { { Condition, new HashSet<string>(SubNodes.Keys) } }));
						}
					}
					return MergeConditionalValues(Condition, newSubNodes); // TODO - could this be more efficient by simplifying with accounted for nodes in the loop?
				}
			}

			internal bool TryCombine(Conditional combine, out Node combinedNode, bool requireComplexityReduction = false)
			{
				if (Condition != combine.Condition)
				{
					combinedNode = null;
					return false;
				}
				else if (SubNodes.Keys.Any(key => combine.SubNodes.Keys.Contains(key)))
				{
					combinedNode = null;
					return false;
				}

				if (requireComplexityReduction)
				{
					// TODO: inefficient and unclear
					IEnumerable<KeyValuePair<string, Node>> combinedSubNodes = SubNodes.Concat(combine.SubNodes);
					if (
						combinedSubNodes.Count() == Condition.Values.Count &&
						!combinedSubNodes.Select(kvp => kvp.Key).Except(Condition.Values).Any() &&
						combinedSubNodes.GroupBy(kvp => kvp.Value).Count() == 1
					)
					{
						combinedNode = SubNodes.First().Value;
						return true;
					}
					else
					{
						combinedNode = null;
						return false;
					}
				}
				else
				{
					combinedNode = new Conditional(Condition, SubNodes.Concat(combine.SubNodes).ToDictionary(kvp => kvp.Key, kvp => kvp.Value));
					combinedNode = combinedNode.Simplify();
					return true;
				}
			}
		}

		internal sealed class Composite : Node
		{
			internal readonly Node[] SubNodes;

			internal Composite(params Node[] subNodes)
				: this((IEnumerable<Node>)subNodes)
			{
			}

			[System.Diagnostics.CodeAnalysis.SuppressMessage("Style", "IDE0019:Use pattern matching", Justification = "Visual Studio detects this incorrectly. Suggested style is not logically equivalent.")]
			internal Composite(IEnumerable<Node> subNodes)
			{
				Debug.Assert(subNodes.Count() >= 2, "A composite node must have at least 2 sub nodes.");
				Debug.Assert(subNodes.All(node => node != null), "Composite node cannot have null sub nodes.");
				Debug.Assert(!subNodes.Any(node => node is Composite), "Composite node cannot have composite sub nodes.");
#if DEBUG
				String lastString = subNodes.First() as String;
				foreach (Node node in subNodes.Skip(1))
				{
					String thisString = node as String;
					if (lastString != null && thisString != null)
					{
						Debug.Fail("Composite node cannot have two adjacent string nodes.");
					}
					lastString = thisString;
				}
#endif
				SubNodes = subNodes.ToArray();
			}

			public override bool Equals(object obj)
			{
				if (!(obj is Composite composite))
				{
					return false;
				}

				if (SubNodes.Length != composite.SubNodes.Length)
				{
					return false;
				}

				for (int i = 0; i < SubNodes.Length; ++i)
				{
					if (!SubNodes[i].Equals(composite.SubNodes[i]))
					{
						return false;
					}
				}

				return true;
			}

			public override int GetHashCode()
			{
				return SubNodes.Length; // fall back to equals
			}

			public override string ToString()
			{
				return string.Join
				(
					string.Empty, 
					SubNodes.Select
					(
						node =>
						{
							if (node is Conditional conditional)
							{
								return $"{{{conditional.ToString()}}}";
							}
							return node.ToString();
						}
					)
				);
			}

			internal override Node Simplify(Dictionary<Condition, HashSet<string>> accountedForValues = null)
			{
				IEnumerable<Node> simplifiedNodes = SubNodes.Select(node => node.Simplify(accountedForValues)).Where(node => node != null);
				return Concatenate(simplifiedNodes);
			}

			internal override ConditionValueSet GetValueConditions()
			{
				ConditionValueSet conditions = ConditionValueSet.False;
				foreach (Node subNode in SubNodes)
				{
					conditions = conditions.Or(subNode.GetValueConditions());
					if (conditions == ConditionValueSet.True)
					{
						return conditions;
					}
				}
				return conditions;
			}

			internal override Node ReduceByConditions(ConditionValueSet conditionValueSet)
			{
				IEnumerable<Node> reducedSubNodes = SubNodes
					.Select(node => node.ReduceByConditions(conditionValueSet))
					.Where(node => node != null);
				if (!reducedSubNodes.Any())
				{
					return null;
				}
				return Concatenate(reducedSubNodes);
			}

			internal override Node ExcludeConditions(ConditionValueSet conditionValueSet)
			{
				IEnumerable<Node> reducedSubNodes = SubNodes
					.Select(node => node.ExcludeConditions(conditionValueSet))
					.Where(node => node != null);
				if (!reducedSubNodes.Any())
				{
					return null;
				}
				return Concatenate(reducedSubNodes);
			}

			internal override Dictionary<string, ConditionValueSet> Expand(ConditionValueSet forConditions = null, bool throwOnMissing = true)
			{
				Dictionary<string, ConditionValueSet> expansion = SubNodes.First().Expand(forConditions, throwOnMissing);
				foreach (Node subNode in SubNodes.Skip(1))
				{
					Dictionary<string, ConditionValueSet> innerExpansion = new Dictionary<string, ConditionValueSet>();
					foreach (KeyValuePair<string, ConditionValueSet> accumulated in expansion)
					{
						foreach (KeyValuePair<string, ConditionValueSet> newExpansions in subNode.Expand(forConditions, throwOnMissing))
						{
							string combinedString = accumulated.Key + newExpansions.Key;
							ConditionValueSet combinedCondtions = accumulated.Value.And(newExpansions.Value);
							if (innerExpansion.TryGetValue(combinedString, out ConditionValueSet existingConditions))
							{
								innerExpansion[combinedString] = existingConditions.Or(combinedCondtions);
							}
							else
							{
								innerExpansion[combinedString] = combinedCondtions;
							}
						}
					}
					expansion = innerExpansion;
				}
				return expansion;
			}
		}

		internal static Node Join(string separator, IEnumerable<Node> nodes)
		{
			nodes = nodes ?? throw new ArgumentNullException(nameof(nodes));

			if (!nodes.Any())
			{
				return string.Empty;
			}

			String separatorNode = separator;
			IEnumerable<Node> separatedNodes = nodes
				.Skip(1)
				.SelectMany(node => new Node[] { separatorNode, node })
				.Prepend(nodes.First());
			return Concatenate(separatedNodes);
		}

		internal static Node MergeConditionalValues(Condition condition, Dictionary<string, Node> values, Dictionary<Condition, HashSet<string>> accountedForValues = null)
		{
			accountedForValues = accountedForValues ?? new Dictionary<Condition, HashSet<string>>();
			if (!accountedForValues.TryGetValue(condition, out HashSet<string> accountedForThisCondition))
			{
				accountedForThisCondition = new HashSet<string>();
			}

			// simplify all the subnodes, only allowing one value for current condition
			Dictionary<string, Node> simplifiedUnaccountedSubNodes = values
				.Where(kvp => !accountedForThisCondition.Contains(kvp.Key))
				.Select
				(
					kvp =>
					{
						Dictionary<Condition, HashSet<string>> nestedAccountedFor = new Dictionary<Condition, HashSet<string>>(accountedForValues)
						{
							[condition] = condition.Values.Where(key => key != kvp.Key).ToHashSet() // todo
							};
						return new KeyValuePair<string, Node>(kvp.Key, kvp.Value.Simplify(nestedAccountedFor));
					}
				)
				.ToDictionary(kvp => kvp.Key, kvp => kvp.Value);

			// if everything has been handled already then return null 
			if (!simplifiedUnaccountedSubNodes.Any())
			{
				/* this can happen in a case like the following:
				Conditional
				(
					MyCondition,
					"true": "alpha",
					"false": Composite
					(
						"beta" +
						Conditional				<- this simplifies to null, because MyCondition is already handle by root Conditonal
						(
							MyCondition,
							"true" : "gamma"
						)
					)
				)
				*/
				return null;
			}

			// if all values for the condition are accounted for already or
			// have the same then return that value, otherwise return the simplified
			// unnaccounted for nodes
			Node value = null;
			foreach (string validValue in condition.Values)
			{
				if (accountedForThisCondition.Contains(validValue))
				{
					continue;
				}
				else if (!simplifiedUnaccountedSubNodes.TryGetValue(validValue, out Node currentValue))
				{
					return new Conditional(condition, simplifiedUnaccountedSubNodes);
				}
				else if (value is null)
				{
					value = currentValue;
				}
				else if (!value.Equals(currentValue))
				{
					return new Conditional(condition, simplifiedUnaccountedSubNodes);
				}
			}
			return value;
		}

		internal static Node Concatenate(IEnumerable<Node> nodes) => Concatenate(nodes.ToArray());

		internal static Node Concatenate(params Node[] nodes)
		{
			Debug.Assert(!nodes.Any(node => node is null));

			List<Node>concatenatedNodes = null;
			Node lastNode = null;
			foreach (Node reducedNode in nodes)
			{
				if (reducedNode is String stringNode && lastNode is String lastStringNode)
				{
					lastNode = lastStringNode.Value + stringNode.Value;
				}
				else if (reducedNode is Conditional conditionalNode && lastNode is Conditional lastConditionalNode && conditionalNode.TryCombine(lastConditionalNode, out Node combinedNode))
				{
					lastNode = combinedNode;
				}
				else if (reducedNode is Composite compositeNode)
				{
					foreach (Node nestedSubNode in compositeNode.SubNodes)
					{
						if (nestedSubNode is String nestedStringSubNode && lastNode is String nestedLastStringNode)
						{
							lastNode = nestedLastStringNode.Value + nestedStringSubNode.Value;
						}
						else if (lastNode != null)
						{
							concatenatedNodes = concatenatedNodes ?? new List<Node>();
							concatenatedNodes.Add(lastNode);
							lastNode = nestedSubNode;
						}
						else
						{
							lastNode = nestedSubNode;
						}
					}
				}
				else if (lastNode != null)
				{
					concatenatedNodes = concatenatedNodes ?? new List<Node>();
					concatenatedNodes.Add(lastNode);
					lastNode = reducedNode;
				}
				else
				{
					lastNode = reducedNode;
				}
			}
			if (concatenatedNodes != null)
			{
				concatenatedNodes.Add(lastNode);
				return new Composite(concatenatedNodes);
			}
			return lastNode;
		}

		private static List<Node> InternalSplitToListWithTrailing(Node node, char[] delimArray, ref List<Node> trailingGroup, bool separateTrailing)
		{
			List<Node> finalNodes = new List<Node>();
			if (node is String stringNode)
			{
				int splitBegin = 0;
				int index = 0;
				CharEnumerator charEnumerator = stringNode.Value.GetEnumerator();
				while (charEnumerator.MoveNext())
				{
					if (delimArray.Contains(charEnumerator.Current))
					{
						if (splitBegin != index)
						{
							string splitString = stringNode.Value.Substring(splitBegin, index - splitBegin);
							List<Node> newNodeGroup = new List<Node> { splitString };
							if (trailingGroup != null)
							{
								newNodeGroup = CrossAppend(trailingGroup, newNodeGroup);
								newNodeGroup = MergeSplitSiblings(newNodeGroup);
								trailingGroup = null;
							}
							finalNodes.AddRange(newNodeGroup);
						}
						else if (!finalNodes.Any() && trailingGroup != null)
						{
							trailingGroup = MergeSplitSiblings(trailingGroup);
							finalNodes.AddRange(trailingGroup);
							trailingGroup = null;
						}
						splitBegin = index + 1;
					}
					++index;
				}
				if (splitBegin < index)
				{
					Node lastNode = stringNode.Value.Substring(splitBegin, index - splitBegin);
					List<Node> newtrailingGroup = new List<Node> { lastNode };
					if (trailingGroup != null)
					{
						newtrailingGroup = CrossAppend(trailingGroup, newtrailingGroup);
					}
					if (separateTrailing)
					{
						trailingGroup = newtrailingGroup;
					}
					else
					{
						finalNodes.AddRange(newtrailingGroup);
						trailingGroup = null;
					}
				}
			}
			else if (node is Conditional conditionalNode)
			{
				List<List<Node>> combinedSplitNodes = new List<List<Node>>();
				List<Node> combinedTrailingNodes = null;
				foreach (KeyValuePair<string, Node> subNode in conditionalNode.SubNodes)
				{
					List<Node> subNodeTrailing = trailingGroup;
					List<Node> subSplitNodes = InternalSplitToListWithTrailing(subNode.Value, delimArray, ref subNodeTrailing, separateTrailing)
						.Select
						(
							splitNode => new Conditional(conditionalNode.Condition, new Dictionary<string, Node> { { subNode.Key, splitNode } })
						)
						.Cast<Node>()
						.ToList();

					combinedSplitNodes.Add(subSplitNodes);

					if (subNodeTrailing != null)
					{
						subNodeTrailing = subNodeTrailing
							.Select(subTrailingNode => new Conditional(conditionalNode.Condition, new Dictionary<string, Node> { { subNode.Key, subTrailingNode } }))
							.Cast<Node>()
							.ToList();
						if (combinedTrailingNodes is null)
						{
							combinedTrailingNodes = subNodeTrailing;
						}
						else
						{
							combinedTrailingNodes.AddRange(subNodeTrailing);
						}
					}
				}

				trailingGroup = combinedTrailingNodes;
				finalNodes.AddRange(MergeSplitSiblings(ref combinedSplitNodes));
			}
			else if (node is Composite compositeNode)
			{
				foreach (Node subNode in compositeNode.SubNodes)
				{
					List<Node> subNodeSplits = InternalSplitToListWithTrailing(subNode, delimArray, ref trailingGroup, separateTrailing: true);
					finalNodes.AddRange(subNodeSplits);
				}
				if (!separateTrailing)
				{
					MergeSplitSiblings(trailingGroup);
					finalNodes.AddRange(trailingGroup);
					trailingGroup = null;
				}
			}
			else
			{
				throw new NotImplementedException(); // paranoia todo
			}
			return finalNodes;
		}

		private static List<Node> CrossAppend(List<Node> first, List<Node> second)
		{
			return first.SelectMany(firstEntry => second.Select(secondEntry => firstEntry.Append(secondEntry))).ToList();
		}

		private static List<Node> MergeSplitSiblings(List<Node> mergeSiblings)
		{
			// todo: eeeww
			List<List<Node>> wrapped = mergeSiblings.Select(node => new List<Node> { node }).ToList();
			return MergeSplitSiblings(ref wrapped);
		}

		// "ref" parameters here merely documents that we are going to mess with the list internals
		// we don't actually expect calling code to use the output - in fact it would probably break
		// if they tried
		private static List<Node> MergeSplitSiblings(ref List<List<Node>> mergeSiblings)
		{
			// get max merge depth, also we can optimize by seeing if all depths are the same
			// in which case we don't need a reverse merge
			int mergeWidth = mergeSiblings.Count();
			int mergeDepth = -1;
			bool needReverseMerge = false;
			foreach (List<Node> column in mergeSiblings)
			{
				int depth = column.Count();
				int newMergeDepth = Math.Max(mergeDepth, depth);
				if (mergeDepth != -1)
				{
					needReverseMerge = needReverseMerge || newMergeDepth != mergeDepth || depth != newMergeDepth;
				}
				mergeDepth = newMergeDepth;
			}
			int lowMergeDepth = needReverseMerge ? -mergeDepth : 0;
			bool[] requiredComplexityModes = needReverseMerge ? new bool[] { true, false } : new bool[] { false };

			foreach (bool requireComplexityReduction in requiredComplexityModes)
			{
				for (int mergeDepthIndex = mergeDepth - 1; mergeDepthIndex >= lowMergeDepth; --mergeDepthIndex)
				{
					for (int mergeFromBreadthIndex = 1; mergeFromBreadthIndex < mergeWidth; ++mergeFromBreadthIndex)
					{
						// find a merge from candidate
						List<Node> mergeFromColumn = mergeSiblings[mergeFromBreadthIndex];
						int fromColumnSize = mergeFromColumn.Count();
						int mergeFromDepthIndex = mergeDepthIndex >= 0 ? mergeDepthIndex : fromColumnSize + mergeDepthIndex;
						if (mergeFromDepthIndex >= fromColumnSize || mergeFromDepthIndex < 0)
						{
							continue;
						}

						Node mergeFromCandidate = mergeFromColumn[mergeFromDepthIndex];
						if (mergeFromCandidate == null)
						{
							continue;
						}

						for (int mergeToBreadthIndex = 0; mergeToBreadthIndex < mergeFromBreadthIndex; ++mergeToBreadthIndex)
						{
							// find a merge to candidate
							List<Node> mergeToColumn = mergeSiblings[mergeToBreadthIndex];
							int toColumnSize = mergeToColumn.Count();
							int mergeToDepthIndex = mergeDepthIndex >= 0 ? mergeDepthIndex : toColumnSize + mergeDepthIndex;
							if (mergeToDepthIndex >= toColumnSize || mergeToDepthIndex < 0)
							{
								continue;
							}
							if (mergeDepthIndex < 0 && mergeFromDepthIndex == mergeToDepthIndex)
							{
								continue; // this is negative index, we can skip if actual indexes are the same as we did this during positive index
							}

							Node mergeToCandidate = mergeToColumn[mergeToDepthIndex];
							if (mergeToCandidate == null)
							{
								continue;
							}

							// merge replacing both instances with a the same reference to merged value
							if (mergeToCandidate.Equals(mergeFromCandidate)) // TODO: can we skip this pass if require complexit red is false?
							{
								mergeFromColumn[mergeFromDepthIndex] = null; // null out merge from
								mergeToColumn[mergeToDepthIndex] = mergeToCandidate;
							}
							else if (
								mergeToCandidate is Conditional conditionalMergeTo &&
								mergeFromCandidate is Conditional conditionalMergeFrom &&
								conditionalMergeTo.TryCombine(conditionalMergeFrom, out Node mergeCombine, requireComplexityReduction)
							)
							{
								mergeFromColumn[mergeFromDepthIndex] = null; // null out merge from
								mergeToColumn[mergeToDepthIndex] = mergeCombine;
							}
						}
					}
				}
			}

			List<Node> finalNodes = new List<Node>();
			for (int mergeDepthIndex = 0; mergeDepthIndex < mergeDepth; ++mergeDepthIndex)
			{
				finalNodes.AddRange(mergeSiblings
					.Where(column => column.Count() > mergeDepthIndex)
					.Select(column => column[mergeDepthIndex])
					.Where(node => node != null));
			}
			return finalNodes;
		}
	}
}
 