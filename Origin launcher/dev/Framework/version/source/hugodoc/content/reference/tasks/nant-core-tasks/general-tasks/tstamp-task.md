---
title: < tstamp > Task
weight: 1133
---
## Syntax
```xml
<tstamp property="" pattern="" printtime="" failonerror="" verbose="" if="" unless="" />
```
## Summary ##
Sets properties with the current date and time.

## Remarks ##
By default  `tstamp`  displays the current date and time and sets the following properties:


 -  `tstamp.date`  to yyyyMMdd
 -  `tstamp.time`  to HHmm
 -  `tstamp.now`  using the default DateTime.ToString() method
To set an additional property with a custom date/time use the property and pattern attributes.  To set a number of additional properties all with the exact same date and time use the formatter nested element (see example).

The date and time string displayed by the tstamp task uses the computer&#39;s default long date and time string format.  You might consider setting these to the ISO 8601 standard for date and time notation.



### Example ###
Set the build.date property.


```xml

<project default="DoStamp">
    <target name="DoStamp">
        <tstamp property="build.date" pattern="yyyyMMdd" verbose="true"/>
    </target>
</project>

```


### Example ###
Set a number of properties for Ant like compatibility.


```xml

<project default="DoStamp">
    <target name="DoStamp">
        <tstamp verbose="true">
            <formatter property="TODAY" pattern="dd MMM yyyy"/>
            <formatter property="DSTAMP" pattern="yyyyMMdd"/>
            <formatter property="TSTAMP" pattern="HHmm"/>
        </tstamp>
    </target>
</project>

```



### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| property | The property to receive the date/time string in the given pattern. | String | False |
| pattern | The date/time pattern to be used. | String | False |
| printtime | Print time stamp into the output, default is false. | Boolean | False |
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | Boolean | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

## Full Syntax
```xml
<tstamp property="" pattern="" printtime="" failonerror="" verbose="" if="" unless="" />
```
