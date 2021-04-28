---
title: Create Build Graph picture
weight: 215
---

Target to create &#39;dot&#39; language description of the build graph. Generated graph is compatible with Graphviz package.

<a name="Section1"></a>
## report-dep-graph target ##

It may be useful to visualize build graph and dependencies between packages. &#39;report-dep-graph&#39; target allows to do exactly that.

#### Generate dependency graph images ####
Target |Standard |Short description |
--- |--- |--- |
| report-dep-graph | ![requirements 1a]( requirements1a.gif ) | Creates graphviz &#39;dot&#39; file that can be used to create dependency graph image. |
| test-report-dep-graph | ![requirements 1a]( requirements1a.gif ) | Same as above for &#39;test&#39;, &#39;example&#39;, and &#39;tool&#39; groups. |

By default module dependency graph is created where modules are grouped by packages and all dependencies are reflected.In big projects such graph may be too complicated.
Following properties can be used to configure dependency graph output.

#### Dependency graph configuration ####
Property |Default value |Description |
--- |--- |--- |
| report-dep-graph.packagename | Current package: ${package-name} | Name of the package or module to start dependency graph. |
| report-dep-graph.graphfile | ${nant.project.temproot}/${report-dep-graph.packagename}_graph.viz | Name of the output file |
| report-dep-graph.graphmodules | true | Report dependencies between modules, if this property is &#39;false&#39; dependencies between packages are reported |
| report-dep-graph.groupbypackages | true | If true modules from the same package are groupded together on the graph. In big projects this can complicate graph layout and setting this property to false may improve the image. |
| report-dep-graph.dependency-filter | &#39;build interface link&#39; | Types of dependencies to include in the generated picture. Possible values are: &#39;build&#39;, &#39;interface&#39;, &#39;link&#39;, &#39;copylocal&#39;, &#39;auto&#39;, or any combination of these, enter values space separated. |
| report-dep-graph.exclude-dependency-filter |  | Types of dependencies to exclude from the generated picture. Possible values are: &#39;build&#39;, &#39;interface&#39;, &#39;link&#39;, &#39;copylocal&#39;, &#39;auto&#39;, or any combination of these, enter values space separated. |
| report-dep-graph.excludemodules |  | List of modules to exclude from the generated graph. |
| report-dep-graph.find-ancestors | false | If &#39;true&#39; graph of the modules or packages that depend on a given module or package is created. Otherwise graph contains dependents of given module or package. |
| report-dep-graph.dependency-color-scheme | Standard | Valid values: Standard, ByDependent. In *Standard* scheme dependencies and module colors are defined by the type of the dependency and type of the module respectively.<br>In *ByDependent* color scheme modules are colored randomly, and dependency lines going into the module are colored by the color of the module. |

The legend file describing how different dependency types and module types are shown is also generated in the same directory:

![report dep graph legend]( report_dep_graph_legend.png )<a name="Example"></a>
## Example ##

Here is an example of dependency graph for EAThread test module:


```
D:\packages\EAThread\dev>nant test-report-dep-graph
[package] EAThread-dev (pc64-vc-dev-debug)  test-report-dep-graph
. . . . .
[WritePackageGraphViz] Input parameters:
graphfile:                 "D:\packages\EAThread\dev\build\framework_tmp/EAThread_graph.viz"
packagename:               "EAThread"
graphmodules:              TRUE
groupbypackages:           TRUE
configurations:            pc64-vc-dev-debug
dependency-filter:         "build|interface|link"
exclude-dependency-filter:
exclude-modules:
Populating Package Graph for Package: EAThread [pc64-vc-dev-debug]

Writing Package Graph to: D:\packages\EAThread\dev\build\framework_tmp\EAThread_graph-pc64-vc-dev-debug.viz

[nant] BUILD SUCCEEDED (00:00:00)
```
Generated image:

![EAThread graph]( eathread_graph.png )Another example shows graph where modules are not grouped by a package:


```
nant test-report-dep-graph -G:report-dep-graph.groupbypackages=false
```
![EAThread graph-nopackage]( eathread_graph-nopackage.png )This example shows graph where only &#39;build&#39; dependencies are reflected:


```
nant test-report-dep-graph -G:report-dep-graph.dependency-filter=build
```
![EAThread graph builddep]( eathread_graph_builddep.png )