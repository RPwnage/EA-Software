---
title: < as > Task
weight: 1160
---
## Syntax
```xml
<as type="" outputdir="" outputname="" collectcompilationtime="" gendependencyonly="">
  <asmsources defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" />
  <asmincludedirs />
  <asmoptions />
  <asmdefines />
</as>
```
## Summary ##
A generic assembler compiler task.

## Remarks ##
The  `as`  task requires the following property to be set:

Property |Description |
--- |--- |
| ${as} | The absolute pathname of the compiler executable. | 
| ${as.template.includedir} | The syntax template to transform  `${as.includes}`  into compiler flags. | 

The task will make use of the following properties:

Property |Description |
--- |--- |
| ${as.defines} | The defines for the compilation. | 
| ${as.includedirs} | The include directories for the compilation. | 
| ${as.options} | The options flags for the compilation.  All the files from the  `source`  file set will be compiled with the same options.  If you want different compile options, you must invoke the  `as`  task with those options. | 
| ${as.template.commandline} | The template to use when creating the command line.  Default is %defines% %includedirs% %options%. | 
| ${as.template.define} | The syntax template to transform  `${as.defines}`  into compiler flags. | 
| ${as.template.sourcefile} | The syntax template to transform a  `source file`  into compiler source file. | 
| ${as.template.responsefile} | The syntax template to transform the responsefile into a response file flag. Default is @&quot;%responsefile%&quot;. | 
| ${as.nodep} | If  `true`  dependency files will not be created. | 
| ${as.nocompile} | If  `true`  object files not be created. | 
| ${as.useresponsefile} | If  `true`  a response file, containing the entire commandline, will be passed as the only argument to the compiler. Default is false. | 
| ${as.userelativepaths} | If  `True`  the working directory of the assembler will be set to the base directory of the  `asmsources`  fileset. All source and output files will then be made relative to this path. Default is  `false` . | 
| ${as.threadcount} | The number of threads to use when compiling. Default is 1 per cpu. | 
| ${as.objfile.extension} | The file extension for object files.  Default is &quot;.obj&quot;. | 
| ${as.forcelowercasefilepaths} | If  `true` file paths (folder and file names) will be<br>forced to lower case (useful in avoiding redundant NAnt builds<br>between VS.net and command line environments in the case of capitalized<br>folder names when file name case is unimportant).  Default is false. | 
| ${as.template.responsefile.commandline} | The template to use when creating the command line for response file. Default value is ${as.template.responsefile.commandline} | 
| ${as.responsefile.separator} | Separator used in response files. Default is &quot; &quot;. | 
| ${as.template.responsefile.objectfile} | The syntax template to transform the objects file set into compiler flags in response file. Default ${as.template.objectfile} | 

The task declares the following template items in order to help defining the above
properties:

Template item |Description |
--- |--- |
| %define% | Used by the ${as.template.define} property to represent the current value of the ${as.defines} property during template processing. | 
| %includedir% | Used by the ${as.template.includedir} property to represent the individual value of the ${as.includedirs} property. | 
| %responsefile% | Used by the ${cc..template.responsefile} property to represent the filename of the response file. | 
| %objectfile% | Used by the ${as.options} property to represent the object file name.  Object file names are formed by appending  `".obj"`  to the source file name. | 
| %outputdir% | Used by the ${as.options} property to represent the actual values of the  `%outputdir%`  attribute. | 
| %outputname% | Used by the ${as.options} property to represent the actual value of the  `%outputname%`  attribute. | 
| %sourcefile% | Used by the ${as.options} property to represent the individual value of the sources file set. | 

This task can use a named option set for specifying options for unique components.  For more information see the Fundamentals-&gt;Option Sets topic in help file.




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
| {{< task "NAnt.Core.FileSet" "asmsources" >}}| The set of files to compile. | {{< task "NAnt.Core.FileSet" >}} | False |
| {{< task "NAnt.Core.PropertyElement" "asmincludedirs" >}}| Custom include directories for this set files.  Use complete paths and place<br>each directory on it&#39;s own line. | {{< task "NAnt.Core.PropertyElement" >}} | False |
| {{< task "NAnt.Core.PropertyElement" "asmoptions" >}}| Custom program options for these files.  Options get appended to the end of<br>the options specified by the `${cc.options}`  property. | {{< task "NAnt.Core.PropertyElement" >}} | False |
| {{< task "NAnt.Core.PropertyElement" "asmdefines" >}}| Custom #defines for these files.  Defines get appended to the end of the<br>defines specified in the `${cc.defines}`  property.  Place each define on it&#39;s own line. | {{< task "NAnt.Core.PropertyElement" >}} | False |

## Full Syntax
```xml
<as type="" outputdir="" outputname="" collectcompilationtime="" gendependencyonly="" failonerror="" verbose="" if="" unless="">
  <asmsources defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" if="" unless="">
    <includes />
    <excludes />
    <do />
    <group />
  </asmsources>
  <asmincludedirs />
  <asmoptions />
  <asmdefines />
</as>
```
