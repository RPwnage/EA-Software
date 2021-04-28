---
title: < sleep > Task
weight: 1123
---
## Syntax
```xml
<sleep hours="" minutes="" seconds="" milliseconds="" failonerror="" verbose="" if="" unless="" />
```
## Summary ##
A task for sleeping a specified period of time, useful when a build or deployment process
requires an interval between tasks. If none of the time attributes are specified then the task sleeps for 0 milliseconds.

### Example ###
Sleep 123 milliseconds.


```xml

<project default="SleepForAWhile">
    <target name="SleepForAWhile">
        <sleep milliseconds="123" />
    </target>
</project>

```


### Example ###
Sleep 1 hour, 2 minutes, 3 seconds and 4 milliseconds.


```xml

<project xmlns="schemas/ea/framework3.xsd">
    <target name="SleepForAWhile">
        <sleep hours="1" minutes="2" seconds="3" milliseconds="4" />
    </target>
</project>

```



### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| hours | Hours to add to the sleep time. | Int32 | False |
| minutes | Minutes to add to the sleep time. | Int32 | False |
| seconds | Seconds to add to the sleep time. | Int32 | False |
| milliseconds | Milliseconds to add to the sleep time. | Int32 | False |
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | Boolean | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

## Full Syntax
```xml
<sleep hours="" minutes="" seconds="" milliseconds="" failonerror="" verbose="" if="" unless="" />
```
