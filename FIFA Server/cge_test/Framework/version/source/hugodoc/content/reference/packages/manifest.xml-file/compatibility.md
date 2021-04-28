---
title: compatibility
weight: 69
---

This element allows to set up version and revision restrictions on dependent packages.

Often packages require particular version (minimal version) of another dependent package to work properly.
NAnt has a function `StrCompareVersions` that can be used in the build scripts to test for package versions.
Framework offers simple standard way to put version restrictions on dependent packages through `compatibility`  element in {{< url ManifestFile >}}.

When working with unreleased packages that are changing using version restrictions does not help. `compatibility` element allows to declare package revision string
and set revision restrictions on dependent packages. Revision strings should follow same formatting conventions as package versions, function `StrCompareVersions` is used to compare revisions.

## Usage ##

Enter the element in the Manifest file and specify optional nested element `dependent` .

##### compatibility #####
Root element for compatibility info. This element can declare package revision string and optionally restrictions on dependent packages.

 `compatibility`  element can contain zero or more  `dependent` elements.

 **Attributes:** 

   `revision` - optional attribute defines this package revision string.<br>It is needed when other packages declare revision restrictions on this package.
   `package` - Not used in Framework 3+, but it is a required attribute in Framework 1 and 2.<br><br><br>{{% panel theme = "primary" header = "Tip:" %}}<br>When defining `compatibility` element define package attribute to make Manifest file compatible with both Framework2 and Framework 3.<br>{{% /panel %}}

##### dependent #####
Specifies version or revision restrictions on dependent package.

 **Attributes:** 

   `package`  -  *required* . Name of the dependent package.
   `minrevision`  -  *optional* . minimal revision of dependent package requested.<br><br><br>{{% panel theme = "info" header = "Note:" %}}<br>Dependent package must have `revision`  defined in  `compatibibilty` element in its Manifest.xml file<br>{{% /panel %}}
   `maxrevision`  -  *optional* . maximal revision of dependent package compatible with this package.<br><br><br>{{% panel theme = "info" header = "Note:" %}}<br>Dependent package must have `revision`  defined in  `compatibibilty` element in its Manifest.xml file<br>{{% /panel %}}
   `minversion`  -  *optional* . minimal version of dependent package requested.
   `maxversion`  -  *optional* . maximal version of dependent package compatible with this package.
   `fail`  -  *optional, boolean* . If true, build will fail if condition is not met,<br>otherwise warning message is printed. Default is *false* .
   `message`  -  *optional* . Additional text included in the message printed by Framework when condition is not met.


{{% panel theme = "info" header = "Note:" %}}
For `max`  or  `min`  conditions either  *revision*  or  *version* restrictions should be defined but not both.
{{% /panel %}}

{{% panel theme = "info" header = "Note:" %}}
Version / revision restrictions are verified when dependency is actually used, i.e. &lt;dependent&gt; task is called, they aren&#39;t checked for all packages listed in masterconfig, only for packages that actually participate in the given build.
{{% /panel %}}
## Example ##

Framework package declares revision restriction on eaconfig package


```xml
Framework\3.02.00\Manifest.xml:
          
  <compatibility package="Framework">
      <dependent package="eaconfig" minrevision="2.02.00.837635" fail="true" message="Framework 3 requires eaconfig with feature XYZ"/>
  </compatibility>
          
eaconfig\2.02.00\Manifest.xml:

  <compatibility package="eaconfig" revision="2.02.00.837640"/>
```
If masterconfig file specifies incompatible version of eaconfig following error message is printed:


```
c:\packages\EAThread\dev>nant build
          
Package 'Framework' requires minimal revision '2.02.00.837635' of package 'eaconfig', but package 'eaconfig-2.01.00' (c:\packages\eaconfig\2.01.00) has revision '2.01.00.00'
Framework 3 requires eaconfig with feature XYZ
Try 'nant -help' for more information
```
Framework and configuration package can declare mutual version conditions


```xml
Framework\3.02.00\Manifest.xml:

  <compatibility package="Framework" revision="3.02.00.00">
      <dependent package="eaconfig" minrevision="2.02.00.00" fail="true" message="Framework 3 requires FW3 compatible version of eaconfig 2.02.00 or higher"/>
  </compatibility>

eaconfig\2.02.00\Manifest.xml:

  <compatibility package="eaconfig" revision="2.02.00.00">
      <dependent package="Framework" minrevision="3.02.00.00" fail="false" message="This version of eaconfig requires Framework-3.02.00 or higher"/>
  </compatibility>
```
Using version restrictions does not require revision attribute to be set in &lt;compatibility&gt; element of dependent package 


```xml
EAthread\dev\masterconfig.xml:

  <project xmlns="schemas/ea/framework3.xsd">
      <masterversions>
          <package name="EABase" version="2.00.31" />
          . . . .
      </masterversions>
  </project>

EAthread\dev\Manifest.xml:

  <compatibility package="EAThread">
      <dependent package="EABase" minversion="2.00.32" fail="false"/>
  </compatibility>
```

```
c:\packages\EAThread\dev>nant test-build
          
[package] EAThread-dev (pc-vc-dev-debug)  test-build
6>        [package] EASTL-dev (pc-vc-dev-debug)  load-package
2>        [package] MemoryMan-dev (pc-vc-dev-debug)  load-package
5>        [package] PPMalloc-dev (pc-vc-dev-debug)  load-package
3>        [package] EAStdC-dev (pc-vc-dev-debug)  load-package
1>        [package] EAAssert-1.03.00 (pc-vc-dev-debug)  load-package
4>        [package] EATest-dev (pc-vc-dev-debug)  load-package
2>    [warning] Package 'EAThread' requires minimal version '2.00.32' of package 'EABase', but package 'EABase' (F:\packages\EABase\2.00.31) has version '2.00.31'
3>    [warning] Package 'EAThread' requires minimal version '2.00.32' of package 'EABase', but package 'EABase' (F:\packages\EABase\2.00.31) has version '2.00.31'
6>    [warning] Package 'EAThread' requires minimal version '2.00.32' of package 'EABase', but package 'EABase' (F:\packages\EABase\2.00.31) has version '2.00.31'
[create-build-graph] test [Packages 13, modules 10, active modules 10]  (406 millisec)
1>        [nant-build] building EAAssert-1.03.00 (pc-vc-dev-debug) 'runtime.EAAssert'
3>        [nant-build] building EAStdC-dev (pc-vc-dev-debug) 'runtime.EAStdC'
6>        [nant-build] building EASTL-dev (pc-vc-dev-debug) 'runtime.EASTL'
4>        [nant-build] building EATest-dev (pc-vc-dev-debug) 'runtime.EATest'
[nant-build] building EAThread-dev (pc-vc-dev-debug) 'runtime.EAThread'
2>        [nant-build] building MemoryMan-dev (pc-vc-dev-debug) 'runtime.MemoryMan'
5>        [nant-build] building PPMalloc-dev (pc-vc-dev-debug) 'runtime.PPMalloc'
[nant-build] building EAThread-dev (pc-vc-dev-debug) 'test.EAThreadTest'
[nant-build] finished target 'test-build' (7 modules) (173 millisec)
[nant] BUILD SUCCEEDED (00:00:02)
```

##### Related Topics: #####
-  [Manifest.xml file]( {{< ref "reference/packages/manifest.xml-file/_index.md" >}} ) 
