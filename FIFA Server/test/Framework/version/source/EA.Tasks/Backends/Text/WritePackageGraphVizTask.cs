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
using System.Collections.Generic;
using System.Collections.Specialized;
using System.IO;
using System.Linq;
using EA.Eaconfig.Core;
using EA.Eaconfig.Modules;
using EA.FrameworkTasks.Model;
using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Attributes;

namespace EA.Eaconfig.Backends.Text
{
	/// <summary>
	/// Generates 'dot' language description of the dependency graph between modules or packages. Generated file can be opened by "Graphviz" tools to create an image
	/// </summary>
	[TaskName("WritePackageGraphViz")]
	public class WritePackageGraphVizTask : Task
	{
		public enum DependencyColorSchemes { Standard, ByDependent, ByAncestor };

		private class DependencyStyle
		{
			public string LineStyle { get; set; }
			public string ArrowHeadStyle { get; set; }
			public string Color { get; set; }

			public DependencyStyle(string lineStyle, string arrowHeadStyle, string color = null)
			{
				LineStyle = lineStyle;
				ArrowHeadStyle = arrowHeadStyle;
				Color = color;
			}
		}

		private const string Preamble =
	"digraph G {{\n"
	+ "ranksep=1.25;\n"
	+ "label=\"{0}\";\n"
	+ "node [shape=plaintext, fontsize=16];\n"
	+ "bgcolor=white;\n"
	+ "edge [arrowsize=1, color=black];\n";

		private const string Epilogue = "}";
		private const string Dependency = "\"{0}\"->\"{1}\" [style=\"{2}\"] [arrowhead=\"{3}\"]{4}";
		private const string Node = "\"{0}\" [label=\"{1}\",shape=\"{2}\"{3}{4}];";
		private const string SubGraph = 
	"subgraph \"{0}\" {{\n"
	+ "style=filled; color=lightgrey; node [style=filled,fillcolor=white];\n"
	+ "{1};\n"
	+ "label = \"{2}\";\n"
	+ "}}\n";

		/// <summary>
		/// Name of a package to use a root of generated dependency graph picture. Separate graph is generated for each configuration.
		/// Package name can contain module name: valid values are 'package_name' or 'package_name/module_name' or 'package_name/group/module_name', where group is one of 'runtime, test, example, tool'".
		/// </summary>
		[TaskAttribute("packagename", Required = true)]
		public string PackageName
		{
			get { return _packageName; }
			set 
			{
				_fullPackageNameInput = value;
				SetPackageName(value);
			}
		}

		/// <summary>
		/// Full path to the output file. Configguration name is added to the file name.
		/// </summary>
		[TaskAttribute("graphfile", Required = true)]
		public string GraphFile { get; set; }

		/// <summary>
		/// If 'true' picture of dependencies between modules is generated. Otherwise dependencies between packages are used, no modules are added to the picture. Default is 'true'.
		/// </summary>
		[TaskAttribute("graphmodules", Required = false)]
		public bool GraphModules { get; set; } = true;

		/// <summary>
		/// Modules from the same package are grouped together on an image. This may complicate outline for very big projects. Only has effect when graphmodules='true', default is 'true'.
		/// </summary>
		[TaskAttribute("groupbypackages", Required = false)]
		public bool GroupByPackages { get; set; } = true;

		/// <summary>
		/// List of configurations to use.
		/// </summary>
		[TaskAttribute("configurations", Required = false)]
		public string Configurations
		{
			get 
			{ 
				return String.IsNullOrEmpty(_configurations.TrimWhiteSpace()) ? Project.Properties["package.configs"] : _configurations; 
			}
			set { _configurations = value; }
		}

		/// <summary>
		/// Types of dependencies to include in the generated picture. Possible values are: 'build', 'interface', 'link', 'copylocal', 'auto', or any combination of these, enter values space separated. Default value is: 'build interface link'.
		/// </summary>
		[TaskAttribute("dependency-filter", Required = false)]
		public string DependencyFilter { get; set; }

		/// <summary>
		/// Types of dependencies to exinclude from the generated picture. Possible values are: 'build', 'interface', 'link', 'copylocal', 'auto', or any combination of these, enter values space separated. Default value is empty.
		/// </summary>

		[TaskAttribute("exclude-dependency-filter", Required = false)]
		public string ExcludeDependencyFilter { get; set; }

		/// <summary>
		/// Graph ancestors of a given package/module indead of dependents. Default value is false.
		/// </summary>

