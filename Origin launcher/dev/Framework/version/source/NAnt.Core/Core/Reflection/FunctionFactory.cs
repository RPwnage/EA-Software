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
// Gerry Shaw (gerry_shaw@yahoo.com)
// Ian MacLean (ian_maclean@another.com)
// Electronic Arts (Frostbite.Team.CM@ea.com)

using System;
using System.Collections.Generic;
using System.Collections.Concurrent;
using System.Reflection;
using System.Threading;
using System.Linq;

using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Threading;
using NAnt.Core.Util;

namespace NAnt.Core.Reflection
{
	public sealed class FunctionFactory
	{
		private class MethodKey
		{
			readonly internal string Name;
			readonly internal int NumParams;

			internal MethodKey(string name, int numParams)
			{
				Name = name;
				NumParams = numParams;
			}

			public override int GetHashCode()
			{
				return Name.GetHashCode() ^ NumParams;
			}

			public override bool Equals(object obj)
			{
				MethodKey other = (MethodKey)obj; // assume obj is always a MethodKey, this is a private nested class so this should only be called in controlled circumstances
				if (Name != other.Name)
				{
					return (false);
				}
				return (NumParams == other.NumParams);
			}
		};

		private readonly ConcurrentDictionary<MethodKey, MethodInfo> m_methodCollection;

		private static readonly Lazy<FunctionFactory> s_globalFunctionFactory = new Lazy<FunctionFactory>(() => new FunctionFactory(AssemblyLoader.WellKnownAssemblies));

		internal FunctionFactory()
		{
			m_methodCollection = new ConcurrentDictionary<MethodKey, MethodInfo>(s_globalFunctionFactory.Value.m_methodCollection);
		}

		internal FunctionFactory(FunctionFactory other)
		{
			m_methodCollection = new ConcurrentDictionary<MethodKey, MethodInfo>(other.m_methodCollection);
		}

		// constructor for the global function factory
		private FunctionFactory(IEnumerable<Assembly> assemblies)
		{
			m_methodCollection = new ConcurrentDictionary<MethodKey, MethodInfo>();
			foreach (Assembly assembly in assemblies)
			{
				AddFunctions(assembly);
			}
		}

		public bool Contains(string name)
		{
			foreach (var pair in m_methodCollection)
			{
				if (pair.Key.Name.Equals(name))
					return true;
			}
			return false;
		}

		/// <summary>
		/// Run a function given the NAnt function name. 
		/// This corresponds to the name defined in the FunctionAttribute by the static method.
		/// </summary>
		public string Run(string functionName, List<string> parameterList, Project project)
		{
			int numParam = parameterList.Count + 1; // add one for project

			bool retval = m_methodCollection.TryGetValue(new MethodKey(functionName, numParam), out MethodInfo methodInfo);

			// check for existence of function
			if (retval == false || methodInfo == null)
			{
				string msg = String.Format("Function '{0}' with {1} parameters is undeclared.", functionName, numParam - 1);

				string found = m_methodCollection.Where(kvp => kvp.Key.Name == functionName).Select(kvp => kvp.Value)
					.ToString(Environment.NewLine + "\t", mi => mi.ReturnType.Name + " " + mi.Name + "(" + mi.GetParameters().ToString(", ", par => par.Name) + ")");

				if (!String.IsNullOrWhiteSpace(found))
				{
					msg += " Found methods:" + Environment.NewLine + "\t" + found;
				}
				throw new BuildException(msg);
			}

			// set up the parameters
			ParameterInfo[] parameterInfo = methodInfo.GetParameters();
			object[] paramObj = new object[parameterInfo.Length];
			try
			{
				// the first parameter is the project
				paramObj[0] = project;

				// convert each string parameter into its requested type
				for (int i = 0; i < parameterList.Count; i++)
				{
					try
					{
						Type type = parameterInfo[i + 1].ParameterType;
						paramObj[i + 1] = PropertyConverter.Convert(type, parameterList[i]);
					}
					catch (Exception e)
					{
						string msg = String.Format("Failed to change parameter '{0}' with value '{1}' to type '{2}' in function '{3}'.",
							i, parameterList[i], parameterInfo[i + 1].ParameterType.ToString(), functionName);
						throw new BuildException(msg, e);
					}
				}

				// looping from + 1 because of paramObj[0] being project
				for (int i = parameterList.Count + 1; i < parameterInfo.Length; ++i)
				{
					paramObj[i] = parameterInfo[i].DefaultValue;
				}
			}
			catch (Exception e)
			{
				ThreadUtil.RethrowThreadingException("Error in FunctionFactory", Location.UnknownLocation, e);
			}

			// invoke the function to evaluates the string
			string eval = null;

			try
			{
				eval = (string)methodInfo.Invoke(this, paramObj);
			}
			catch (Exception e)
			{
				string msg = String.Format("Function '{0}' failed to execute.", functionName);
				throw new BuildException(msg, e.InnerException);
			}
			return eval;
		}

