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
// 
// 
// Kosta Arvanitis (karvanitis@ea.com)
using System;
using System.Collections.Specialized;

using NAnt.Core;
using NAnt.Core.Attributes;

namespace NAnt.Core.Functions
{
	/// <summary>
	/// Collection of NAnt Math routines.
	/// </summary>
	[FunctionClass("Math Functions")]
	public class MathFunctions : FunctionClassBase
	{
        static readonly string DoubleFormat = "R";

		/// <summary>
		/// The ratio of the circumference of a circle to its diameter.
		/// </summary>
		/// <param name="project"></param>
		/// <returns>PI</returns>
        /// <remarks>Full precision ensures that numbers converted to strings will 
        /// have the same value when they are converted back to numbers.</remarks>
        /// <include file='Examples\Math\MathPI.example' path='example'/>        
        [Function()]
		public static string MathPI(Project project)
		{
            return Math.PI.ToString(DoubleFormat);
		}

        /// <summary>
        /// The natural logarithmic base.
        /// </summary>
        /// <param name="project"></param>
        /// <returns>E</returns>
        /// <remarks>Full precision ensures that numbers converted to strings will 
        /// have the same value when they are converted back to numbers.</remarks>
        /// <include file='Examples\Math\MathE.example' path='example'/>        
        [Function()]
        public static string MathE(Project project)
        {
            return Math.E.ToString(DoubleFormat);
        }

        /// <summary>
        /// Compare two numbers to determine if first number is less than second.
        /// </summary>
        /// <param name="project"></param>
        /// <param name="a">The first number.</param>
        /// <param name="b">The second number.</param>
        /// <returns>True if less than, otherwise False</returns>
        [Function()]
        public static string MathLT(Project project, double a, double b)
        {
            return (a < b).ToString();
        }

        /// <summary>
        /// Compare two numbers to determine if first number is less than or equal to second.
        /// </summary>
        /// <param name="project"></param>
        /// <param name="a">The first number.</param>
        /// <param name="b">The second number.</param>
        /// <returns>True if less than or equal, otherwise False</returns>
        [Function()]
        public static string MathLTEQ(Project project, double a, double b)
        {
            return (a <= b).ToString();
        }

        /// <summary>
        /// Compare two numbers to determine if first number is greater than second.
        /// </summary>
        /// <param name="project"></param>
        /// <param name="a">The first number.</param>
        /// <param name="b">The second number.</param>
        /// <returns>True if greater than, otherwise False</returns>
        [Function()]
        public static string MathGT(Project project, double a, double b)
        {
            return (a > b).ToString();
        }

        /// <summary>
        /// Compare two numbers to determine if first number is greater than or equal to second.
        /// </summary>
        /// <param name="project"></param>
        /// <param name="a">The first number.</param>
        /// <param name="b">The second number.</param>
        /// <returns>True if greater than or equal, otherwise False</returns>
        [Function()]
        public static string MathGTEQ(Project project, double a, double b)
        {
            return (a >= b).ToString();
        }

        /// <summary>
        /// Compare two numbers to determine if first number is equal to second.
        /// </summary>
        /// <param name="project"></param>
        /// <param name="a">The first number.</param>
        /// <param name="b">The second number.</param>
        /// <returns>True if equal, otherwise False</returns>
        [Function()]
        public static string MathEQ(Project project, double a, double b)
        {
            return (a == b).ToString();
        }

        /// <summary>
        /// Add two integer numbers.
        /// </summary>
        /// <param name="project"></param>
        /// <param name="a">The first number.</param>
        /// <param name="b">The second number.</param>
        /// <returns>The sum of two numbers.</returns>
        /// <include file='Examples\Math\MathAdd.example' path='example'/>        
        [Function()]
        public static string MathAdd(Project project, int a, int b)
        {
            int sum = a + b;
            return sum.ToString();
        }

        /// <summary>
        /// Add two floating-point numbers.
        /// </summary>
        /// <param name="project"></param>
        /// <param name="a">The first number.</param>
        /// <param name="b">The second number.</param>
        /// <returns>The sum of two numbers.</returns>
        /// <remarks>Full precision ensures that numbers converted to strings will 
        /// have the same value when they are converted back to numbers.</remarks>
        /// <include file='Examples\Math\MathAddf.example' path='example'/>        
        [Function()]
        public static string MathAddf(Project project, double a, double b)
        {
            double sum = a + b;
            return sum.ToString(DoubleFormat);
        }

        /// <summary>
        /// Subtract two integer numbers.
        /// </summary>
        /// <param name="project"></param>
        /// <param name="a">The first number.</param>
        /// <param name="b">The second number.</param>
        /// <returns>The difference of two numbers.</returns>
        /// <include file='Examples\Math\MathSub.example' path='example'/>        
        [Function()]
        public static string MathSub(Project project, int a, int b)
        {
            int sum = a - b;
            return sum.ToString();
        }

