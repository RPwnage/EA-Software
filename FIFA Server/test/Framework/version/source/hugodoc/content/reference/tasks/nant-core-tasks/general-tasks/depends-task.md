---
title: < depends > Task
weight: 1097
---
## Syntax
```xml
<depends property="" failonerror="" verbose="" if="" unless="">
  <inputs defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" />
  <outputs defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" />
</depends>
```
## Summary ##
Performs a generic dependency check.

## Remarks ##
When properly used, this task sets the specified property to true if any file in the  `inputs` is newer than any
file in the `outputs`  fileset.

Proper usage requires that at least one file needs to be present in the inputs fileset, as well as the outputs fileset.

If either fileset is empty, the property will be set to true.
Note that a fileset can be empty if it includes a nonexistent file(name) &quot;asis&quot;
(by setting the attribute asis=&quot;true&quot;).
Since the asis attribute is false by default, it&#39;s impossible to create
a fileset that includes a nonexistent file without overriding asis.

Additionally, if any file included in the outputs fileset does not exist, the
property will be set to true.



### Example ###
Shows how to setup basic dependency checking for any task.
Notice the use of theto create a persistent file.
This is useful if you are doing recursive builds.
Remember to create the -start file at the start of your build using
anotherinstance.
Note that if the depends task was not used, the core-build target would keep
getting called because of the recursive nature of the build.


```xml

<project default="build">
    <touch file="start"/>

    <target name="build" depends="apple banana">
        <fail unless="@{FileExists('apple')}"/>
        <fail unless="@{FileExists('banana')}"/>
    </target>

    <target name="apple">
        <nant buildfile="core.xml" target="apple-build"/>
    </target>

    <target name="banana">
        <nant buildfile="core.xml" target="banana-build"/>
    </target>
</project>

```
Where  `core.xml`  contains:


```xml

<project xmlns="schemas/ea/framework3.xsd">
    <target name="core">
        <depends property="needs-building">
            <inputs>
                <includes name="start"/>
            </inputs>
            <outputs>
                <includes name="core-built"/>
            </outputs>
        </depends>
        <nant
            if="${needs-building}"
            buildfile="core.xml" target="core-build"
            />
        <touch file="core-built"/>
    </target>

    <target name="apple-build" depends="core">
        <echo message="Building apple"/>
        <touch file="apple"/>
    </target>

    <target name="banana-build" depends="core">
        <echo message="Building banana"/>
        <touch file="banana"/>
    </target>

    <target name="core-build">
        <echo message="Building core"/>
        <touch file="core"/>
    </target>
</project>

```



### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| property | The property name to set to hold the result of the dependency check.  The value in<br>this property after the task has run successfully will be either `true` or `false` . | String | True |
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | Boolean | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

### Nested Elements
| Name | Description | Type | Required |
| ---- | ----------- | ---- | -------- |
| {{< task "NAnt.Core.FileSet" "inputs" >}}| Set of input files to check against. | {{< task "NAnt.Core.FileSet" >}} | False |
| {{< task "NAnt.Core.FileSet" "outputs" >}}| Set of output files to check against. | {{< task "NAnt.Core.FileSet" >}} | False |

## Full Syntax
```xml
<depends property="" failonerror="" verbose="" if="" unless="">
  <inputs defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" if="" unless="">
    <includes />
    <excludes />
    <do />
    <group />
  </inputs>
  <outputs defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" if="" unless="">
    <includes />
    <excludes />
    <do />
    <group />
  </outputs>
</depends>
```
