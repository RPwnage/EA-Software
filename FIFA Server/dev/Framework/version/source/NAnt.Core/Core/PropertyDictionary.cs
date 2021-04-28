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
// 
// 
// File Maintainers:
// Electronic Arts (Frostbite.Team.CM@ea.com)
// Gerry Shaw (gerry_shaw@yahoo.com)

using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using System.Threading;

using NAnt.Core.Util;

namespace NAnt.Core
{
	public sealed class PropertyDictionary : IEnumerable<Property>, IDisposable
	{
		// PropertyReadScope for access of multiple properties without overhead
		public struct PropertyReadScope : IDisposable
		{
			internal readonly PropertyDictionary m_properties;

			internal PropertyReadScope(PropertyDictionary propertyDictionary)
			{
				m_properties = propertyDictionary;
				propertyDictionary.m_propertiesLock.EnterReadLock();
			}

			public string GetProperty(string name)
			{
				PropertyKey key = PropertyKey.Create(name);
				return GetProperty(key);
			}

			public string GetProperty(PropertyKey key)
			{
				Property property = null;
				bool isLocal = m_properties.LocalContext.TryGetValue(key, out property);
				if (!isLocal)
				{
					m_properties.m_properties.TryGetValue(key, out property);
				}

				string value = null;
				if (property != null)
				{
					value = property.Value;
					if (property.Deferred)
					{
						value = m_properties.ExpandProperties(property.Value);
					}
				}
				return value;
			}

			public string this[string name]
			{
				get { return GetProperty(name); }
			}

			public string this[PropertyKey key]
			{
				get { return GetProperty(key); }
			}

			public void Dispose()
			{
				m_properties.m_propertiesLock.ExitReadLock();
			}
		}

		// PropertyWriteScope for access of multiple properties without overhead
		public struct PropertyWriteScope : IDisposable
		{
			internal readonly PropertyDictionary m_properties;

			internal PropertyWriteScope(PropertyDictionary propertyDictionary)
			{
				m_properties = propertyDictionary;
				propertyDictionary.m_propertiesLock.EnterWriteLock();
			}

			public void SetOrAdd(string name, string value, bool readOnly = false, bool local = false, string defaultValue = "")
			{
				var key = PropertyKey.Create(name);
				m_properties.UnsafeSetOrAdd(key, v => value, readOnly, local, defaultValue);
			}

			public void SetOrAdd(PropertyKey key, string value, bool readOnly = false, bool local = false, string defaultValue = "")
			{
				m_properties.UnsafeSetOrAdd(key, v => value, readOnly, local, defaultValue);
			}

			public void SetOrAdd(string name, Func<string, string> valueFactory, bool readOnly = false, bool local = false, string defaultValue = "")
			{
				var key = PropertyKey.Create(name);
				m_properties.UnsafeSetOrAdd(key, valueFactory, readOnly, local, defaultValue);
			}
			public void SetOrAdd(PropertyKey key, Func<string, string> valueFactory, bool readOnly = false, bool local = false, string defaultValue = "")
			{
				m_properties.UnsafeSetOrAdd(key, valueFactory, readOnly, local, defaultValue);
			}

			public void Dispose()
			{
				m_properties.m_propertiesLock.ExitWriteLock();
			}
		}

		private readonly Dictionary<PropertyKey, Property> m_properties;

        private readonly ReaderWriterLockSlim m_propertiesLock;
		private bool m_disposed = false;

		private static HashSet<string> s_traceProperties = null;
		private static bool s_breakOnTraceProp = false;

		public PropertyDictionary(Project project)
		{
			Project = project;
			LocalContext = new Dictionary<PropertyKey, Property>();

			m_propertiesLock = new ReaderWriterLockSlim(LockRecursionPolicy.SupportsRecursion);
			m_properties = new Dictionary<PropertyKey, Property>();
		}

		// creates a copy instance of a property dictionary with local context
		public PropertyDictionary(Project project, PropertyDictionary globalContext)
		{
			Project = project;
			LocalContext = new Dictionary<PropertyKey, Property>();

			m_propertiesLock = globalContext.m_propertiesLock;
			m_properties = globalContext.m_properties;
		}