		[TaskAttribute("find-ancestors", Required = false)]
		public bool FindAncestors { get; set; }

		/// <summary>
		/// Dependency Color scheme. Standard - by dependency type, ByDependent - same color as dependent module.
		/// </summary>
		[TaskAttribute("dependency-color-scheme", Required = false)]
		public DependencyColorSchemes DependencyColorScheme { get; set; } = DependencyColorSchemes.Standard;

		/// <summary>
		/// List of modules to exclude
		/// </summary>
		[Property("excludemodules", Required = false)]
		public PropertyElement ExcludeModules { get; } = new PropertyElement();

		private void SetPackageName(string input)
		{
			
			_group = null;
			_moduleName = null;

			IList<string> depDetails = StringUtil.ToArray(input, "/");
			switch (depDetails.Count)
			{
				case 1: // Package name
					_packageName = depDetails[0];
					break;
				case 2: // Package name + module name
					_packageName = depDetails[0];
					_group = "runtime";
					_moduleName = depDetails[1];
					break;
				case 3: //Package name + group name + module name;
					_packageName = depDetails[0];
					_group = depDetails[1];
					_moduleName = depDetails[2];
					break;
				default:
					throw new BuildException( $"{LogPrefix} Invalid 'packagename' value: {input}, valid format are 'package_name' or 'package_name/module_name' or 'package_name/group/module_name', where group is one of 'runtime, test, example, tool'");
			}
		}

		protected override void ExecuteTask()
		{
			InitInputData();

			foreach (var config in Configurations.ToArray())
			{
				_packageDependencies = new StringDictionary();
				_moduleDependencies = new StringDictionary();
				_packageModules = new StringDictionary();
				_nodes = new StringDictionary();
				_subgraphs = new StringDictionary();
				_processedModules = new HashSet<string>();

				Log.Status.WriteLine("Populating Package Graph for Package: {0} [{1}]", _fullPackageNameInput, config);
				Log.Status.WriteLine();

				if (FindAncestors)
				{
					GetAncestorDependencies(config);
				}
				else
				{
					foreach (IPackage package in Project.BuildGraph().Packages.Values.Where(p => p.Name == PackageName && p.ConfigurationName == config))
					{
						GetPackageDependencies(package);
					}
				}

				Log.Status.WriteLine();

				WriteGraph(GetGraphFile(config), config);
			}

			WriteLegend(GetGraphLegendFile());

			Log.Status.WriteLine();
		}

		private HashSet<string> FindAncestorModules(IEnumerable<IModule> rootmodules)
		{
			// Create DAG of build modules:
			var modKeys = new Dictionary<string, DAGPath<IModule>>();
			var dag = new List<DAGPath<IModule>>();

			foreach (var mod in Project.BuildGraph().SortedActiveModules)
			{
				var newNode = new DAGPath<IModule>(mod);
				modKeys.Add(mod.Key, newNode);
				dag.Add(newNode);
			}

			// Populate dependencies
			foreach (var dagNode in dag)
			{
				// Sort dependents so that we always have the same order after sorting the whole graph
				foreach (var dep in dagNode.Value.Dependents.Where(d => d.IsKindOf(_includeDependencyFilterValue) && !d.IsKindOf(_excludeDependencyFilterValue)).OrderBy(d => d.Dependent.Key))
				{
					DAGPath<IModule> childDagNode;
					if (modKeys.TryGetValue(dep.Dependent.Key, out childDagNode))
					{
						dagNode.Dependencies.Add(childDagNode);
					}
					else
					{
						throw new BuildException("INTERNAL ERROR");
					}
				}
			}

			DAGPath<IModule>.FindAncestors(dag, (m) => rootmodules.Any(rm=>rm.Key==m.Key), (a, b) => {});

			var ancestormodules = new HashSet<string>(rootmodules.Select(am=>am.Key));

			foreach(var node in dag)
			{
				if(node.IsInPath)
				{
					ancestormodules.Add(node.Value.Key);
				}
			}
			return ancestormodules;
		}

