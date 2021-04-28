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
// Scott Hernandez (ScottHernandez@hotmail.com)

using System;
using System.IO;

using NAnt.Core.Attributes;
using NAnt.Core.Logging;

namespace NAnt.Core.Tasks 
{
	/// <summary>Checks conditional attributes.  (Deprecated)</summary>
	/// <remarks>
	///     <para>This task is deprecated.  Use the <see cref="DoTask"/> with functions instead.</para>
	/// </remarks>
	[TaskName("if")]
	public class IfTask : TaskContainer
	{
        
		protected string _propNameTrue = null;
		protected string _propNameExists = null;
		protected string _targetName = null;

		/// <summary>Check the property value is true.  If property does not exist a build error will occur.</summary>
		[TaskAttribute("propertytrue")]
		public string PropertyNameTrue 
		{
			set {_propNameTrue = value;}
		}

		/// <summary>Check existance of a property.</summary>
		[TaskAttribute("propertyexists")]
		public string PropertyNameExists 
		{
			set {_propNameExists = value;}
		}

		/// <summary>Check existance of a target.</summary>
		[TaskAttribute("targetexists")]
		public string TargetNameExists 
		{
			set {_targetName = value;}
		}

		protected override void ExecuteTask() 
		{
			if (Project.Properties["package.frameworkversion"] == "1")
			{
				Log.Info.WriteLine(LogPrefix + "*** Using Deprecated task <{0}> ***", Name);
				if (ConditionsTrue) 
				{
					base.ExecuteTask();
				}
			}
			else
			{
				throw new BuildException(string.Format("Task <{0}> is unsupported for a Framework 2 package", Name), Location);
			}
		}

		protected virtual bool ConditionsTrue 
		{
			get 
			{
				bool ret = true;

				//check for target
				if(_targetName != null) 
				{
					ret = ret && (Project.Targets.Find(_targetName) != null);
					if (!ret) return false;
				}

				//Check for the Property value of true.
				if(_propNameTrue != null) 
				{
					try 
					{
						ret = ret && bool.Parse(Properties[_propNameTrue]);
					}
					catch (Exception e) 
					{
						throw new BuildException("Property True test failed for '" + _propNameTrue + "'", Location, e);
					}
				}

				//Check for Property existance
				if(_propNameExists != null) 
				{
					ret = ret && (Properties[_propNameExists] != null);
				}

				return ret;
			}
		}
	}

	/// <summary>
	/// Opposite of if task.  (Deprecated)
	/// </summary>
	/// <remarks>
	///     <para>This task is deprecated.  Use the <see cref="DoTask"/> with functions instead.</para>
	/// </remarks>
	[TaskName("ifnot")]
	public class IfNotTask : IfTask
	{
		protected override bool ConditionsTrue 
		{
			get 
			{
				return !base.ConditionsTrue;
			}
		}
	}
}