        /// <summary>
        /// Subtract two floating-point numbers.
        /// </summary>
        /// <param name="project"></param>
        /// <param name="a">The first number.</param>
        /// <param name="b">The second number.</param>
        /// <returns>The difference of two numbers.</returns>
        /// <remarks>Full precision ensures that numbers converted to strings will 
        /// have the same value when they are converted back to numbers.</remarks>
        /// <include file='Examples\Math\MathSubf.example' path='example'/>        
        [Function()]
        public static string MathSubf(Project project, double a, double b)
        {
            double sum = a - b;
            return sum.ToString(DoubleFormat);
        }

        /// <summary>
        /// Multiply two integer numbers.
        /// </summary>
        /// <param name="project"></param>
        /// <param name="a">The first number.</param>
        /// <param name="b">The second number.</param>
        /// <returns>The product of two numbers.</returns>
        /// <include file='Examples\Math\MathMul.example' path='example'/>        
        [Function()]
        public static string MathMul(Project project, int a, int b)
        {
            int sum = a * b;
            return sum.ToString();
        }

        /// <summary>
        /// Multiply two floating-point numbers.
        /// </summary>
        /// <param name="project"></param>
        /// <param name="a">The first number.</param>
        /// <param name="b">The second number.</param>
        /// <returns>The product of two numbers.</returns>
        /// <remarks>Full precision ensures that numbers converted to strings will 
        /// have the same value when they are converted back to numbers.</remarks>
        /// <include file='Examples\Math\MathMulf.example' path='example'/>        
        [Function()]
        public static string MathMulf(Project project, double a, double b)
        {
            double sum = a * b;
            return sum.ToString(DoubleFormat);
        }

        /// <summary>
        /// Computes the modulo of two numbers.
        /// </summary>
        /// <param name="project"></param>
        /// <param name="a">The first number.</param>
        /// <param name="b">The second number.</param>
        /// <returns>The modulo of two numbers.</returns>
        /// <include file='Examples\Math\MathMod.example' path='example'/>        
        [Function()]
        public static string MathMod(Project project, int a, int b)
        {
            int sum = a % b;
            return sum.ToString();
        }

        /// <summary>
        /// Computes the modulo of two numbers.
        /// </summary>
        /// <param name="project"></param>
        /// <param name="a">The first number.</param>
        /// <param name="b">The second number.</param>
        /// <returns>The modulo of two numbers.</returns>
        /// <include file='Examples\Math\MathModf.example' path='example'/>        
        [Function()]
        public static string MathModf(Project project, double a, double b)
        {
            double sum = a % b;
            return sum.ToString(DoubleFormat);
        }

        /// <summary>
        /// Divide two integer numbers.
        /// </summary>
        /// <param name="project"></param>
        /// <param name="a">The first number.</param>
        /// <param name="b">The second number.</param>
        /// <returns>The division of two numbers.</returns>
        /// <include file='Examples\Math\MathDiv.example' path='example'/>        
        [Function()]
        public static string MathDiv(Project project, int a, int b)
        {
            if (b == 0)
                throw new DivideByZeroException();
            int sum = a / b;
            return sum.ToString();
        }

        /// <summary>
        /// Divide two floating-point numbers.
        /// </summary>
        /// <param name="project"></param>
        /// <param name="a">The first number.</param>
        /// <param name="b">The second number.</param>
        /// <returns>The division of two numbers.</returns>
        /// <remarks>Full precision ensures that numbers converted to strings will 
        /// have the same value when they are converted back to numbers.</remarks>
        /// <include file='Examples\Math\MathDivf.example' path='example'/>        
        [Function()]
        public static string MathDivf(Project project, double a, double b)
        {
            if (b == 0)
                throw new DivideByZeroException();
            double sum = a / b;
            return sum.ToString(DoubleFormat);
        }

        /// <summary>
        /// Calculates the absolute value of a specified number.
        /// </summary>
        /// <param name="project"></param>
        /// <param name="a">The number.</param>
        /// <returns>The absolute value of a specified number.</returns>
        /// <remarks>Full precision ensures that numbers converted to strings will 
        /// have the same value when they are converted back to numbers.</remarks>
        /// <include file='Examples\Math\MathAbs.example' path='example'/>        
        [Function()]
        public static string MathAbs(Project project, int a)
        {
            return Math.Abs(a).ToString();
        }

        /// <summary>
        /// Calculates the absolute value of a specified number.
        /// </summary>
        /// <param name="project"></param>
        /// <param name="a">The number.</param>
        /// <returns>The absolute value of a specified number.</returns>
        /// <remarks>Full precision ensures that numbers converted to strings will 
        /// have the same value when they are converted back to numbers.</remarks>
        /// <include file='Examples\Math\MathAbsf.example' path='example'/>        
        [Function()]
        public static string MathAbsf(Project project, double a)
        {
            return Math.Abs(a).ToString(DoubleFormat);
        }

        
        /// <summary>
        /// Returns the smallest whole number greater than or equal to the specified number.
        /// </summary>
        /// <param name="project"></param>
        /// <param name="a">The number.</param>
        /// <returns>The smallest whole number greater than or equal to the specified number.</returns>
        /// <remarks>Full precision ensures that numbers converted to strings will 
        /// have the same value when they are converted back to numbers.</remarks>
        /// <include file='Examples\Math\MathCeiling.example' path='example'/>        
        [Function()]
        public static string MathCeiling(Project project, double a)
        {
            return Math.Ceiling(a).ToString(DoubleFormat);
        }