		private HashSet<string> FindAncestorPackages(IEnumerable<IPackage> rootpackages)
		{
			// Create DAG of build modules:
			var pkKeys = new Dictionary<string, DAGPath<IPackage>>();
			var dag = new List<DAGPath<IPackage>>();

			foreach (var pk in Project.BuildGraph().Packages)
			{
				var newNode = new DAGPath<IPackage>(pk.Value);
				pkKeys.Add(pk.Value.Key, newNode);
				dag.Add(newNode);
			}

			// Populate dependencies
			foreach (var dagNode in dag)
			{
				// Sort dependents so that we always have the same order after sorting the whole graph
				//foreach (var dep in dagNode.Value.DependentPackages.Where(d => d.IsKindOf(_includeDependencyFilterValue) && !d.IsKindOf(_excludeDependencyFilterValue)).OrderBy(d => d.Dependent.Key))
				foreach (var dep in dagNode.Value.DependentPackages.OrderBy(d => d.Dependent.Key))
				{
					DAGPath<IPackage> childDagNode;
					if (pkKeys.TryGetValue(dep.Dependent.Key, out childDagNode))
					{
						dagNode.Dependencies.Add(childDagNode);
					}
					else
					{
					   // throw new BuildException("INTERNAL ERROR");
					}
				}
			}

			DAGPath<IPackage>.FindAncestors(dag, (m) => rootpackages.Any(rm => rm.Key == m.Key), (a, b) => { });

			var ancestorpackages = new HashSet<string>(rootpackages.Select(am => am.Key));

			foreach (var node in dag)
			{
				if (node.IsInPath)
				{
					ancestorpackages.Add(node.Value.Key);
				}
			}
			return ancestorpackages;
		}

		private void InitInputData()
		{
			if (String.IsNullOrEmpty(GraphFile))
				throw new BuildException("Non empty 'graphfile' parameter must be provided to task 'WritePackageGraphViz'");

			_includeDependencyFilterValue = GetDependencyFilterValue(DependencyFilter, defaultval: DependencyTypes.Build | DependencyTypes.Interface | DependencyTypes.Link);

			_excludeDependencyFilterValue = GetDependencyFilterValue(ExcludeDependencyFilter, defaultval: 0);

			if(ExcludeModules != null && !String.IsNullOrEmpty(ExcludeModules.Value))
			{
				_modulesToExclude = new HashSet<string>(Project.ExpandProperties(ExcludeModules.Value).ToArray());
			}

				
			var newline = Environment.NewLine + Log.Padding + "         ";

			Log.Status.WriteLine(LogPrefix +
				"Input parameters:" + newline +
				"graphfile:                 {0}" + newline +
				"packagename:               {1}" + newline +
				"graphmodules:              {2}" + newline +
				"groupbypackages:           {3}" + newline +
				"configurations:            {4}" + newline +
				"dependency-filter:         {5}" + newline +
				"exclude-dependency-filter: {6}" + newline +
				"exclude-modules:           {7}" + newline +
				"find-ancestors:            {8}" + newline +
				"dependency-color-scheme:   {9}",
				GraphFile.Quote(),
				PackageName.Quote(),
				GraphModules.ToString().ToUpperInvariant(),
				GroupByPackages.ToString().ToUpperInvariant(),
				Configurations.ToArray().ToString(", "),
				DependencyTypes.ToString(_includeDependencyFilterValue).Quote(),
				DependencyTypes.ToString(_excludeDependencyFilterValue).Quote(),
				_modulesToExclude != null ? _modulesToExclude.ToString(", ") : string.Empty,
				FindAncestors.ToString().ToLowerInvariant(),
				DependencyColorScheme
				);

		}

		private string FormatGraphLabel(string config)
		{
			var graphLabel = String.Format("{0} for Package: {1} [{2}, inc='{3}', excl='{4}', graphmodules={5}, groupbypackages={6}, dependency-color-scheme={7}]", 
				FindAncestors ? "Ancestors" : "Dependency Graph",
				_fullPackageNameInput, config, 
				DependencyTypes.ToString(_includeDependencyFilterValue), DependencyTypes.ToString(_excludeDependencyFilterValue),
				GraphModules.ToString().ToLowerInvariant(), GroupByPackages.ToString().ToLowerInvariant(), DependencyColorScheme);
			return graphLabel;
		}

