---
title: < fileset > Task
weight: 1105
---
## Syntax
```xml
<fileset name="" append="" basedir="" fromfileset="" sort="" failonmissing="" asis="" optionset="" force="" depends="" defaultexcludes="" failonerror="" verbose="" if="" unless="">
  <includes />
  <excludes />
  <do />
  <group />
</fileset>
```
## Summary ##
Filesets are groups of files defined by a set of patterns.

## Remarks ##
Filesets are groups of files defined by a set of patterns.
Filesets can be specified in 2 ways:  named and unnamed.

Named filesets are created by the `<fileset>` task, and so are defined
at the project level.  They are a mechanism allowing multiple tasks to reference and modify the same group of files.
This is conceptually the same as the creation of project level properties by the `<property>` task.

An unnamed fileset, also known as a fileset &quot;element&quot;, may exist for certain tasks
other than the `<fileset>` task
(e.g. &quot;inputs&quot; is a fileset element in the exec task).
But an unnamed fileset is accessible only to the task that owns it.  It can&#39;t
be accessed by an arbitrary task; a named fileset is needed for that.

Filesets are rooted at a base directory; by default, this is the directory containing the NAnt build file. Pattern matching may be used to exclude or include specific contents and children of this directory; by default, everything but CVS-specific directories and files are included in the fileset.

Attributes/parameters differ between named (task) and unnamed (element) filesets.

**Speed tip:**

Avoiding directory wildcards (\** or \*\) in filesets will speed up
NAnt.  This would entail explicit, known folder names in the fileset though.
Also more script is needed since directory wildcards aren&#39;t used.

##### Table showing which attributes/parameters are supported #####



```xml

Attribute          FileSet Task (named)            FileSet Element (unnamed)
----------------------------------------------------------------------------
name                     Yes                             No
append                   Yes                             No
failonerror              Yes                             No
verbose                  Yes                             No
if                       Yes                             No
unless                   Yes                             No
              
defaultexcludes          No                              Yes
failonmissing            No                              Yes
              
basedir                  Yes                             Yes
fromfileset              Yes                             Yes
sort                     Yes                             Yes


```
##### Syntax for named filesets #####



```xml

<fileset
  name=""
  append=""
  basedir=""
  fromfileset=""
  sort=""
  if=""
  unless=""
  failonerror=""
  verbose=""
  >
</fileset>

```
##### Parameters for named filesets #####


Attribute |Description |Must Provide |
--- |--- |--- |
| name | Name of the fileset | Yes | 
| append | If append is true, the patterns specified by this task are added to the patterns contained in the named file set.  If append is false, the named file set contains the patterns specified by this task. Default is &quot;false&quot;. | No | 
| basedir | The base of the directory tree defining the fileset.  Default is the directory containing the build file. | No | 
| fromfileset | Name of a fileset to include. | No | 
| sort | Indicates if the fileset contents should be sorted by file name or left in the order specified.  Default is &quot;false&quot;. | No | 
| if | Execute only if condition is true. | No | 
| unless | Execute only if condition is false. | No | 
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | No | 
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | No | 

##### Syntax for unnamed fileset element:  exec task example #####



```xml

<exec program="cmd.exe"

  <!-- inputs is a fileset element of exec task -->
  <inputs
    basedir=""
    fromfileset=""
    sort=""
    defaultexcludes=""
    failonmissing=""
    >
  /input>
</exec>

```
##### Parameters for unnamed fileset element #####


Attribute |Description |Must Provide |
--- |--- |--- |
| basedir | The base of the directory tree defining the fileset.  Default is the directory containing the build file. | No | 
| defaultexcludes | If &quot;false&quot;, default excludes are not used otherwise default excludes are used.  Default is &quot;true&quot;<br><br>The following patterns are excluded from filesets by default:<br> - **/CVS/*<br> - **/.cvsignore<br> - **/*~<br> - **/#*#<br> - **/.#*<br> - **/%*%<br> - **/SCCS<br> - **/SCCS/**<br> - **/vssver.scc<br><br><br> | No | 
| failonmissing | Indicates if a build error should be raised if an explictly included file does not exist.  Default is &quot;true&quot;. | No | 
| fromfileset | Name of a fileset to include. | No | 
| sort | Indicates if the fileset contents should be sorted by file name or left in the order specified.  Default is &quot;false&quot;. | No | 

