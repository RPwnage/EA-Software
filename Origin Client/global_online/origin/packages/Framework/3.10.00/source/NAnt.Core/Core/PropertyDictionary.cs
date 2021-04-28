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
// File Maintainers:
// Gerry Shaw (gerry_shaw@yahoo.com)

using System;
using System.Collections;
using System.Collections.Generic;
using System.Text.RegularExpressions;
using System.Threading;

using NAnt.Core.Events;
using NAnt.Core.Logging;

namespace NAnt.Core
{

    public sealed class PropertyDictionary : IEnumerable<Property>, IDisposable
    {
        private readonly Project _project;
        private readonly Dictionary<string,Property> _properties;
        private readonly Dictionary<string, Property> _localContext;
        private readonly StringParser.PropertyEvaluator _evaluateProperty;
        private readonly StringParser.FunctionEvaluator _evaluateFunction;
        private readonly ReaderWriterLockSlim _propertiesLock;
        private bool _disposed = false;

        public event PropertyEventHandler PropertyChanged;
        public event PropertyEventHandler PropertyRemoved;

        public PropertyDictionary(Project project)
        {
            _project = project;

            _propertiesLock = new ReaderWriterLockSlim(LockRecursionPolicy.SupportsRecursion);
            _properties = new Dictionary<string, Property>();

#if NANT_COMPATIBILITY
            _evaluateProperty = new StringParser.PropertyEvaluator(EvaluateParameterDeprecated);
#else
            _evaluateProperty = new StringParser.PropertyEvaluator(EvaluateParameter);
#endif
            _evaluateFunction = new StringParser.FunctionEvaluator(EvaluateFunction);
            _localContext = new Dictionary<string, Property>();
        }

        // creates a copy instance of a property dictionary with local context
        public PropertyDictionary(Project project, PropertyDictionary globalContext, bool propagateLocalContext = false )
        {
            _project = project;

            _propertiesLock = globalContext._propertiesLock;
            _properties = globalContext._properties;

            _evaluateProperty = new StringParser.PropertyEvaluator(EvaluateParameter);
            _evaluateFunction = new StringParser.FunctionEvaluator(EvaluateFunction);
            _disposed = true;
            _localContext = propagateLocalContext ? new Dictionary<string, Property>(globalContext._localContext) : new Dictionary<string, Property>();
        }

        // creates a copy attached to a different project
        public PropertyDictionary(Project project, Project oldProject)
        {
            _project = project;

            _propertiesLock = new ReaderWriterLockSlim(LockRecursionPolicy.SupportsRecursion);
            _properties = new Dictionary<string, Property>(oldProject.Properties._properties);

#if NANT_COMPATIBILITY
            _evaluateProperty = new StringParser.PropertyEvaluator(EvaluateParameterDeprecated);
#else
            _evaluateProperty = new StringParser.PropertyEvaluator(EvaluateParameter);
#endif
            _evaluateFunction = new StringParser.FunctionEvaluator(EvaluateFunction);
            _localContext = new Dictionary<string, Property>(oldProject.Properties._localContext);
        }


        ~PropertyDictionary()
        {
            // clear any events so that we can be garbage collected
            PropertyChanged = null;
            PropertyRemoved = null;
        }

        public void Clear()
        {
            _properties.Clear();
            _localContext.Clear();

        }

        public void Add(string name, string value)
        {
            Property p = new Property(name, value, false, false);
            Add(p, false);
        }

        public void AddLocal(string name, string value)
        {
            Property p = new Property(name, value, false, false);
            Add(p, true);
        }

        public void AddReadOnly(string name, string value)
        {
            Property p = new Property(name, value, true, false);
            Add(p, false);
        }

        public void AddReadOnly(string name, string value, bool deferred, bool local)
        {
            Property p = new Property(name, value, true, deferred);
            Add(p, local);
        }

        public void Add(string name, string value, bool readOnly)
        {
            Property p = new Property(name, value, readOnly);
            Add(p, false);
        }

        public void Add(string name, string value, bool readOnly, bool deferred, bool local)
        {
            Property p = new Property(name, value, readOnly, deferred);
            Add(p, local);
        }

