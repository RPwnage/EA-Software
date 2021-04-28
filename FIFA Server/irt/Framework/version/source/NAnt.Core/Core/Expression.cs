// NAnt - A .NET build tool
// Copyright (C) 2003-2015 Electronic Arts Inc.
// Copyright (C) 2002-2003 Gerry Shaw
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
// 
// 
// File Maintainers
// Electronic Arts (Frostbite.Team.CM@ea.com)
// Gerry Shaw (gerry_shaw@yahoo.com)

//#define DEBUG_OUTPUT    //  define this to enable debugging output collection.
using System;
using System.Runtime.Serialization;
using System.Collections;
using System.Collections.Generic;
using System.Text;

using NAnt.Core.Util;

namespace NAnt.Core
{

	/// <summary>Thrown whenever an exception is invalid.</summary>
	[Serializable]
	public class ExpressionException : ApplicationException
	{
		public ExpressionException() : base() { }
		public ExpressionException(String message) : base(message) { }
		public ExpressionException(String message, Exception e) : base(message, e) { }
		public ExpressionException(SerializationInfo info, StreamingContext context) : base(info, context) { }
	}


	/// <summary>
	/// This class implements a string expression evaluator.
	/// </summary>
	/// <remarks><![CDATA[
	/// Definitions:
	///     boolean         ::  true false 
	///     binary_operator ::  (C operators for LESS LESS_EQUALS GREATER GREATER_EQUALS EQUALS NOT_EQUALS AND OR: stupid X_M_L)
	///     unary_operator  ::  !
	///     parentheses:    ::  ( )
	///     op_char         ::  binary_operator | unary_operator | parentheses
	///     string          ::  '[any chars]' | 
	///                         "[any chars]" | 
	///                         `[any chars]` |
	///                         [any string of chars that doesn't include an op_char and 
	///                         doesn't begin with with a boolean.]
	///                         In addition, any of the quote delimited forms can include the quote delimiter
	///                         if it is escaped by a backslash '\', (e.g. "this \" has a quote")
	///     expression      ::  boolean |
	///                         string  |
	///                         ( expression ) |
	///                         unary_operator expression |
	///                         expression binary_operator expression
	/// 
	/// Operator precedence (highest to lowest, those on same lines are equal):
	///     ()
	///     NOT
	///     LESS LESS_EQUALS GREATER GREATER_EQUALS
	///     EQUALS NOT_EQUALS
	///     AND
	///     OR  
	/// NOTES:
	///     Strings are Case Sensitive.
	///     An empty expression or a null expression generates an exception.                    
	/// ]]>
	/// </remarks>
	/// <exception cref="ExpressionException">Thrown when an illegal expression is input.</exception>

	public class Expression
	{

		/// <summary>Takes in a string expression, parsers and evaluates it down to a single value returned as a bool.</summary>
		/// <param name="expression">Input expression to evaluate.</param>
		/// <returns>Boolean value of the expression.</returns>
		public static bool Evaluate(string expression)
		{
			Lexer lexer = new Lexer(expression);
			Stack operands = new Stack();
			Stack<Operator> operators = new Stack<Operator>();

			lexer.SkipWhiteSpace();
			if (lexer.AtEnd)
				throw new ExpressionException("Empty expression detected" + lexer.OffsetInfo());

			// Parse the expression and evaluate it as we go.
			bool wantOp = false;
			do
			{
				if (wantOp)
				{
					if (lexer.GetRightParenthesis())
					{
						while (operators.Count > 0 && operators.Peek() != Operator.PARENTHESIS)
							operators.Pop().RunOperation(operands);
						if (operators.Count == 0)
							throw new ExpressionException("Unbalanced ')' detected" + lexer.OffsetInfo());
						//  Pop the '(' off the stack.
						operators.Pop();
					}
					else
					{
						Operator bop = lexer.GetBinaryOperator();
						if (bop == null)
							throw new ExpressionException("Expected binary operator" + lexer.OffsetInfo());
						while (operators.Count > 0 && operators.Peek().Precedence <= bop.Precedence)
							operators.Pop().RunOperation(operands);
						operators.Push(bop);
						wantOp = false;
					}
				}
				else
				{
					Operator uop;
					object literal;
					if (lexer.GetLeftParenthesis())
					{
						operators.Push(Operator.PARENTHESIS);
					}
					else if ((uop = lexer.GetUnaryOperator()) != null)
					{
						operators.Push(uop);
					}
					else if ((literal = lexer.GetLiteral()) != null)
					{
						operands.Push(literal);
						wantOp = true;
					}
					else
						throw new ExpressionException("Expected value not found" + lexer.OffsetInfo());
				}
				lexer.SkipWhiteSpace();
			}
			while (!lexer.AtEnd);

			//  Complete the evaluation.
			while (operators.Count > 0 && operators.Peek() != Operator.PARENTHESIS)
				operators.Pop().RunOperation(operands);
			if (operators.Count > 0)
				throw new ExpressionException("Unbalanced '(' detected" + lexer.OffsetInfo());

			//  At this point, the operator stack should be empty and the operand stack should have the final result.
			bool? result = operands.Pop() as bool?;
			if (result == null)
				throw new ExpressionException("Result is not boolean" + lexer.OffsetInfo());

			return (bool)result;
		}