##### Nested Elements for both named and unnamed filesets #####


###### <group> ######


Groups help you organize related files into groups that can be conditionally included or excluded using if/unless attributes.  Note that groups can be nested.  See below for an example.

Attribute |Description |Must Provide |
--- |--- |--- |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | No | 
| unless | Opposite of if. If false then the task will be executed; otherwise skipped.  Default is &quot;false&quot;. | No | 
| asis | If true then the file name will be added to the fileset without pattern matching or checking if the file exists. Default is &quot;false&quot;. | No | 
| force | If true the file name will be added to the fileset regardless if it is already included. Default is &quot;false&quot;. | No | 
| optionset | The name of an optionset to associate with this group of files. | No | 

###### <includes> ######


Defines a file pattern to be included in the fileset.

Attribute |Description |Must Provide |
--- |--- |--- |
| name | The filename or pattern used for file inclusion.  Default specifies no file. | No | 
| fromfileset | The name of a fileset defined by the `<fileset>` task.  This fileset will be used for file inclusion.  Default is empty. | No | 
| optionset | The name of an optionset to associate with this set of includes. | No | 
| fromfile | The name of a file containing a newline delimited set of files/patterns to include. | No | 
| asis | If true then the file name will be added to the fileset without pattern matching or checking if the file exists. Default is &quot;false&quot;. | No | 
| force | If true a the file name will be added to the fileset regardless if it is already included. Default is &quot;false&quot;. | No | 
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | No | 
| unless | Opposite of if. If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | No | 

###### <excludes> ######


Defines a file pattern of files to be excluded from the fileset.

Attribute |Description |Must Provide |
--- |--- |--- |
| name | The patterns used for file exclusion.  Default specifies no file. | No | 
| fromfileset | The name of a fileset defined by the &lt;fileset&gt; task.  This fileset will be used for file exclusion.  Default is empty. | No | 
| optionset | The name of an optionset to associate with this set of excludes. | No | 
| fromfile | The name of a file containing a newline delimited set of files/patterns to exclude. | No | 
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | No | 
| unless | Opposite of if. If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | No | 

**Note:** Multiple  `includes`  and  `excludes` elements can be used to specify the fileset.
The fileset will be constructed by including all the `includes` elements,
then excluding all the `excludes` elements without considering statement order.

**Note:**Preceding and trailing spaces will be trimmed off from the name attribute of
&lt;fileset&gt;.

##### Examples: named filesets #####


**Specifying a fileset with all the files in the base directory.**


```xml

<fileset>
    <includes name="*.*"/>
</fileset>

```
**Specifying a fileset using an absolute path:**


```xml

<fileset>
    <includes name="/packages/ToUpper/2.0.0/lib/ToUpper.lib" asis='true' />
</fileset>

```
**Specifying a fileset using multiple patterns:**


```xml

<fileset>
    <includes name="*.c"/>
    <includes name="*.cpp"/>
</fileset>

```
**Specifying a fileset with all the files in the current folder except one:**


```xml

<fileset>
    <includes name="*.*"/>
    <excludes name="error.log"/>
</fileset>

```
**Specifying a fileset with groups:**


```xml

<fileset>
    <group if="${platform} == win">
        <includes name="*.*"/>
    </group>
    <excludes name="error.log"/>
</fileset>

```
**Specifying a named fileset namedsourcedirs:**


```xml

<fileset name="sourcedirs">
    <group if="${platform} == pc-vc7">
        <includes name="sources/pc-vc7/**"/>
    </group>
    <excludes name="*~"/>
    <excludes name="*.bak"/>
</fileset>

```
**Specifying a fileset from a file:**