		private void WriteGraph(string filepath, string config)
		{
			Log.Status.WriteLine("Writing Package Graph to: {0}", filepath);

			Directory.CreateDirectory(Path.GetDirectoryName(filepath));

			using (StreamWriter sw = new StreamWriter(filepath))
			{
				sw.WriteLine(Preamble, FormatGraphLabel(config));

				if (GraphModules)
				{
					foreach (string subgraph in _subgraphs.Keys)
					{
						sw.WriteLine(_subgraphs[subgraph]);
					}

					foreach (string moduleDep in _moduleDependencies.Keys)
					{
						sw.WriteLine(_moduleDependencies[moduleDep]);
					}
				}
				else
				{
					foreach (string packDep in _packageDependencies.Keys)
					{
						sw.WriteLine(_packageDependencies[packDep]);
					}
				}

				foreach (string node in _nodes.Keys)
				{
					sw.WriteLine(_nodes[node]);
				}

				sw.WriteLine(Epilogue);
			}
		}

		 private void WriteLegend(string filepath)
		{
			Directory.CreateDirectory(Path.GetDirectoryName(filepath));

			var labels = new List<string>();

			using (StreamWriter sw = new StreamWriter(filepath))
			{
				sw.WriteLine("digraph G {");
				sw.WriteLine("      labelloc=t;");
				sw.WriteLine("      rankdir=LR;");

				sw.WriteLine("      subgraph cluster_key {");

				sw.WriteLine("              label=Legend;");

				int count = 0;

				foreach (var deptype in new uint[] {DependencyTypes.Build, DependencyTypes.Interface, DependencyTypes.Link, DependencyTypes.CopyLocal, DependencyTypes.AutoBuildUse })
				{
					count++;

					DependencyStyle ds = GetDependencyStyle(new BitMask(deptype));
					sw.WriteLine("              {0}{1}[style=invis];", "dl",count);
					sw.WriteLine("              {0}{1}[style=invis];", "dr", count);

					sw.WriteLine("              " + Dependency, "dl"+count, "dr"+count, ds.LineStyle, ds.ArrowHeadStyle, String.IsNullOrEmpty(ds.Color) ? String.Empty : String.Format(" [color=\"{0}\"]", ds.Color));

					sw.WriteLine("              lb{0}[shape=plaintext, style=solid, width=4.0, label=\"Dependency type '{1}':\\r\"];", count, DependencyTypes.ToString(deptype));
					sw.WriteLine("              lb{0}->dl{0}[style=\"invis\",dir=\"none\"];", count);
					sw.WriteLine("");

					labels.Add("lb" + count);
				}

				WriteModuleTypeShape(sw, labels, ++count, "native", "square", "", "", "Native module");
				WriteModuleTypeShape(sw, labels, ++count, ".Net", "oval", "rounded", "", ".Net module");
				WriteModuleTypeShape(sw, labels, ++count, "Utility", "diamond", "rounded", "", "Utility module");
				WriteModuleTypeShape(sw, labels, ++count, "Makestyle", "oval", "", "", "Makestyle module");
				WriteModuleTypeShape(sw, labels, ++count, "Use Dependency", "box", "rounded,dotted", "", "Use dependency module");

				sw.WriteLine("");
				sw.WriteLine("");

				WriteModuleTypeColor(sw, labels, ++count, "Program", "blue", "Executable module");
				WriteModuleTypeColor(sw, labels, ++count, "Library", "green", "Static library module");
				WriteModuleTypeColor(sw, labels, ++count, "Dll", "brown", "Dynamic library module");
				 
				sw.WriteLine("{{ rank=source; {0} }}", labels.ToString(" "));

				sw.WriteLine("      }");
				sw.WriteLine("}");
			}
		}
		 private void WriteModuleTypeShape(StreamWriter sw, List<string> labels, int count, string name, string shape, string style, string color, string label)
		{
				sw.WriteLine("");
				sw.WriteLine("              {0}", FormatNode(name, shape, style, color));
				sw.WriteLine("              lb{0}[shape=plaintext, style=solid,  label=\"{1}:\\r\"];", count, label);
				sw.WriteLine("              lb{0}->{1}[style=\"invis\",dir=\"none\"];", count,name.Replace(".", "dot").Replace(" ", ""));
				labels.Add("lb" + count);
		}

		private void WriteModuleTypeColor(StreamWriter sw, List<string> labels, int count, string name, string color, string label)
		{
			sw.WriteLine("");
			sw.WriteLine("              {0}", FormatNode(name, "square", "", color));
			sw.WriteLine("              lb{0}[shape=plaintext, style=solid,  label=\"{1}:\\r\"];", count, label);
			sw.WriteLine("              lb{0}->{1}[style=\"invis\",dir=\"none\"];", count, name.Replace(".", "dot").Replace(" ", ""));
			labels.Add("lb" + count);
		}