		private abstract class Operator
		{
			public readonly static Operator NOT = NotOperator.Instance;
			public readonly static Operator LESSTHAN = LessThanOperator.Instance;
			public readonly static Operator LESSOREQUAL = LessOrEqualOperator.Instance;
			public readonly static Operator GREATERTHAN = GreaterThanOperator.Instance;
			public readonly static Operator GREATEROREQUAL = GreaterOrEqualOperator.Instance;
			public readonly static Operator EQUAL = EqualOperator.Instance;
			public readonly static Operator NOTEQUAL = NotEqualOperator.Instance;
			public readonly static Operator AND = AndOperator.Instance;
			public readonly static Operator OR = OrOperator.Instance;
			public readonly static Operator PARENTHESIS = Parenthesis.Instance;

			/// <summary>
			/// Precedence of the operator, with 0 being the highest.
			/// </summary>
			public int Precedence { get; }

			public Operator(int precedence)
			{
				Precedence = precedence;
			}

			/// <summary>
			/// Runs the operation popping required operands from the stack and returning the result to it.
			/// </summary>
			/// <param name="stack">Given stack for operands and result.</param>
			public abstract void RunOperation(Stack stack);

			protected static bool IsBoolean(object value)
			{
				return value is bool;
			}

			protected static bool GetBoolean(object value)
			{
				bool? boolean = value as bool?;
				if (boolean != null)
					return (bool)boolean;
				else
					throw new ExpressionException("String detected when expecting boolean.");
			}

			protected static string GetString(object value)
			{
				return value.ToString();
			}
		}

		private abstract class UnaryOperator : Operator
		{
			public UnaryOperator(int precedence) : base(precedence) { }

			public sealed override void RunOperation(Stack stack) { stack.Push(RunOperation(stack.Pop())); }

			protected abstract object RunOperation(object op);
		}

		private abstract class BinaryOperator : Operator
		{
			public BinaryOperator(int precedence) : base(precedence) { }

			public sealed override void RunOperation(Stack stack)
			{
				object op2 = stack.Pop();
				object op1 = stack.Pop();
				stack.Push(RunOperation(op1, op2));
			}

			protected abstract object RunOperation(object op1, object op2);
		}

		private sealed class NotOperator : UnaryOperator
		{
			public static readonly Operator Instance = new NotOperator();

			private NotOperator() : base(1) { }

			protected override object RunOperation(object op) { return !GetBoolean(op); }
		}

		private sealed class LessThanOperator : BinaryOperator
		{
			public static readonly Operator Instance = new LessThanOperator();

			private LessThanOperator() : base(4) { }

			protected override object RunOperation(object op1, object op2)
			{
				return NumericalStringComparator.Compare(GetString(op1), GetString(op2)) < 0;
			}
		}

		private sealed class LessOrEqualOperator : BinaryOperator
		{
			public static readonly Operator Instance = new LessOrEqualOperator();

			private LessOrEqualOperator() : base(4) { }

			protected override object RunOperation(object op1, object op2)
			{
				return NumericalStringComparator.Compare(GetString(op1), GetString(op2)) <= 0;
			}
		}

		private sealed class GreaterThanOperator : BinaryOperator
		{
			public static readonly Operator Instance = new GreaterThanOperator();

			private GreaterThanOperator() : base(4) { }

			protected override object RunOperation(object op1, object op2)
			{
				return NumericalStringComparator.Compare(GetString(op1), GetString(op2)) > 0;
			}
		}

