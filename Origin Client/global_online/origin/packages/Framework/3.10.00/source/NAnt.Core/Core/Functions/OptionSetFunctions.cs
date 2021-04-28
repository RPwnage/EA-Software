// NAnt - A .NET build tool
// Copyright (C) 2001-2003 Gerry Shaw
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
// Gerry Shaw (gerry_shaw@yahoo.com)
// Kosta Arvanitis (karvanitis@ea.com)
using System;
using System.Text;

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Attributes;

namespace NAnt.Core.Functions
{
	/// <summary>
	/// Option set manipulation routines.
	/// </summary>
	[FunctionClass("OptionSet Functions")]
	public class OptionSetFunctions : FunctionClassBase
	{
		/// <summary>Get the value of an option in a named optionset.</summary>
        /// <param name="project"/>
        /// <param name="optionSetName">The name of the optionset to get from.</param>
        /// <param name="optionName">The name of the option to get.</param>
        /// <returns>The value of the option or an empty string if no option defined.</returns>
        /// <include file='Examples\OptionSet\OptionSetGetValue.example' path='example'/>        
        [Function()]
		public static string OptionSetGetValue(Project project, string optionSetName, string optionName) 
        {
            OptionSet optionSet;
            if(project.NamedOptionSets.TryGetValue(optionSetName, out optionSet))
            {
               return StringUtil.GetValueOrDefault(optionSet.Options[optionName], String.Empty);
            }
            return string.Empty;
		}

        /// <summary>Check if the specified optionset is defined.</summary>
        /// <param name="project"/>
        /// <param name="optionSetName">The name of the optionset.</param>
        /// <returns>True if the option set is defined, otherwise False.</returns>returns>
        /// <include file='Examples\OptionSet\OptionSetExists.example' path='example'/>        
        [Function()]
        public static string OptionSetExists(Project project, string optionSetName) 
        {
            if (project.NamedOptionSets.Contains(optionSetName)) {
                return Boolean.TrueString;
            }
            return Boolean.FalseString;
        }
    
        /// <summary>Check if the specified option is defined within the specified optionset.</summary>
        /// <param name="project"/>
        /// <param name="optionSetName">The name of the optionset.</param>
        /// <param name="optionName">The name of the option.</param>
        /// <returns>True if the option is defined, otherwise False.</returns>returns>
        /// <include file='Examples\OptionSet\OptionSetOptionExists.example' path='example'/>        
        [Function()]
        public static string OptionSetOptionExists(Project project, string optionSetName, string optionName) 
        {
            OptionSet optionSet = project.NamedOptionSets[optionSetName];
            if (optionSet != null) {
                if (optionSet.Options.Contains(optionName)) {
                    return Boolean.TrueString;
                }
            }
            return Boolean.FalseString;
        }

        /// <summary>
        /// Undefine the specified optionset.
        /// </summary>
        /// <param name="project"></param>
        /// <param name="optionSetName">The optionset name to undefine.</param>
        /// <returns>True if the optionset was undefined properly, otherwise False.</returns>
        /// <include file='Examples\OptionSet\OptionSetUndefine.example' path='example'/>        
        [Function()]
        public static string OptionSetUndefine(Project project, string optionSetName) 
        {
             OptionSet oldSet;
             return project.NamedOptionSets.TryRemove(optionSetName, out oldSet).ToString();
        }

        /// <summary>
        /// Undefine the specified optionset option.
        /// </summary>
        /// <param name="project"></param>
        /// <param name="optionSetName">The name of the optionset.</param>
        /// <param name="optionName">The name of the option to undefine.</param>
        /// <returns>True if the optionset option was undefined properly, otherwise False.</returns>
        /// <include file='Examples\OptionSet\OptionSetOptionUndefine.example' path='example'/>        
        [Function()]
        public static string OptionSetOptionUndefine(Project project, string optionSetName, string optionName)
        {
            bool ret = false;
            OptionSet optionSet;
            if (project.NamedOptionSets.TryGetValue(optionSetName, out  optionSet))
            {
                ret = optionSet.Options.Remove(optionName);
            }
            return ret.ToString();
        }

        /// <summary>
        /// Output option set to string
        /// </summary>
        /// <param name="project"></param>
        /// <param name="optionSetName">The optionset name.</param>
        /// <returns>string with optionset content.</returns>
        [Function()]
        public static string OptionSetToString(Project project, string optionSetName)
        {
            var sb = new StringBuilder();
            sb.AppendLine("start optionset name=" + optionSetName);

            OptionSet optionSet;
            if (project.NamedOptionSets.TryGetValue(optionSetName, out  optionSet))
            {
                if(!String.IsNullOrEmpty(optionSet.FromOptionSetName))
                {
                    sb.AppendLine("    FromOptionSetName=" + optionSet.FromOptionSetName);
                }
                sb.AppendLine("    options");
                foreach (var option in optionSet.Options)
                {
                    sb.AppendLine("        "  +option.Key + "=" + option.Value);
                }
            }
            else
            {
                sb.AppendLine("    Does not exist");
            }
            sb.AppendLine("end optionset");
            return sb.ToString();
        }
    }
}