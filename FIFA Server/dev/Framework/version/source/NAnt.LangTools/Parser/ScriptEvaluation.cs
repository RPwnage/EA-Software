using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace NAnt.LangTools
{
	internal static class ScriptEvaluation
	{
		internal abstract class Node
		{
		}

		internal sealed class String : Node
		{
			internal static readonly String Null = new String(null);
			internal static readonly String Empty = new String(string.Empty);
			internal readonly string Value;

			internal String(string value)
			{
				Value = value;
			}

			public override string ToString()
			{
				return Value;
			}
		}

		internal sealed class Evaluation : Node
		{
			internal readonly Node PropertyName;
			internal readonly Node Default;

			public Evaluation(Node evaluationElement, Node defaultElement = null)
			{
				PropertyName = evaluationElement;
				Default = defaultElement;
			}

			public override string ToString()
			{
				if (Default is null)
				{
					return $"${{{PropertyName}}}";
				}
				else
				{
					return $"${{{PropertyName}??{Default}}}";
				}
			}
		}

		internal sealed class Function : Node
		{
			internal readonly string FunctionName;
			internal readonly Node[] Parameters;

			internal Function(string functionName, IEnumerable<Node> parameters)
			{
				FunctionName = functionName;
				Parameters = parameters.ToArray();
			}

			public override string ToString()
			{
				return $"@{{{FunctionName}({string.Join(", ", Parameters.Select(param => $"'{param}'"))})}}";
			}
		}

		internal sealed class Composite : Node
		{
			internal readonly Node[] SubNodes;

			internal Composite(IEnumerable<Node> elements)
			{
				SubNodes = elements.ToArray();
			}

			public override string ToString()
			{
				return string.Join(string.Empty, SubNodes.Select(s => s.ToString()));
			}
		}

		internal static Node Evaluate(string value)
		{
			value = value ?? throw new ArgumentNullException(nameof(value)); // TODO

			CharEnumerator charEnumerator = value.GetEnumerator();
			List<Node> nodeList = null;

			int index = -1;
			int tokenEnd = -1;
			char last = default;

			while (charEnumerator.MoveNext())
			{
				++index;
				switch (charEnumerator.Current)
				{
					case '{' when last == '$':
						{
							int evalBegin = index - 1;
							HandleNewCompositeNode(EvaluateEvaluation(value, charEnumerator, ref index), value, ref nodeList, tokenEnd + 1, evalBegin);
							tokenEnd = index;
						}
						break;
					case '{' when last == '@':
						{
							int evalBegin = index - 1;
							HandleNewCompositeNode(EvaluationFunction(value, charEnumerator, ref index), value, ref nodeList, tokenEnd + 1, evalBegin);
							tokenEnd = index;
						}
						break;
					default:
						break;
				}
				last = charEnumerator.Current;
			}

			return HandleLastCompositeNode(value, ref nodeList, tokenEnd + 1, index + 1) ?? String.Empty;
		}

		private static Evaluation EvaluateEvaluation(string value, CharEnumerator charEnumerator, ref int index)
		{
			int tokenEnd = index;
			char last = default;

			Node evaluationElement = null;
			List<Node> nodeList = null;

			while (true)
			{
				if (!charEnumerator.MoveNext())
				{
					throw new FormatException(); // todo
				}

				++index;
				switch (charEnumerator.Current)
				{
					case '{' when last == '$':
						{
							int evalBegin = index - 1;
							HandleNewCompositeNode(EvaluateEvaluation(value, charEnumerator, ref index), value, ref nodeList, tokenEnd + 1, evalBegin);
							tokenEnd = index;
						}
						break;
					case '{' when last == '@':
						{
							int evalBegin = index - 1;
							HandleNewCompositeNode(EvaluationFunction(value, charEnumerator, ref index), value, ref nodeList, tokenEnd + 1, evalBegin);
							tokenEnd = index;
						}
						break;
					case '?' when last == '?':
					case '}':
						int lastConstBegin = tokenEnd + 1;
						int tokenBegin = charEnumerator.Current == '}' ? index : index - 1;
						Node innerNode = HandleLastCompositeNode(value, ref nodeList, lastConstBegin, tokenBegin);

						if (charEnumerator.Current == '}')
						{
							if (evaluationElement == null)
							{
								return new Evaluation(innerNode);
							}
							else
							{
								return new Evaluation(evaluationElement, innerNode);
							}
						}
						else
						{
							tokenEnd = index;
							if (evaluationElement != null)
							{
								throw new FormatException();
							}
							evaluationElement = innerNode;
							nodeList = null;
						}
						break;
					default:
						break;
				}

				last = charEnumerator.Current;
			}
		}

		private static Function EvaluationFunction(string value, CharEnumerator charEnumerator, ref int index)
		{
			int tokenEnd = index;

			// function name
			string functionName = null;
			while (true)
			{
				if (!charEnumerator.MoveNext())
				{
					throw new FormatException(); // todo
				}

				++index;
				if (Char.IsWhiteSpace(charEnumerator.Current) || charEnumerator.Current == '(')
				{
					int functionNameBegin = tokenEnd + 1;
					if (index != functionNameBegin)
					{
						if (functionName != null)
						{
							throw new FormatException(); // todo
						}
						functionName = value.Substring(functionNameBegin, index - functionNameBegin);
					}

					if (charEnumerator.Current == '(')
					{
						if (functionName == null)
						{
							throw new FormatException(); // todo
						}
						tokenEnd = index;
						break;
					}
					tokenEnd = index;
				}
				else if (!Char.IsLetterOrDigit(charEnumerator.Current))
				{
					throw new FormatException();
				}
			}

			// parameters
			List<Node> parameters = new List<Node>();
			List<Node> compositeParameterNodeList = null;
			bool needParameter = true;
			char last = default;

			while (true)
			{
				if (!charEnumerator.MoveNext())
				{
					throw new FormatException(); // todo
				}

				++index;
				if (Char.IsWhiteSpace(charEnumerator.Current))
				{
					Node node = HandleLastCompositeNode(value, ref compositeParameterNodeList, tokenEnd + 1, index);
					if (node != null)
					{
						parameters.Add(node);
					}
					tokenEnd = index;
				}
				else
				{
					if (charEnumerator.Current == ')')
					{
						Node node = HandleLastCompositeNode(value, ref compositeParameterNodeList, tokenEnd + 1, index);
						if (node != null)
						{
							parameters.Add(node);
						}
						break;
					}

					switch (charEnumerator.Current)
					{
						case '(':
							throw new NotImplementedException();
						case '{' when last == '$':
							{
								int evalBegin = index - 1;
								HandleNewCompositeNode(EvaluateEvaluation(value, charEnumerator, ref index), value, ref compositeParameterNodeList, tokenEnd + 1, evalBegin);
								tokenEnd = index;
							}
							break;
						case '{' when last == '@':
							{
								int evalBegin = index - 1;
								HandleNewCompositeNode(EvaluationFunction(value, charEnumerator, ref index), value, ref compositeParameterNodeList, tokenEnd + 1, evalBegin);
								tokenEnd = index;
							}
							break;
						case '\"':
						case '\'':
							parameters.Add(EvaluateStringParameter(value, charEnumerator, ref index, charEnumerator.Current));
							needParameter = false;
							tokenEnd = index;
							break;
						case ',':
							int paramBegin = tokenEnd + 1;
							Node node = HandleLastCompositeNode(value, ref compositeParameterNodeList, paramBegin, index);
							if (node != null)
							{
								parameters.Add(node);
								needParameter = false;
							}
							if (needParameter)
							{
								throw new FormatException();
							}
							tokenEnd = index;
							break;
					}
				}

				last = charEnumerator.Current;
			}

			// close
			while (true)
			{
				if (!charEnumerator.MoveNext())
				{
					throw new FormatException(); // todo
				}

				++index;
				if (charEnumerator.Current == '}')
				{
					return new Function(functionName, parameters);
				}
				else if (!Char.IsWhiteSpace(charEnumerator.Current))
				{
					throw new FormatException(); // todo
				}
			}
		}

		private static Node EvaluateStringParameter(string value, CharEnumerator charEnumerator, ref int index, char endQuote)
		{
			int tokenEnd = index;
			char last = default;

			StringBuilder builder = null;

			List<Node> nodeList = null;

			while (true)
			{
				if (!charEnumerator.MoveNext())
				{
					throw new FormatException(); // todo
				}

				++index;
				if (charEnumerator.Current == endQuote)
				{
					if (last == '\\')
					{
						builder = builder ?? new StringBuilder();
						builder.Append(value.Substring(tokenEnd + 1, index + 1));
						builder.Append(endQuote);
						tokenEnd = index;
					}
					else
					{
						return HandleLastCompositeNode(value, ref nodeList, tokenEnd + 1, index) ?? String.Empty;
					}
				}
				else if (charEnumerator.Current == '{' && last == '$')
				{								
					int evalBegin = index - 1;
					HandleNewCompositeNode(EvaluateEvaluation(value, charEnumerator, ref index), value, ref nodeList, tokenEnd + 1, evalBegin);
					tokenEnd = index;
				}
				else if (charEnumerator.Current == '{' && last == '@')
				{
					int evalBegin = index - 1;
					HandleNewCompositeNode(EvaluationFunction(value, charEnumerator, ref index), value, ref nodeList, tokenEnd + 1, evalBegin);
					tokenEnd = index;
				}

				last = charEnumerator.Current;
			}
		}

		private static void HandleNewCompositeNode(Node newNode, string value, ref List<Node> nodeList, int begin, int end)
		{
			nodeList = nodeList ?? new List<Node>();
			if (begin != end)
			{
				nodeList.Add(new String(value.Substring(begin, end - begin)));
			}
			nodeList.Add(newNode);
		}

		private static void HandleNewCompositeNode(Node newNode, string value, ref StringBuilder stringBuilder, ref List<Node> nodeList, int begin, int end)
		{
			nodeList = nodeList ?? new List<Node>();
			if (begin != end)
			{
				if (stringBuilder != null)
				{
					stringBuilder.Append(value.Substring(begin, end - begin));
				}
				else
				{
					nodeList.Add(new String(value.Substring(begin, end - begin)));
				}
			}

			if (stringBuilder != null)
			{
				nodeList.Add(new String(stringBuilder.ToString()));
			}

			stringBuilder = null;
			nodeList.Add(newNode);
		}

		private static Node HandleLastCompositeNode(string value, ref List<Node> nodeList, int begin, int end)
		{
			Node innerNode;
			if (nodeList == null)
			{
				if (end != begin)
				{
					return new String(value.Substring(begin, end - begin));
				}
				return null;
			}
			else
			{
				if (end != begin)
				{
					nodeList.Add(new String(value.Substring(begin, end - begin)));
				}

				if (nodeList.Count == 1)
				{
					innerNode = nodeList.First();
				}
				else
				{
					innerNode = new Composite(nodeList);
				}

				nodeList = null;
				return innerNode;
			}
		}
	}
}