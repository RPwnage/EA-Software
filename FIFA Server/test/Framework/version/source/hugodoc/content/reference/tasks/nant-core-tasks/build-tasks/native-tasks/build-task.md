---
title: < build > Task
weight: 1161
---
## Syntax
```xml
<build outputdir="" name="" type="" outputextension="" outputprefix="" failonerror="" verbose="" if="" unless="">
  <sources defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" />
  <objects defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" />
  <includedirs />
  <usingdirs />
  <forceusing-assemblies defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" />
  <ccoptions />
  <defines />
  <asmsources defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" />
  <asmincludedirs />
  <asmoptions />
  <asmdefines />
  <libraries defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" />
  <dependencies defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" />
</build>
```
## Summary ##
Builds C++ source files into a program or library.

## Remarks ##
The `build`  task combines the , ,, and to provide an easy way to build a program or library.  The `build` task expects
the compiler, linker, and librarian properties to be set
correctly before it is invoked.

All the C++ tasks refer to the  `build.pathstyle` property to determine how
format filename paths.  For Unix (&#39;/&#39;) based SDK&#39;s this property should be set to `Unix` .
For Windows (&#39;\&#39;) based SDK&#39;s this property should be set to `Windows` .  If the
property is not set no path conversions will take place.

To compile individual files use the `build.sourcefile.X`  property where  `X` is a unique identifier.
This will let all subsequent `build`  tasks know to only build these files from the given  `build`  tasks  `sources` file set.
If any `build.sourcefile.X` properties are defined no linking or archiving will occur.
You may pass these values in through the command line using the `-D` argument for defining properties.
For example the command `nant -D:build.sourcefile.0=src/test.c`  will let all  `build`  tasks know to only compile the source file  `src/test.c` .

If the  `outputdir` attribute is not specified the task will look for a property called `build.outputdir` .  The value of the property may contain the term  `%outputname%` which
will be replaced with value specified in the `name`  attribute.  If the  `build.outputdir` property is not defined and you are building a package the output directory will be either 1) `${package.builddir}/build/${package.config}/%outputname%` for a Framework 1 package, or
2) `${package.builddir}/${package.config}/build/%outputname%` for a Framework 2 package.
If you are not building a package then
the output directory will be the current directory.

**NOTE: **Empty outputdir will be treated as not specifying outputdir. If outputdir is specified,
absolute path is recommended. Relative path such as &quot;.&quot; or &quot;temp&quot; may have
unexpected result.

This task can use a named option set for specifying options for unique components.  For more information see the Fundamentals-&gt;Option Sets topic in help file.



### Example ###
An example of compiling and linking the proverbial &quot;Hello World&quot; program using a single task.


```xml

<project default="build">
    <dependent name="VisualStudio" version="7.1.1-4"/>
    <property name="outdir" value="build"/>

    <target name="build">
        <build type="Program" outputdir="${outdir}" name="hello">
            <sources>
                <includes name="hello.c"/>
            </sources>
        </build>
        <fail message="Program did not build" unless="@{FileExists('${outdir}/hello.exe')}"/>
    </target>
</project>

```
Where  `hello.c`  contains:


```xml

#include <stdio.h>

void main()
{
    printf("Hello, World!\n");
}

```


### Example ###
An example of compiling and linking using a fileset and optionset.


```xml

<project default="build">
    <dependent name="VisualStudio" version="7.1.1-4"/>
    <property name="outdir" value="build"/>

    <fileset name="MyFileSet">
        <includes name="hello.c"/>
    </fileset>

    <optionset name="MyOptionSet">
        <option name="build.tasks" value="cc lib link" />
        <option name="cc.threadcount" value="1" />
    </optionset>

    <target name="build">
        <build type="MyOptionSet" outputdir="${outdir}" name="hello">
            <sources>
                <includes fromfileset="MyFileSet" />
            </sources>
        </build>
        <fail message="Program did not build" unless="@{FileExists('${outdir}/hello.exe')}"/>
    </target>
</project>

```
Where  `hello.c`  contains:


```xml

#include <stdio.h>

void main()
{
    printf("Hello, World!\n");
}

```



### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| outputdir | Directory where all output files are placed.  See task details for more information if this attribute is not specified. | String | False |
| name | The base name of any associated output files.  The appropriate extension will be added. | String | True |
| type | Type of the  `build`  task output.  Valid values are  `Program` ,  `Library`  or the name of an named option set. | String | True |
| outputextension | primary output extension | String | False |
| outputprefix | primary output prefix | String | False |
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | Boolean | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

### Nested Elements
| Name | Description | Type | Required |
| ---- | ----------- | ---- | -------- |
| {{< task "NAnt.Core.FileSet" "sources" >}}| The set of source files to compile. | {{< task "NAnt.Core.FileSet" >}} | False |
| {{< task "NAnt.Core.FileSet" "objects" >}}| Additional object files to link or archive with. | {{< task "NAnt.Core.FileSet" >}} | False |
| {{< task "NAnt.Core.PropertyElement" "includedirs" >}}| Custom include directories.  New line  `'\n'`  or semicolon  `';'`  delimited. | {{< task "NAnt.Core.PropertyElement" >}} | False |
| {{< task "NAnt.Core.PropertyElement" "usingdirs" >}}| Custom using directories.  New line  `'\n'`  or semicolon  `';'`  delimited. | {{< task "NAnt.Core.PropertyElement" >}} | False |
| {{< task "NAnt.Core.FileSet" "forceusing-assemblies" >}}| Force using assemblies.  New line  `'\n'`  or semicolon  `';'`  delimited. | {{< task "NAnt.Core.FileSet" >}} | False |
| {{< task "NAnt.Core.PropertyElement" "ccoptions" >}}| Custom compiler options for these files. | {{< task "NAnt.Core.PropertyElement" >}} | False |
| {{< task "NAnt.Core.PropertyElement" "defines" >}}| Custom compiler defines. | {{< task "NAnt.Core.PropertyElement" >}} | False |
| {{< task "NAnt.Core.FileSet" "asmsources" >}}| The set of source files to assemble. | {{< task "NAnt.Core.FileSet" >}} | False |
| {{< task "NAnt.Core.PropertyElement" "asmincludedirs" >}}| Custom include directories for these files.  New line  ` '\n'`  or semicolon  `';'`  delimited. | {{< task "NAnt.Core.PropertyElement" >}} | False |
| {{< task "NAnt.Core.PropertyElement" "asmoptions" >}}| Custom assembler options for these files. | {{< task "NAnt.Core.PropertyElement" >}} | False |
| {{< task "NAnt.Core.PropertyElement" "asmdefines" >}}| Custom assembler defines for these files. | {{< task "NAnt.Core.PropertyElement" >}} | False |
| {{< task "NAnt.Core.FileSet" "libraries" >}}| The set of libraries to link with. | {{< task "NAnt.Core.FileSet" >}} | False |
| {{< task "NAnt.Core.FileSet" "dependencies" >}}| Additional file dependencies of a build | {{< task "NAnt.Core.FileSet" >}} | False |

## Full Syntax
```xml
<build outputdir="" name="" type="" outputextension="" outputprefix="" failonerror="" verbose="" if="" unless="">
  <sources defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" if="" unless="">
    <includes />
    <excludes />
    <do />
    <group />
  </sources>
  <objects defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" if="" unless="">
    <includes />
    <excludes />
    <do />
    <group />
  </objects>
  <includedirs />
  <usingdirs />
  <forceusing-assemblies defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" if="" unless="">
    <includes />
    <excludes />
    <do />
    <group />
  </forceusing-assemblies>
  <ccoptions />
  <defines />
  <asmsources defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" if="" unless="">
    <includes />
    <excludes />
    <do />
    <group />
  </asmsources>
  <asmincludedirs />
  <asmoptions />
  <asmdefines />
  <libraries defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" if="" unless="">
    <includes />
    <excludes />
    <do />
    <group />
  </libraries>
  <dependencies defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" if="" unless="">
    <includes />
    <excludes />
    <do />
    <group />
  </dependencies>
</build>
```
