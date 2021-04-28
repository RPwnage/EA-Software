---
title: importmsbuildprojects
weight: 121
---

 `importmsbuildprojects` property lets to import additional MSBuild projects in the Visual Studio project.

## Usage ##

 `importmsbuildprojects` will result in additional &#39;Import&#39; elements in the generated Visual Studio project file right
after import of the standard MSbuild targets. For each line in `importmsbuildprojects` property following XML element is added
in generated project:


```xml
<Import Project="${line}" />
```
Properties used to define additional import projects:

 - {{< url groupname >}} `.importmsbuildprojects`

The following properties are usually setup by the configuration package.
It will affect all modules in the package.

 - `eaconfig.importmsbuildprojects`

Framework checks properties in the order they are listed above and takes first one found.

## Example ##


```xml
<property name="runtime.MyModule.buildtype" value="Program" />

<property name="runtime.MyModule.importmsbuildprojects" value="C:\game\targets\mygame.Targets"/>
```
