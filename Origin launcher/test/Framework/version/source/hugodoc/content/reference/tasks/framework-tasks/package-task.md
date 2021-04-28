---
title: < package > Task
weight: 1178
---
## Syntax
```xml
<package name="" targetversion="" initializeself="" failonerror="" verbose="" if="" unless="" />
```
## Summary ##
Declares a package and loads configuration. Also automatically loads the package&#39;s Initialize.xml.

## Remarks ##
This task should be called only once per package build file.

The task declares the following properties:

Property |Description |
--- |--- |
| ${package.name} | The name of the package. | 
| ${package.targetversion} | The version number of the package, determined from the **targetversion** attribute | 
| ${package.version} | The version number of the package, determined from the **path** to the package | 
| ${package.${package.name}.version} | Same as **${package.version}** but the property name includes the package name. | 
| ${package.config} | The configuration to build. | 
| ${package.configs} | For a Framework 1 package, it&#39;s a space delimited list<br>of all the configs found in the config folder.  For a Framework 2 package, this property excludes any configs<br>specified by &lt;config excludes&gt; in masterconfig.xml | 
| ${package.dir} | The directory of the package build file (depends on packageroot(s) but path should end in **/${package.name}/${package.version}**. | 
| ${package.${package.name}.dir} | The directory of the package build file (depends on packageroot(s) but path should end in **/${package.name}/${package.version}**. | 
| ${package.builddir} | The current package build directory (depends on **buildroot** but path should end in **/${package.name}/${package.version}**. | 
| ${package.${package.name}.builddir} | Same as **${package.builddir}** but the property name includes the package name. | 
| ${package.${package.name}.buildroot} | The directory for building binaries (can be set in masterconfig.xml via **buildroot** and **grouptype**) . | 
| ${package.${package.name}.parent} | Name of this package&#39;s parent package (applies only when this package is set to autobuildclean). | 

Configuration properties are not loaded until this task has been executed in your .build file.



### Example ###
Declares a package named EAThread with version number 1.24.05.


```xml

<project xmlns="schemas/ea/framework3.xsd">
    <package name="EAThread" targetversion="3.24.05"/>
    <echo message="Current package version: ${package.version}"/>
</project>
```



### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| name | Obsolete. Package name is now derived from the name of the .build file. | String | False |
| targetversion | The version of the package in development. This parameter is optional.<br>If it is not specified, Framework will use the actual version. | String | False |
| initializeself | Obsolete. Initialize | Nullable`1 | False |
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | Boolean | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

## Full Syntax
```xml
<package name="" targetversion="" initializeself="" failonerror="" verbose="" if="" unless="" />
```