        public void Remove(string name)
        {
            if (name == null)
            {
                throw new ArgumentNullException("name");
            }

            _propertiesLock.EnterUpgradeableReadLock();
            try
            {
                Property property = FindProperty(name);
                if (property != null && property.ReadOnly)
                {
                    string msg = String.Format("Cannot remove readonly property '{0}'.", name);
                    throw new InvalidOperationException(msg);
                }
                _propertiesLock.EnterWriteLock();
                try
                {
                    if (!_localContext.Remove(name))
                        _properties.Remove(name);
                }
                finally
                {
                    _propertiesLock.ExitWriteLock();
                }
                if (PropertyRemoved != null)
                {
                    PropertyRemoved(this, new PropertyEventArgs(property));
                }
            }
            finally
            {
                _propertiesLock.ExitUpgradeableReadLock();
            }

        }

        public string this[string name]
        {
            get
            {
                Property property;
                _propertiesLock.EnterReadLock();
                try
                {
                    property = FindProperty(name);
                }
                finally
                {
                    _propertiesLock.ExitReadLock();
                }

                if (property != null)
                {
                    return (property.Deferred) ? ExpandProperties(property.Value) : property.Value;
                }
                else
                {
                    return null;
                }
            }
            set
            {
                Add(name, value);
            }
        }

        // Internal use only, use caution!
        public string UpdateReadOnly(string name, string value)
        {
            _propertiesLock.EnterUpgradeableReadLock();
            try
            {
                Property property = FindProperty(name);
                if (property == null)
                {
                    AddReadOnly(name, value);
                }
                else
                {
                    string retVal;
                    _propertiesLock.EnterWriteLock();
                    try
                    {
                        retVal = property.Value;
                        property.ReadOnly = false;
                        property.Value = value;
                        property.ReadOnly = true;
                    }
                    finally
                    {
                        _propertiesLock.ExitWriteLock();
                    }

                    if (PropertyChanged != null)
                    {
                        PropertyChanged(this, new PropertyEventArgs(property));
                    }

                    return retVal;
                }
            }
            finally
            {
                _propertiesLock.ExitUpgradeableReadLock();
            }
            return null;
        }

        public void RemoveReadOnly(string name)
        {
            if (name == null)
            {
                throw new ArgumentNullException("name");
            }

            _propertiesLock.EnterUpgradeableReadLock();
            try
            {
                Property property = FindProperty(name);
                if (property != null)
                {
                    _propertiesLock.EnterWriteLock();
                    try
                    {
                        if (!_localContext.Remove(name))
                            _properties.Remove(name);
                    }
                    finally
                    {
                        _propertiesLock.ExitWriteLock();
                    }
                    if (PropertyRemoved != null)
                    {
                        PropertyRemoved(this, new PropertyEventArgs(property));
                    }
                }
            }
            finally
            {
                _propertiesLock.ExitUpgradeableReadLock();
            }

        }


        public bool Contains(string name)
        {
            _propertiesLock.EnterReadLock();
            try
            {
                return (FindProperty(name) != null);
            }
            finally
            {
                _propertiesLock.ExitReadLock();
            }
        }

        public bool IsPropertyReadOnly(string name)
        {
            _propertiesLock.EnterReadLock();
            try
            {
                Property property = FindProperty(name);
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
                _propertiesLock.ExitReadLock();
            }
        }

        public bool IsPropertyLocal(string name)
        {
            bool local;
            _propertiesLock.EnterReadLock();
            try
            {
                FindProperty(name, out local);
            }
            finally
            {
                _propertiesLock.ExitReadLock();
            }
            return local;
        }

        public Project Project
        {
            get { return _project; }
            //set { _project = value; }
        }

        /// <summary>
        /// Property evaluator callback without deprecated functions.
        /// </summary>
        /// <param name="name">The name of the property to evaluate.</param>
        /// <returns>The paramters evalued value.</returns>
        private string EvaluateParameter(string name)
        {
            if (Project == null)
            {
                throw new InvalidOperationException("Cannot evaluate properties until a Project has been set.");
            }
            return this[name];
        }

#if NANT_COMPATIBILITY
        // [Obsolete("This function uses the old technique of parsing functions and should not be used.")]
        // The old syntax for calling a function is ${function_name}. The new syntax is @{function_name}
        // Support the old syntax - EvaluateFunctionDeprecated() will output a warning at runtime
        private string EvaluateParameterDeprecated(string name)
        {
            if (Project == null)
            {
                throw new InvalidOperationException("Cannot evaluate properties until a Project has been set.");
            }

            string result = this[name];

            if (result == null)
            {
                /* TODO: Deprecated...remove this */
                // Some game build scripts use libraries with this deprecated parameter. Need to keep it for now.
                result = DeprecatedFunctionEvaluator.EvaluateFunction(name, Project);
            }

            return result;
        }
#endif