```xml

<project xmlns="schemas/ea/framework3.xsd">
  <fileset name='test'>
    <includes fromfile='files.txt' />
  </fileset>

  <fail unless='@{FileSetCount("test")} == 2'
    message='FileSet fromfile attribute failed.' />
</project>

```
files.txt for the above example:
```xml

01.cpp
02.cpp

```
**Creates a named fileset which can be referenced by other tasks.**


```xml

<project xmlns="schemas/ea/framework3.xsd">
    <!-- first create some files to include in the fileset -->
    <echo file="foo.txt">foo file</echo>
    <echo file="bar.txt">bar file</echo>
    <echo file="hack.txt">hack file</echo>

    <!-- now create the fileset to include the just created files -->
    <fileset name="sources">
        <includes name="*.txt"/>
        <excludes name="hack.txt"/>
    </fileset>
</project>

```
**Creates a named fileset using named groups to help organize the files a conditionally include groups of files.**


```xml

<project xmlns="schemas/ea/framework3.xsd">
    <!-- first create some files to include in the fileset -->
    <echo file="good.txt">A temp file</echo>
    <echo file="fast.txt">A temp file</echo>
    <echo file="hack.txt">A temp file</echo>
    
    <property name="platform" value="win"/>
    <property name="debug" value="true"/>

    <!-- now create the fileset to include the just created files -->
    <fileset name="sources">
        <!-- include these files in the 'debug' config. -->
        <group if="${platform} == win">
            <includes name="fast.txt"/>
            <!-- groups can be nested inside of each other -->
            <group if="${debug}">
                <includes name="hack.txt"/>
            </group>
        </group>
        <includes name="good.txt"/>
    </fileset>
    <fail message="fileset 'sources' should have had 3 files but instead had @{FileSetCount('sources')}." unless="@{FileSetCount('sources')} == 3"/>
</project>

```
#### Fileset Patterns ####


The patterns used for file inclusion and exclusion look like the patterns used in DOS and UNIX:

 `*`  matches zero or more characters,  `?` matches one character.

 `*.cs`  matches  `.cs` ,  `x.cs`  and  `FooBar.cs` , but not  `FooBar.xml`  (does not end with  `.cs` ).

 `?.cs`  matches  `x.cs` ,  `A.cs` , but not  `.cs`  or  `xyz.cs`  (neither have one character before  `.cs` ).

Combinations of `*` &#39;s and  `?` &#39;s are allowed.

Matching is done per-directory. This means that the first directory in the pattern is matched against the first directory in the path. Then the second directory in the pattern is matched against the second directory in the path, and so on. For example, when we have the pattern `/?abc/*/*.cs`  and the path  `/xabc/foobar/test.cs` , the first  `?abc`  is matched with  `xabc` , then  `*`  is matched with  `foobar` , and finally  `*.cs`  is matched with  `test.cs` . They all match, so the path matches the pattern.

To make things a bit more flexible, we add one extra feature, which makes it possible to match multiple directory levels. This can be used to match a complete directory tree, or a file anywhere in the directory tree. To do this, ** must be used as the name of a directory. When `**` is used as the name of a directory in the pattern, it matches zero or more directories.

##### Examples #####