		private void GetAncestorDependencies(string config)
		{
			if (GraphModules)
			{
				var rootmodules = Project.BuildGraph().Packages.Values.Where(p => p.Name == PackageName && p.ConfigurationName == config).SelectMany(p => p.Modules.Where(m=> (_group==null || _group == m.BuildGroup.ToString()) && (_moduleName==null || _moduleName == m.Name)));
				_ancestorModules = FindAncestorModules(rootmodules);

				var packageModules = new Dictionary<string, List<string>>();

				foreach (IModule module in Project.BuildGraph().Packages.Values.SelectMany(p => p.Modules))
				{
					GetModuleDependencies(module, true);
				}
				foreach (IModule module in Project.BuildGraph().Packages.Values.SelectMany(p => p.Modules))
				{ 
					string moduleName = GetModuleName(module);
					if(_nodes.ContainsKey(moduleName))
					{
						string packageName = GetPackageName(module.Package);
						List<string> packagemodules;
						if (!packageModules.TryGetValue(packageName, out packagemodules))
						{
							packagemodules = new List<string>();
							packageModules.Add(packageName, packagemodules);
						}
						packagemodules.Add(moduleName);
					}
				}

				foreach (var pme in packageModules)
				{
					if (pme.Value != null && pme.Value.Count > 0)
						_packageModules[pme.Key] = pme.Value.ToString(" ", m => m.Replace(".", "dot").Replace(" ", "").Quote());
					else
						_packageModules[pme.Key] = pme.Key.Quote();

					if (GroupByPackages && !_subgraphs.ContainsKey(pme.Key))
					{
						_subgraphs[pme.Key] = String.Format(SubGraph, "cluster_" + pme.Key, _packageModules[pme.Key], pme.Key);
					}
				}
			}
			else
			{
				var rootpackages = Project.BuildGraph().Packages.Values.Where(p => p.Name == PackageName && p.ConfigurationName == config);
				_ancestorPackages = FindAncestorPackages(rootpackages);

				var packageModules = new Dictionary<string, List<string>>();

				foreach (var package in Project.BuildGraph().Packages.Values)
				{
					GetPackageDependencies(package);
				}
			}
		}

		private void GetPackageDependencies(IPackage package)
		{
			string packageName = GetPackageName(package);

			if (GraphModules)
			{
				if (!_packageModules.ContainsKey(packageName))
				{
					string modules = "";
					foreach (IModule module in package.Modules)
					{
						
						if((!String.IsNullOrEmpty(_moduleName) && _moduleName != module.Name) || (!String.IsNullOrEmpty(_group) && _group != module.BuildGroup.ToString()))
						{
							continue;
						}
						modules += String.Format("\"{0}\" ", module.Name.Replace(".", "dot").Replace(" ", ""));
						GetModuleDependencies(module, true);
					}

					modules = modules.Trim();
					if (!String.IsNullOrEmpty(modules))
						_packageModules[packageName] = modules;
					else
						_packageModules[packageName] = String.Format("\"{0}\"", packageName);

					if (GroupByPackages && !_subgraphs.ContainsKey(packageName))
					{
						_subgraphs[packageName] = String.Format(SubGraph, "cluster_" + packageName, _packageModules[packageName], packageName);
					}
				}
			}
			else
			{
				if (FindAncestors && !_ancestorPackages.Contains(package.Key))
				{
					return;
				}

				if (!_nodes.ContainsKey(packageName))
				{
					_nodes[packageName] = FormatNode(packageName.Replace(".", "dot"), "box", "", "");

					foreach (var dependent in package.DependentPackages)
					{
						if (FindAncestors && !_ancestorPackages.Contains(dependent.Dependent.Key))
						{
							continue;
						}

						string packageDepName = GetPackageName(dependent.Dependent);
						string depstyle = dependent.IsKindOf(PackageDependencyTypes.Build) ? "solid" : "dotted";
						string packDep = String.Format(Dependency, packageName.Replace(".", "dot"), packageDepName.Replace(".", "dot"), depstyle, "normal", String.Empty);
						if (_packageDependencies.ContainsKey(packDep)) continue;
						_packageDependencies[packDep] = packDep;

						GetPackageDependencies(dependent.Dependent);
					}
				}
			}
		}

