using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace NAnt.LangTools
{
	internal static class Expression
	{
		internal static readonly Constant True = new Constant();
		internal static readonly Constant False = new Constant();

		internal enum Operation
		{
			Equals,
			NotEquals,
			GreaterThan,
			GreaterThanOrEqual,
			LessThan,
			LessThanOrEqual,
			Or,
			And,
			Not
		}

		internal abstract class Node
		{
		}

		internal sealed class Constant : Node
		{
			internal Constant() { }

			public override string ToString()
			{
				return (this == True).ToString();
			}
		}

		internal sealed class String : Node
		{
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

		internal sealed class NotOperator : Node
		{
			internal readonly Node Operand;

			internal NotOperator(Node operand)
			{
				Operand = operand;
			}

			public override string ToString()
			{
				return $"!({Operand.ToString()})";
			}
		}

		internal sealed class BinaryOperator : Node
		{
			internal readonly Operation Operation;
			internal readonly Node FirstOperand;
			internal readonly Node SecondOperand;

			private static readonly Dictionary<Operation, string> s_operatorToString = new Dictionary<Operation, string>()
			{
				{ Operation.Equals, "==" },
				{ Operation.Or, "||" },
				{ Operation.And, "&&" },
				{ Operation.NotEquals, "!=" },
				{ Operation.GreaterThan, ">" },
				{ Operation.GreaterThanOrEqual, ">=" },
				{ Operation.LessThan, "<" },
				{ Operation.LessThanOrEqual, "<=" }
			};

			internal BinaryOperator(Operation operation, Node firstOperand, Node secondOperand)
			{
				Operation = operation;
				FirstOperand = firstOperand;
				SecondOperand = secondOperand;
			}

			public override string ToString()
			{
				return $"({FirstOperand.ToString()} {s_operatorToString[Operation]} {SecondOperand.ToString()})";
			}
		}

		internal static Node Evaluate(string expression)
		{
			CharEnumerator charEnumerator = expression.GetEnumerator();

			int index = -1;
			int tokenEnd = -1;
			char last = default;
			bool needBinaryOperator = false;

			Stack<Node> operandStack = new Stack<Node>();
			Stack<Operation> operatorStack = new Stack<Operation>();

			while (charEnumerator.MoveNext())
			{
				++index;
				if (Char.IsWhiteSpace(charEnumerator.Current))
				{
					int constBegin = tokenEnd + 1;
					if (index != constBegin)
					{
						needBinaryOperator = HandleExpressionToken(expression.Substring(constBegin, index - constBegin), needBinaryOperator, ref operatorStack, ref operandStack);
					}
					tokenEnd = index;
				}
				else
				{
					int operatorSize = -1;
					Operation currentOperator = default;
					switch (charEnumerator.Current)
					{
						case ')':
							throw new FormatException(); // todo
						case '(':
							throw new NotImplementedException();
						case '=' when last == '=':
							currentOperator = Operation.Equals;
							operatorSize = 2;
							break;
						case '|' when last == '|':
							currentOperator = Operation.Or;
							operatorSize = 2;
							break;
						case '&' when last == '&':
							currentOperator = Operation.And;
							operatorSize = 2;
							break;
						case '=' when last == '!':
							currentOperator = Operation.NotEquals;
							operatorSize = 2;
							break;
						case '=' when last == '<':
							currentOperator = Operation.LessThanOrEqual;
							operatorSize = 2;
							break;
						case '=' when last == '>':
							currentOperator = Operation.GreaterThanOrEqual;
							operatorSize = 2;
							break;
						case '\'' when last != '\\':
						case '`' when last != '\\':
						case '\"' when last != '\\':
							needBinaryOperator = HandleOperand(EvaluateString(expression, charEnumerator, ref index, charEnumerator.Current), needBinaryOperator, ref operatorStack, ref operandStack);
							tokenEnd = index;
							break;
						default:
							if (last == '!')
							{
								currentOperator = Operation.Not;
								operatorSize = 1;
							}
							else if (last == '<')
							{
								currentOperator = Operation.LessThan;
								operatorSize = 1;
							}
							else if (last == '>')
							{
								currentOperator = Operation.GreaterThan;
								operatorSize = 1;
							}
							break;
					}

					if (operatorSize > 0)
					{
						int constBegin = tokenEnd + 1;
						int operatorBegin = index - 1; // we always to have to look one past the operator to know if it's a 2 char operator, so go back 1
						if (operatorBegin != constBegin)
						{
							needBinaryOperator = HandleExpressionToken(expression.Substring(constBegin, operatorBegin - constBegin), needBinaryOperator, ref operatorStack, ref operandStack);
						}
						tokenEnd = operatorBegin + operatorSize - 1;
						needBinaryOperator = HandleOperator(currentOperator, needBinaryOperator, ref operatorStack, ref operandStack);
					}
				}

				last = charEnumerator.Current;
			}

			if (index < 0)
			{
				return HandleStringOrConstant(expression);
			}

			int lastConstBegin = tokenEnd + 1;
			int beyondEnd = index + 1;
			if (lastConstBegin != beyondEnd)
			{
				HandleExpressionToken(expression.Substring(lastConstBegin, beyondEnd - lastConstBegin), needBinaryOperator, ref operatorStack, ref operandStack);
			}

			while (operatorStack.Any())
			{
				Operation previousOperation = operatorStack.Pop();
				Node secondOperand = operandStack.Pop();
				Node firstOperand = operandStack.Pop();

				operandStack.Push(new BinaryOperator(previousOperation, firstOperand, secondOperand));
			}

			return operandStack.Pop();
		}

		internal static Node Reduce(Node node)
		{
			if (node is Constant || node is String)
			{
				return node;
			}
			else if (node is NotOperator unaryOperator)
			{
				if (unaryOperator.Operand == True)
				{
					return False;
				}
				else if (unaryOperator.Operand == False)
				{
					return True;
				}
				else if (unaryOperator.Operand is NotOperator unaryOperand)
				{
					return Reduce(unaryOperand.Operand);
				}
				else
				{
					return unaryOperator;
				}
			}
			else if (node is BinaryOperator binaryOperator)
			{
				Node reducedFirstOperand = Reduce(binaryOperator.FirstOperand);
				Node reducedSecondOperand = Reduce(binaryOperator.SecondOperand);

				switch (binaryOperator.Operation)
				{
					case Operation.Not:
						throw new ArgumentException(); // paranoia, we should ever get here // TODO

					case Operation.And when reducedFirstOperand == False || reducedSecondOperand == False:
						return False;
					case Operation.And when reducedFirstOperand == True:
						return reducedSecondOperand;
					case Operation.And when reducedSecondOperand == True:
						return reducedFirstOperand;

					case Operation.Or when reducedFirstOperand == True || reducedSecondOperand == True:
						return True;
					case Operation.Or when reducedFirstOperand == False:
						return reducedSecondOperand;
					case Operation.Or when reducedSecondOperand == False:
						return reducedFirstOperand;

					case Operation.Equals when reducedFirstOperand == True:
						return reducedSecondOperand;
					case Operation.Equals when reducedSecondOperand == True:
						return reducedFirstOperand;
					case Operation.Equals when reducedFirstOperand == False:
						return Reduce(Not(reducedSecondOperand));
					case Operation.Equals when reducedSecondOperand == False:
						return Reduce(Not(reducedFirstOperand));
					case Operation.Equals when reducedFirstOperand is String firstString && reducedSecondOperand is String secondString:
						return firstString.Value == secondString.Value ? True : False;

					case Operation.NotEquals when reducedFirstOperand == True:
						return Reduce(Not(reducedSecondOperand));
					case Operation.NotEquals when reducedSecondOperand == True:
						return Reduce(Not(reducedFirstOperand));
					case Operation.NotEquals when reducedFirstOperand == False:
						return reducedSecondOperand;
					case Operation.NotEquals when reducedSecondOperand == False:
						return reducedFirstOperand;
					case Operation.NotEquals when reducedFirstOperand is String firstString && reducedSecondOperand is String secondString:
						return firstString.Value != secondString.Value ? True : False;
					case Operation.GreaterThan:
						return ReduceComparison(reducedFirstOperand, reducedSecondOperand,
							(first, second) => first > second);
					case Operation.GreaterThanOrEqual:
						return ReduceComparison(reducedFirstOperand, reducedSecondOperand,
							(first, second) => first >= second);
					case Operation.LessThan:
						return ReduceComparison(reducedFirstOperand, reducedSecondOperand,
							(first, second) => first < second);
					case Operation.LessThanOrEqual:
						return ReduceComparison(reducedFirstOperand, reducedSecondOperand,
							(first, second) => first <= second);
					default:
						throw new NotImplementedException(); // todo: handle more cases
				}
			}
			else
			{
				throw new InvalidOperationException(); // todo
			}
		}

		private static Node Not(Node operand)
		{
			return new NotOperator(operand);
		}

		private static Node ReduceComparison(Node reducedFirstOperand, Node reducedSecondOperand, Func<int, int, bool> comparison)
		{
			if (
				reducedFirstOperand is String firstString &&
				reducedSecondOperand is String secondString
			)
			{
				int compareValue = Compare(firstString.Value, secondString.Value);
				if (comparison(compareValue, 0))
				{
					return True;
				}
				return False;
			}
			throw new NotImplementedException();
		}

		// from NumericalStringComparator in NAnt.Core.Util
		private static int Compare(string x, string y)
		{
			x = x ?? throw new ArgumentNullException(nameof(x));
			y = y ?? throw new ArgumentNullException(nameof(y));

			int xIndex = 0;
			int yIndex = 0;
			int xLength = x.Length;
			int yLength = y.Length;
			long diff = 0;
			while (diff == 0 && xIndex < xLength && yIndex < yLength)
			{
				diff = x[xIndex].CompareTo(y[yIndex]);
				if ((diff != 0 && (Char.IsDigit(x[xIndex]) || Char.IsDigit(y[yIndex]))) || (diff == 0 && Char.IsDigit(x[xIndex]) && Char.IsDigit(y[yIndex])))
				{
					long xNumber = -1;
					if (Char.IsDigit(x[xIndex]))
					{
						int startingIndex = xIndex;
						for (; xIndex < xLength && Char.IsDigit(x[xIndex]); ++xIndex) ;
						xNumber = Convert.ToInt64(x.Substring(startingIndex, xIndex - startingIndex));
					}

					long yNumber = -1;
					if (Char.IsDigit(y[yIndex]))
					{
						int startingIndex = yIndex;
						for (; yIndex < yLength && Char.IsDigit(y[yIndex]); ++yIndex) ;
						yNumber = Convert.ToInt64(y.Substring(startingIndex, yIndex - startingIndex));
					}
					diff = xNumber - yNumber;
				}
				else
				{
					++xIndex;
					++yIndex;
				}
			}

			int comparison;
			if (diff == 0)
			{
				if (xIndex == xLength && yIndex < yLength)
				{
					comparison = -1;
				}
				else if (xIndex < xLength && yIndex == yLength)
				{
					comparison = 1;
				}
				else
				{
					comparison = 0;
				}
			}
			else if (diff < 0)
			{
				comparison = -1;
			}
			else
			{
				comparison = 1;
			}

			return comparison;
		}

		private static Node EvaluateString(string expression, CharEnumerator charEnumerator, ref int index, char endQuote)
		{
			StringBuilder builder = null;
			char last = default;
			int tokenEnd = index;

			while (true)
			{
				if (!charEnumerator.MoveNext())
				{
					throw new FormatException(); // todo
				}

				++index;
				if (last == '\\')
				{
					if (charEnumerator.Current == endQuote || charEnumerator.Current == '\\')
					{
						int constBegin = tokenEnd + 1;
						int escapeBegin = index - 1;
						if (constBegin != escapeBegin)
						{
							builder.Append(expression.Substring(constBegin, escapeBegin - constBegin));
						}
						builder.Append(charEnumerator.Current);
						tokenEnd = index;
					}
				}
				else if (charEnumerator.Current == endQuote)
				{
					int constBegin = tokenEnd + 1;
					string trailing = expression.Substring(constBegin, index - constBegin);
					if (builder == null)
					{
						return new String(trailing);
					}
					else
					{
						builder.Append(trailing);
						return new String(builder.ToString());
					}
				}

				last = charEnumerator.Current;
			}
		}

		private static bool HandleExpressionToken(string subString, bool needBinaryOperator, ref Stack<Operation> operatorStack, ref Stack<Node> operandStack)
		{
			if (IsOperator(subString, out Operation op))
			{
				return HandleOperator(op, needBinaryOperator, ref operatorStack, ref operandStack);
			}
			else
			{
				return HandleOperand(HandleStringOrConstant(subString), needBinaryOperator, ref operatorStack, ref operandStack);
			}
		}

		private static Node HandleStringOrConstant(string expression)
		{
			if (Boolean.TryParse(expression, out bool asBool))
			{
				return asBool ? True : False;
			}

			return new String(expression);
		}

		private static bool HandleOperand(Node operand, bool needBinaryOperator, ref Stack<Operation> operatorStack, ref Stack<Node> operandStack)
		{
			if (needBinaryOperator)
			{
				throw new FormatException(); // todo
			}

			while (operatorStack.Any() && IsUnaryOperator(operatorStack.Peek()))
			{
				operatorStack.Pop();
				operand = new NotOperator(operand);
			}

			operandStack.Push(operand);
			return true;
		}

		private static bool HandleOperator(Operation op, bool needBinaryOperator, ref Stack<Operation> operatorStack, ref Stack<Node> operandStack)
		{
			if (IsUnaryOperator(op))
			{
				if (needBinaryOperator)
				{
					throw new FormatException(); // todo
				}
				operatorStack.Push(op);
				return true;
			}

			uint precedence = GetOperatorPrecedence(op);
			while (operatorStack.Any() && GetOperatorPrecedence(operatorStack.Peek()) > precedence)
			{
				Operation previousOperation = operatorStack.Pop();
				Node secondOperand = operandStack.Pop();
				Node firstOperand = operandStack.Pop();

				operandStack.Push(new BinaryOperator(previousOperation, firstOperand, secondOperand));
			}
			operatorStack.Push(op);

			return false;
		}

		private static bool IsOperator(string operatorString, out Operation op)
		{
			string lowerString = operatorString.ToLowerInvariant();
			switch (lowerString)
			{
				case "||":
				case "or":
					op = Operation.Or;
					return true;
				case "&&":
				case "and":
					op = Operation.And;
					return true;
				case "!=":
				case "neq":
					op = Operation.NotEquals;
					return true;
				case "==":
				case "eq":
					op = Operation.Equals;
					return true;
				case ">":
				case "gt":
					op = Operation.GreaterThan;
					return true;
				case ">=":
				case "gte":
					op = Operation.GreaterThanOrEqual;
					return true;
				case "<":
				case "lt":
					op = Operation.LessThan;
					return true;
				case "<=":
				case "lte":
					op = Operation.LessThanOrEqual;
					return true;
				case "!":
					op = Operation.Not;
					return true;
				default:
					op = default;
					return false;
			}
		}

		private static bool IsUnaryOperator(Operation operation)
		{
			return operation == Operation.Not;
		}

		private static uint GetOperatorPrecedence(Operation operation)
		{
			switch (operation)
			{
				case Operation.LessThan:
				case Operation.LessThanOrEqual:
				case Operation.GreaterThan:
				case Operation.GreaterThanOrEqual:
					return 4;
				case Operation.Equals:
				case Operation.NotEquals:
					return 3;
				case Operation.And:
					return 2;
				case Operation.Or:
					return 1;
				default:
					return 0;
			}
		}
	}
}