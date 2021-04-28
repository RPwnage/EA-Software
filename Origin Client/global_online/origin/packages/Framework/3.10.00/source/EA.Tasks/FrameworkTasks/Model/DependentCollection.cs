using System;
using System.Linq;
using System.Collections;
using System.Collections.Generic;
using System.Collections.Concurrent;

using NAnt.Core;
using EA.Eaconfig.Modules;

namespace EA.FrameworkTasks.Model
{
    //NOTE! This class is NOT thread safe
    public class DependentCollection : ICollection<Dependency<IModule>>, IEnumerable<Dependency<IModule>>
    {
        public DependentCollection(IModule owner)
        {
            _dependentsIndex = new Dictionary<string, int>();
            _dependentsList = new List<Dependency<IModule>>();
            _owner = owner;
        }

        public DependentCollection(DependentCollection other)
        {
            _dependentsIndex = new Dictionary<string, int>(other._dependentsIndex);
            _dependentsList = new List<Dependency<IModule>>();
            foreach (var entry in other._dependentsList)
            {
                _dependentsList.Add(new Dependency<IModule>(entry));
            }

            _owner = other._owner;
        }

        public void Add(IModule module, uint type=0)
        {
            int index = -1;

            if(_dependentsIndex.TryGetValue(module.Key, out index))
            {
                var d = _dependentsList[index];
                d.SetType(type);
                if (d.Dependent is Module_UseDependency)
                {
                    _dependentsList[index] = new Dependency<IModule>(module, module.BuildGroup, d.Type);
                }
            }
            else
            {
                _dependentsList.Add(new Dependency<IModule>(module, module.BuildGroup, type));
                _dependentsIndex.Add(module.Key, _dependentsList.Count-1);
            }
        }

        public void AddRange(IEnumerable<Dependency<IModule>> range)
        {
            foreach (var dep in range)
            {
                Add(dep);
            }
        }


        public class ParentDependencyPair
        {
            public ParentDependencyPair(IModule p, Dependency<IModule> d)
            {
                ParentModule = p;
                Dependency = d;
            }
            public IModule ParentModule;
            public Dependency<IModule> Dependency;
        }


        public IEnumerable<ParentDependencyPair> FlattenParentChildPair(bool isTransitive = true, uint transitiveFilter = UInt32.MaxValue)
        {
            var collection = new Dictionary<string, ParentDependencyPair>();

            foreach (var d in this)
            {
                string key = _owner.Key + d.Dependent.Key;
                if (!collection.ContainsKey(key) && _owner.Key != d.Dependent.Key)
                {
                    collection.Add(key, new ParentDependencyPair(_owner, d));
                }
            }

            if (isTransitive)
            {
                var stack = new Stack<Dependency<IModule>>();

                foreach (var d in this)
                {
                    if (d.IsKindOf(transitiveFilter))
                    {
                        stack.Push(d);
                        while (stack.Count > 0)
                        {
                            var n = stack.Pop();

                            if (!n.Dependent.CollectAutoDependencies)
                            {
                                foreach (var child in n.Dependent.Dependents)
                                {
                                    string key = n.Dependent.Key + child.Dependent.Key;
                                    if (!collection.ContainsKey(key) && _owner.Key != child.Dependent.Key)
                                    {
                                        stack.Push(child);
                                        collection.Add(key, new ParentDependencyPair(n.Dependent, child));
                                    }
                                }
                            }
                        }
                    }
                }
            }

            return collection.Values;
        }



        // Flatten dependents collection. Stop at CollectAuto for modules with link step. Pass root to detect circular dependency
        public DependentCollection Flatten()
        {
            DependentCollection flattened = new DependentCollection(this);

            var stack = new Stack<Dependency<IModule>>();

            foreach (var d in this)
            {
                stack.Push(d);
                while (stack.Count > 0)
                {
                    var n = stack.Pop();

                    if (!n.Dependent.CollectAutoDependencies)
                    {
                        foreach (var child in n.Dependent.Dependents)
                        {
                            if (!flattened.ContainsIf(child, existing => !(existing.Dependent is Module_UseDependency)) && _owner.Key != child.Dependent.Key)
                            {
                                stack.Push(child);
                            }
                            flattened.Add(new Dependency<IModule>(child));
                        }
                    }
                }
            }

            return flattened;
        }
        
        public DependentCollection Flatten(uint dependencytype)
        {
            DependentCollection flattened = new DependentCollection(_owner);

            var stack = new Stack<Dependency<IModule>>();

            foreach (var d in this)
            {
                if (d.IsKindOf(dependencytype))
                {
                    flattened.Add(new Dependency<IModule>(d));
                    stack.Push(d);
                    while (stack.Count > 0)
                    {
                        var n = stack.Pop();

                        if (!n.Dependent.CollectAutoDependencies)
                        {
                            foreach (var child in n.Dependent.Dependents)
                            {
                                if (child.IsKindOf(dependencytype))
                                {
                                    if (!flattened.ContainsIf(child, existing => !(existing.Dependent is Module_UseDependency)) && child.Dependent.Key != _owner.Key)
                                    {
                                        stack.Push(child);
                                    }
                                    flattened.Add(new Dependency<IModule>(child));
                                }
                            }
                        }
                    }
                }
            }

            return flattened;
        }

        public DependentCollection FlattenAll()
        {
            DependentCollection flattened = new DependentCollection(this);

            var stack = new Stack<Dependency<IModule>>();

            foreach (var d in this)
            {
                flattened.Add(new Dependency<IModule>(d));
                stack.Push(d);
                while (stack.Count > 0)
                {
                    var n = stack.Pop();

                    foreach (var child in n.Dependent.Dependents)
                    {
                        if (!flattened.ContainsIf(child, existing => !(existing.Dependent is Module_UseDependency)))
                        {
                            stack.Push(child);
                        }
                        flattened.Add(new Dependency<IModule>(child));
                    }
                }
            }

            return flattened;
        }


