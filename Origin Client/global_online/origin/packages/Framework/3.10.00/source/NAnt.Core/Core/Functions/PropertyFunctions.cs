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
using System.Text.RegularExpressions;

using NAnt.Core;
using NAnt.Core.Attributes;

namespace NAnt.Core.Functions
{
	/// <summary>
	/// Collection of property manipulation routines.
	/// </summary>
	[FunctionClass("Property Functions")]
	public class PropertyFunctions : FunctionClassBase
	{
		/// <summary>
		/// Check if the specified property is defined.
		/// </summary>
		/// <param name="project"></param>
		/// <param name="propertyName">The property name to check.</param>
		/// <returns>True or False.</returns>
        /// <include file='Examples\Property\PropertyExists.example' path='example'/>        
        [Function()]
		public static string PropertyExists(Project project, string propertyName)
		{
            if (project.Properties.Contains(propertyName)) {
                return Boolean.TrueString;
            }
            return Boolean.FalseString;
		}

		/// <summary>
		/// Check if the specified property value is true. 
		/// If property does not exist, an Exception will be thrown.
		/// </summary>
		/// <param name="project"></param>
		/// <param name="propertyName">The property name to check.</param>
		/// <returns>True or False.</returns>
        /// <include file='Examples\Property\PropertyTrue.example' path='example'/>        
        [Function()]
		public static string PropertyTrue(Project project, string propertyName)
		{
			string val = project.Properties[propertyName];
            if (val == null) {
                string msg = String.Format("Property '{0}' does not exist.", propertyName);
                throw new BuildException(msg);
            }

			try {
				val = bool.Parse(val).ToString();
			}
			catch(Exception e) {
                throw new BuildException(String.Format("Property {0} can't be converted to a boolean value: {1}", propertyName, e.Message));
			}
			return val;

		}

		/// <summary>
		/// Expand the specified property.
		/// </summary>
		/// <param name="project"></param>
		/// <param name="propertyName">The property name to expand.</param>
		/// <returns>The value of the specified property. If the property does not exits a BuildException is thrown.</returns>
        /// <include file='Examples\Property\PropertyExpand.example' path='example'/>        
        [Function()]
		public static string PropertyExpand(Project project, string propertyName)
		{
			string val = project.Properties[propertyName];
            if (val == null) {
                string msg = String.Format("Property '{0}' does not exist.", propertyName);
                throw new BuildException(msg);
            }
            return val;
		}

        /// <summary>
        /// Undefine the specified property.
        /// </summary>
        /// <param name="project"></param>
        /// <param name="propertyName">The property name to undefine.</param>
        /// <returns>True if the property was undefined properly, otherwise False.</returns>
        /// <include file='Examples\Property\PropertyUndefine.example' path='example'/>        
        [Function()]
        public static string PropertyUndefine(Project project, string propertyName)
        {
            string val = project.Properties[propertyName];
            if (val != null) {
                project.Properties.Remove(propertyName);
                return Boolean.TrueString;
            }
            return Boolean.FalseString;
        }

        /// <summary>
        /// Check if the specified property is defined as global property in masterconfig.
        /// </summary>
        /// <param name="project"></param>
        /// <param name="propertyName">The property name to check.</param>
        /// <returns>True or False.</returns>
        /// <include file='Examples\Property\IsPropertyGlobal.example' path='example'/>        
        [Function()]
        public static string IsPropertyGlobal(Project project, string propertyName)
        {
            foreach (var gprop in Project.GlobalProperties.EvaluateExceptions(project))
            {
                if (0 == String.Compare(gprop.Name, propertyName))
                {
                    return Boolean.TrueString;
                }
            }
            return Boolean.FalseString;
        }

        /// <summary>
        /// Returns property value or default value if property does not exist.
        /// </summary>
        /// <param name="project"></param>
        /// <param name="propertyName">The property name to check.</param>
        /// <param name="defaultVal">default value.</param>
        /// <returns>property value.</returns>
        [Function()]
        public static string GetPropertyOrDefault(Project project, string propertyName, string defaultVal)
        {
            if (project.Properties.Contains(propertyName))
            {
                string val = project.Properties[propertyName];

                if (val == null)
                {
                    val = defaultVal;
                }
                return val;
            }
            return defaultVal;
        }

        private static char[] TRIM_CHARS = new char[] { '\n', '\r', '\t', ' ' };

        // Utility function for trimming leading and trailing whitespace, and then splitting on any form of whitespace.
        // String.Split() just doesn't cut it.
        private static string[] TrimAndSplit(string inputString)
        {
            return Regex.Split(inputString.Trim(TRIM_CHARS), @"\s+");
        }

	}
}