		private sealed class GreaterOrEqualOperator : BinaryOperator
		{
			public static readonly Operator Instance = new GreaterOrEqualOperator();

			private GreaterOrEqualOperator() : base(4) { }

			protected override object RunOperation(object op1, object op2)
			{
				return NumericalStringComparator.Compare(GetString(op1), GetString(op2)) >= 0;
			}
		}

		private sealed class EqualOperator : BinaryOperator
		{
			public static readonly Operator Instance = new EqualOperator();

			private EqualOperator() : base(5) { }

			protected override object RunOperation(object op1, object op2)
			{
				return IsBoolean(op1) || IsBoolean(op2) ? GetBoolean(op1) == GetBoolean(op2) :
										 NumericalStringComparator.Compare(GetString(op1), GetString(op2)) == 0;
			}
		}

		private sealed class NotEqualOperator : BinaryOperator
		{
			public static readonly Operator Instance = new NotEqualOperator();

			private NotEqualOperator() : base(5) { }

			protected override object RunOperation(object op1, object op2)
			{
				return IsBoolean(op1) || IsBoolean(op2) ? GetBoolean(op1) != GetBoolean(op2) :
										 NumericalStringComparator.Compare(GetString(op1), GetString(op2)) != 0;
			}
		}

		private sealed class AndOperator : BinaryOperator
		{
			public static readonly Operator Instance = new AndOperator();

			private AndOperator() : base(6) { }

			protected override object RunOperation(object op1, object op2)
			{
				return GetBoolean(op1) && GetBoolean(op2);
			}
		}

		private sealed class OrOperator : BinaryOperator
		{
			public static readonly Operator Instance = new OrOperator();

			private OrOperator() : base(7) { }

			protected override object RunOperation(object op1, object op2)
			{
				return GetBoolean(op1) || GetBoolean(op2);
			}
		}

		private sealed class Parenthesis : Operator
		{
			public static readonly Operator Instance = new Parenthesis();

			private Parenthesis() : base(8) { }

			public override void RunOperation(Stack stack) { }
		}

		/// <summary>
		/// Takes an input string expression and returns a queue of string tokens representing the expression.
		/// </summary>
		private class Lexer
		{
			public string Expression { get; }

			public int Offset { get; private set; }

			public bool AtEnd { get { return Offset == _length; } }

			public Lexer(string expression)
			{
				Expression = expression;
				Offset = 0;
				_length = expression.Length;
			}

			public void SkipWhiteSpace()
			{
				for (; Offset < _length && Char.IsWhiteSpace(Expression, Offset); ++Offset) ;
			}

			public bool GetLeftParenthesis()
			{
				if (Expression[Offset] == '(')
				{
					++Offset;
					return true;
				}
				else
					return false;
			}

			public bool GetRightParenthesis()
			{
				if (Expression[Offset] == ')')
				{
					++Offset;
					return true;
				}
				else
					return false;
			}

			public Operator GetUnaryOperator()
			{
				const string not = "not";
				int offset = Offset;

				if (Expression[offset] == '!')
				{
					// ensure we don't mistake '!=' for a unary not operator when the first property evalutates to an empty string.
					if (offset + 1 < _length && Expression[offset + 1] == '=')
					{
						return null;
					}

					++Offset;
					return Operator.NOT;
				}

				int offsetPlusNot = offset + 3;

				if (offsetPlusNot >= _length)
					return null;

				if (offsetPlusNot < _length && Expression[offsetPlusNot] != ' ')
					return null;

				char n = Expression[offset];
				char o = Expression[offset+1];
				char t = Expression[offset+2];
				if ((n == 'n' || n == 'N') && (o == 'o' || o == 'O') && (t == 't' || t == 'T'))
				{
					Offset += not.Length;
					return Operator.NOT;
				}
				else
					return null;
			}

