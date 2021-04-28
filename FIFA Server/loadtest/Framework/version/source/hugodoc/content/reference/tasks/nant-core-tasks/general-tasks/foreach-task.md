---
title: < foreach > Task
weight: 1106
---
## Syntax
```xml
<foreach property="" item="" in="" delim="" local="" failonerror="" verbose="" if="" unless="" />
```
## Summary ##
Iterates over a set of items.

## Remarks ##
Can iterate over files in directory, lines in a file, etc.

The `property`  value is set for each item in turn as the  `foreach` task
iterates over the given set of items. Any previously existing value in `property` is stored before the block of tasks specified in the `foreach` task are invoked,
and restored when the block of tasks completes.

This storage ensures that the  `property` will have the same value it had
before the `foreach`  task was invoked once the  `foreach` task completes.

Valid foreach items are &quot;File&quot;, &quot;Folder&quot;, &quot;Directory&quot;, &quot;String&quot;, &quot;Line&quot;, &quot;FileSet&quot;,
and &quot;OptionSet&quot;.

File - return each file name in an iterated directory

Folder - return each folder in an iterated directory

Directory - return each directory in an iterated directory

String - return each splitted string from a long string with user specified delimiter in delim.

Line - return each line in a text file.

FileSet - return each file name in a FileSet.

OptionSet - return each option in an OptionSet.



NOTE: When iterating over strings and lines extra leading and trailing whitespace
characters will be trimmed and blank lines will be ignored.

NOTE: When iterating over option sets the property name specified is used for a
property prefix to the actual option name and values.  The name of option is available
in the `<property>.name`  property, the value in the  `<property>.value` property.

NOTE: When iterating over FileSet the BaseDirectory associated with the FileItem (see &#39;fileset&#39; `<include>.name` help) is available
in the `<property>.basedir` property.
property.



### Example ###
Loops over the files in C:\


```xml

<project default="doloop">
    <target name="doloop">
        <foreach item="File" in="c:\" property="filename">
            <echo message="${filename}"/>
        </foreach>
    </target>
</project>

```


### Example ###
Loop over all *.txt files in C:\


```xml

<project default="doloop">
    <target name="doloop">
        <foreach item="File" in="c:\*.txt" property="filename">
            <echo message="${filename}"/>
        </foreach>
    </target>
</project>

```


### Example ###
Iterates over the directories in the current directory.


```xml

<project default="doloop">
    <target name="doloop">
        <foreach item="Directory" in="." property="path">
            <echo message="path='${path}' name='@{PathGetFileName('${path}')}"/>
        </foreach>
    </target>
</project>

```


### Example ###
Iterate over all pc-* directories in the current directory.


```xml

<project default="doloop">
    <target name="doloop">
        <foreach item="Directory" in="pc-*" property="path">
            <echo message="${path}"/>
        </foreach>
    </target>
</project>

```


### Example ###
Loops over a list


```xml

<project default="doloop">
    <target name="doloop">
        <foreach item="String" in="1 2 3" delim=" " property="count">
            <echo message="${count}"/>
        </foreach>
    </target>
</project>

```


### Example ###
Loops over a fileset


```xml

<project default="doloop">
    <fileset name="myfileset">
        <includes name="*" />
    </fileset>
    <target name="doloop">
        <foreach item="FileSet" in="myfileset" property="filename">
            <echo message="${filename}"/>
        </foreach>
    </target>
</project>

```


### Example ###
Loops over an optionset


```xml

<project default="doloop">
    <optionset name="SharedLibrary">
        <option name="cc.options" value="-c -nologo"/>
        <option name="cc.program=" value="cl.exe"/>
    </optionset>
    <target name="doloop">
        <foreach item="OptionSet" in="SharedLibrary" property="option">
            <echo message="${option.name} = ${option.value}"/>
        </foreach>
    </target>
</project>

```



### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| property | The property name that holds the current iterated item value. | String | True |
| item | The type of iteration that should be done. Valid values are:<br>File, Directory, String, Line, FileSet, and OptionSet.<br>When choosing the Line option, you specify a file and the task<br>iterates over all the lines in the text file. | ItemTypes | True |
| in | The source of the iteration. | String | True |
| delim | The delimiter string array. Default is whitespace.<br>Multiple characters are allowed. | String | False |
| local | Indicates if the property that holds the iterated value is going to be defined in a local context and thus, it will be restricted to a local scope .<br>Default is false. | Boolean | False |
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | Boolean | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

## Full Syntax
```xml
<foreach property="" item="" in="" delim="" local="" failonerror="" verbose="" if="" unless="" />
```
