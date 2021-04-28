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

namespace NAnt.Core
{
    /// <summary>
    /// Classes which define static methods to be used as nant function must derive from this class.
    /// </summary>
    /// <remarks>
    /// To declare a method as available to be called from within nant you must declare a method in a class where: 
    /// 
    ///		- the class derives the NAnt.FunctionClassBase abstract class
    ///		- the class defines a NAnt.Attributes.FunctionClassAttribute attribute
    ///		- the method defines a NAnt.Attributes.FunctionAttribute attribute
    ///		- the method is static
    ///		- the method returns a string
    ///		- the first parameter of the method is of type NAnt.Project
    ///		- the remaining parameters are capable of beiing converted from a string to their assigned type
    ///		
    ///	Function names need not be unique. However, functions are distinguished by name and number of parameters.
    ///	So you may not have two functions with the same name and same number of parameters. Regardless of the types.
    /// </remarks>
    /// <example>
    /// [FunctionClass()]
    /// public class NantFunctionClass
    /// {
    ///		[Function()]
    ///		public static string Echo(Project project, string s)
    ///		{
    ///			return s;
    ///		}
    ///		
    ///		[Function()]
    ///		public static string Add(Project project, int a, int b)
    ///		{
    ///			int sum = a + b;
    ///			return sum.ToString();
    ///		}
    ///	}
    /// </example>
    public abstract class FunctionClassBase
    {
    }
}