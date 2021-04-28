---
title: < style > Task
weight: 1125
---
## Syntax
```xml
<style basedir="" destdir="" extension="" style="" in="" out="" failonerror="" verbose="" if="" unless="" />
```
## Summary ##
Process a document with XSLT.
This is useful for building views of XML based documentation, or in generating code.

### Example ###
Create a report in HTML.


```xml

<project xmlns="schemas/ea/framework3.xsd">
    <target name="DoXslt">
        <style style="mytest.xsl" in="data.xml"/>
    </target>
</project>

```



### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| basedir | Where to find the source XML file, default is the project&#39;s basedir. | String | False |
| destdir | Directory in which to store the results. The default is the current directory. | String | False |
| extension | Desired file extension to be used for the targets. The default is &quot;html&quot;. | String | False |
| style | Name of the stylesheet to use - given either relative to the project&#39;s basedir or as an absolute path. | String | True |
| in | Specifies a single XML document to be styled. Should be used with the  `out`  attribute. | String | True |
| out | Specifies the output name for the styled result from the in attribute. The default is the name specified by the in attribute but with an .html extension. | String | False |
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | Boolean | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

## Full Syntax
```xml
<style basedir="" destdir="" extension="" style="" in="" out="" failonerror="" verbose="" if="" unless="" />
```
