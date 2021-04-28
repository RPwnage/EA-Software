---
title: < lib > Task
weight: 1163
---
## Syntax
```xml
<lib type="" outputdir="" outputname="" libextension="" libprefix="" failonerror="" verbose="" if="" unless="">
  <objects defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" />
  <options />
</lib>
```
## Summary ##
A generic C/C++ library manager tool.

## Remarks ##
The task requires the following properties:

Property |Description |
--- |--- |
| ${lib} | The absolute pathname of the librarian executable to be used with this invocation of the task. | 
| ${lib.userresponsefile} | If  `true`  a response file, containing the entire commandline, will be passed as the only argument to the compiler. Default is false. | 

The task declares the following properties:

Property |Description |
--- |--- |
| ${lib.options} | The options flags for the librarian. | 
| ${lib.useresponsefile} | If  `true`  a response file, containing the entire commandline, will be passed as the only argument to the archiver. Default is false. | 
| ${lib.userelativepaths} | If  `true`  the working directory of the archiver will be set to the  `outputdir` . All object files will then be made relative to this path. Default is  `false` . | 
| ${lib.template.objectfile} | The syntax template to transform the objects file set into librarian flags. | 
| ${lib.template.responsefile} | The syntax template to transform the responsefile into a response file flag. Default is @&quot;%responsefile%&quot;. | 
| ${lib.template.commandline} | The template to use when creating the command line.  Default is %options% %objectfiles%. | 
| ${lib.template.responsefile.commandline} | The template to use when creating the command line for response file. Default value is ${lib.template.responsefile.commandline} | 
| ${lib.responsefile.separator} | Separator used in response files. Default is &quot; &quot;. | 
| ${lib.template.responsefile.objectfile} | The syntax template to transform the objects file set into librarian flags in response file. Default ${lib.template.objectfile} | 
| %responsefile% | Used by the ${cc.template.responsefile} property to represent the filename of the response file. | 
| ${lib.forcelowercasefilepaths} | If  `true` file paths (folder and file names) will be<br>forced to lower case (useful in avoiding redundant NAnt builds<br>between VS.net and command line environments in the case of capitalized<br>folder names when file name case is unimportant).  Default is false. | 

The task declares the following template items in order to help user defining the above properties:

Template item |Description |
--- |--- |
| %outputdir% | Used in the  `${lib.options}`  property to represent the actual values of the  `%outputdir%`  attribute. | 
| %outputname% | Used in the  `${lib.options}`  property to represent the actual value of the  `%outputname%`  attribute. | 
| %objectfile% | Used in the  `${lib.template.objectfile}`  property to represent the individual value of the  `objects`  file set. | 

This task can use a named option set for specifying options for unique components.  For more information see the Fundamentals-&gt;Option Sets topic in help file.



### Example ###
Archive the given object files into the specified library.


```xml

<project>
    <dependent name="VisualStudio" version="7.0.0"/>
            
    <lib outputdir="lib" outputname="mylib">
        <objects>
            <includes name="obj/*.obj"/>
        </objects>
    </lib>
</project>
    
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
| type | Name of option set to use for compiling options.  If null<br>given values from the global `lib.*`  properties.  Default is null. | String | False |
| outputdir | Directory where all output files are placed.  Default is the base directory of the build file. | String | False |
| outputname | The base name of the library and any associated output files.  Default is an empty string. | String | False |
| libextension | library output extension | String | False |
| libprefix | library output prefix | String | False |
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | Boolean | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

### Nested Elements
| Name | Description | Type | Required |
| ---- | ----------- | ---- | -------- |
| {{< task "NAnt.Core.FileSet" "objects" >}}| The list of object files to combine into a library. | {{< task "NAnt.Core.FileSet" >}} | False |
| {{< task "NAnt.Core.PropertyElement" "options" >}}| Custom program options for this task.  These get appended to the options specified in the `lib.options` property. | {{< task "NAnt.Core.PropertyElement" >}} | False |

## Full Syntax
```xml
<lib type="" outputdir="" outputname="" libextension="" libprefix="" failonerror="" verbose="" if="" unless="">
  <objects defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" if="" unless="">
    <includes />
    <excludes />
    <do />
    <group />
  </objects>
  <options />
</lib>
```
