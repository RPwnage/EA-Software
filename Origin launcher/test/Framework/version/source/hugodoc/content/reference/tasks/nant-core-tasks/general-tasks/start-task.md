---
title: < start > Task
weight: 1124
---
## Syntax
```xml
<start filename="" workingdir="" kill="" clearenv="" useshell="" detached="" failonerror="" verbose="" if="" unless="">
  <args />
  <env fromoptionset="" />
</start>
```
## Summary ##
Launches an application or document.

## Remarks ##
The main purpose of this class is to open documents, web pages and launch GUI apps.
If you want to capture the output of a program use the.

### Example ###
Open a document:


```xml

<project>
    <start filename='file.txt' />
</project>

```


### Example ###
Open a web page:


```xml

<project>
    <start filename='iexplore.exe'>
        <args>
            <arg value='www' />
        </args>
    </start>
</project>

```



### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| filename | The program or document to run.  Can be a program, document or URL. | String | True |
| workingdir | The working directory to start the program from. | String | False |
| kill | If true the process will be killed right after being started.  Used by automated tests. | Boolean | False |
| clearenv | Set true to clear all environmental variable before executing, environmental variable specified in env OptionSet will still be added as environmental variable. | Boolean | False |
| useshell | Start process in a separate shell. Default is true.<br>NOTE. When useshell is set to true environment variables are not passed to the new process. | Boolean | False |
| detached |  | Boolean | False |
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | Boolean | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

### Nested Elements
| Name | Description | Type | Required |
| ---- | ----------- | ---- | -------- |
| {{< task "NAnt.Core.ArgumentSet" "args" >}}| The set of command line arguments. | {{< task "NAnt.Core.ArgumentSet" >}} | False |
| {{< task "NAnt.Core.OptionSet" "env" >}}| The set of environment variables for when the program runs.<br>Benefit of setting variables, like &quot;Path&quot;, here is that it will be<br>local to this program execution (i.e. global path is unaffected). | {{< task "NAnt.Core.OptionSet" >}} | False |

## Full Syntax
```xml
<start filename="" workingdir="" kill="" clearenv="" useshell="" detached="" failonerror="" verbose="" if="" unless="">
  <args if="" unless="">
    <arg />
  </args>
  <env fromoptionset="">
    <option />
  </env>
</start>
```
