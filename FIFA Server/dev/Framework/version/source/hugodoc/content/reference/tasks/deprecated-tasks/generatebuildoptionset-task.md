---
title: < GenerateBuildOptionset > Task
weight: 1226
---
## Syntax
```xml
<GenerateBuildOptionset configsetname="" controllingproperties="" failonerror="" verbose="" if="" unless="" />
```
## Summary ##
(Deprecated) Createsbuildtypeoptionset from a meta build optionset.
This task is deprecated please use the Structured XML &lt;BuildType&gt; task instead.

### Example ###
This is an example of how to generate a build type using GenerateBuildOptionset:


```xml

<optionset name="config-options-library-custom" fromoptionset="config-options-library">
  <option name="buildset.name" value="Library-Custom" />
  <option name="optimization" value="off"/>
</optionset>
<GenerateBuildOptionset configsetname="config-options-library-custom"/>

```
The same thing can be done much more simply now with Structured XML:


```xml

<BuildType name="Library-Custom" from="Library">
  <option name="optimization" value="off"/>
</BuildType>

```



### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| configsetname | The name of the meta optionset.<br>The name of the targetbuildtypeoptionset is provided by &#39;buildset.name&#39; option | String | False |
| controllingproperties | List of the controlling properties. Intended to be used in the configuration packages to override default list of controlling properties like<br>&#39;eaconfig.debugflags&quot;, eaconfig.optimization, etc. | String | False |
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | Boolean | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

## Full Syntax
```xml
<GenerateBuildOptionset configsetname="" controllingproperties="" failonerror="" verbose="" if="" unless="" />
```
