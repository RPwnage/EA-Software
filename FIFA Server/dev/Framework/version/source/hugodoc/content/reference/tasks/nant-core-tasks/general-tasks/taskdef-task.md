---
title: < taskdef > Task
weight: 1126
---
## Syntax
```xml
<taskdef assembly="" override="" failonerror="" debugbuild="" compiler="" verbose="" if="" unless="">
  <sources defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" />
  <references defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" />
  <nugetreferences value="" append="" />
</taskdef>
```
## Summary ##
Loads tasks from a specified assembly. When source files are provided as input assembly is built first.

## Remarks ##
All assemblies already loaded in the NAnt Application Domain are automatically added as references when assembly is built from sources.

Task definitions are propagated to dependent packages like global properties.

NAnt by default will scan any assemblies ending in *Task.dll in the
same directory as NAnt.  You can use this task to include assemblies
in different locations or which use a different file
convention.  (Some .NET assemblies end will end in .net
instead of .dll)

NAnt can only use .NET assemblies; other types of files which
end in .dll won&#39;t work.



### Example ###
Include the tasks in an assembly.


```xml

<project xmlns="schemas/ea/framework3.xsd">
    <target name="init">
        <taskdef assembly="MyCustomTasks.dll"/>
    </target>
</project>

```


### Example ###
Include the tasks in an assembly.


```xml

<project xmlns="schemas/ea/framework3.xsd">
    <target name="init">
        <taskdef assembly="MyCustomTasks.dll">
          <sources>
              <includes name="src/*.cs"/>
          </sources>
          <references>
            <includes name="System.Drawing.dll" asis="true"/>
          </references>
        </taskdef>
    </target>
</project>

```



### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| assembly | File name of the assembly containing the NAnt task. | String | True |
| override | Override task with the same name if it already loaded into the Project. Default is &quot;false&quot; | Boolean | False |
| failonerror |  | Boolean | False |
| debugbuild | Setting this to false will generate optimized code. Defaults to true to aid debugging because &lt;taskdef&gt; code is rarely performance critical. | Boolean | False |
| compiler | Version of .Net compiler to use when building tasks. Format is: &#39;v3.5&#39;, or &#39;v4.0&#39;, or &#39;v4.5&#39;, ... . Default is &quot;v4.0&quot; | String | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

### Nested Elements
| Name | Description | Type | Required |
| ---- | ----------- | ---- | -------- |
| {{< task "NAnt.Core.FileSet" "sources" >}}| If defined, Tasks DLL will be built using these source files. | {{< task "NAnt.Core.FileSet" >}} | False |
| {{< task "NAnt.Core.FileSet" "references" >}}| Reference assembles. The following assemblies are automatically referenced: System, System.Core System.Runtime, System.Xml, NAnt.Core, NAnt.Tasks and EA.Tasks. | {{< task "NAnt.Core.FileSet" >}} | False |
| {{< task "NAnt.Core.Structured.ConditionalPropertyElement" "nugetreferences" >}}| Nuget package references. Nuget packages will be automatically downloaded and containted assemblies added as references. | {{< task "NAnt.Core.Structured.ConditionalPropertyElement" >}} | False |

## Full Syntax
```xml
<taskdef assembly="" override="" failonerror="" debugbuild="" compiler="" verbose="" if="" unless="">
  <sources defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" if="" unless="">
    <includes />
    <excludes />
    <do />
    <group />
  </sources>
  <references defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" if="" unless="">
    <includes />
    <excludes />
    <do />
    <group />
  </references>
  <nugetreferences if="" unless="" value="" append="">
    <do />
  </nugetreferences>
</taskdef>
```
