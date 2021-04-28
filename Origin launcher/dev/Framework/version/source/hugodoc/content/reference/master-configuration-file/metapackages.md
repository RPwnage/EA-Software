---
title: Meta-Packages
weight: 258
---

This page is a brief overview of the meta-packages feature.

<a name="Summary"></a>
## Summary ##

A Meta-package is a package which describes a collection of packages and versions of those packages that work together.
The idea is that users may want to take a bundle of packages without the hassle of micro-managing the versions of all of the packages
and instead have a set of versions that are known to work together.

The way the concept of a Meta-package is currently implemented in Framework is as a package that contains a special masterconfig fragment in the root of the package called
masterconfig.meta.xml. This fragment contains all of the packages, and the versions of those packages, that make up the meta-package.
The Meta-package is then specified in the main masterconfig or any masterconfig fragment the same as any package.
When Meta-packages are listed in the masterconfig they need to have the metapackage attribute.

###### Example of how a meta package is declared in the main masterconfig file ######

```xml
<project xmlns="schemas/ea/framework3.xsd">
  <masterversions>
    <metapackage name="TestPackage" version="1.00.00"/>
  </masterversions>
</project>
```

{{% panel theme = "info" header = "Note:" %}}
Framework will completely load the main masterconfig and any fragments first before loading any meta-package&#39;s fragments.
Packages listed in the main masterconfig or fragments will trump what is listed in the meta-package&#39;s masterconfig.
So if the meta package lists the version as 2 but the main masterconfig lists the version as 3, framework will use the version 3.
It is done this way so that if you want to use a different version than the one in the meta-package it can be overriden easily.
{{% /panel %}}

{{% panel theme = "info" header = "Note:" %}}
When using package workflows commands like &quot;fb workon&quot;, &quot;fb fork&quot;, etc. It will currently ignore meta-packages.
If you want to fork or work on a meta-package you should first add that package to your main masterconfig
in order to explicitly indicate that you want to override the version declared by the meta-package.
{{% /panel %}}