		// creates a copy attached to a different project
		public PropertyDictionary(Project project, Project oldProject)
		{
			Project = project;

			m_propertiesLock = new ReaderWriterLockSlim(LockRecursionPolicy.SupportsRecursion);

			oldProject.Properties.m_propertiesLock.EnterReadLock();
			try
			{;
				m_properties = new Dictionary<PropertyKey, Property>(oldProject.Properties.m_properties.Count);
				foreach (var prop in oldProject.Properties.m_properties)
				{
					m_properties.Add(prop.Key, new Property(prop.Value));
				}

				LocalContext = new Dictionary<PropertyKey, Property>(oldProject.Properties.LocalContext.Count);
				foreach (var prop in oldProject.Properties.LocalContext)
				{
					LocalContext.Add(prop.Key, new Property(prop.Value));
				}
			}
			finally
			{
				oldProject.Properties.m_propertiesLock.ExitReadLock();
			}
		}

        public PropertyReadScope ReadScope()
        {
            return new PropertyReadScope(this);
        }

        public void Clear()
		{
			m_properties.Clear();
			LocalContext.Clear();
		}

		public void Add(string name, string value, bool readOnly = false, bool deferred = false, bool local = false, bool inheritable = false)
		{
			Add(PropertyKey.Create(name), value, readOnly, deferred, local, inheritable);
		}

		public void Add(PropertyKey key, string value, bool readOnly = false, bool deferred = false, bool local = false, bool inheritable = false)
		{
			Property property = new Property(key, value, readOnly, deferred, inheritable);
			Add(property, local);
		}

		public void AddLocal(string name, string value, bool readOnly = false, bool deferred = false, bool inheritable = false)
		{
			Add(PropertyKey.Create(name), value, readOnly, deferred, true, inheritable);
		}

		public void AddLocal(PropertyKey key, string value, bool readOnly = false, bool deferred = false, bool inheritable = false)
		{
			Add(key, value, readOnly, deferred, true, inheritable);
		}

		public void AddReadOnly(string name, string value, bool deferred = false, bool local = false, bool inheritable = false)
		{
			Add(PropertyKey.Create(name), value, true, deferred, local, inheritable);
		}

		public void AddReadOnly(PropertyKey key, string value, bool deferred = false, bool local = false, bool inheritable = false)
		{
			Add(key, value, true, deferred, local, inheritable);
		}

		/// <summary>Removes a property if it exists, returns a boolean to indicate if the property was found</summary>
		public bool Undefine(string name)
		{
			if (Contains(name))
			{
				Remove(name);
				return true;
			}
			return false;
		}

		public void Remove(string name)
		{
			if (name == null)
			{
				throw new ArgumentNullException("name");
			}

			m_propertiesLock.EnterUpgradeableReadLock();
			try
			{
				var key = PropertyKey.Create(name);
				Property property = FindProperty(key);
				if (property != null && property.ReadOnly)
				{
					string msg = String.Format("Cannot remove readonly property '{0}'.", name);
					throw new InvalidOperationException(msg);
				}
				m_propertiesLock.EnterWriteLock();
				try
				{
					if (!LocalContext.Remove(key))
					{
						m_properties.Remove(key);
					}
				}
				finally
				{
					m_propertiesLock.ExitWriteLock();
				}
				PropertyRemoved(property);
			}
			finally
			{
				m_propertiesLock.ExitUpgradeableReadLock();
			}

		}

		public string this[string name]
		{
			get
			{
				return EvaluateParameter(name, null);
			}
			set
			{
				Add(name, value);
			}
		}

		public string this[PropertyKey key]
		{
			get
			{
				return EvaluateParameter(key, null);
			}
			set
			{
				Add(key, value);
			}
		}

		public void SetOrAdd(string name, Func<string, string> valueFactory, bool readOnly = false, bool local = false, string defaultValue = "")
		{
			var key = PropertyKey.Create(name);
			SetOrAdd(key, valueFactory, readOnly, local, defaultValue);
		}

		public void SetOrAdd(PropertyKey key, Func<string, string> valueFactory, bool readOnly = false, bool local = false, string defaultValue = "")
		{
			try
			{
				m_propertiesLock.EnterWriteLock();
				UnsafeSetOrAdd(key, valueFactory, readOnly, local, defaultValue);
			}
			finally
			{
				m_propertiesLock.ExitWriteLock();
			}
		}

