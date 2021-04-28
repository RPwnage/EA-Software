---
title: < publicdata > Task
weight: 1065
---
## Syntax
```xml
<publicdata packagename="" processors="" libnames="" add-defaults="" configbindir="" failonerror="" verbose="" if="" unless="" />
```
## Summary ##
Container task to declare module public data in Initialize.xml file.  Refer to the Module
task for a full description of the NAnt script that may be placed inside the publicdata task.
You can also look at the documentation for Initialize.xml (Reference/Packages/Initialize.xml file)
for some examples of how to use the publicdata task.


### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| packagename | Name of the package. | String | True |
| processors | List of supported processors to be used in adding default include directories and libraries. | String | False |
| libnames | List of additional library names to be added to default set of libraries. | String | False |
| add-defaults | (Deprecated) If set to true default values for include directories and libraries will be added to all modules. Can be overwritten by &#39;add-defaults&#39; attribute in module. | Boolean | False |
| configbindir | If your package overrides the &quot;package.configbindir&quot; property this should be set if you want public data paths to be auto resolved. | String | False |
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | Boolean | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

## Full Syntax
```xml
<publicdata packagename="" processors="" libnames="" add-defaults="" configbindir="" failonerror="" verbose="" if="" unless="" />
```
