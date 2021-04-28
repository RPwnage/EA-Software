---
title: < cc > Task
weight: 1162
---
## Syntax
```xml
<cc type="" outputdir="" outputname="" collectcompilationtime="" gendependencyonly="">
  <sources defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" />
  <forceusing-assemblies defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" />
  <includedirs />
  <usingdirs />
  <options />
  <defines />
</cc>
```
## Summary ##
A generic C/C++ compiler task.

## Remarks ##
The  `cc`  task requires the following property to be set:

Property |Description |
--- |--- |
| ${cc} | The absolute pathname of the compiler executable. | 
| ${cc.template.includedir} | The syntax template to transform  `${cc.includes}`  into compiler flags. | 

The task will make use of the following properties:

Property |Description |
--- |--- |
| ${cc.defines} | The defines for the compilation. | 
| ${cc.forcelowercasefilepaths} | If  `true` file paths (folder and file names) will be<br>forced to lower case (useful in avoiding redundant NAnt builds<br>between VS.net and command line environments in the case of capitalized<br>folder names when file name case is unimportant).  Default is false. | 
| ${cc.includedirs} | The include directories for the compilation. | 
| ${cc.nodep} | If  `true`  dependency files will not be created. | 
| ${cc.nocompile} | If  `true`  object files not be created. | 
| ${cc.objfile.extension} | The file extension for object files.  Default is &quot;.obj&quot;. | 
| ${cc.options} | The options flags for the compilation.  All the files from the  `source`  fileset will be compiled with the same options.  If you want different compile options, you must invoke the  `cc`  task with those options. | 
| ${cc.parallelcompiler} | If true, compilation will occur on parallel threads if possible<br>.  Default is false. | 
| ${cc.template.define} | The syntax template to transform  `${cc.defines}`  into compiler flags. | 
| ${cc.template.includedir} | The syntax template to transform  `${cc.includedirs}`  into compiler flags. | 
| ${cc.template.usingdir} | The syntax template to transform  `${cc.usingdirs}`  into compiler flags. | 
| ${cc.template.commandline} | The template to use when creating the command line.  Default is %defines% %includedirs% %options%. | 
| ${cc.template.sourcefile} | The syntax template to transform a  `source file`  into compiler source file. | 
| ${cc.template.responsefile} | The syntax template to transform the  `responsefile`  into a response file flag. Default is @&quot;%responsefile%&quot;. | 
| ${cc.threadcount} | The number of threads to use when compiling. Default is 1 per cpu. | 
| ${cc.useresponsefile} | If  `true`  a response file, containing the entire commandline, will be passed as the only argument to the compiler. Default is false. | 
| ${cc.userelativepaths} | If  `True`  the working directory of the compiler will be set to the base directory of the  `sources`  fileset. All source and output files will then be made relative to this path. Default is  `false` . | 
| ${cc.usingdirs} | The using directories for managed C++, such as VC++. Compiler that support using directive should read this property. | 
| ${cc.template.responsefile.commandline} | The template to use when creating the command line for response file. Default value is ${cc.template.responsefile.commandline} | 
| ${cc.responsefile.separator} | Separator used in response files. Default is &quot; &quot;. | 
| ${cc.template.responsefile.objectfile} | The syntax template to transform the  `objects`  file set into compiler flags in response file. Default ${cc.template.objectfile} | 

The following terms can be used in the above properties:

Term |Description |
--- |--- |
| %define% | Used by the  `${cc.template.define}`  property to represent the current value of the  `${cc.defines}`  property during template processing. | 
| %includedir% | Used by the  `${cc.template.includedir}`  property to represent the individual value of the  `${cc.includedirs}`  property. | 
| %usingdir% | Used by the  `${cc.template.usingdir}`  property to represent the individual value of the  `${cc.usingdirs}`  property. | 
| %responsefile% | Used by the  `${cc..template.responsefile}`  property to represent the filename of the response file. | 
| %objectfile% | Used by the  `${cc.options}`  property to represent the object file name.  Object file names are formed by appending  `".obj"`  to the source file name. | 
| %outputdir% | Used by the  `${cc.options}`  property to represent the actual values of the  `%outputdir%`  attribute. | 
| %outputname% | Used by the  `${cc.options}`  property to represent the actual value of the  `%outputname%`  attribute. | 
| %sourcefile% | Used by the  `${cc.options}`  property to represent the individual value of the  `sources`  fileset. | 

This task can use a named option set for specifying options for unique components.  For more information see the Fundamentals-&gt;Option Sets topic in help file.



### Example ###
An example of compiling and linking the proverbial &quot;Hello World&quot; program.


```xml

<project default="build">
    <package name="Hello" />
    <dependent name="VisualStudio" version="7.1.1-4"/>

    <target name="build">
        <cc outputname="${package.name}" outputdir="${package.dir}/build">
            <sources>
                <includes name="hello.c"/>
            </sources>
        </cc>
        <link outputname="${package.name}" outputdir="${package.dir}/build">
            <objects>
                <includes name="${package.dir}/build/hello.c.obj"/>
            </objects>
        </link>
        <fail message="Program did not link" unless="@{FileExists('${package.dir}/build/hello.exe')}"/>
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
This is a complete configuration for setting up the C++ tasks to use Visual C++.  Use this example as
a guide for creating configurations for other compilers.


```xml

