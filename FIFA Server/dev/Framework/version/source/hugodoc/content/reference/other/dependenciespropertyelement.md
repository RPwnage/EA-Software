---
title: DependenciesPropertyElement
weight: 1016
---
## Syntax
```xml
<DependenciesPropertyElement skip-runtime-dependency="">
  <auto interface="" link="" copylocal="" internal="" />
  <use interface="" link="" copylocal="" internal="" />
  <build interface="" link="" copylocal="" internal="" />
  <interface interface="" link="" copylocal="" internal="" />
  <link interface="" link="" copylocal="" internal="" />
  <echo />
  <do />
</DependenciesPropertyElement>
```
## Summary ##
Specifies dependencies at both the package and module level. For more details see dependency section of docs, filed under &quot;Reference/Packages/build scripts/Dependencies&quot;.


### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| skip-runtime-dependency | Allows you to disable the automatic build dependency on runtime modules that is used for test and example builds. | Boolean | False |

### Nested Elements
| Name | Description | Type | Required |
| ---- | ----------- | ---- | -------- |
| {{< task "EA.Eaconfig.Structured.DependencyDefinitionPropertyElement" "auto" >}}| (Note: New in Framework 3+) Using autodependencies is always a safe option. They simplify build scripts when module declaring dependencies can be built as a static or dynamic library depending on configuration settings. | {{< task "EA.Eaconfig.Structured.DependencyDefinitionPropertyElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.DependencyDefinitionPropertyElement" "use" >}}| This type of dependency is often used with static libraries. Acts as a combined interface and link dependency. | {{< task "EA.Eaconfig.Structured.DependencyDefinitionPropertyElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.DependencyDefinitionPropertyElement" "build" >}}| Sets the dependencies to be built by the package. Ignored when dependent package is non buildable, or has autobuildclean=false. | {{< task "EA.Eaconfig.Structured.DependencyDefinitionPropertyElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.DependencyDefinitionPropertyElement" "interface" >}}| These dependencies are used when only header files from dependent package are needed. Adds include directories and defines set in the Initialize.xml file of dependent package. | {{< task "EA.Eaconfig.Structured.DependencyDefinitionPropertyElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.DependencyDefinitionPropertyElement" "link" >}}| Use these dependencies to make sure no header files or defines from dependent package are used. Library directories, using directories, assemblies and DLLs are also taken from the dependent package. | {{< task "EA.Eaconfig.Structured.DependencyDefinitionPropertyElement" >}} | False |

## Full Syntax
```xml
<DependenciesPropertyElement skip-runtime-dependency="">
  <auto interface="" link="" copylocal="" internal="" if="" unless="" value="" append="">
    <do />
    <!--Text-->
  </auto>
  <use interface="" link="" copylocal="" internal="" if="" unless="" value="" append="">
    <do />
    <!--Text-->
  </use>
  <build interface="" link="" copylocal="" internal="" if="" unless="" value="" append="">
    <do />
    <!--Text-->
  </build>
  <interface interface="" link="" copylocal="" internal="" if="" unless="" value="" append="">
    <do />
    <!--Text-->
  </interface>
  <link interface="" link="" copylocal="" internal="" if="" unless="" value="" append="">
    <do />
    <!--Text-->
  </link>
  <echo />
  <do />
</DependenciesPropertyElement>
```
