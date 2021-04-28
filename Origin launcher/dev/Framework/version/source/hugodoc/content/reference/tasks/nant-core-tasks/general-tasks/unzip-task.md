---
title: < unzip > Task
weight: 1139
---
## Syntax
```xml
<unzip zipfile="" outdir="" preserve_symlink="" failonerror="" verbose="" if="" unless="" />
```
## Summary ##
Unzip the contents of a zip file to a specified directory.

### Example ###
Zip and UnZip files to a directory.


```xml

<project xmlns="schemas/ea/framework3.xsd">
    <zip zipfile="backup.zip">
        <fileset basedir=".">
            <includes name="*.*"/>
            <excludes name="backup.zip"/>
        </fileset>
    </zip>
    
    <unzip zipfile="backup.zip" outdir="backup" />
</project>

```



### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| zipfile | The full path to the zip file. | String | True |
| outdir | The full path to the destination folder. | String | True |
| preserve_symlink | If zip file contains entry with symlink, preserve the symlink if possible (default true). | Boolean | False |
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | Boolean | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

## Full Syntax
```xml
<unzip zipfile="" outdir="" preserve_symlink="" failonerror="" verbose="" if="" unless="" />
```