<project xmlns="schemas/ea/framework3.xsd">
    <dependent name="VisualStudio" version="7.1.1-4"/>
    
    <!-- Location of the compiler -->
    <property name="cc" value="${package.VisualStudio.appdir}\VC7\BIN\cl.exe"/>
    <property name="link" value="${package.VisualStudio.appdir}\VC7\BIN\link.exe"/>
    <property name="lib" value="${package.VisualStudio.appdir}\VC7\BIN\lib.exe"/>

    <!-- Include search patsh for the compiler -->
    <property name="cc.includedirs">
        ${package.VisualStudio.appdir}\VC7\ATLMFC\INCLUDE
        ${package.VisualStudio.appdir}\VC7\INCLUDE
        ${package.VisualStudio.appdir}\VC7\PlatformSDK\include\prerelease
        ${package.VisualStudio.appdir}\VC7\PlatformSDK\include
        ${package.VisualStudio.appdir}\FrameworkSDK\include
    </property>

    <!-- Library search paths for the linker -->
    <property name="link.librarydirs">
        ${property.value}
        ${package.VisualStudio.appdir}\VC7\ATLMFC\LIB
        ${package.VisualStudio.appdir}\VC7\LIB
        ${package.VisualStudio.appdir}\VC7\PlatformSDK\lib\prerelase
        ${package.VisualStudio.appdir}\VC7\PlatformSDK\lib
        ${package.VisualStudio.appdir}\FrameworkSDK\lib
    </property>

    <!-- Option flags the compiler -->
    <property name="cc.options">
        -nologo         <!-- turn off MS copyright message -->
        -c              <!-- compile only -->
        -W4             <!-- warning level -->
        -Zi             <!-- enable debugging information (.pdb) -->

        <!-- program database -->
        -Fd"%outputdir%/%outputname%.pdb"

        <!-- object file name -->
        -Fo"%objectfile%"

        <!-- source file -->
        "%sourcefile%"
    </property>

    <!-- Option flags the linker -->
    <property name="link.options">
        /NOLOGO
        /INCREMENTAL:YES
        /DEBUG
        /SUBSYSTEM:CONSOLE
        /MACHINE:IX86
        /OUT:"%outputdir%/%outputname%.exe"
    </property>

    <!-- Option flags the librarian -->
    <property name="lib.options">
        /nologo
        /OUT:"%outputdir%/%outputname%.lib"
    </property>

    <!-- Templates to convert cc.includedirs and cc.defines properties into compiler flags -->
    <property name='cc.template.includedir'    value='-I "%includedir%"'/>
    <property name='cc.template.define'        value='-D %define%'/>

    <!-- Templates to convert the objects fileset into librarian flags -->
    <property name='lib.template.objectfile'   value='"%objectfile%"'/>

    <!-- Templates to convert the objects fileset into linker flags -->
    <property name='link.template.librarydir'  value='/LIBPATH:"%librarydir%"'/>
    <property name='link.template.libraryfile' value='"%libraryfile%"'/>
    <property name='link.template.objectfile'  value='"%objectfile%"'/>
</project>

```



### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| type | Name of option set to use for compiling options.  If null<br>given values from the global `cc.*`  properties.  Default is null. | String | False |
| outputdir | Directory where all output files are placed.  Default is the base directory<br>of the build file. | String | False |
| outputname | The base name of any associated output files (e.g. the symbol table name).<br>Default is an empty string. | String | False |
| collectcompilationtime | Collect compilation time.<br>Default is an empty string. | Boolean | False |
| gendependencyonly | Doesn&#39;t actually do build.  Just create the dependency .dep file only for each source file. | Boolean | False |
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | Boolean | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

### Nested Elements
| Name | Description | Type | Required |
| ---- | ----------- | ---- | -------- |
| {{< task "NAnt.Core.FileSet" "sources" >}}| The set of files to compile. | {{< task "NAnt.Core.FileSet" >}} | False |
| {{< task "NAnt.Core.FileSet" "forceusing-assemblies" >}}| Force using assemblies for managed builds. | {{< task "NAnt.Core.FileSet" >}} | False |
| {{< task "NAnt.Core.PropertyElement" "includedirs" >}}| Custom include directories for this set files.  Use complete paths and place<br>each directory on it&#39;s own line. | {{< task "NAnt.Core.PropertyElement" >}} | False |
| {{< task "NAnt.Core.PropertyElement" "usingdirs" >}}| Custom using directories for this set files.  Use complete paths and place<br>each directory on it&#39;s own line. Only applied to managed C++ compiler, such as VC++. | {{< task "NAnt.Core.PropertyElement" >}} | False |
| {{< task "NAnt.Core.PropertyElement" "options" >}}| Custom program options for these files.  Options get appended to the end of<br>the options specified by the `${cc.options}`  property. | {{< task "NAnt.Core.PropertyElement" >}} | False |
| {{< task "NAnt.Core.PropertyElement" "defines" >}}| Custom #defines for these files.  Defines get appended to the end of the<br>defines specified in the `${cc.defines}`  property.  Place each define on it&#39;s own line. | {{< task "NAnt.Core.PropertyElement" >}} | False |

## Full Syntax
```xml
<cc type="" outputdir="" outputname="" collectcompilationtime="" gendependencyonly="" failonerror="" verbose="" if="" unless="">
  <sources defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" if="" unless="">
    <includes />
    <excludes />
    <do />
    <group />
  </sources>
  <forceusing-assemblies defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" if="" unless="">
    <includes />
    <excludes />
    <do />
    <group />
  </forceusing-assemblies>
  <includedirs />
  <usingdirs />
  <options />
  <defines />
</cc>
```
