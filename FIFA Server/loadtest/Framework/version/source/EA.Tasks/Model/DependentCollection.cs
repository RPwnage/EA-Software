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
using System.Linq;
using System.Collections;
using System.Collections.Generic;
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
			if (s_breakDependencies?.Contains(module.Name.ToLowerInvariant()) ?? false)
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

			if (_dependentsIndex.TryGetValue(module.Key, out int index))
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
				_dependentsIndex.Add(module.Key, _dependentsList.Count - 1);
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


		public IEnumerable<ParentDependencyPair> FlattenParentChildPair(bool isTransitive = true, uint transitiveFilter = UInt32.MaxValue, bool ignoreCollectAutoDependenciesFlag = false)
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

							if (!n.Dependent.CollectAutoDependencies || ignoreCollectAutoDependenciesFlag)
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

		public HashSet<IModule> GetAllDependentModules(HashSet<IModule> set = null, bool enterUseDependencies = false, uint dependencyType = ~0u)
		{
			if (set == null)
				set = new HashSet<IModule>();

			var stack = new Stack<DependentCollection>();
			stack.Push(this);

			while (true)
			{
				DependentCollection col = stack.Pop();

				foreach (var d in col)
				{
					if (!enterUseDependencies && (d.Dependent is Module_UseDependency))
						continue;

					if (!d.IsKindOf(dependencyType))
						continue;

					if (set.Add(d.Dependent))
						stack.Push(d.Dependent.Dependents);
				}

				if (stack.Count == 0)
					break;
			}
			return set;
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
					flattened.Add(new Dependency<IModule>(d));

					while (stack.Count > 0)
					{
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
			if (s_breakDependencies?.Contains(dep.Dependent.Name.ToLowerInvariant()) ?? false)
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

			if (_dependentsIndex.TryGetValue(dep.Dependent.Key, out int index))
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
			if (_dependentsIndex.TryGetValue(item.Dependent.Key, out int existingIndex))
			{
				return func(_dependentsList[existingIndex]);
			}
			return false;
		}

		public bool TryGet(IModule item, out Dependency<IModule> dep)
		{
			dep = null;
			if (_dependentsIndex.TryGetValue(item.Key, out int index))
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
			if (_dependentsIndex.TryGetValue(item.Dependent.Key, out int index))
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

		public static void SetBreakOnDependencies(IEnumerable<string> breakDependencies)
		{
			if (breakDependencies.Any())
			{
				s_breakDependencies = new HashSet<string>(breakDependencies.Select(x => x.ToLowerInvariant()));
			}
			else
			{
				s_breakDependencies = null;
			}
		}

		private readonly Dictionary<string, int> _dependentsIndex;
		private readonly List<Dependency<IModule>> _dependentsList;
		private readonly IModule _owner;

		private static HashSet<string> s_breakDependencies;
	}
}
