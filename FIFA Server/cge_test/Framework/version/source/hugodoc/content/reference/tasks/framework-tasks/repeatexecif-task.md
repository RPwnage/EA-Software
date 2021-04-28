---
title: < repeatexecif > Task
weight: 1181
---
## Syntax
```xml
<repeatexecif repeatpattern="" maxrepeatcount="" maxlinestoscan="" program="" message="" commandline="" responsefile="" commandlinetemplate="" output="" append="" stdout="" redirectout="" redirectin="" createinwindow="" silent="" silent-err="" workingdir="" clearenv="" timeout="">
  <env fromoptionset="" />
  <inputs defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" />
  <outputs defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" />
  <args />
</repeatexecif>
```
## Summary ##
Executes a system command. Repeats execution if condition is met


### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| repeatpattern | List of patterns. | String | False |
| maxrepeatcount | List of patterns. | Int32 | False |
| maxlinestoscan |  | Int32 | False |
| program | The program to execute. Specify the fully qualified name unless the program is in the path. | String | True |
| message | The message to display to the log.  If present then no other output will be displayed unless there is an error or the verbose attribute is true. | String | False |
| commandline | The command line arguments for the program.  Consider using  `arg`  elements (see task description) for improved readability. | String | False |
| responsefile | Use the contents of this file as input to the program. | String | False |
| commandlinetemplate | The template used to form the command line. Default is &#39;%commandline% %responsefile%&#39;. | String | False |
| output | The file to which the standard output will be redirected. By default, the standard output is redirected to the console. | String | False |
| append | true if the output file is to be appended to. Default = &quot;false&quot;. | Boolean | False |
| stdout | Write to standard output. Default is true. | Boolean | False |
| redirectout | Set true to redirect stdout and stderr. When set to false, this will disable options such as stdout, outputfile etc.  Default is true. | Boolean | False |
| redirectin | Set true to redirect stdin. Default is true. | Boolean | False |
| createinwindow | Set true to create process in a new window.. Default is false. | Boolean | False |
| silent | If true standard output from the external program (as well as the execution command line) will not be written to log. Does not affect standard error. Default is false. | Boolean | False |
| silent-err | If true standard error from the external program (as well as the execution command line) will not be written to log. Does not affect standard output. Default is false. | Boolean | False |
| workingdir | The directory in which the command will be executed. The workingdir will be evaluated relative to the build file&#39;s directory. | String | False |
| clearenv | Set true to clear all environmental variable before executing, environmental variable specified in env OptionSet will still be added as environmental variable. | Boolean | False |
| timeout | Stop the build if the command does not finish within the specified time.  Specified in milliseconds.  Default is no time out. | Int32 | False |
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | Boolean | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

### Nested Elements
| Name | Description | Type | Required |
| ---- | ----------- | ---- | -------- |
| {{< task "NAnt.Core.OptionSet" "env" >}}| The set of environment variables for when the program runs.<br>Benefit of setting variables, like &quot;Path&quot;, here is that it will be<br>local to this program execution (i.e. global path is unaffected). | {{< task "NAnt.Core.OptionSet" >}} | False |
| {{< task "NAnt.Core.FileSet" "inputs" >}}| Program inputs. | {{< task "NAnt.Core.FileSet" >}} | False |
| {{< task "NAnt.Core.FileSet" "outputs" >}}| Program outputs. | {{< task "NAnt.Core.FileSet" >}} | False |
| {{< task "NAnt.Core.ArgumentSet" "args" >}}| A set of command line arguments. | {{< task "NAnt.Core.ArgumentSet" >}} | False |

## Full Syntax
```xml
<repeatexecif repeatpattern="" maxrepeatcount="" maxlinestoscan="" program="" message="" commandline="" responsefile="" commandlinetemplate="" output="" append="" stdout="" redirectout="" redirectin="" createinwindow="" silent="" silent-err="" workingdir="" clearenv="" timeout="" failonerror="" verbose="" if="" unless="">
  <env fromoptionset="">
    <option />
  </env>
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
  <args if="" unless="">
    <arg />
  </args>
</repeatexecif>
```