		private void GetModuleDependencies(IModule module, bool hasIncomingDep)
		{
			if (FindAncestors && !_ancestorModules.Contains(module.Key))
			{
				return;
			}

			if (!_processedModules.Add(module.Key + "-" + hasIncomingDep.ToString()))
			{
				return;
			}
			if(_modulesToExclude != null && _modulesToExclude.Contains(module.Name))
			{
				Log.Status.WriteLine(LogPrefix + "Skipping module '{0}' because it is listed in the list of modules to exclude", module.Name);
				return;
			}

			int depcount = 0;

			foreach (var dependent in module.Dependents)
			{
				if (_modulesToExclude != null && _modulesToExclude.Contains(dependent.Dependent.Name))
				{
					//Log.Status.WriteLine(LogPrefix + "Skipping module '{0}' because it is listed in the list of modules to exclude", dependent.Dependent.Name);
					continue;
				}
				if (FindAncestors && !_ancestorModules.Contains(dependent.Dependent.Key))
				{
					continue;
				}

				bool incomingDep = false;

				if (dependent.IsKindOf(_includeDependencyFilterValue))
				{
					if (!dependent.IsKindOf(_excludeDependencyFilterValue))
					{
						DependencyStyle ds = GetDependencyStyle(module, dependent);
						string moduleDep = String.Format(Dependency, module.Name.Replace(".", "dot").Replace(" ", ""), GetModuleName(dependent.Dependent).Replace(".", "dot").Replace(" ", ""), ds.LineStyle, ds.ArrowHeadStyle, String.IsNullOrEmpty(ds.Color) ? String.Empty : String.Format(" [color=\"{0}\"]", ds.Color));
						if (_moduleDependencies.ContainsKey(moduleDep)) continue;

						_moduleDependencies[moduleDep] = moduleDep;
						depcount++;
						incomingDep = true;
					}
				}
				GetModuleDependencies(dependent.Dependent, incomingDep);
			}

			if (hasIncomingDep || depcount > 0)
			{
				string moduleName = GetModuleName(module);
				if (GraphModules && !_nodes.ContainsKey(moduleName))
				{
					_nodes[moduleName] = FormatNode(moduleName, module);
				}
			}
		}

		private DependencyStyle GetDependencyStyle(IModule ancestor, Dependency<IModule> dependency)
		{
			var ds = GetDependencyStyle((BitMask)dependency);

			if(DependencyColorScheme == DependencyColorSchemes.ByDependent)
			{
				ds.Color = GetRandomModuleColor(dependency.Dependent);
				ds.ArrowHeadStyle = "normal";
				ds.LineStyle = "solid";
			}
			else if (DependencyColorScheme == DependencyColorSchemes.ByAncestor)
			{
				ds.Color = GetRandomModuleColor(ancestor);
				ds.ArrowHeadStyle = "normal";
				ds.LineStyle = "solid";
			}

			return ds;
		}

		private DependencyStyle GetDependencyStyle(BitMask module)
		{
			DependencyStyle style = new DependencyStyle("dotted", "diamond");

			if (module.IsKindOf(DependencyTypes.Build))
			{
				style.ArrowHeadStyle = "normal";
				style.LineStyle  = "solid";
			}

			if (module.IsKindOf(DependencyTypes.Interface))
			{
			}

			if (module.IsKindOf(DependencyTypes.Link))
			{
				style.ArrowHeadStyle = "normal";
			}
			if (module.IsKindOf(DependencyTypes.AutoBuildUse))
			{
				style.ArrowHeadStyle = "dot";
			}
			if (module.IsKindOf(DependencyTypes.CopyLocal))
			{
					style.Color = "chocolate2";
			}
			return style;
		}

		private uint GetDependencyFilterValue(string filterstr, uint defaultval)
		{
			uint dependencyfilter = 0;
			bool isSet = false;
			if (!String.IsNullOrEmpty(filterstr.TrimWhiteSpace()))
			{
				foreach (var depstr in filterstr.ToArray(_delimArray))
				{
					switch(depstr.ToLowerInvariant())
					{
						case "build":
							dependencyfilter |= DependencyTypes.Build;
							isSet = true;
							break;
						case "interface":
							dependencyfilter |= DependencyTypes.Interface;
							isSet = true;
							break;
						case "link":
							dependencyfilter |= DependencyTypes.Link;
							isSet = true;
							break;
						case "auto":
							dependencyfilter |= DependencyTypes.AutoBuildUse;
							isSet = true;
							break;
						case "copylocal":
							dependencyfilter |= DependencyTypes.CopyLocal;
							isSet = true;
							break;
						default:
							Log.Error.WriteLine(LogPrefix + "Invalid dependency filter value: '{0}'. Valid values are: 'build, interface, link, auto, copylocal'.", depstr.ToLowerInvariant());
							break;
					}
				}
			}
			if(!isSet)
			{
				dependencyfilter = defaultval;
			}

			return dependencyfilter;
		}

