---
title: masterversions
weight: 243
---

The masterversions section of the Masterconfig file lists all of the packages and all of the versions of those package used in a build..

<a name="Masterversions"></a>
## Masterversions ##

The masterversions section of the Masterconfig is a central location for indicating which versions of dependent packages to use during a build.
When a package uses the &lt;dependent&gt; task or a module declares a dependency on another package, Framework refers to the list of version in masterversion in order to determine which version of that package to use.

In the early days of Framework the versions were not centralized in a single list like this, instead each dependency specified a version.
That quickly became problematic when different packages depended on different versions of the same package
and created a lot of work for package maintainers to update all of the dependent package versions throughout their build scripts.
So the solution was to centralize this into a single list in what became the masterconfig file.


```xml
<masterversions>
 <package name="eaconfig"              version="2.04.03"          />
 <package name="VisualStudio"          version="10.0.40219-2-sp1" />
 <package name="EAIO"                  version="2.05.00"          />

 <grouptype name="docs">
  <package  name="Doxygen"            version="1.5.6"   />
  <package  name="GraphViz"           version="2.6.0"   >
 </grouptype>
</masterversions>
```
The masterversions section can contain [package elements]( {{< ref "reference/master-configuration-file/masterversions/package.md" >}} ) and groups of packages can be organized with [grouptype elements]( {{< ref "reference/master-configuration-file/masterversions/grouptype.md" >}} ) that can influence how and where packages are built.

