---
title: Targets
weight: 208
---

A NAnt&lt;target&gt;is an XML script fragment associated with a name and other target attributes.
Target scripts can be executed after a target is defined by specifying the targets name on the NAnt command line or by using the&lt;call&gt;task.

<a name="TargetUsage"></a>
## Usage ##

 **Targets can be specified on the NAnt command line:** 

nanttarget1 target2-buildfile:example.build

NAnt loads and executes build script (example.build) and then executes targets in the order they are listed on the command line.



###### Targets can be invoked using the call task, and targets can call other targets: ######

```xml
<project xmlns="schemas/ea/framework3.xsd">

 <target name="helloworld">
  <echo message="Hello, World!"/>
 </target>

 <target name="run">
  <call target="helloworld"/>
 </target>

 <call target="helloworld"/>

</project>
```


###### NAnt provides extension method to invoke targets from C# code: ######

```c#
project.Execute(targetName, force:true, failIfTargetDoesNotExist:false);
```
<a name="TargetDefinition"></a>
## Definition ##

A&lt;target&gt;task has following attributes:

Attribute |Description |Required |
--- |--- |--- |
| name | The name of the target. The name used to refer to the target to invoke it or to reference it in a dependency list. | ![requirements 1a]( requirements1a.gif ) |
| depends | A comma or space separated list of names of targets on which this target depends. |  |
| if, unless | Target executes when the expression evaluates to true (if) or to false (unless) |  |
| description | A short description of this target&#39;s function. |  |
| hidden | Prevents the target from being listed in the project help. Default is false. |  |
| style | Framework 2 packages only: Style can be &#39;use&#39;, &#39;build&#39;, or &#39;clean&#39;. See{{< url AutoBuildClean >}}for details. |  |
| override | Target definition will replace previous definition instead of raising error in case if previous definition contained attribute `'allowoverride'=true` . Default value is false. |  |
| allowoverride | If set to true this target definition can be replaced by another target definition. Default value is false. |  |

