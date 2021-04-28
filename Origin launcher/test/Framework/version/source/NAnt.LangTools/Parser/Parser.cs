using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace NAnt.LangTools
{
	public partial class Parser
	{
		private sealed class ConditionStack
		{
			internal readonly Stack<ConditionValueSet> m_stack = new Stack<ConditionValueSet>(new ConditionValueSet[] { ConditionValueSet.True });

			internal void Push(ConditionValueSet newValues)
			{
				m_stack.Push(m_stack.Peek().And(newValues));
			}

			internal void Pop()
			{
				if (m_stack.Count == 1)
				{
					throw new InvalidOperationException(); // TODO
				}
				m_stack.Pop();
			}

			internal ConditionValueSet Peek()
			{
				return m_stack.Peek();
			}
		}

		internal readonly string PackageName;
		internal readonly string PackageDirToken;

		internal IReadOnlyDictionary<string, Property.Node> Properties => m_properties;
		internal IReadOnlyDictionary<string, OptionSet.Map.Entry> OptionSets => m_optionSets;

		private readonly Property.Map m_properties = new Property.Map();
		private readonly OptionSet.Map m_optionSets = new OptionSet.Map();

		private readonly Stack<Property.Map> m_localProperties = new Stack<Property.Map>();

		private readonly ConditionStack m_conditionStack = new ConditionStack();

		private const string BuildGroupPrivateProperty = "__private_structured_buildgroup";

		private Parser(string packageName, string packageBuildFile, params Feature[] features)
		{
			PackageName = packageName;
			PackageDirToken = $"%PackageRootPlaceHolder[{PackageName}]%";

			m_localProperties.Push(new Property.Map()); // always keep one map on the stack to simplify the logic

			m_properties.SetProperty("nant.version", new Property.String("8.01.00")); // todo - ignore pre 8.1 stuff for now, but think about smarter handling later
			m_properties.SetProperty("Dll", "false"); // this always starts off false - package task changes it's value to true or false depending on build settings

			foreach (Feature feature in features)
			{
				foreach (KeyValuePair<string, Property.Node> property in feature.Properties)
				{
					m_properties.SetProperty(property.Key, property.Value);
				}
			}

			IncludeFile(packageBuildFile);
		}

		public static Parser Parse(string packageName, string packageBuildFile, params Feature[] features) // TODO temp testing api
		{
			return new Parser(packageName, packageBuildFile, features);
		}

		private Property.Node ResolveScriptEvaluation(ScriptEvaluation.Node scriptEvalNode, Func<string, Property.Node> resolveProperty = null)
		{
			if (scriptEvalNode is ScriptEvaluation.String constNode)
			{
				return new Property.String(constNode.Value);
			}
			else if (scriptEvalNode is ScriptEvaluation.Evaluation evalNode)
			{
				Property.Node resolvedName = ResolveScriptEvaluation(evalNode.PropertyName, resolveProperty);
				if (resolvedName is Property.String resolvedString)
				{
					Property.Node evaluated = resolveProperty?.Invoke(resolvedString.Value) ??
						GetPropertyOrNull(resolvedString.Value) ??
						throw new KeyNotFoundException($"Cannot resolve property '{resolvedString.Value}' in current context."); // TODO: need more info in error - what is "current context"?

					return evaluated;
				}
				throw new NotImplementedException();
			}
			else if (scriptEvalNode is ScriptEvaluation.Composite compositeNode)
			{
				IEnumerable<Property.Node> evaluatedSubNodes = compositeNode.SubNodes.Select(subEvalNode => ResolveScriptEvaluation(subEvalNode, resolveProperty));
				return Property.Concatenate(evaluatedSubNodes);
			}
			else if (scriptEvalNode is ScriptEvaluation.Function functionElement)
			{
				return ResolveFunction(functionElement.FunctionName, functionElement.Parameters.Select(param => ResolveScriptEvaluation(param, resolveProperty)).ToArray());
			}
			else
			{
				throw new InvalidOperationException(); // TODO
			}
		}

		private Property.Node ResolveFunction(string functionName, Property.Node[] arguments)
		{
			// TODO: hacky hardcoding for now - setup function registry or similar
			switch (functionName)
			{
				case "StrCompareVersions":
					if (arguments.Length != 2)
					{
						throw new ArgumentException(); // todo
					}

					Property.Node nodeA = arguments[0];
					Property.Node nodeB = arguments[1];

					Property.Node returnNode = null;

					Dictionary<string, ConditionValueSet> expandedA = nodeA.Expand(m_conditionStack.Peek());
					Dictionary<string, ConditionValueSet> expandedB = nodeB.Expand(m_conditionStack.Peek());

					foreach (KeyValuePair<string, ConditionValueSet> aExpansion in expandedA)
					{
						foreach (KeyValuePair<string, ConditionValueSet> bExpansion in expandedB)
						{
							int compare = StrCompareVersions(aExpansion.Key, bExpansion.Key);

							Property.Node newNode = new Property.String(compare.ToString()).AttachConditions(aExpansion.Value.And(bExpansion.Value));
							returnNode = returnNode?.Update(newNode) ?? newNode;
						}
					}
					return returnNode;
				case "PropertyExists":
					if (arguments.Length != 1)
					{
						throw new ArgumentException(); // todo
					}

					Property.Node propertyNameNode = arguments[0];

					ConditionValueSet trueConditions = ConditionValueSet.False;

					Dictionary<string, ConditionValueSet> propertyNameExpanded = propertyNameNode.Expand(m_conditionStack.Peek()); // todo - could probably be more efficient with something bespoke here?
					foreach (KeyValuePair<string, ConditionValueSet> nameExpansion in propertyNameExpanded)
					{
						if (!Properties.TryGetValue(nameExpansion.Key, out Property.Node existingNode))
						{
#if DEBUG
							throw new KeyNotFoundException($"Cannot find property '{nameExpansion.Key}' in PropertyExists call in any context. This may be an undetected global control flow property."); // TODO
#else
							// todo: log warning that we have no knowledge of this property
							existingNode = null;
#endif
						}
						else
						{
							existingNode = existingNode.ReduceByConditions(nameExpansion.Value);
						}
						
						if (existingNode != null)
						{
							Dictionary<string, ConditionValueSet> propertyValueExpanded = propertyNameNode.Expand(m_conditionStack.Peek());
							foreach (KeyValuePair<string, ConditionValueSet> valueExpansion in propertyValueExpanded)
							{
								trueConditions = trueConditions.Or(nameExpansion.Value.And(valueExpansion.Value));
							}
						}
					}

					if (trueConditions == ConditionValueSet.False)
					{
						return "false";
					}
					else if (trueConditions == ConditionValueSet.True)
					{
						return "true";
					}
					else
					{
						return new Property.String("false").Update(new Property.String("true").AttachConditions(trueConditions));
					}
				default:
					throw new NotImplementedException($"No implementation for function '{functionName}'."); // todo
			}
		}

		private Property.Node GetPropertyOrNull(string propertyName)
		{
			Property.Node property = m_properties.GetPropertyOrNull(propertyName);
			Property.Node localProperty = m_localProperties.Peek().GetPropertyOrNull(propertyName);
			if (localProperty != null)
			{
				if (property != null)
				{
					property = property.Update(localProperty); // local property might not have as wide conditions as the non-local - so combine preferring local
				}
				else
				{
					property = localProperty;
				}
			}
			return property?.ReduceByConditions(m_conditionStack.Peek());
		}

		private void SetProperty(string name, Property.Node property, bool local)
		{
			property = ExpandByCurrentConditions(property);
			if (local)
			{
				m_localProperties.Peek().SetProperty(name, property);
			}
			else
			{
				Property.Node localProperty = m_localProperties.Peek().GetPropertyOrNull(name);
				if (localProperty != null)
				{
					// TODO: ugly, can do this in a more efficient way if we bundle this into a separate operation
					ConditionValueSet localConditions = localProperty.GetValueConditions();
					Property.Node newLocalProperty = property.ReduceByConditions(localConditions); // TODO do we need to expand by current again? what if there is overlap between current and local?
					if (newLocalProperty != null)
					{
						m_localProperties.Peek().SetProperty(name, property);
					}
					property = property.ExcludeConditions(localConditions);
					if (property != null)
					{
						m_properties.SetProperty(name, property);
					}
				}
				else
				{
					m_properties.SetProperty(name, property);
				}
			}
		}

		private void RemoveProperty(string property)
		{
			// TODO - probably need to remove only in the current condition context
			m_localProperties.Peek().Remove(property);
			m_properties.Remove(property);
		}

		private OptionSet GetOptionSetOrNull(string name)
		{
			if (m_optionSets.TryGetValue(name, out OptionSet set, m_conditionStack.Peek()))
			{
				return set;
			}
			return null;
		}

		private void SetOptionSet(string name, OptionSet newSet)
		{
			m_optionSets.SetOptionSet(name, newSet, m_conditionStack.Peek());
		}

		private Property.Node ExpandByCurrentConditions(Property.Node node)
		{
			return node.AttachConditions(m_conditionStack.Peek());
		}

		private void ResolvePropertyNode(Property.Node node, Action<string> onResolve)
		{
			foreach (KeyValuePair<string, ConditionValueSet> valueToConditions in node.Expand(m_conditionStack.Peek()))
			{
				m_conditionStack.Push(valueToConditions.Value);
				onResolve(valueToConditions.Key);
				m_conditionStack.Pop();
			}
		}

		private static Dictionary<string, TaskHandler> CombineTaskHandlers(Dictionary<string, TaskHandler> left, Dictionary<string, TaskHandler> right)
		{
			Dictionary<string, TaskHandler> combinedHandlers = new Dictionary<string, TaskHandler>();
			foreach (KeyValuePair<string, TaskHandler> handler in left.Concat(right))
			{
				if (combinedHandlers.ContainsKey(handler.Key))
				{
					throw new InvalidOperationException($"Cannot have duplicated handlers for key '{handler.Key}'.");
				}
				combinedHandlers[handler.Key] = handler.Value;
			}
			return combinedHandlers;
		}

		// TODO: move functions to thier own file / namespace / thing
		// copy paste from FW, but with a few syntax modernizations
		private static int StrCompareVersions(string strA, string strB)
		{
			int iA = 0;
			int iB = 0;

			if (strA is null)
			{
				throw new ArgumentNullException("StrCompareVersions: null comparison for first argument, strA.");
			}

			if (strB is null)
			{
				throw new ArgumentNullException("StrCompareVersions: null comparison for second argument, strB.");
			}

			if (strA != String.Empty)
			{
				if (strA.StartsWith("work", StringComparison.Ordinal)) strA = "999999" + strA.Substring(4);
				if (strA.StartsWith("dev", StringComparison.Ordinal)) strA = "999998" + strA.Substring(3);
				if (strA.StartsWith("stable", StringComparison.Ordinal)) strA = "999997" + strA.Substring(6);
			}
			if (strB != String.Empty)
			{
				if (strB.StartsWith("work", StringComparison.Ordinal)) strB = "999999" + strB.Substring(4);
				if (strB.StartsWith("dev", StringComparison.Ordinal)) strB = "999998" + strB.Substring(3);
				if (strB.StartsWith("stable", StringComparison.Ordinal)) strB = "999997" + strB.Substring(6);
			}

			if (ValueAt(strA, iA) == '-' || ValueAt(strB, iB) == '-') return 0; // treat options as same

			int diff = ValueAt(strA, iA) - ValueAt(strB, iB);

			while (iA <= strA.Length)
			{
				if (Char.IsDigit(ValueAt(strA, iA)) || Char.IsDigit(ValueAt(strB, iB)))
				{
					int n0 = -1;
					if (Char.IsDigit(ValueAt(strA, iA)))
					{
						StringBuilder digits = new StringBuilder();

						while (iA < strA.Length && Char.IsDigit(ValueAt(strA, iA)))
						{
							digits.Append(strA[iA++]);
						}

						n0 = (int)Int64.Parse(digits.ToString());
					}

					int n1 = -1;
					if (Char.IsDigit(ValueAt(strB, iB)))
					{
						StringBuilder digits = new StringBuilder();

						while (iB < strB.Length && Char.IsDigit(ValueAt(strB, iB)))
						{
							digits.Append(strB[iB++]);
						}

						n1 = (int)Int64.Parse(digits.ToString());
					}
					diff = n0 - n1;
				}
				else
				{
					diff = ValueAt(strA, iA) - ValueAt(strB, iB);
					iA++;
					iB++;
				}

				if (diff != 0) break;
			}
			return diff;
		}

		private static char ValueAt(string str, int pos)
		{
			char val = '\0';
			if (pos < str.Length)
			{
				val = str[pos];
			}
			return val;
		}
	}
}