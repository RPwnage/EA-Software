// Originally based on NAnt - A .NET build tool
// Copyright (C) 2003-2015 Electronic Arts Inc.
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
// Electronic Arts (Frostbite.Team.CM@ea.com)
// Kosta Arvanitis (karvanitis@ea.com)

using NAnt.Core.Attributes;

namespace NAnt.Core.Tasks 
{
	/// <summary>Evaluate a block of code.</summary>
	/// <remarks>
	/// The eval task will evaluate a specified block of code and optionally store the result in a property. If 
	/// no property name is specified the result ignored. This task is useful for running functions which do not 
	/// require output.
	/// </remarks>
	/// <include file='Examples/Eval/Eval.example' path='example'/>
	/// <include file='Examples/Eval/EvalProperty.example' path='example'/>
	/// <include file='Examples/Eval/EvalExpression.example' path='example'/>
	[TaskName("eval", NestedElementsCheck = true)]
	public class EvalTask : Task {

		public enum EvalType {
			Property,
			Function,
			Expression,
		}

		/// <summary>The code to evaluate.</summary>
		[TaskAttribute("code", Required = true, ExpandProperties = false)]
		public string Code { get; set; } = null;

		/// <summary>The type of code to evaluate. Valid values are <c>Property</c>, <c>Function</c> and <c>Expression</c>.</summary>
		[TaskAttribute("type", Required = true)]
		public EvalType Type { get; set; }

		/// <summary>The name of the property to place the result into. If none is specified result is ignored.</summary>
		[TaskAttribute("property", Required = false)]
		public string PropertyName { get; set; } = null;

		protected override void ExecuteTask() {

			string val;
			
			try {
				switch(Type) {
					case EvalType.Expression:
						val = Project.ExpandProperties(Code);
						val = Expression.Evaluate(val).ToString();
						break;
					case EvalType.Function:
					case EvalType.Property:
						val = Project.ExpandProperties(Code);
						break;
					default:
						// we should never get here
						val = "";
						break;
				}
				if (PropertyName != null) {
					Project.Properties.Add(PropertyName, val);
				}
			}
			catch(System.Exception e) {
				throw new BuildException("Failed to evaluate code.", e);
			}
		}
	}
}