		/// <summary>Scans the given assembly for any classes derived from Function.</summary>
		/// <param name="assembly">The Assembly containing the new functions to be loaded.</param>
		/// <param name="overrideExistingFunc"></param>
		/// <param name="failOnError"></param>
		/// <param name="assemblyName"></param>
		/// <returns>The count of tasks found in the assembly.</returns>
		internal int AddFunctions(Assembly assembly, bool overrideExistingFunc, bool failOnError, string assemblyName)
		{
			int retVal = 0;
			try
			{
				NAntUtil.ParallelForEach
				(
					assembly.GetTypes(),
					type =>
					{
						if (Attribute.GetCustomAttribute(type, typeof(FunctionClassAttribute)) is FunctionClassAttribute)
						{
							Interlocked.Add(ref retVal, AddMethods(type, overrideExistingFunc, failOnError));
						}
					},
					noParallelOnMono: true // in older mono, serial execution was faster than parallel - hasn't been tested in newer versions
				);
			}
			catch (Exception e)
			{
				Exception ex = ThreadUtil.ProcessAggregateException(e);

				string assemblyLocation = String.IsNullOrEmpty(assembly.Location) ? assemblyName : assembly.Location;
				string msg = String.Format("Could not load functions from '{0}'. {1}", assemblyLocation, ex.Message);
				if (ex is ReflectionTypeLoadException)
				{
					msg = String.Format("Could not load functions from '{0}'. Reasons:", assemblyLocation);
					foreach (string reason in (ex as ReflectionTypeLoadException).LoaderExceptions.Select(le => le.Message).OrderedDistinct())
					{
						msg += Environment.NewLine + "\t" + reason;
					}

					ex = null; // We retrieved messages from inner exception, no need to add it.
				}
				throw new BuildException(msg, ex);
			}
			return retVal;
		}

		internal int AddFunctions(Assembly assembly)
		{
			return AddFunctions(assembly, false, false, "unknown assembly location");
		}

		private int AddMethods(Type type, bool overrideExistingFunc, bool failOnError)
		{
			int retVal = 0;
			NAntUtil.ParallelForEach
			(
				type.GetMethods(),
				methodInfo =>
				{
					if (Attribute.GetCustomAttribute(methodInfo, typeof(FunctionAttribute)) is FunctionAttribute)
					{
						Interlocked.Add(ref retVal, AddMethod(methodInfo, overrideExistingFunc, failOnError));
					}
				},
				noParallelOnMono: true
			);

			return retVal;
		}

		private int AddMethod(MethodInfo methodInfo, bool overrideExistingMethod, bool failOnError)
		{
			// check for static method
			if (methodInfo.IsStatic == false)
			{
				string msg = String.Format("Function '{0}' must be declared static.", methodInfo.Name);
				throw new BuildException(msg);
			}

			// check return type
			if (methodInfo.ReturnType != typeof(String))
			{
				string msg = String.Format("Function '{0}' must return a string.", methodInfo.Name);
				throw new BuildException(msg);
			}

			// get parameters
			ParameterInfo[] parameterInfo = methodInfo.GetParameters();

			// check first parameter type
			if ((parameterInfo.Length < 1 || parameterInfo[0].ParameterType != typeof(Project)))
			{
				string msg = String.Format("Function '{0}' must have the first parameter set to type {1}.", methodInfo.Name, typeof(Project).ToString());
				throw new BuildException(msg);
			}

			int ret = 0;
			MethodKey[] methodKeys = GetMethodsKeys(methodInfo);
			foreach (MethodKey methodKey in methodKeys)
			{
				// check if function exists, functions are distinguished by name and number of parameters (not type)
				if (overrideExistingMethod)
				{
					m_methodCollection[methodKey] = methodInfo;
					++ret;
				}
				else
				{
					if (m_methodCollection.TryAdd(methodKey, methodInfo))
					{
						++ret;
					}
					else if (failOnError)
					{
						throw new BuildException("ERROR: Function '" + methodInfo.Name + "' already exists!");
					}
				}
			}

			return ret;
		}

		private MethodKey[] GetMethodsKeys(MethodInfo methodInfo)
		{
			string methodName = methodInfo.Name;

			// get number of required parameters (those without defaults)
			// and total number of parameters
			ParameterInfo[] parameters = methodInfo.GetParameters();
			int numRequired = parameters.Count(pInfo => pInfo.DefaultValue == DBNull.Value);
			int numTotal = parameters.Length;

			// generate a method key for every valid number of parameters
			return Enumerable.Range(numRequired, 1 + numTotal - numRequired)
				.Select(i => new MethodKey(methodName, i))
				.ToArray();
		}

		private static FunctionAttribute GetFunctionAttribute(MethodInfo method)
		{
			object[] functionAttributes = method.GetCustomAttributes(typeof(FunctionAttribute), false);
			if (functionAttributes.Length > 0)
			{
				return (FunctionAttribute)functionAttributes[0];
			}
			return null;
		}
	}
}
