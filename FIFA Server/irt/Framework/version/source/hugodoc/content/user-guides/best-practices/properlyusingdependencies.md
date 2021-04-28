---
title: Define Dependencies Correctly
weight: 25
---

Use dependencies properly.

<a name="Dependencies"></a>
## Use proper dependency types. ##

Use appropriate dependency types to express exactly what you want from the build system. Basic types of dependencies:

 - **Auto** - behaves as build dependency if module has link step, behaves as use dependency if module is library
 - **Build** – build + interface + link
 - **Use** – interface + link
 - **Interface** – interface only
 - **Link** –  build + link
 - **copylocal** –  binaries from the dependent package are copied into parent package bin folder.

For more information about dependency types see [Overview]( {{< ref "reference/packages/build-scripts/dependencies/overview.md" >}} ) 

![Dependencies]( dependencies.png )<a name="SXMLConfiguringDependencies"></a>
### Configuring dependencies in SXML ###

Use  **build**  or  **use** dependency types to specify whether dependent should be built or not.

Use  *interface*  or/and  *link* attributes to specify which public data from the dependent modules should be used

![Dependencies Configure]( dependenciesconfigure.png )<a name="SXMLModuleDependies"></a>
### SXML makes it easy to define dependencies at package or module level ###

 ** `PackageName/[group]/ModuleName` ** 

Dependencies between different {{< url groupname >}}s are supported in Framework.

![Dependencies Module Level]( dependenciesmodulelevel.png )