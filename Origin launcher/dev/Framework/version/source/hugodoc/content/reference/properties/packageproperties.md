---
title: Package Properties
weight: 218
---

These are properties that describe a package. They are setup by the &lt;package&gt; task and cannot be used until that task has run.

<a name="PackageProperties"></a>
## Package Properties ##

These properties are setup when you call the package task.

property name |is readonly |Description |
--- |--- |--- |
| package.name | (readonly) | Name of the package. |
| package.version |  | The version number of the package, determined from the<br>path to the package |
| package.targetversion |  | The version number of the package, determined from<br>the targetversion attribute |
| package.dir | (readonly) | The directory of the package build file (depends on packageroot(s)<br>but path should end in /${package.name}/${package.version}. |
| package.builddir |  | Build directory for the Package described in the Build File. NAnt sets it to `<buildroot>/<package name>/<package version>`  |
| package.configbindir |  | Directory where executables and DLL files are built. Default `${package.builddir}/${config}/bin`  |
| package.configbuilddir |  | Location for temporary build files. Default `${package.builddir}/${config}/build`  |
| package.configlibdir |  | Directory where library files are built. Default `${package.builddir}/${config}/lib`  |
| package.configs |  | All valid configurations. Used in buildall, solution generation,<br>and other targets. |
| package.config | (readonly) | Configuration to use. Alias for config property. |

By using the name of the package in the property you can get the value for a specific package.
These properties are only available for packages that have already been loaded as dependencies.

property name |is readonly |Description |
--- |--- |--- |
| package.[name].version |  | The version number of the package, determined from the path to the package. |
| package.[name].dir |  | The directory of the package build file (depends on packageroot(s) but<br>path should end in */${package.name}/${package.version}* .<br>Same as *package.dir* but can reference other packages from a build script. |
| package.[name].builddir |  | Same as *${package.builddir}* but the property name includes the<br>package name. *${package.builddir}* is only available in the package build script and contains build directory for this package.<br>When `<dependent>`  task is executed  *package.[name].builddir* will contain values for dependent packages |
| package.[name].buildroot |  | The directory for building binaries (can be set in masterconfig.xml via buildroot and grouptype) |
| package.[name].frameworkversion |  | Same as package.frameworkversion but referenced by Package name to access other packages info. |
| package.[name].parent | (readonly) | Name of this package&#39;s parent package (applies only when this package is<br>set to autobuildclean). |