		private void UnsafeSetOrAdd(PropertyKey key, Func<string, string> valueFactory, bool readOnly, bool local, string defaultValue)
		{
			bool isLocal;
			Property oldProperty = FindProperty(key, out isLocal);
			if (oldProperty != null && oldProperty.ReadOnly)
			{
				return;
			}

			if (oldProperty == null)
			{
				if (!Property.VerifyPropertyName(key.AsString()))
				{
					throw new BuildException(String.Format("Invalid property name '{0}'.", key.AsString()));
				}
				isLocal = local;
			}

			Property newProperty = new Property(key, valueFactory(oldProperty == null ? defaultValue : oldProperty.Value), readOnly);

			if (isLocal)
			{
				LocalContext[key] = newProperty;
			}
			else
			{
				m_properties[key] = newProperty;
			}
			PropertyChanged(newProperty);
		}

		public PropertyWriteScope WriteScope()
		{
			return new PropertyWriteScope(this);
		}

		// Internal use only, use caution!
		public string UpdateReadOnly(string name, string value, bool local = false, bool inheritable = false)
		{
			m_propertiesLock.EnterUpgradeableReadLock();
			try
			{
				PropertyKey key = PropertyKey.Create(name);
				Property property = FindProperty(key, out bool isLocal);
				if (property == null || (!isLocal && local))
				{
					AddReadOnly(name, value, local: local, inheritable: inheritable);
				}
				else
				{
					string retVal;
					m_propertiesLock.EnterWriteLock();
					try
					{
						retVal = property.Value;
						property.ReadOnly = false;
						property.Inheritable = inheritable;
						property.Value = value;
						property.ReadOnly = true;
					}
					finally
					{
						m_propertiesLock.ExitWriteLock();
					}

					PropertyChanged(property);

					return retVal;
				}
			}
			finally
			{
				m_propertiesLock.ExitUpgradeableReadLock();
			}
			return null;
		}

		public void RemoveReadOnly(string name)
		{
			if (name == null)
			{
				throw new ArgumentNullException("name");
			}

			var key = PropertyKey.Create(name);

			m_propertiesLock.EnterUpgradeableReadLock();
			try
			{
				Property property = FindProperty(key);
				if (property != null)
				{
					m_propertiesLock.EnterWriteLock();
					try
					{
						if (!LocalContext.Remove(key))
							m_properties.Remove(key);
					}
					finally
					{
						m_propertiesLock.ExitWriteLock();
					}

					PropertyRemoved(property);
				}
			}
			finally
			{
				m_propertiesLock.ExitUpgradeableReadLock();
			}

		}

        public void ForceNonLocal(string name)
        {
			var key = PropertyKey.Create(name);

			m_propertiesLock.EnterUpgradeableReadLock();
            try
            {
                Property property = null;
				if (LocalContext.TryGetValue(key, out property))
                {
                    m_propertiesLock.EnterWriteLock();
                    try
                    {
                        LocalContext.Remove(key);
                        m_properties[key] = property;
                    }
                    finally
                    {
                        m_propertiesLock.ExitWriteLock();
                    }
                }
            }
            finally
            {
                m_propertiesLock.ExitUpgradeableReadLock();
            }
        }

        public bool Contains(string name)
		{
			var key = PropertyKey.Create(name);
			m_propertiesLock.EnterReadLock();
			try
			{
				return (FindProperty(key) != null);
			}
			finally
			{
				m_propertiesLock.ExitReadLock();
			}
		}

		public bool IsPropertyReadOnly(string name)
		{
			var key = PropertyKey.Create(name);
			m_propertiesLock.EnterReadLock();
			try
			{
				Property property = FindProperty(key);
				if (property != null)
				{
					return property.ReadOnly;
				}
				else
				{
					return false;
				}
			}
			finally
			{
				m_propertiesLock.ExitReadLock();
			}
		}

		public bool IsPropertyLocal(string name)
		{
			bool local;
			var key = PropertyKey.Create(name);
			m_propertiesLock.EnterReadLock();
			try
			{
				FindProperty(key, out local);
			}
			finally
			{
				m_propertiesLock.ExitReadLock();
			}
			return local;
		}

		public Project Project { get;
		//set { _project = value; }
		}