			private bool IsOperatorChar(char ch)
			{
				return ch == '<' ||
					ch == '=' ||
					ch == '>' ||
					ch == '!' ||
					ch == '(' ||
					ch == ')' ||
					ch == '&' ||
					ch == '|';
			}
			public Operator GetBinaryOperator()
			{
				int startingOffset = Offset;
				if (Char.IsLetter(Expression, Offset))
				{
					StringBuilder opTextBuilder = new StringBuilder(3);
					do
					{
						opTextBuilder.Append(char.ToLower(Expression[Offset]));
						++Offset;
					}
					while (opTextBuilder.Length < 3 && Offset < _length && Char.IsLetter(Expression, Offset));
					if (Offset < _length && Char.IsLetter(Expression, Offset))
					{
						Offset = startingOffset;
						return null;
					}

					switch (opTextBuilder.ToString())
					{
						case "or": return  Operator.OR;
						case "and": return Operator.AND;
						case "neq": return Operator.NOTEQUAL;
						case "eq": return Operator.EQUAL;
						case "gt": return Operator.GREATERTHAN;
						case "lte": return Operator.LESSOREQUAL;
						case "lt": return Operator.LESSTHAN;
						case "gte": return Operator.GREATEROREQUAL;
						default: return null;
					}
				}
				else
				{
					char ch1 = Expression[Offset++];
					char ch2 = '\0';
					if (Offset < _length)
						ch2 = Expression[Offset++];

					switch (ch1)
					{
						case '=':
							return ch2 == '=' ? Operator.EQUAL : null;
						case '|':
							return ch2 == '|' ? Operator.OR : null;
						case '&':
							return ch2 == '&' ? Operator.OR : null;
						case '!':
							return ch2 == '=' ? Operator.NOTEQUAL : null;
						case '<':
							return ch2 == '=' ? Operator.LESSOREQUAL : Operator.LESSTHAN;
						case '>':
							return ch2 == '=' ? Operator.GREATEROREQUAL : Operator.GREATERTHAN;
						default:
							return null;
					}
				}
			}

			public unsafe object GetLiteral()
			{
				int offset = Offset;
				fixed (char* expr = Expression)
				{
					char charOnOffset = expr[offset];

					if (charOnOffset == '\'' || charOnOffset == '"' || charOnOffset == '`')
					{
						char closingQuote = charOnOffset;
						++offset;
						StringBuilder stringText = new StringBuilder();
						for (; offset < _length && expr[offset] != closingQuote; ++offset)
						{
							if (expr[offset] == '\\' && offset + 1 < _length && (expr[offset + 1] == '\\' || expr[offset + 1] == closingQuote))
								++offset;
							stringText.Append(expr[offset]);
						}

						Offset = offset;

						if (Offset == _length)
							throw new ExpressionException("Unmatched string delimiter [" + closingQuote + "]" + OffsetInfo());
						++Offset;
						return stringText.ToString();
					}
					else
					{
						int startingOffset = offset;
						while (true)
						{
							char c = expr[offset];
							if (Char.IsWhiteSpace(c) || c == 0 || c == '<' || c == '=' || c == '>' || c == '!' || c == '(' || c == ')' || c == '&' || c == '|' || c == '"')
								break;
							++offset;

						}

						Offset = offset;

						if ((offset - startingOffset) == 4 && 
							(expr[startingOffset + 0] == 't' || expr[startingOffset + 0] == 'T') &&
							(expr[startingOffset + 1] == 'r' || expr[startingOffset + 1] == 'R') &&
							(expr[startingOffset + 2] == 'u' || expr[startingOffset + 2] == 'U') &&
							(expr[startingOffset + 3] == 'e' || expr[startingOffset + 3] == 'E'))
							return true;

						if ((offset - startingOffset) == 5 &&
							(expr[startingOffset + 0] == 'f' || expr[startingOffset + 0] == 'F') &&
							(expr[startingOffset + 1] == 'a' || expr[startingOffset + 1] == 'A') &&
							(expr[startingOffset + 2] == 'l' || expr[startingOffset + 2] == 'L') &&
							(expr[startingOffset + 3] == 's' || expr[startingOffset + 3] == 'S') &&
							(expr[startingOffset + 4] == 'e' || expr[startingOffset + 4] == 'E'))
							return false;

						return new String(expr, startingOffset, offset - startingOffset);
					}
				}
			}

			/// <summary>
			/// Outputs some offset info for the exceptions.
			/// </summary>
			/// <returns>
			/// Formatted string that is the expression followed by another line below it which points to 
			/// the offset with a '^' character.
			/// </returns>  
			public string OffsetInfo()
			{
				StringBuilder ret = new StringBuilder(" at offset " + Offset + " in expression \n'" + Expression + "'\n ");
				for (int j = 0; j < Offset; ++j)
					ret.Append(' ');
				ret.Append('^');

				return ret.ToString();
			}

			private readonly int _length;
		}
	}
}
