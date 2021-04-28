---
title: < do.once > Task
weight: 1097
---
## Syntax
```xml
<do.once key="" context="" blocking="" failonerror="" verbose="" if="" unless="" />
```
## Summary ##
Container task. Executes nested elements once for a given unique key and a context.

## Remarks ##
do.once task is thread safe.
This task acts within single NAnt process. It can not cross NAnt process boundary.

### Example ###
In the following example an expensive code generation step is preformed which the build script writer only wants to happen once.


```xml

<do.once key="package.TestPackage.generate-code" context="global">
  <do if="${config-system}==pc"/>
    <Generate-PC-Code/>
  </do>
</do.once>

```



### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| key | Unique key string. When using global context make sure that key string does<br>not collide with keys that may be defined in other packages.<br>Using &quot;package.[package name].&quot; prefix is a good way to ensure unique values. | String | True |
| context | Context for the key. Context can be either**global** or **project**. Default value is **global**.<br> - **global** context means that for each unique key task is executed once in NAnt process.<br> - **project**context means that for each unique key task is executed once for each project.<br>A project is typically created once per package per configuration, but more may be created if nested &lt;nant&gt;<br>tasks are used to create additional build graphs.<br> | DoOnceContexts | False |
| blocking | Defines behavior when several instances of do.once task with the same key are invoked simultaneously.<br> - **blocking=false** (default) - One instance will execute nested elements, other instances will return immediately without waiting.<br> - **blocking=true**- One instance will execute nested elements, other instances will wait for the first instance to complete and<br>then return without executing nested elements.<br> | Boolean | False |
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | Boolean | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

## Full Syntax
```xml
<do.once key="" context="" blocking="" failonerror="" verbose="" if="" unless="" />
```
