---
title: < exec > Task
weight: 1103
---
## Syntax
```xml
<exec program="" message="" commandline="" responsefile="" commandlinetemplate="" output="" append="" stdout="" redirectout="" redirectin="" createinwindow="" silent="" silent-err="" workingdir="" clearenv="" timeout="" failonerror="" verbose="" if="" unless="">
  <env fromoptionset="" />
  <inputs defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" />
  <outputs defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" />
  <args />
</exec>
```
## Summary ##
Executes a system command.

## Remarks ##
Any options in the  `env` option set get added as environment variables when
the program is run.

You can use &lt;![CDATA[&lt;arg value=&quot;switch&quot;/&gt; or &lt;arg file=&quot;filename.txt&quot;/&gt;]]&gt;to
specify command line options in a more manageable way. The values in these elements get
appended to whatever value is in the commandline attribute if that attribute exists.
File values specified using the `file`  attribute get expanded to their full paths.

The  `inputs`  and  `outputs` file sets allow you to perform dependency checking.
The `program` will only get executed if one of the inputs is newer than all of the
outputs.

After each  task is completed the read-only property  `${exec.exitcode}` is set to the value that the process specified when it terminated. The default value is -1 and
is specified when the process fails to return a valid value.

Note: By default errors will not be generated if files are missing from the output file
set.  Normally filesets will throw a build exception if a specifically named file does not
exist.  This behaviour is controlled by the `failonmissing` file set attribute, which
defaults to `true` .  If set to  `false` , no build exception will be thrown when a
specifically named file is not found.

Note: Dependency checking requires at least one file in the  `inputs` fileset and one
file in the `outputs` fileset.  If either of these filesets is empty, no dependency
checking will be performed.



### Example ###
Run nant and display its help.


```xml

<project xmlns="schemas/ea/framework3.xsd">
    <exec program="nant" commandline="-help"/>
</project>

```


### Example ###
Use of the  `arg`  element and environment variables in the  `env`  option set.


```xml

<project xmlns="schemas/ea/framework3.xsd">
    <exec program="cmd.exe">
        <args>
            <arg value="/c echo"/>
            <arg value="Hello, %MyName%!"/>
        </args>
        <env>
            <option name="MyName" value="Paul"/>
            <option name="PATH" value="${sys.env.PATH};c:\phoenix;"/>
        </env>
    </exec>
</project>

```


### Example ###
This is an example of using the  `inputs`  and  `outputs` file sets to perform
dependency checking.  The program will be executed if the files in the inputs are newer
than the files in the outputs.  This example also shows how to redirect standard output to a file.


```xml

<project xmlns="schemas/ea/framework3.xsd">
    <touch file="in.0.txt"/>
    <touch file="in.1.txt"/>
    <exec program='cmd.exe' output="out.txt">
        <args>
            <arg value='/c echo ${exec.inputs.0} ${exec.inputs.1}'/>
        </args>
        <inputs failonmissing='false'>
            <includes name='in.*.txt'/>
        </inputs>
        <outputs>
            <includes name='out.txt' asis='true'/>
        </outputs>
    </exec>
    <fail message="out.txt file was not created" unless="@{FileExists('out.txt')}"/>
</project>

```



### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
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
<exec program="" message="" commandline="" responsefile="" commandlinetemplate="" output="" append="" stdout="" redirectout="" redirectin="" createinwindow="" silent="" silent-err="" workingdir="" clearenv="" timeout="" failonerror="" verbose="" if="" unless="">
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
</exec>
```
