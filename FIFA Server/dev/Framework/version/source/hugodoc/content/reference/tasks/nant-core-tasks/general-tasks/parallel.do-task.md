---
title: < parallel.do > Task
weight: 1114
---
## Syntax
```xml
<parallel.do />
```
## Summary ##
Executes nested tasks in parallel. Only immediate nested tasks are executed in parallel.**NOTE.**Make sure there are no race conditions, properties with same names, etc each group.

**NOTE.**local properties are local to each thread. Normal NAnt properties are shared between threads.



### Example ###
Load Nant scripts in parallel.


```xml

<project xmlns="schemas/ea/framework3.xsd">
  <parallel.do>
    <include file="file1.xml" />
    <include file="file2.xml" />
  </parallel.do>
</project>

```


### Example ###
Executes three groups of tasks in parallel.


```xml

<project xmlns="schemas/ea/framework3.xsd">
  <parallel.do>
     <do>
         <echo>Started test1.exe</echo>
         <exec program="test1.exe"/>
         <echo>Finished test1.exe</echo>
     </do>
     <do>
         <echo>Started test2.exe</echo>
         <exec program="test2.exe"/>
         <echo>Finished test2.exe</echo>
     </do>
     <do>
         <echo>Started test3.exe</echo>
         <exec program="test3.exe"/>
         <echo>Finished test32.exe</echo>
     </do>
  </parallel.do>
</project>

```



### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | Boolean | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

## Full Syntax
```xml
<parallel.do failonerror="" verbose="" if="" unless="" />
```