        /// <summary>
        /// Returns the largest whole number less than or equal to the specified number.
        /// </summary>
        /// <param name="project"></param>
        /// <param name="a">The number.</param>
        /// <returns>The largest whole number less than or equal to the specified number.</returns>
        /// <remarks>Full precision ensures that numbers converted to strings will 
        /// have the same value when they are converted back to numbers.</remarks>
        /// <include file='Examples\Math\MathFloor.example' path='example'/>        
        [Function()]
        public static string MathFloor(Project project, double a)
        {
            return Math.Floor(a).ToString(DoubleFormat);
        }
        
        /// <summary>
        /// Raise the specified number to the specified power.
        /// </summary>
        /// <param name="project"></param>
        /// <param name="x">The number.</param>
        /// <param name="y">The power.</param>
        /// <returns>The number x raised to the power y.</returns>
        /// <remarks>Full precision ensures that numbers converted to strings will 
        /// have the same value when they are converted back to numbers.</remarks>
        /// <include file='Examples\Math\MathPow.example' path='example'/>        
        [Function()]
        public static string MathPow(Project project, double x, double y)
        {
            return Math.Pow(x, y).ToString(DoubleFormat);
        }

        /// <summary>
        /// Computes the square root of a specified number.
        /// </summary>
        /// <param name="project"></param>
        /// <param name="a">The specified number.</param>
        /// <returns>The square root of a specified number.</returns>
        /// <remarks>Full precision ensures that numbers converted to strings will 
        /// have the same value when they are converted back to numbers.</remarks>
        /// <include file='Examples\Math\MathSqrt.example' path='example'/>        
        [Function()]
        public static string MathSqrt(Project project, double a)
        {
            return Math.Sqrt(a).ToString(DoubleFormat);
        }

        /// <summary>
        /// Converts an angle from degrees to radians.
        /// </summary>
        /// <param name="project"></param>
        /// <param name="a">The specified number of degrees.</param>
        /// <returns>The degree of a.</returns>
        /// <remarks>Full precision ensures that numbers converted to strings will 
        /// have the same value when they are converted back to numbers.</remarks>
        /// <include file='Examples\Math\MathDegToRad.example' path='example'/>        
        [Function()]
        public static string MathDegToRad(Project project, int a)
        {
            double r = Math.PI * ((double)a) / 180.0f;
            return r.ToString(DoubleFormat);
        }

        /// <summary>
        /// Calculates the sine of the specified angle.
        /// </summary>
        /// <param name="project"></param>
        /// <param name="a">The specified angle in radians.</param>
        /// <returns>The sine of a.</returns>
        /// <remarks>Full precision ensures that numbers converted to strings will 
        /// have the same value when they are converted back to numbers.</remarks>
        /// <include file='Examples\Math\MathSin.example' path='example'/>        
        [Function()]
        public static string MathSin(Project project, double a)
        {
            return Math.Sin(a).ToString(DoubleFormat);
        }

        /// <summary>
        /// Calculates the cosine of the specified angle.
        /// </summary>
        /// <param name="project"></param>
        /// <param name="a">The specified angle in radians.</param>
        /// <returns>The cosine of a.</returns>
        /// <remarks>Full precision ensures that numbers converted to strings will 
        /// have the same value when they are converted back to numbers.</remarks>
        /// <include file='Examples\Math\MathCos.example' path='example'/>        
        [Function()]
        public static string MathCos(Project project, double a)
        {
            return Math.Cos(a).ToString(DoubleFormat);
        }

        /// <summary>
        /// Calculates the tangent of the specified angle.
        /// </summary>
        /// <param name="project"></param>
        /// <param name="a">The specified angle in radians.</param>
        /// <returns>The tangent of a.</returns>
        /// <remarks>Full precision ensures that numbers converted to strings will 
        /// have the same value when they are converted back to numbers.</remarks>
        /// <include file='Examples\Math\MathTan.example' path='example'/>        
        [Function()]
        public static string MathTan(Project project, double a)
        {
            return Math.Tan(a).ToString(DoubleFormat);
        }

        /// <summary>
        /// Changes the precision of a specified floating-point number.
        /// </summary>
        /// <param name="project"></param>
        /// <param name="a">The specified floating-point number.</param>
        /// <param name="precision">The new precision.</param>
        /// <returns>The value of a using the specified precision.</returns>
        /// <remarks>By default NAnt math functions return doubles using full precision (17 bits). Typically only 
        /// 15 bits are needed.</remarks>
        /// <include file='Examples\Math\MathChangePrecision.example' path='example'/>        
        [Function()]
        public static string MathChangePrecision(Project project, double a, int precision)
        {
            string format = String.Format("G{0}", precision);
            return a.ToString(format);
        }
    }
}