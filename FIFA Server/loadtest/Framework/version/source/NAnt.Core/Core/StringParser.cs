// NAnt - A .NET build tool
// Copyright (C) 2003-2015 Electronic Arts Inc.
// Copyright (C) 2001-2002 Gerry Shaw
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

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace NAnt.Core
{
	public class StringParser
	{
		public delegate string PropertyEvaluator(string parameter, Stack<string> evaluationStack);
		public delegate string FunctionEvaluator(string functionName, List<string> parameterList);
		public delegate IEnumerable<string> FindClosePropertyMatches(string parameter);

		/// <summary>
		/// Expands properties and functions within a string.
		/// </summary>
		/// <remarks>
		///   <para>Can handle nested expressions like ${${a}}.</para>
		///   <para>Cannot use '${' in expressions without invoking expansion.</para>
		/// </remarks>
		/// <param name="expression">The string to expand.</param>
		/// <param name="evaluateProperty">The delegate function to call when a property needs evaluating.</param>
		/// <param name="evaluateFunction">The delegate function to call when a function needs evaluating.</param>
		/// <param name="findClosePropertyMatches">The delegate function to call when a property fails to evaluate and close matches need to be gound for error message</param>
		public static string ExpandString(string expression, PropertyEvaluator evaluateProperty, FunctionEvaluator evaluateFunction, FindClosePropertyMatches findClosePropertyMatches)
		{
			Stack<string> evaluationStack = null;
			return ExpandString(expression, evaluateProperty, evaluateFunction, findClosePropertyMatches, ref evaluationStack);
		}

		public static string ExpandString(string expression, PropertyEvaluator evaluateProperty, FunctionEvaluator evaluateFunction, FindClosePropertyMatches findClosePropertyMatches, ref Stack<string> evaluationStack)
		{
			int offset = 0;
			int length = expression.Length;
			StringBuilder parsedExpression = null;
			int startingOffset = offset;
			ReadOnlySpan<char> expressionSpan = expression.AsSpan();
			while (offset < length)
			{
				char currentChar = expressionSpan[offset];
				if ((currentChar == '$' || currentChar == '@') && offset + 1 < length && expressionSpan[offset + 1] == '{')
				{
					if (parsedExpression == null)
						parsedExpression = new StringBuilder();
					if (offset != startingOffset)
						parsedExpression.Append(expression, startingOffset, offset - startingOffset);
					parsedExpression.Append(currentChar == '$' ?
						ParseProperty(expression, ref offset, evaluateProperty, evaluateFunction, findClosePropertyMatches, ref evaluationStack) :
						ParseFunction(expression, ref offset, evaluateProperty, evaluateFunction, findClosePropertyMatches, ref evaluationStack));
					startingOffset = offset;
				}
				else
				{
					++offset;
				}
			}
			if (parsedExpression != null)
			{
				if (offset != startingOffset)
					parsedExpression.Append(expression, startingOffset, offset - startingOffset);
				return parsedExpression.ToString();
			}
			else
				return expression;
		}

		/// <summary>
		/// Returns the expansion of a property within a string.
		/// </summary>
		/// <remarks>
		///   <para>Can handle nested expressions like ${${a}}.</para>
		///   <para>Cannot use '${' in expressions without invoking expansion.</para>
		/// </remarks>
		/// <param name="expression">The expression to expand.</param>
		/// <param name="offset">The offset in the expression of the property to expand.</param>
		/// <param name="evaluateProperty">The delegate function to call when a property needs evaluating.</param>
		/// <param name="evaluateFunction">The delegate function to call when a function needs evaluating.</param>
		/// <param name="findClosePropertyMatches">The delegate function to call when a property fails to evaluate and close matches need to be gound for error message</param>
		/// <param name="evaluationStack">A stack of properties that have already been attempt to be resolved in recursive calls (for dealing with deferred properties).</param>
		private static string ParseProperty(string expression, ref int offset, PropertyEvaluator evaluateProperty, FunctionEvaluator evaluateFunction, FindClosePropertyMatches findClosePropertyMatches, ref Stack<string> evaluationStack)
		{
			// Obtain the name of the property to evaluate - possibly through recursion.
			offset += 2;
			int length = expression.Length;
			StringBuilder propertyNameBuilder = null;
			int startingOffset = offset;
			ReadOnlySpan<char> expressionSpan = expression.AsSpan();
			while (offset < length && expressionSpan[offset] != '}')
			{
				if ((expressionSpan[offset] == '$' || expressionSpan[offset] == '@') && offset + 1 < length && expressionSpan[offset + 1] == '{')
				{
					if (propertyNameBuilder == null)
						propertyNameBuilder = new StringBuilder();
					if (offset != startingOffset)
						propertyNameBuilder.Append(expression, startingOffset, offset - startingOffset);
					propertyNameBuilder.Append(expressionSpan[offset] == '$' ?
						ParseProperty(expression, ref offset, evaluateProperty, evaluateFunction, findClosePropertyMatches, ref evaluationStack) :
						ParseFunction(expression, ref offset, evaluateProperty, evaluateFunction, findClosePropertyMatches, ref evaluationStack));
					startingOffset = offset;
				}
				else
					++offset;
			}
			if (offset == length)
				throw new BuildException("Unmatched brace" + Offset(offset, expression));

			string propertyName;
			if (propertyNameBuilder != null)
			{
				if (offset != startingOffset)
					propertyNameBuilder.Append(expression, startingOffset, offset - startingOffset);
				propertyName = propertyNameBuilder.ToString();
			}
			else
				propertyName = expression.Substring(startingOffset, offset - startingOffset);
			++offset;

			// Evaluate the property.
			propertyName = propertyName.Trim();
			string value = null;

			if (evaluationStack == null)
			{
				evaluationStack = new Stack<string>();
			}
			else if (evaluationStack.Contains(propertyName)) // if we're in a deferred expansion situation make sure we haven't already tried to evaluate this property
			{
				throw new BuildException(String.Format("Cyclic deferred property expansion: {0}.", String.Join(" => ",
					evaluationStack.Reverse()
					.Concat(new string[] { propertyName })
					.Select(s => String.Format("${{{0}}}", s))
				)));
			}

			evaluationStack.Push(propertyName);
			value = evaluateProperty(propertyName, evaluationStack);
			evaluationStack.Pop();
			if (value == null)
			{
				string defaultingPropertyName = propertyName;
				int defaultOperator = propertyName.IndexOf("??", StringComparison.Ordinal);
				if (defaultOperator > 0)
				{
					propertyName = propertyName.Substring(0, defaultOperator).Trim();
					if (evaluationStack.Contains(propertyName)) // if we're in a deferred expansion situation make sure we haven't already tried to evaluate this property
					{
						throw new BuildException(String.Format("Cyclic deferred property expansion: {0}.", String.Join(" => ",
							evaluationStack.Reverse()
							.Concat(new string[] { propertyName })
							.Select(s => String.Format("${{{0}}}", s))
						)));
					}
					evaluationStack.Push(propertyName);
					value = evaluateProperty(propertyName, evaluationStack);
					evaluationStack.Pop();
					if (value == null)
					{
						value = (defaultOperator + 2 >= defaultingPropertyName.Length) ? String.Empty : defaultingPropertyName.Substring(defaultOperator + 2).Trim();
					}
				}
				else if (defaultOperator == 0)
				{
					throw new BuildException(String.Format("Property name can not be empty in expression ${{property??default}}: '{0}'", defaultingPropertyName));
				}
				if (value == null)
				{
					string message = "Failed to evaluate property '" + propertyName + "'.";
					if (findClosePropertyMatches != null)
					{
						string[] closestMatches = findClosePropertyMatches(propertyName).ToArray();
						if (closestMatches.Any())
						{
							message += " Closest matches:\n" + String.Join("", closestMatches.Select(m => "\n\t" + m));
						}
					}
					throw new BuildException(message);

				}
			}
			return value;
		}

		/// <summary>
		/// Returns the expansion of a function within a string.
		/// </summary>
		/// <param name="expression">The expression to expand.</param>
		/// <param name="offset">The offset in the expression of the property to expand.</param>
		/// <param name="evaluateProperty">The delegate function to call when a property needs evaluating.</param>
		/// <param name="evaluateFunction">The delegate function to call when a function needs evaluating.</param>
		/// <param name="findClosePropertyMatches">The delegate function to call when a property fails to evaluate and close matches need to be gound for error message</param>
		/// <param name="evaluationStack">A stack of properties that have already been attempt to be resolved in recursive calls (for dealing with deferred properties).</param>
		private static string ParseFunction(string expression, ref int offset, PropertyEvaluator evaluateProperty, FunctionEvaluator evaluateFunction, FindClosePropertyMatches findClosePropertyMatches, ref Stack<string> evaluationStack)
		{
			// Obtain the name of the function.
			offset += 2;
			int length = expression.Length;
			int startingOffset = offset;
			ReadOnlySpan<char> expressionSpan = expression.AsSpan();
			for (; offset < length && (Char.IsLetterOrDigit(expressionSpan[offset]) || Char.IsWhiteSpace(expressionSpan[offset])); ++offset) ;
			if (offset == length || expressionSpan[offset] != '(')
				throw new BuildException("Invalid character" + Offset(offset, expression));
			string functionName = expression.Substring(startingOffset, offset - startingOffset).Trim();

			// Obtain the parameter list.
			List<string> parameterList = new List<string>();
			do
			{
				++offset;
				// Skip leading whitespace...
				for (; offset < length && Char.IsWhiteSpace(expressionSpan[offset]); ++offset) ;
				if (offset == length)
					throw new BuildException("Parameter not found" + Offset(offset, expression));
				// ... read the parameter...
				if (expressionSpan[offset] == '\'' || expressionSpan[offset] == '"')
				{
					char closingQuote = expressionSpan[offset];
					++offset;
					StringBuilder stringText = new StringBuilder();
					while (offset < length && expressionSpan[offset] != closingQuote)
					{
						if (expressionSpan[offset] == '$' && offset + 1 < length && expressionSpan[offset + 1] == '{')
							stringText.Append(ParseProperty(expression, ref offset, evaluateProperty, evaluateFunction, findClosePropertyMatches, ref evaluationStack));
						else if (expressionSpan[offset] == '@' && offset + 1 < length && expressionSpan[offset + 1] == '{')
							stringText.Append(ParseFunction(expression, ref offset, evaluateProperty, evaluateFunction, findClosePropertyMatches, ref evaluationStack));
						else
						{
							if (expressionSpan[offset] == '\\' && offset + 1 < length && (expressionSpan[offset + 1] == '\\' || expressionSpan[offset + 1] == closingQuote))
								++offset;
							stringText.Append(expressionSpan[offset]);
							++offset;
						}
					}
					if (offset == length)
						throw new BuildException("Unmatched quote" + Offset(offset, expression));
					++offset;
					parameterList.Add(stringText.ToString());
				}
				else if (expressionSpan[offset] == '$' && offset + 1 < length && expressionSpan[offset + 1] == '{')
					parameterList.Add(ParseProperty(expression, ref offset, evaluateProperty, evaluateFunction, findClosePropertyMatches, ref evaluationStack));
				else if (expressionSpan[offset] == '@' && offset + 1 < length && expressionSpan[offset + 1] == '{')
					parameterList.Add(ParseFunction(expression, ref offset, evaluateProperty, evaluateFunction, findClosePropertyMatches, ref evaluationStack));
				else if (expressionSpan[offset] != ')' && expressionSpan[offset] != ',')
				{
					startingOffset = offset;
					do
					{
						++offset;
					}
					while (offset < length && expressionSpan[offset] != ')' && expressionSpan[offset] != ',' && !Char.IsWhiteSpace(expressionSpan[offset]));
					parameterList.Add(expression.Substring(startingOffset, offset - startingOffset));
				}
				else if (expressionSpan[offset] != ')' || parameterList.Count != 0)
					throw new BuildException("Invalid character" + Offset(offset, expression));
				// ... and skip trailing whitespace.
				for (; offset < length && Char.IsWhiteSpace(expressionSpan[offset]); ++offset) ;
			}
			while (offset < length && expressionSpan[offset] == ',');
			if (offset == length || expressionSpan[offset] != ')')
				throw new BuildException("Unmatched bracket" + Offset(offset, expression));
			++offset;
			for (; offset < length && Char.IsWhiteSpace(expressionSpan[offset]); ++offset) ;
			if (offset == length || expressionSpan[offset] != '}')
				throw new BuildException("Unmatched brace" + Offset(offset, expression));
			++offset;

			// Evaluate the function and return the result.
			return evaluateFunction(functionName, parameterList);
		}

		private static string Offset(int i, string expression)
		{
			StringBuilder ret = new StringBuilder(" at offset " + i + " in expression \n'" + expression + "'\n ");
			for (int j = 0; j < i; ++j)
				ret.Append(' ');
			ret.Append('^');

			return ret.ToString();
		}
	}
}
