---
title: < get > Task
weight: 1106
---
## Syntax
```xml
<get src="" dest="" httpproxy="" ignoreerrors="" usetimestamp="" failonerror="" verbose="" if="" unless="">
  <fileset defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" />
</get>
```
## Summary ##
Get a particular file from a URI source.

## Remarks ##
Options include verbose reporting, timestamp based fetches and controlling actions on failures.

Currently, only HTTP and UNC (Windows shared directory names of the form  `//<server>/<directory>` ) protocols are supported. FTP support may be added when more pluggable protocols are added to the System.Net assembly.

The  `useTimeStamp`  option enables you to control downloads so that the remote file is only fetched if newer than the local copy. If there is no local copy, the download always takes place. When a file is downloaded, the timestamp of the downloaded file is set to the remote timestamp.

This timestamp facility only works on downloads using the HTTP protocol.

### Example ###
Gets the index page of the NAnt home page, and stores it in the file help/index.html.


```xml

<project xmlns="schemas/ea/framework3.xsd">
    <target name="fetch" description="Downloads the NAnt home page">
        <get src="http://nant.sourceforge.org/" dest="index.html"/>
    </target>
</project>

```



### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| src | The URL from which to retrieve a file. | String | True |
| dest | The file where to store the retrieved file. | String | True |
| httpproxy | If inside a firewall, proxy server/port information<br>Format: {proxy server name}:{port number}<br>Example: proxy.mycompany.com:8080 | String | False |
| ignoreerrors | Log errors but don&#39;t treat as fatal. (&quot;true&quot;/&quot;false&quot;). Default is &quot;false&quot;. | Boolean | False |
| usetimestamp | Conditionally download a file based on the timestamp of the local copy. HTTP only. (&quot;true&quot;/&quot;false&quot;). Default is &quot;false&quot;. | Boolean | False |
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | Boolean | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

### Nested Elements
| Name | Description | Type | Required |
| ---- | ----------- | ---- | -------- |
| {{< task "NAnt.Core.FileSet" "fileset" >}}| FileSets are used to select files to get. | {{< task "NAnt.Core.FileSet" >}} | False |

## Full Syntax
```xml
<get src="" dest="" httpproxy="" ignoreerrors="" usetimestamp="" failonerror="" verbose="" if="" unless="">
  <fileset defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" if="" unless="">
    <includes />
    <excludes />
    <do />
    <group />
  </fileset>
</get>
```
