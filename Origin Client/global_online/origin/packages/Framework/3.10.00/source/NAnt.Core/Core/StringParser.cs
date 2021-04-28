// NAnt - A .NET build tool
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
using System.Text;

using NAnt.Core.Logging;

namespace NAnt.Core
{
    public class StringParser
    {
        public delegate string PropertyEvaluator(string parameter);
        public delegate string FunctionEvaluator(string functionName, List<string> parameterList);

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
        public static string ExpandString(string expression, PropertyEvaluator evaluateProperty, FunctionEvaluator evaluateFunction)
        {
            int offset = 0;
            int length = expression.Length;
            StringBuilder parsedExpression = null;
            int startingOffset = offset;
            while (offset < length)
            {
                if ((expression[offset] == '$' || expression[offset] == '@') && offset + 1 < length && expression[offset + 1] == '{')
                {
                    if (parsedExpression == null)
                        parsedExpression = new StringBuilder();
                    if (offset != startingOffset)
                        parsedExpression.Append(expression, startingOffset, offset - startingOffset);
                    parsedExpression.Append(expression[offset] == '$' ?
                        ParseProperty(expression, ref offset, evaluateProperty, evaluateFunction) :
                        ParseFunction(expression, ref offset, evaluateProperty, evaluateFunction));
                    startingOffset = offset;
                }
#if NANT_COMPATIBILITY
                // This is a hack for compatibility with older versions of NAnt.
                else if (expression[offset] == '\\' && offset + 1 < length && expression[offset + 1] == '@' && (offset == 0 || expression[offset - 1] != '\\'))
                {
                    if (parsedExpression == null)
                        parsedExpression = new StringBuilder();
                    if (offset > 0 && offset - 1 != startingOffset)
                        parsedExpression.Append(expression, startingOffset, offset - startingOffset - 1);
                    parsedExpression.Append('@');
                    offset += 2;
                    startingOffset = offset;
                }
#endif
                else
                    ++offset;
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
        /// <param name="expression">The expresion to expand.</param>
        /// <param name="offset">The offset in the expression of the property to expand.</param>
        /// <param name="evaluateProperty">The delegate function to call when a property needs evaluating.</param>
        /// <param name="evaluateFunction">The delegate function to call when a function needs evaluating.</param>
        private static string ParseProperty(string expression, ref int offset, PropertyEvaluator evaluateProperty, FunctionEvaluator evaluateFunction)
        {
            // Obtain the name of the property to evaluate - possibly through recursion.
            offset += 2;
            int length = expression.Length;
            StringBuilder propertyNameBuilder = null;
            int startingOffset = offset;
            while (offset < length && expression[offset] != '}')
            {
                if ((expression[offset] == '$' || expression[offset] == '@') && offset + 1 < length && expression[offset + 1] == '{')
                {
                    if (propertyNameBuilder == null)
                        propertyNameBuilder = new StringBuilder();
                    if (offset != startingOffset)
                        propertyNameBuilder.Append(expression, startingOffset, offset - startingOffset);
                    propertyNameBuilder.Append(expression[offset] == '$' ?
                        ParseProperty(expression, ref offset, evaluateProperty, evaluateFunction) :
                        ParseFunction(expression, ref offset, evaluateProperty, evaluateFunction));
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

            string value = evaluateProperty(propertyName);
            if (value == null)
            {
                int defaultOperator = propertyName.IndexOf("??");
                if (defaultOperator > 0)
                {
                    value = evaluateProperty(propertyName.Substring(0, defaultOperator).Trim());
                    if (value == null)
                    {
                        value = (defaultOperator + 2 >= propertyName.Length) ? String.Empty : propertyName.Substring(defaultOperator + 2).Trim();
                    }
                }
                else if (defaultOperator == 0)
                {
                    throw new BuildException(String.Format("Property name can not be empty in expression ${{property??default}}: '{0}'", propertyName));
                }
                if (value == null)
                {
                    throw new BuildException(String.Format("Failed to evaluate property '{0}'", propertyName));
                }
            }
            return value;
        }

        /// <summary>
        /// Returns the expansion of a function within a string.
        /// </summary>
        /// <param name="expression">The expresion to expand.</param>
        /// <param name="offset">The offset in the expression of the property to expand.</param>
        /// <param name="evaluateProperty">The delegate function to call when a property needs evaluating.</param>
        /// <param name="evaluateFunction">The delegate function to call when a function needs evaluating.</param>
        private static string ParseFunction(string expression, ref int offset, PropertyEvaluator evaluateProperty, FunctionEvaluator evaluateFunction)
        {
            // Obtain the name of the function.
            offset += 2;
            int length = expression.Length;
            int startingOffset = offset;
            for (; offset < length && (Char.IsLetterOrDigit(expression[offset]) || Char.IsWhiteSpace(expression[offset])); ++offset) ;
            if (offset == length || expression[offset] != '(')
                throw new BuildException("Invalid character" + Offset(offset, expression));
            string functionName = expression.Substring(startingOffset, offset - startingOffset).Trim();

            // Obtain the parameter list.
            List<string> parameterList = new List<string>();
            do
            {
                ++offset;
                // Skip leading whitespace...
                for (; offset < length && Char.IsWhiteSpace(expression[offset]); ++offset) ;
                if (offset == length)
                    throw new BuildException("Parameter not found" + Offset(offset, expression));
                // ... read the parameter...
                if (expression[offset] == '\'' || expression[offset] == '"')
                {
                    char closingQuote = expression[offset];
                    ++offset;
                    StringBuilder stringText = new StringBuilder();
                    while (offset < length && expression[offset] != closingQuote)
                    {
                        if (expression[offset] == '$' && offset + 1 < length && expression[offset + 1] == '{')
                            stringText.Append(ParseProperty(expression, ref offset, evaluateProperty, evaluateFunction));
                        else if (expression[offset] == '@' && offset + 1 < length && expression[offset + 1] == '{')
                            stringText.Append(ParseFunction(expression, ref offset, evaluateProperty, evaluateFunction));
                        else
                        {
                            if (expression[offset] == '\\' && offset + 1 < length && (expression[offset + 1] == '\\' || expression[offset + 1] == closingQuote))
                                ++offset;
                            stringText.Append(expression[offset]);
                            ++offset;
                        }
                    }
                    if (offset == length)
                        throw new BuildException("Unmatched quote" + Offset(offset, expression));
                    ++offset;
                    parameterList.Add(stringText.ToString());
                }
                else if (expression[offset] == '$' && offset + 1 < length && expression[offset + 1] == '{')
                    parameterList.Add(ParseProperty(expression, ref offset, evaluateProperty, evaluateFunction));
                else if (expression[offset] == '@' && offset + 1 < length && expression[offset + 1] == '{')
                    parameterList.Add(ParseFunction(expression, ref offset, evaluateProperty, evaluateFunction));
                else if (expression[offset] != ')' && expression[offset] != ',')
                {
                    startingOffset = offset;
                    do
                    {
                        ++offset;
                    }
                    while (offset < length && expression[offset] != ')' && expression[offset] != ',' && !Char.IsWhiteSpace(expression[offset]));
                    parameterList.Add(expression.Substring(startingOffset, offset - startingOffset));
                }
                else if (expression[offset] != ')' || parameterList.Count != 0)
                    throw new BuildException("Invalid character" + Offset(offset, expression));
                // ... and skip trailing whitespace.
                for (; offset < length && Char.IsWhiteSpace(expression[offset]); ++offset) ;
            }
            while (offset < length && expression[offset] == ',');
            if (offset == length || expression[offset] != ')')
                throw new BuildException("Unmatched bracket" + Offset(offset, expression));
            ++offset;
            for (; offset < length && Char.IsWhiteSpace(expression[offset]); ++offset) ;
            if (offset == length || expression[offset] != '}')
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