Pattern |Match |
--- |--- |
| /test/** | Matches all files/directories under `/test/` , such as  `/test/x.cs` , or  `/test/foo/bar/xyz.html` , but not  `/xyz.xml` .<br><br> | 
| /test/ | Just like `test/**` .  Matches all files/directories under  `/test/` , such as  `/test/x.cs` , or  `/test/foo/bar/xyz.html` , but not  `/xyz.xml` .<br><br> | 
| **/CVS/* | Matches all files in CVS directories that can be located anywhere in the directory tree.  Matches:<br><br><br> -  `CVS/Repository` <br> -  `org/apache/CVS/Entries` <br> -  `org/apache/jakarta/tools/ant/CVS/Entries` <br>But not:<br><br><br> -  `org/apache/CVS/foo/bar/Entries` <br>because `foo/bar/` part does not match.<br><br> | 
| org/apache/jakarta/** | Matches all files in the `org/apache/jakarta` directory tree.  Matches:<br><br><br> -  `org/apache/jakarta/tools/ant/docs/index.html` <br> -  `org/apache/jakarta/test.xml` <br>But not:<br><br><br> -  `org/apache/xyz.java` <br>because the `jakarta/` part is missing.<br><br> | 
| org/apache/**/CVS/* | Matches all files in CVS directories that are located anywhere in the directory tree under org/apache.  Matches:<br><br><br> -  `org/apache/CVS/Entries` <br> -  `org/apache/jakarta/tools/ant/CVS/Entries` <br>But not:<br><br><br> -  `org/apache/CVS/foo/bar/Entries` <br> `foo/bar/` part does not match.<br><br> | 
| **/test/** | Matches all files that have a `test`  element in their path, including  `test` as a filename.<br><br> | 



### Example ###
Creates a named fileset which can be referenced by other tasks.


```xml

<project xmlns="schemas/ea/framework3.xsd">
    <!-- first create some files to include in the file set -->
    <echo file="foo.txt">foo file</echo>
    <echo file="bar.txt">bar file</echo>
    <echo file="hack.txt">hack file</echo>

    <!-- now create the file set to include the just created files -->
    <fileset name="sources">
        <includes name="*.txt"/>
        <excludes name="hack.txt"/>
    </fileset>
</project>

```


### Example ###
Creates a named fileset using named groups to help organize the files a conditionally include groups of files.


```xml

<project xmlns="schemas/ea/framework3.xsd">
    <!-- first create some files to include in the file set -->
    <echo file="good.txt">A temp file</echo>
    <echo file="fast.txt">A temp file</echo>
    <echo file="hack.txt">A temp file</echo>
    
    <property name="platform" value="win"/>
    <property name="debug" value="true"/>

    <!-- now create the file set to include the just created files -->
    <fileset name="sources">
        <!-- include these files in the 'debug' config. -->
        <group if="${platform} == win">
            <includes name="fast.txt"/>
            <!-- groups can be nested inside of each other -->
            <group if="${debug}">
                <includes name="hack.txt"/>
            </group>
        </group>
        <includes name="good.txt"/>
    </fileset>
    <fail message="File set 'sources' should have had 3 files but instead had @{FileSetCount('sources')}." unless="@{FileSetCount('sources')} == 3"/>
</project>

```



### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| name | Name for fileset. Preceding and trailing spaces will be trimmed off. | String | True |
| append | If append is true, the patterns specified by<br>this task are added to the patterns contained in the<br>named file set.  If append is false, the named file set contains<br>the patterns specified by this task.<br>Default is &quot;false&quot;. | Boolean | False |
| basedir | The base directory of the file set.  Default is the directory where the<br>build file is located. | String | False |
| fromfileset | The name of a file set to include. | String | False |
| sort | Sort the file set by filename. Default is false. | Boolean | False |
| failonmissing | Indicates if a build error should be raised if an explicitly included file does not exist.  Default is true. | Boolean | False |
| asis | If true then the file name will be added to the fileset without pattern matching or checking if the file exists. Default is &quot;false&quot;. | Boolean | False |
| optionset | The name of an optionset to associate with this set of excludes. | String | False |
| force | If true a the file name will be added to the fileset regardless if it is already included. Default is &quot;false&quot;. | Boolean | False |
| depends | **Invalid element in Framework**. Added to prevent existing scripts failing. | String | False |
| defaultexcludes | If &quot;false&quot;, default excludes are not used otherwise default excludes are used. Default is &quot;true&quot;<br> - **/CVS/*<br> - **/.cvsignore<br> - **/*~<br> - **/#*#<br> - **/.#*<br> - **/%*%<br> - **/SCCS<br> - **/SCCS/**<br> - **/vssver.scc<br> | String | False |
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | Boolean | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

## Full Syntax
```xml
<fileset name="" append="" basedir="" fromfileset="" sort="" failonmissing="" asis="" optionset="" force="" depends="" defaultexcludes="" failonerror="" verbose="" if="" unless="">
  <includes />
  <excludes />
  <do />
  <group />
</fileset>
```
