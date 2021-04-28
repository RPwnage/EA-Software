using System;
using System.Collections.Generic;
using System.Collections.Specialized;
using System.IO;
using System.Linq;
using EA.Eaconfig.Core;
using EA.Eaconfig.Modules;
using EA.FrameworkTasks.Model;
using NAnt.Core;
using NAnt.Core.Attributes;

namespace EA.Eaconfig.Backends.Text
{
    [TaskName("WritePackageGraphViz")]
    public class WritePackageGraphVizTask : Task
    {
        private class DependencyStyle
        {
            public string LineStyle { get; set; }
            public string ArrowHeadStyle { get; set; }

            public DependencyStyle(string lineStyle, string arrowHeadStyle)
            {
                LineStyle = lineStyle;
                ArrowHeadStyle = arrowHeadStyle;
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
        private const string Dependency = "\"{0}\"->\"{1}\" [style=\"{2}\"] [arrowhead=\"{3}\"]";
        private const string Node = "\"{0}\" [label=\"{0}\",shape=square];";
        private const string SubGraph = 
    "subgraph \"{0}\" {{\n"
    + "style=filled; color=lightgrey; node [style=filled,color=white];\n"
    + "{1};\n"
    + "label = \"{2}\";\n"
    + "}}\n";


        [TaskAttribute("packagename", Required = true)]
        public string PackageName
        {
            get { return _packageName; }
            set { _packageName = value; }
        }
        
        [TaskAttribute("graphfile", Required = true)]
        public string GraphFile
        {
            get { return _graphFile; }
            set { _graphFile = value; }
        }

        [TaskAttribute("graphmodules", Required = false)]
        public bool GraphModules
        {
            get { return _graphModules; }
            set { _graphModules = value; }
        }

        protected override void ExecuteTask()
        {
            _dependencyStyleMappings = new Dictionary<uint, DependencyStyle>();
            _packageDependencies = new StringDictionary();
            _moduleDependencies = new StringDictionary();
            _packageModules = new StringDictionary();
            _nodes = new StringDictionary();
            _subgraphs = new StringDictionary();

            _dependencyStyleMappings[DependencyTypes.Undefined] = new DependencyStyle("solid", "normal");
            _dependencyStyleMappings[DependencyTypes.Build] = new DependencyStyle("solid", "normal");
            _dependencyStyleMappings[DependencyTypes.Interface] = new DependencyStyle("dotted", "diamond");
            _dependencyStyleMappings[DependencyTypes.Link] = new DependencyStyle("dotted", "ediamond");
            _dependencyStyleMappings[DependencyTypes.AutoBuildUse] = new DependencyStyle("dotted", "dot");

            if (String.IsNullOrEmpty(_graphFile))
                throw new InvalidOperationException("A graph file must be provided GenerateStructuredDocs");

            Log.Status.WriteLine("Populating Package Graph for Package: {0}", _packageName);
            Log.Status.WriteLine();
            foreach (IPackage package in Project.BuildGraph().Packages.Values.Where(p => p.Name == PackageName))
            {
                GetPackageDependencies(package);
            }

            Log.Status.WriteLine();
            Log.Status.WriteLine("Writing Package Graph to: {0}", _graphFile);
            WriteGraph();
            Log.Status.WriteLine();
        }

        private void WriteGraph()
        {
            Directory.CreateDirectory(Path.GetDirectoryName(_graphFile));
            string graphLabel = String.Format("Dependency Graph for Package: {0}", _packageName);

            using (StreamWriter sw = new StreamWriter(_graphFile))
            {
                sw.WriteLine(Preamble, graphLabel);

                if (_graphModules)
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

        private void GetPackageDependencies(IPackage package)
        {
            string packageName = GetPackageName(package);
            if (!_graphModules && !_nodes.ContainsKey(packageName))
                _nodes[packageName] = String.Format(Node, packageName);

            if (!_packageModules.ContainsKey(packageName))
            {
                string modules = "";
                foreach (IModule module in package.Modules)
                {
                    modules += String.Format("\"{0}\" ", module.Name);
                    GetModuleDependencies(module);
                }

                modules = modules.Trim();
                if (!String.IsNullOrEmpty(modules))
                    _packageModules[packageName] = modules;
                else
                    _packageModules[packageName] = String.Format("\"{0}\"", packageName);

                if (_graphModules && !_subgraphs.ContainsKey(packageName))
                    _subgraphs[packageName] = String.Format(SubGraph, "cluster_" + packageName, _packageModules[packageName], packageName);
            }

            foreach (var dependent in package.DependentPackages)
            {
                string packageDepName = GetPackageName(dependent.Dependent);
                string packDep = String.Format(Dependency, packageName, packageDepName, "solid", "normal");
                if (_packageDependencies.ContainsKey(packDep)) continue;
                _packageDependencies[packDep] = packDep;

                GetPackageDependencies(dependent.Dependent);
            }
        }

        private void GetModuleDependencies(IModule module)
        {
            string moduleName = GetModuleName(module);
            if (_graphModules && !_nodes.ContainsKey(moduleName))
                _nodes[moduleName] = String.Format(Node, moduleName);

            foreach (var dependent in module.Dependents)
            {
                DependencyStyle ds = GetDependencyStyle(dependent);
                string moduleDep = String.Format(Dependency, module.Name, GetModuleName(dependent.Dependent), ds.LineStyle, ds.ArrowHeadStyle);
                if (_moduleDependencies.ContainsKey(moduleDep)) continue;
                
                _moduleDependencies[moduleDep] = moduleDep;
                GetModuleDependencies(dependent.Dependent);
            }
        }

        private DependencyStyle GetDependencyStyle(Dependency<IModule> module)
        {
            if (module.IsKindOf(DependencyTypes.Build))
                return _dependencyStyleMappings[DependencyTypes.Build];
            if (module.IsKindOf(DependencyTypes.Interface))
                return _dependencyStyleMappings[DependencyTypes.Interface];
            if (module.IsKindOf(DependencyTypes.Link))
                return _dependencyStyleMappings[DependencyTypes.Link];
            if (module.IsKindOf(DependencyTypes.AutoBuildUse))
                return _dependencyStyleMappings[DependencyTypes.AutoBuildUse];
            return _dependencyStyleMappings[DependencyTypes.Undefined];
        }


        private string GetPackageName(IPackage package)
        {
            return package.Name;
        }

        private string GetModuleName(IModule module)
        {
            //NOTICE(RASHIN):
            //Try to intelligently determine module name if module name is constant PACKAGE_DEPENDENCY_NAME
            string moduleName = module.Package.Name;
            IModule pkgModule = module.Package.Modules.FirstOrDefault((p) => p.BuildGroup == BuildGroups.runtime && p.Name.Contains(module.Package.Name));
            if (pkgModule != null) moduleName = pkgModule.Name;

            if (module.Name != Module_UseDependency.PACKAGE_DEPENDENCY_NAME) moduleName = module.Name;
            return moduleName;
        }

        private Dictionary<uint, DependencyStyle> _dependencyStyleMappings;

        private StringDictionary _moduleDependencies;
        private StringDictionary _packageModules;
        private StringDictionary _packageDependencies;
        private StringDictionary _nodes;
        private StringDictionary _subgraphs;

        private string _packageName;
        private string _graphFile;
        private bool _graphModules;
    }
}