		/// <summary>
		/// Property evaluator callback without deprecated functions.
		/// </summary>
		/// <param name="key">The name of the property to evaluate.</param>
		/// <param name="evaluationStack">Evaluation stack for deferred expansions to prevent recursive self expansion.</param>
		/// <returns>The parameters evaluated value.</returns>
		public string EvaluateParameter(PropertyKey key, Stack<string> evaluationStack)
		{
			Property property;

			m_propertiesLock.EnterReadLock();

			bool isLocal = LocalContext.TryGetValue(key, out property);
			if (!isLocal)
			{
				m_properties.TryGetValue(key, out property);
			}

			m_propertiesLock.ExitReadLock();

			string value = null;
			if (property != null)
			{
				value = (property.Deferred) ? ExpandProperties(property.Value, evaluationStack) : property.Value;
			}

			return value;
		}

		public string EvaluateParameter(string name)
		{
			var key = PropertyKey.Create(name);
			return EvaluateParameter(key, null);
		}

		public string EvaluateParameter(string name, Stack<string> evaluationStack)
		{
			var key = PropertyKey.Create(name);
			return EvaluateParameter(key, evaluationStack);
		}
		/// <summary>
		/// Function evaluator callback for running functions.
		/// </summary>
		public string EvaluateFunction(string functionName, List<string> paramList)
		{
			return Project.FuncFactory.Run(functionName, paramList, Project);
		}

        public IEnumerable<string> FindClosestMatches(string name)
        {
            IEnumerable<string> allPropertyKeys = LocalContext.Keys.Select(key => key.Str).Concat(m_properties.Keys.Select(key => key.Str));
            return StringUtil.FindClosestMatchesFromList(allPropertyKeys, name);
        }

        /// <summary>Expands a string with macros.</summary>
        /// <param name="expression">The string with macros to expand.</param>
        /// <returns>The string with all macros expanded.</returns>
        public string ExpandProperties(string expression)
		{
			return ExpandProperties(expression, null);
		}

		private string ExpandProperties(string expression, Stack<string> evaluationStack)
		{
			return StringParser.ExpandString(expression, EvaluateParameter, EvaluateFunction, FindClosestMatches, ref evaluationStack);
		}

		internal Dictionary<PropertyKey, Property> LocalContext { get; }

		private void Add(Property property, bool local)
		{
			PropertyKey key = property.Key;
			Property existingProperty = null;

			if (local)
			{
				m_propertiesLock.EnterUpgradeableReadLock();
				try
				{
					existingProperty = FindLocalProperty(key);
					if (existingProperty == null || !existingProperty.ReadOnly)
					{
						if (existingProperty == null && !Property.VerifyPropertyName(property.Name))
						{
							String msg = String.Format("Invalid property name '{0}'.", property.Name);
							throw new BuildException(msg);
						}

						m_propertiesLock.EnterWriteLock();
						try
						{
							LocalContext[key] = property;
						}
						finally
						{
							m_propertiesLock.ExitWriteLock();
						}

						PropertyChanged(property);
					}
				}
				finally
				{
					m_propertiesLock.ExitUpgradeableReadLock();
				}
			}
			else
			{
				m_propertiesLock.EnterUpgradeableReadLock();
				try
				{

					existingProperty = FindProperty(key, out bool isLocal);
					if (existingProperty == null || !existingProperty.ReadOnly)
					{
						if (existingProperty == null && !Property.VerifyPropertyName(property.Name))
						{
							String msg = String.Format("Invalid property name '{0}'.", property.Name);
							throw new BuildException(msg);
						}

						if (isLocal)
						{
							m_propertiesLock.EnterWriteLock();
							try
							{
								LocalContext[key] = property;
							}
							finally
							{
								m_propertiesLock.ExitWriteLock();
							}
						}
						else
						{
							m_propertiesLock.EnterWriteLock();
							try
							{
								m_properties[key] = property;
							}
							finally
							{
								m_propertiesLock.ExitWriteLock();
							}
						}

						PropertyChanged(property);
					}
				}
				finally
				{
					m_propertiesLock.ExitUpgradeableReadLock();
				}

			}

			if (existingProperty != null && existingProperty.ReadOnly)
			{
				Project.Log.Debug.WriteLine(Project.LogPrefix + "Attempting to overwrite readonly property: " + existingProperty.Name);
			}
		}