        public DependentCollection FlattenAll(uint dependencytype)
        {
            DependentCollection flattened = new DependentCollection(_owner);

            var stack = new Stack<Dependency<IModule>>();

            foreach (var d in this)
            {
                if (d.IsKindOf(dependencytype))
                {
                    stack.Push(d);
                    while (stack.Count > 0)
                    {
                        flattened.Add(new Dependency<IModule>(d));

                        var n = stack.Pop();

                        if (!n.Dependent.CollectAutoDependencies)
                        {
                            foreach (var child in n.Dependent.Dependents)
                            {
                                if (child.IsKindOf(dependencytype))
                                {
                                    if (!flattened.ContainsIf(child, existing => !(existing.Dependent is Module_UseDependency)))
                                    {
                                        stack.Push(child);
                                    }
                                    flattened.Add(new Dependency<IModule>(child));
                                }
                            }
                        }
                    }
                }
            }

            if ((dependencytype & EA.Eaconfig.Core.DependencyTypes.Build) == EA.Eaconfig.Core.DependencyTypes.Build)
            {
                dependencytype |= EA.Eaconfig.Core.DependencyTypes.AutoBuildUse;
            }

            DependentCollection temp_flattened = new DependentCollection(_owner);

            foreach (var d in flattened.Where(d => d.Dependent.CollectAutoDependencies))
            {
                stack.Push(d);
                while (stack.Count > 0)
                {
                    var n = stack.Pop();

                    foreach (var child in n.Dependent.Dependents)
                    {
                        if (child.IsKindOf(dependencytype))
                        {
                            if (!temp_flattened.ContainsIf(child, existing => !(existing.Dependent is Module_UseDependency)))
                            {
                                stack.Push(child);
                            }
                            temp_flattened.Add(new Dependency<IModule>(child));
                        }
                    }
                }
            }

            flattened.AddRange(temp_flattened);

            return flattened;
        }



        public int Count 
        { 
            get { return _dependentsList.Count; } 
        }

        public bool IsReadOnly 
        { 
            get { return false; } 
        }

        public void Add(Dependency<IModule> dep) 
        {
            int index = -1;

            if (_dependentsIndex.TryGetValue(dep.Dependent.Key, out index))
            {
                var d = _dependentsList[index];
                d.SetType(dep.Type);
                if (d.Dependent is Module_UseDependency)
                {
                    _dependentsList[index] = new Dependency<IModule>(dep.Dependent, dep.BuildGroup, d.Type);
                }
            }
            else
            {
                _dependentsList.Add(dep);
                _dependentsIndex.Add(dep.Dependent.Key, _dependentsList.Count - 1);
            }
        }

        public void Clear() 
        {
            _dependentsIndex.Clear();
            _dependentsList.Clear();
        }

        public bool Contains(IModule item)
        {
            return _dependentsIndex.ContainsKey(item.Key);
        }

        public bool Contains(Dependency<IModule> item)
        {
            return _dependentsIndex.ContainsKey(item.Dependent.Key);
        }

        public bool ContainsIf(Dependency<IModule> item, Func<Dependency<IModule>, bool> func)
        {
            int existingIndex;
            if (_dependentsIndex.TryGetValue(item.Dependent.Key, out existingIndex))
            {
                return func(_dependentsList[existingIndex]);
            }
            return false;
        }

        public bool TryGet(IModule item, out Dependency<IModule> dep)
        {
            dep = null;
            int index = -1;
            if (_dependentsIndex.TryGetValue(item.Key, out index))
            {

                dep = _dependentsList[index];
                return true;
            }
            return false;
        }


        public void CopyTo(Dependency<IModule>[] array, int arrayIndex)
        {
            _dependentsList.CopyTo(array, arrayIndex);
        }

        public bool Remove(Dependency<IModule> item)
        {
            int index = -1;
            if (_dependentsIndex.TryGetValue(item.Dependent.Key, out index))
            {
                _dependentsIndex.Remove(item.Dependent.Key);
                _dependentsList.RemoveAt(index);
                for (int i = 0; i < _dependentsList.Count; i++)
                {
                    _dependentsIndex[_dependentsList[i].Dependent.Key] = i;
                }
                return true;
            }
            return false;
        }

        public IEnumerator<Dependency<IModule>> GetEnumerator()
        {
            return _dependentsList.GetEnumerator();
        }

        IEnumerator IEnumerable.GetEnumerator()
        {
            return GetEnumerator();
        }

        public void RemoveIf(Func<Dependency<IModule>, bool> func)
        {
             var cleanDependentsList = new List<Dependency<IModule>>();
             foreach (var d in _dependentsList)
             {
                 if (!func(d))
                 {
                     cleanDependentsList.Add(d);
                 }
             }

             if (cleanDependentsList.Count != _dependentsList.Count)
             {
                 _dependentsIndex.Clear();
                 _dependentsList.Clear();
                 _dependentsList.AddRange(cleanDependentsList);
                 int ind = 0;
                 foreach (var d in _dependentsList)
                 {
                     _dependentsIndex.Add(d.Dependent.Key, ind);
                     ind++;
                 }
             }
        }

        private readonly Dictionary<string, int> _dependentsIndex;
        private readonly List<Dependency<IModule>> _dependentsList;
        private readonly IModule _owner;
    }
}
