---
title: Package Discovery
weight: 1
---

This page details how Framework tries to satisfy package dependencies.

# Package Discovery Rules #

When Framework encounters the need for a package it will use the provided [masterconfig file]( {{< ref "reference/master-configuration-file/_index.md" >}} ) to determine the version then try to find a matching package in the following way:

* **Search Package Roots** - The [package roots]( {{< ref "reference/master-configuration-file/packageroots.md" >}} ) are searched in order specified in the masterconfig file. Package roots are not searched recursively - the package folder must exist at the root level. The following are considered valid packages:
 * **package-root/package-name/Manifest.xml** - A folder with a root with the name  containing a [Manifest]( {{< ref "reference/packages/manifest.xml-file/_index.md" >}} ) file is considered a valid package. For it to match the masterconfig version requirement it must have a [**```<versionName>```**]( {{< ref "reference/packages/manifest.xml-file/versionname.md" >}} ) that matches the masterconfig version _OR_ the masterconfig must use the special version 'flattened' which allows it to match any package which does not have a version subfolder.
 * **package-root/package-name/package-name.build** - Framework will also accept a package that does not have a _Manifest.xml_ if it has a _.build_ file with the same name as the package. This can only be used if masterconfig uses special version 'flattened'.
 * **package-root/package-name/version/Manifest.xml** - After searching for a flat package in the in a package root directly the first level of sub-folders will be searched for a folder that matches the version specified by the masterconfig that contains a _Manifest.xml_ file. The folder version takes precedence over any version set in the _Manifest.xml_.
  * **package-root/package-name/version/package-name.build** - In the abscense of a _Manifest.xml_, a .build file with same name as the package being sought is also consider a valid package directory.

* **On Demand Download** - If the package is not found anywhere within you local package roots Framework will attempt to download the package from a remote source provided masterconfig enables [ondemand]( {{< ref "reference/master-configuration-file/ondemand/_index.md" >}} ). By default this will use the _EA Package Server_ but the _uri_ attribute can be used to instead to use the [p4]( {{< ref "reference/master-configuration-file/ondemand/ondemand_p4protocol.md" >}} ) or [nuget]( {{< ref "reference/master-configuration-file/ondemand/ondemand_nugetprotocol.md" >}} ) protocols. Packages are downloaded to the [ondemand root]( {{< ref "reference/master-configuration-file/ondemandroot.md" >}} ).

**In _all_ cases, _except_ [compatibility]( {{< ref "reference/packages/manifest.xml-file/compatibility.md" >}} ) checks, Framework will use the version as specified in the masterconfig to represent the version in properties and error messages. If a package is found via the rules above in a case where the _Manifest.xml_ ```<versionName>``` does not match the version specified in the masterconfig ```<versionName>``` is ignored.**

## Special Cases ##

* **Nuget Versions** - Packages with a Nuget uri attribute do not need to have all their dependencies' versions specified in the masterconfig. Framework has a limited ability to automatically determine some of these feature from Nuget dependency information. This is not a full resolution and does not work across multiple Nuget repositories.
* **Local On Demand** - Local on-demand allows for a second location for packages to be downloaded to.
* **Development Root** - The masterconfig can specify a "development root". If a package is given the _switched-from-version_ attribute then it can ONLY be found in this special root.