		private string GetPackageName(IPackage package)
		{
			return package.Name;
		}

		private string GetModuleName(IModule module)
		{
			var moduleName = module.Name;

			if(module.Name == Module_UseDependency.PACKAGE_DEPENDENCY_NAME)
			{
				moduleName = module.Package.Name;
			}

			return moduleName;
		}

		private string FormatNode(string nodename, IModule module)
		{
			var style = String.Empty;
			var color = String.Empty;
			var shape = "box";
			if(module is Module_Native)
			{
				shape = "square";
			}
			else if(module is Module_DotNet)
			{
				style = "rounded";
				shape = "oval";
			}
			else if (module is Module_Utility)
			{
				style = "rounded";
				shape = "diamond";
			}
			else if (module is Module_MakeStyle)
			{
				shape = "oval";
			}
			else if (module is Module_UseDependency)
			{
				style = "rounded,dotted";
				shape = "box";
			}
			if (DependencyColorScheme == DependencyColorSchemes.Standard)
			{
				if (module.IsKindOf(Module.Program))
				{
					color = "blue";
				}
				else if (module.IsKindOf(Module.Library))
				{
					color = "green";
				}
				else if (module.IsKindOf(Module.DynamicLibrary))
				{
					color = "brown";
				}
			}
			else if (DependencyColorScheme != DependencyColorSchemes.Standard)
			{
				color = GetRandomModuleColor(module);
			}

			return FormatNode(nodename, shape, style, color);

		}

		private string GetRandomModuleColor(IModule module)
		{
			string color;
			if (!_moduleColors.TryGetValue(module.Key, out color))
			{
				color = _modulecolortable[_moduleColors.Count % _modulecolortable.Length];
				_moduleColors.Add(module.Key, color);
			}
			return color;
		}

		private string FormatNode(string nodename, string shape, string style, string color)
		{
			string label = nodename;
			nodename = nodename.Replace(".", "dot").Replace(" ", "");

			return String.Format(Node, nodename, label, shape, 
				String.IsNullOrEmpty(style) ? String.Empty : String.Format(", style=\"{0}\"", style),
				String.IsNullOrEmpty(color) ? String.Empty : String.Format(", color=\"{0}\"", color)
				);

		}

		private string GetGraphFile(string config)
		{
			var path = Path.GetDirectoryName(GraphFile);
			var file  = Path.GetFileNameWithoutExtension(GraphFile);
			var ext  = Path.GetExtension(GraphFile);
			return Path.Combine(path, file+"-"+config + ext);
		}

		private string GetGraphLegendFile()
		{
			var path = Path.GetDirectoryName(GraphFile);
			var file = Path.GetFileNameWithoutExtension(GraphFile);
			var ext = Path.GetExtension(GraphFile);
			return Path.Combine(path, file + "-legend" + ext);
		}


		private StringDictionary _moduleDependencies;
		private StringDictionary _packageModules;
		private StringDictionary _packageDependencies;
		private StringDictionary _nodes;
		private StringDictionary _subgraphs;
		private HashSet<string> _processedModules;
		private HashSet<string> _modulesToExclude;

		private string _fullPackageNameInput;
		private string _packageName;
		private string _group;
		private string _moduleName;
		private string _configurations;
		private HashSet<string> _ancestorModules;
		private HashSet<string> _ancestorPackages;

		private readonly Dictionary<string, string> _moduleColors = new Dictionary<string, string>();

		private readonly string[] _modulecolortable = new string[] {"brown", "cyan", "green", "orange", "black", "blue", "red", "purple", "aquamarine", "darkgreen", "azure2", "violet", "dimgrey", "gold", "indigo" };
		

		private uint _includeDependencyFilterValue;
		private uint _excludeDependencyFilterValue;

		private static readonly char[] _delimArray = { 
			'\x0009', // tab
			'\x000a', // newline
			'\x000d', // carriage return
			'\x0020',  // space
			'|',
			',',
			';'
		};

	}
}

