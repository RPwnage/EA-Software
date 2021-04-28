---
title: < link > Task
weight: 1162
---
## Syntax
```xml
<link type="" outputdir="" outputname="" linkoutputextension="" failonerror="" verbose="" if="" unless="">
  <objects defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" />
  <libraries defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" />
  <librarydirs />
  <options />
</link>
```
## Summary ##
A generic C/C++ linker task.

## Remarks ##
The task requires the following properties:

Property |Description |
--- |--- |
| ${link} | The absolute pathname of the linker executable. | 

The task will use the following properties:

Property |Description |
--- |--- |
| ${link.options} | The option flags for the linker. | 
| ${link.libraries} | The system libraries to link with. | 
| ${link.librarydirs} | The library search paths for the linker. | 
| ${link.threadcount} | The number of parallel linker processes. Default is 1 per cpu. | 
| ${link.template.commandline} | The template to use when creating the command line.  Default is %options% %librarydirs% %libraryfiles% %objectfiles%. | 
| ${link.template.librarydir} | The syntax template to transform the ${link.librarydirs} property into linker flags. | 
| ${link.template.libraryfile} | The syntax template to transform the libraries file set into linker flags. | 
| ${link.template.objectfile} | The syntax template to transform the objects file set into linker flags. | 
| ${link.postlink.program} | The path to program to run after linking.  If not defined no post link process will be run. | 
| ${link.postlink.workingdir} | The directory the post link program will run in.  Defaults to the program&#39;s directory. | 
| ${link.postlink.commandline} | The command line to use when running the post link program. | 
| ${link.postlink.redirect} | Whether to redirect output of postlink program. | 
| ${link.useresponsefile} | If  `true`  a response file, containing the entire commandline, will be passed as the only argument to the linker. Default is false. | 
| ${link.userelativepaths} | If  `true`  the working directory of the linker will be set to the <br>```xml<br>outputdir<br>```<br>. All object files will then be made relative to this path. Default is  `false` . | 
| ${link.template.responsefile} | The syntax template to transform the responsefile into a response file flag. Default is @&quot;%responsefile%&quot;. | 
| ${link.template.responsefile.commandline} | The template to use when creating the command line for response file. Default value is ${link.template.responsefile.commandline} | 
| ${link.responsefile.separator} | Separator used in response files. Default is &quot; &quot;. | 
| ${link.template.responsefile.objectfile} | The syntax template to transform the objects file set into linker flags in response file. Default ${link.template.objectfile} | 
| ${link.template.responsefile.libraryfile} | The syntax template to transform the libraries file set into linker flags in response file. Default ${link.template.libraryfile} | 
| ${link.template.responsefile.librarydir} | The syntax template to transform the ${link.librarydirs} property into linker flags in response file. Default ${link.template.librarydir} | 
| ${link.forcelowercasefilepaths} | If  `true` file paths (folder and file names) will be<br>forced to lower case (useful in avoiding redundant NAnt builds<br>between VS.net and command line environments in the case of capitalized<br>folder names when file name case is unimportant).  Default is false. | 

The task declares the following template items in order to help user defining the above properties:

Template item |Description |
--- |--- |
| %outputdir% | Used by the ${link.options} property to represent the actual values of the outputdir attribute. | 
| %outputname% | Used by the ${link.options} property to represent the actual value of the outputname attribute. | 
| %librarydir% | Used by the ${link.template.librarydir} property to represent the individual value of the ${link.librarydirs} property. | 
| %libraryfile% | Used by the ${link.template.libraryfile} property to represent the individual value of the libraries file set. | 
| %objectfile% | Used by the ${link.template.objectfile} property to represent the individual value of the objects file set. | 
| %responsefile% | Used by the ${cc..template.responsefile} property to represent the filename of the response file. | 

The post link program allows you to run another program after linking.  For example if you are
building programs for the XBox you will know that you need to run the imagebld program in order to
use the program on the xbox.  Instead of having to run a second task after each build task you can
setup the properties to run imagebld each time you link.  Use the `%outputdir%`  and  `%outputname` template items in the `link.postlink.commandline` to generalize the task for all the components
in your package.

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


### Example ###
An example of to use the  `${link.template.commandline}`  property to customize the linker command line.


```xml

<project default="build">
    <package name="Hello" />
    <dependent name="VisualStudio" version="7.1.1-4"/>
    
    <property name="link.options">
        -subsystem:CONSOLE
    </property>

    <!--
        Notice how the -out: option has been removed from the link.options property and
        is now directly part of the link.commandline.  This is a made up example but with
        it shows you how you can wrap, say the libraries with options needed by gcc.
    -->
    <property name="link.template.commandline">
        %options%
        -out:"%outputdir%/%outputname%.exe"
        %librarydirs%
        %libraryfiles%
        %objectfiles%
    </property> '/>

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



### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| type | Name of option set to use for compiling options.  If null<br>given values from the global `link.*`  properties.  Default is null. | String | False |
| outputdir | Directory where all output files are placed.  Default is the base directory of the build file. | String | False |
| outputname | The base name of the library or program and any associated output files.  Default is an empty string. | String | True |
| linkoutputextension | library output extension | String | False |
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | Boolean | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

### Nested Elements
| Name | Description | Type | Required |
| ---- | ----------- | ---- | -------- |
| {{< task "NAnt.Core.FileSet" "objects" >}}| The set of object files to link. | {{< task "NAnt.Core.FileSet" >}} | False |
| {{< task "NAnt.Core.FileSet" "libraries" >}}| The list of libraries to link with. | {{< task "NAnt.Core.FileSet" >}} | False |
| {{< task "NAnt.Core.PropertyElement" "librarydirs" >}}| Directories to search for additional libraries.  New line  `"\n"`  or semicolon  `";"`  delimited. | {{< task "NAnt.Core.PropertyElement" >}} | False |
| {{< task "NAnt.Core.PropertyElement" "options" >}}| Custom program options for these files. | {{< task "NAnt.Core.PropertyElement" >}} | False |

## Full Syntax
```xml
<link type="" outputdir="" outputname="" linkoutputextension="" failonerror="" verbose="" if="" unless="">
  <objects defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" if="" unless="">
    <includes />
    <excludes />
    <do />
    <group />
  </objects>
  <libraries defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" if="" unless="">
    <includes />
    <excludes />
    <do />
    <group />
  </libraries>
  <librarydirs />
  <options />
</link>
```