        /// <summary>
        /// Function evaluator callback for running functions.
        /// </summary>
        public string EvaluateFunction(string functionName, List<string> paramList)
        {
            if (Project == null)
            {
                throw new InvalidOperationException("Cannot evaluate function until a Project has been set.");
            }

            return Project.FuncFactory.Run(functionName, paramList, Project);
        }

        /// <summary>Expands a string with macros.</summary>
        /// <param name="expression">The string with macros to expand.</param>
        /// <returns>The string with all macros expanded.</returns>
        public string ExpandProperties(string expression)
        {
            return StringParser.ExpandString(expression, _evaluateProperty, _evaluateFunction);
        }

        internal Dictionary<string, Property> LocalContext
        {
            get { return _localContext; }
        }

        private void Add(Property property, bool local)
        {
            if (property == null)
            {
                throw new ArgumentNullException("property");
            }
            Property p = null;
            if (local)
            {
                _propertiesLock.EnterUpgradeableReadLock();
                try
                {

                    p = FindLocalProperty(property.Name);
                    if (p == null || !p.ReadOnly)
                    {
                        if (p == null && !Property.VerifyPropertyName(property.Name))
                        {
                            String msg = String.Format("Invalid property name '{0}'.", property.Name);
                            throw new BuildException(msg);
                        }

                        _propertiesLock.EnterWriteLock();
                        try
                        {
                            _localContext[property.Name] = property;
                        }
                        finally
                        {
                            _propertiesLock.ExitWriteLock();
                        }
                        if (PropertyChanged != null)
                        {
                            PropertyChanged(this, new PropertyEventArgs(property));
                        }
                    }
                }
                finally
                {
                    _propertiesLock.ExitUpgradeableReadLock();
                }
            }
            else
            {
                bool isLocal;
                _propertiesLock.EnterUpgradeableReadLock();
                try
                {

                    p = FindProperty(property.Name, out isLocal);
                    if (p == null || !p.ReadOnly)
                    {
                        if (p == null && !Property.VerifyPropertyName(property.Name))
                        {
                            String msg = String.Format("Invalid property name '{0}'.", property.Name);
                            throw new BuildException(msg);
                        }

                        if (isLocal)
                        {
                            _propertiesLock.EnterWriteLock();
                            try
                            {
                                _localContext[property.Name] = property;
                            }
                            finally
                            {
                                _propertiesLock.ExitWriteLock();
                            }
                        }
                        else
                        {
                            _propertiesLock.EnterWriteLock();
                            try
                            {
                                _properties[property.Name] = property;
                            }
                            finally
                            {
                                _propertiesLock.ExitWriteLock();
                            }
                        }
                        if (PropertyChanged != null)
                        {
                            PropertyChanged(this, new PropertyEventArgs(property));
                        }
                    }
                }
                finally
                {
                    _propertiesLock.ExitUpgradeableReadLock();
                }

            }

            if (p != null && p.ReadOnly)
            {

                if (Project != null)
                    Project.Log.Debug.WriteLine(Project.LogPrefix + "Attempting to overwrite readonly property: " + p.Name);

            }
        }

        private Property FindProperty(string name)
        {
            Property property = null;

            if (!_localContext.TryGetValue(name, out property))
            {
                _properties.TryGetValue(name, out property);
            }
            return property;
        }

        private Property FindProperty(string name, out bool local)
        {
            Property property = null;
            local = _localContext.TryGetValue(name, out property);
            if (!local)
                _properties.TryGetValue(name, out property);
            return property;
        }

        private Property FindLocalProperty(string name)
        {
            Property property = null;
            _localContext.TryGetValue(name, out property);
            return property;
        }

        public IEnumerator<Property> GetEnumerator()
        {
            List<Property> global;
            List<Property> local;
            _propertiesLock.EnterReadLock();
            try
            {
                local = new List<Property>(_localContext.Values);
                global = new List<Property>(_properties.Values);
            }
            finally
            {
                _propertiesLock.ExitReadLock();
            }

            foreach (Property p in local)
            {
                yield return p;
            }

            foreach (Property p in global)
            {
                if (!_localContext.ContainsKey(p.Name))
                {
                    yield return p;
                }
            }
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
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        void Dispose(bool disposing)
        {
            if (!this._disposed)
            {
                if (disposing)
                {
                    _propertiesLock.Dispose();
                }
            }
            _disposed = true;
        }
    }
}
