// Originally based on NAnt - A .NET build tool
// Copyright (C) 2003-2018 Electronic Arts Inc.
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
// Electronic Arts (Frostbite.Team.CM@ea.com)

using System;
using System.Collections;
using System.Collections.Generic;

namespace NAnt.Core.PackageCore
{
	// DAVE-FUTURE-REFACTOR-TODO how many of the details here really need to be public? need to check how this class is used in and out of Framework
	// and see if we can't start hiding some implementation details
	public sealed partial class MasterConfig
	{
		// masterconfig conditions are used select between various values based on whether their compare value
		// matches the evaluated value of the containing exceptions property in the context of the current project
		public abstract class Condition<TSelectType>
		{
			public abstract TSelectType Value { get; }

			public readonly string PropertyCompareValue;

			internal Condition(string compareValue)
			{
				PropertyCompareValue = compareValue;
			}
		}

		// convient implementation of Condition when we just need to choose between strings
		public sealed class StringCondition : Condition<string>
		{
			public override string Value => m_value;

			private readonly string m_value;

			internal StringCondition(string compareValue, string value)
				: base(compareValue)
			{
				m_value = value;
			}
		}

		// exceptions evaluate the given property name in the context of a project and return a matching
		// condition
		public sealed class Exception<TSelectType> : IEnumerable<Condition<TSelectType>>
		{
			public readonly string PropertyName;
			private readonly List<Condition<TSelectType>> m_conditions = new List<Condition<TSelectType>>();

			internal Exception(string propertyName)
			{
				PropertyName = propertyName;
			}

			public bool TryEvaluateException(Project project, out TSelectType matchingValue)
			{
				string propVal = project.Properties[PropertyName];
				if (!String.IsNullOrEmpty(propVal))
				{
					foreach (Condition<TSelectType> condition in m_conditions)
					{
						if (Expression.Evaluate(condition.PropertyCompareValue + "==" + propVal))
						{
							matchingValue = condition.Value;
							return true;
						}
					}
				}
				matchingValue = default;
				return false;
			}

			public IEnumerator<Condition<TSelectType>> GetEnumerator()
			{
				return m_conditions.GetEnumerator();
			}

			internal void Add(Condition<TSelectType> condition)
			{
				m_conditions.Add(condition);
			}

			IEnumerator IEnumerable.GetEnumerator()
			{
				return m_conditions.GetEnumerator();
			}
		}

		// container for exceptions, added exceptions are evaluated in order based on the current Project
		// and matching value is returned (or default if none of the exceptiosn apply
		public class ExceptionCollection<TSelectType> : IEnumerable<Exception<TSelectType>>
		{
			private readonly List<Exception<TSelectType>> m_exceptions = new List<Exception<TSelectType>>();

			public void Add(Exception<TSelectType> exception)
			{
				m_exceptions.Add(exception);
			}

			public TSelectType ResolveException(Project project, TSelectType defaultValue)
			{
				foreach (Exception< TSelectType> exception in m_exceptions)
				{
					if (exception.TryEvaluateException(project, out TSelectType matchedValue))
					{
						return matchedValue;
					}
				}

				return defaultValue;
			}

			public IEnumerator<Exception<TSelectType>> GetEnumerator()
			{
				return m_exceptions.GetEnumerator();
			}

			IEnumerator IEnumerable.GetEnumerator()
			{
				return m_exceptions.GetEnumerator();
			}
		}
	}
}