		private Property FindProperty(PropertyKey key)
		{
			if (!LocalContext.TryGetValue(key, out Property property))
			{
				m_properties.TryGetValue(key, out property);
			}
			return property;
		}

		private Property FindProperty(PropertyKey key, out bool local)
		{
			local = LocalContext.TryGetValue(key, out Property property);
			if (!local)
			{
				m_properties.TryGetValue(key, out property);
			}
			return property;
		}

		private Property FindLocalProperty(PropertyKey key)
		{
			Property property = null;
			LocalContext.TryGetValue(key, out property);
			return property;
		}

		public IEnumerator<Property> GetEnumerator()
		{
			List<Property> global;
			List<Property> local;
			m_propertiesLock.EnterReadLock();
			try
			{
				local = new List<Property>(LocalContext.Values);
				global = new List<Property>(m_properties.Values);
			}
			finally
			{
				m_propertiesLock.ExitReadLock();
			}

			foreach (Property p in local)
			{
				yield return p;
			}

			foreach (Property p in global)
			{
				var key = p.Key;
				if (!LocalContext.ContainsKey(key))
				{
					yield return p;
				}
			}
		}

		public IEnumerable<Property> GetGlobalProperties()
		{
			m_propertiesLock.EnterReadLock();
			var list = new List<Property>(m_properties.Values);
			m_propertiesLock.ExitReadLock();
			return list;
		}

		public IEnumerable<Property> GetLocalProperties()
		{
			m_propertiesLock.EnterReadLock();
			var list = new List<Property>(LocalContext.Values);
			m_propertiesLock.ExitReadLock();
			return list;
		}

		IEnumerator IEnumerable.GetEnumerator()
		{
			return GetEnumerator();
		}

		public void ForEach(Action<Property> action)
		{
			foreach (Property item in this) action.Invoke(item);
		}

		public void Dispose()
		{
			if (!m_disposed)
			{
				m_disposed = true;
				m_propertiesLock.Dispose();
			}		
		}

		public static void SetTraceProperties(IEnumerable<string> tracePropertyNames, bool breakOnTraceProp)
		{
			if (tracePropertyNames.Any())
			{
				s_traceProperties = new HashSet<string>(tracePropertyNames.Select(x => x.ToLower()));
			}
			else
			{
				s_traceProperties = null;
			}
			s_breakOnTraceProp = breakOnTraceProp;
		}

		private void PropertyChanged(Property changedProperty)
		{
			if (s_traceProperties != null && changedProperty != null && s_traceProperties.Contains(changedProperty.Name.ToLower()))
			{
				string qualifiers = ((changedProperty.ReadOnly ? "readonly" : String.Empty) + " " + (changedProperty.Deferred ? "deferred" : String.Empty)).TrimWhiteSpace();
				Project.Log.Minimal.WriteLine("TRACE set property {0}={1}{2} at {3}", changedProperty.Name, changedProperty.Value, (String.IsNullOrEmpty(qualifiers) ? String.Empty : " [" + qualifiers.ToArray().ToString("," + "]") + "]"), Project.CurrentScriptFile);
				if (s_breakOnTraceProp)
				{
					if (System.Diagnostics.Debugger.IsAttached)
					{
						System.Diagnostics.Debugger.Break();
					}
					else
					{
						System.Diagnostics.Debugger.Launch();
					}
				}
			}
		}

		private void PropertyRemoved(Property removedProperty)
		{
			if (s_traceProperties != null && removedProperty != null && s_traceProperties.Contains(removedProperty.Name))
			{
				string qualifiers = ((removedProperty.ReadOnly ? "readonly" : String.Empty) + " " + (removedProperty.Deferred ? "deferred" : String.Empty)).TrimWhiteSpace();
				Project.Log.Minimal.WriteLine("TRACE removed property {0}={1}{2} at {3}", removedProperty.Name, removedProperty.Value, (String.IsNullOrEmpty(qualifiers) ? String.Empty : " [" + qualifiers.ToArray().ToString("," + "]") + "]"), Project.CurrentScriptFile);
				if (s_breakOnTraceProp)
				{
					if (System.Diagnostics.Debugger.IsAttached)
					{
						System.Diagnostics.Debugger.Break();
					}
					else
					{
						System.Diagnostics.Debugger.Launch();
					}
				}
			}
		}
	}
}
