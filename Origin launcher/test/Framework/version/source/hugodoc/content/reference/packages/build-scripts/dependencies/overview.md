---
title: Overview
weight: 89
---

General information about dependencies in Framework

<a name="DependenciesOveriew"></a>
## Overview ##

Internally in Framework dependencies are always defined between modules,
but in the build scripts dependencies can be specified both at [package]( {{< ref "reference/packages/whatispackage.md" >}} ) and [module]( {{< ref "reference/packages/modules.md" >}} ) level.

<a name="FrameworkDependencyFlags"></a>
### Dependency flags ###

Framework supports several types of dependencies, these dependency types can be described by following three flags:

##### interface #####
Add include directories and defines set in the Initialize.xml file of dependent package

##### link #####
Link with libraries from dependent package (module). Library directories, using directories, assemblies and DLLs are also taken from the dependent package

##### build #####
Build dependent package.


{{% panel theme = "info" header = "Note:" %}}
`build` flag is ignored when dependent package is non buildable, or has autobuildclean=false.
{{% /panel %}}
There is another, fourth flag, which affects how Framework automatically sets dependency flag `build` 

##### auto #####
 - When module that declares auto dependency is a Utility or MakeStyle<br>the `auto` dependency is automatically converted to build dependency. This means that Framework<br>adds `build` flag.
 - When module that declares auto dependency is a Program, Dynamic Library (i.e. has link step) and dependency has `link` flag set<br>the `auto` dependency is automatically converted to build dependency. This means that Framework<br>adds `build` flag.
 - When module that declares auto dependency is a Library, the `build` flag is not set for auto dependency.  This means<br>the dependency will not be built before building this Library module.

### Types of dependencies available in Framework ###

Type |interface |link |build |auto |transitively propagated |Description |
--- |--- |--- |--- |--- |--- |--- |
| use | ![requirements 1a]( requirements1a.gif ) | ![requirements 1a]( requirements1a.gif ) |  |  | ![requirements 1a]( requirements1a.gif ) | This type of dependencies often used with prebuilt external third party libraries. |
| interface | ![requirements 1a]( requirements1a.gif ) |  |  |  |  | These dependencies are used when only header files from dependent package are needed. |
| uselink |  | ![requirements 1a]( requirements1a.gif ) |  |  | ![requirements 1a]( requirements1a.gif ) | These dependencies can be used to ensure no header files from dependent package were used. |
|  |
| build | ![requirements 1a]( requirements1a.gif ) | ![requirements 1a]( requirements1a.gif ) | ![requirements 1a]( requirements1a.gif ) |  |  |  |
| link |  | ![requirements 1a]( requirements1a.gif ) | ![requirements 1a]( requirements1a.gif ) |  | ![requirements 1a]( requirements1a.gif ) | Use these dependencies to make sure no header files or defines from dependent package were used. |
| buildinterface | ![requirements 1a]( requirements1a.gif ) |  | ![requirements 1a]( requirements1a.gif ) |  |  | These dependencies are used when only header files from dependent package are needed and same time modules in dependent package must be built. |
| buildonly |  |  | ![requirements 1a]( requirements1a.gif ) |  |  | This type of dependency ensures build but no public data from dependent package are used in a build |
| auto | ![requirements 1a]( requirements1a.gif ) | ![requirements 1a]( requirements1a.gif ) | ??? | ![requirements 1a]( requirements1a.gif ) | ![requirements 1a]( requirements1a.gif ) | Using autodependencies is usually a safe option. They simplify build scripts when module declaring dependencies<br>can be built as a static or dynamic library depending on configuration settings. |
| autointerface | ![requirements 1a]( requirements1a.gif ) |  | ??? | ![requirements 1a]( requirements1a.gif ) |  |  |
| autolink |  | ![requirements 1a]( requirements1a.gif ) | ??? | ![requirements 1a]( requirements1a.gif ) | ![requirements 1a]( requirements1a.gif ) |  |


{{% panel theme = "info" header = "Note:" %}}
When the same module is included in several dependency definitions the flags from all these definitions are merged.
{{% /panel %}}

##### Related Topics: #####
-  [Everything you wanted to know about Framework defines, but were afraid to ask]( {{< ref "user-guides/everythingaboutdefines.md" >}} ) 
