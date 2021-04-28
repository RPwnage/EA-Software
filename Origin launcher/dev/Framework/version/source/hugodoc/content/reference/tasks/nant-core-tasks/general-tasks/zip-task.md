---
title: < zip > Task
weight: 1140
---
## Syntax
```xml
<zip zipfile="" ziplevel="" zipentrydir="" usemodtime="" failonerror="" verbose="" if="" unless="">
  <fileset defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" />
</zip>
```
## Summary ##
Creates a zip file from a specified fileset.

## Remarks ##
Uses NZipLib, an open source Zip/GZip library written entirely in C#.

Full zip functionality is not available; all you can do
is to create a zip file from a fileset.



### Example ###
Zip all files in the subdirectory  `build`  to  `backup.zip` .


```xml

<project default="DoZip">
    <target name="DoZip">
        <zip zipfile="backup.zip">
            <fileset basedir=".">
                <includes name="*.*"/>
                <excludes name="backup.zip"/>
            </fileset>
        </zip>
    </target>
</project>

```



### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| zipfile | The zip file to create. Use a qualified name to create the zip file in a<br>location which is different than the current working directory. | String | True |
| ziplevel | Desired level of compression Default is 6. | Int32 | False |
| zipentrydir | Prepends directory to each zip file entry. | String | False |
| usemodtime | Preserve last modified timestamp of each file. | Boolean | False |
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | Boolean | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

### Nested Elements
| Name | Description | Type | Required |
| ---- | ----------- | ---- | -------- |
| {{< task "NAnt.Core.FileSet" "fileset" >}}| The set of files to be included in the archive. | {{< task "NAnt.Core.FileSet" >}} | False |

## Full Syntax
```xml
<zip zipfile="" ziplevel="" zipentrydir="" usemodtime="" failonerror="" verbose="" if="" unless="">
  <fileset defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" if="" unless="">
    <includes />
    <excludes />
    <do />
    <group />
  </fileset>
</zip>
```
