---
title: < dependent > Task
weight: 1172
---
## Syntax
```xml
<dependent name="" version="" ondemand="" initialize="" warninglevel="" dropcircular="" failonerror="" verbose="" if="" unless="" />
```
## Summary ##
This task indicates that another package needs to be loaded as a dependency

## Remarks ##
This task looks in the named package&#39;s**scripts**folder
and executes the**Initialize.xml**if the package has one.
The file is loaded the same as using a NAnt**include**Task.

The `initialize.xml` file is a mechanism to expose information
about a package to other packages that are using it&#39;s contents.
It does this by defining properties that describe itself.

Any code required for initializing a dependent package should also appear in the**Initialize.xml**file.

When NAnt executes the dependent task it will create these properties by default:|   |   |
|---|---|
| package.all | List of descriptive names of all the packages this build file depends on. | 
| package.name.dir | Base directory of package name. | 
| package.name.version | Specific version number of package name. | 
| package.name.frameworkversion | The version number of the Framework the given package is designed for,<br>determined from the**&lt;frameworkVersion&gt;**of the package&#39;s Manifest.xml.<br>Default value is 1 if Manifest.xml doesn&#39;t exist. | 
| package.name.sdk-version | Version of the SDK or third party files contained within the package.<br>By default this is just the package version number up to the first dash character,<br>unless it has been overridden in the package&#39;s initialize.xml file.<br>Also, if the package&#39;s version starts with &quot;dev&quot; this property will be undefined. | 



If you only want these default properties and don&#39;t want to load the rest of the package&#39;s initialize.xml file
you can set the attribute**initialize** to **false**when you call the dependent task.

By convention, packages should only define properties using
names of the form `package.name.property` Following this convention avoids namespace problems and makes
it much easier for other people to use the properties your package sets.

One common use for the**Initialize.xml**file is for compiler packages to indicate where
the compiler is installed on the local machine by providing a property with the path.
It is important to use these properties to compilers rather `package.name.dir` because proxy SDK packages may point to a compiler installed elsewhere on the system.
SDK packages typically define the property `package.name.appdir` which is supposed to be the directory where the executable software resides.

This task must be embedded within a &lt;target&gt; with style &#39;build&#39;
or &#39;clean&#39; if this dependent,**Framework 2**package is to autobuildclean.



### Example ###
Declare an explicit dependency on the VisualStudio package.


```xml

<project xmlns="schemas/ea/framework3.xsd">
    <package name="HelloWorld"/>
    <dependent name="VisualStudio"/>
    <echo message="Visual Studio is installed at ${package.VisualStudio.appdir}"/>
</project>
```



### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| name | The name of the package to depend on. | String | True |
| version | Deprecated (Used in Framework 1) | String | False |
| ondemand | If true the package will be automatically downloaded from the package server. Default is true. | Boolean | False |
| initialize | If false the execution of the Initialize.xml script will be suppressed. Default is true. | Boolean | False |
| warninglevel | Warning level for missing or mismatching version. Default is NOTHING (0). | WarningLevel | False |
| dropcircular | Drop circular build dependency. If false throw on circular build dependencies | Boolean | False |
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | Boolean | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

## Full Syntax
```xml
<dependent name="" version="" ondemand="" initialize="" warninglevel="" dropcircular="" failonerror="" verbose="" if="" unless="" />
```
