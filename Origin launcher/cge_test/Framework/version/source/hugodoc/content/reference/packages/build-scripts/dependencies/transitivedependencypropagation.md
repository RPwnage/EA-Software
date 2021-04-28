---
title: Transitive Dependency Propagation
weight: 90
---

Topics in this section describe what is transitive dependency propagation in Framework and how does it work

<a name="TransitiveDependencyOverview"></a>
## Overview ##

Transitive dependency propagation means that framework can automatically propagate certain types of dependencies up the dependency chain.
Dependency flags can also be changed when dependency is propagated.

How transitive dependency propagation is useful? Consider simple example:

Program `A`  depends on library  `B`  and library  `B` in turn depends on library `C` . Without transitive propagation we have explicitly declare
following set of dependencies:


```
1)  B --- use (or interface) dependency on     ---> C
2)  A --- use + build dependency on            ---> B
3)  A --- link (or use) + build dependency on  ---> C
```
Suppose library `B` was changed and does not explicitly depend on library  `C`.
All packages that contain a Program or Dynamic Library that depend on B build scripts now need to be edited to remove dependency
on library `C` because it is not needed anymore. Often this cleanup is not done, and build scripts contain
dependencies that are not needed anymore.

Opposite example is when a library module adds new dependency on another library all Program or Dynamic Library library modules that link
with first library need to be updated.

With transitive dependency propagation Framework will automatically add last dependency `A-->C` .
This makes build scripts simpler. packages can add new dependencies without need to adjust other package

<a name="TransitiveDependencyPropagationRules"></a>
### Transitive Dependency Propagation Rules ###

 - Dependencies that have [link]( {{< ref "reference/packages/build-scripts/dependencies/overview.md#FrameworkDependencyFlags" >}} ) flag set are propagated.<br><br>This means that  `use`  or  `build` dependency is transitively propagated,<br>but `interface` dependency is not propagated.<br><br>include directories and exported defines are  **not** transitively propagated.
 - Transitive propagation stops in any module that has a `Link` build step (Program or DynamicLibrary),<br>or at Utility or MakeStyle project.<br><br>NOTE: If your Utility or MakeStyle module is actually a wrapper for a static library module and you do want transitive dependency propagation for<br>these two module types, you can set boolean attribute &#39;transitivelink&#39; to true under &lt;Utility&gt; and &lt;MakeStyle&gt; SXML module definition to<br>override the default behaviour.
 - [build]( {{< ref "reference/packages/build-scripts/dependencies/overview.md#FrameworkDependencyFlags" >}} ) flag is added to propagated dependency<br>if dependent library is in a [buildable]( {{< ref "reference/packages/manifest.xml-file/buildable.md" >}} )  package and  `autobuildclean` attribute is set to true for this package.

The Figure below demonstrates transitive dependency propagation.

![Trasnsitive Dependencies]( trasnsitivedependencies.png )<a name="InterfaceDependencyPropagation"></a>
### Interface Dependency Propagation ###

Include directories and defines are not automatically transitively propagated the same way as libraries. The reasons for such behavior are as follows:

 - If library module A depends on a library module B certainly both libraries need to be linked into executable or DLL (otherwise you can use interfacedependency between two modules).
 - Library module A may depend on a library module B, but it does not mean that public headers of library A include headers of library B.<br>Module A can use module B internally without exposing it functionality through public headers.<br><br>This means that in many cases transitive propagation of include directories is not required.
 - Big include search paths are one of main sources of slow compilation, and increasing them unnecessarily will have very negative impact on the build speed.<br>Transitive propagation of include directories can increase size of include search paths many folds and for that reason we decided not to propagate include directories automatically.
 - If the list of include directories becomes too large it can exceed Microsoft&#39;s limit on response file size.

If a module does need to propagate an interface dependency it must do so explicitly in it&#39;s [Initialize.xml]( {{< ref "reference/packages/initialize.xml-file/_index.md" >}} ) by declaring public dependencies. This can be useful in situations where a module&#39;s public headers include a dependent module&#39;s public headers. See the Initialize.zml reference
for more information.


##### Related Topics: #####
-  [nant.transitive]( {{< ref "reference/properties/nant.transitive_property.md" >}} ) 
-  [Initialize.xml file]( {{< ref "reference/packages/initialize.xml-file/_index.md" >}} ) 
