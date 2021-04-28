---
title: AndroidGradle
weight: 1003
---
## Syntax
```xml
<AndroidGradle gradle-directory="" gradle-project="" gradle-jvm-args="" native-activity="">
  <javasourcefiles append="" name="" optionset="" />
  <gradle-properties />
  <echo />
  <do />
</AndroidGradle>
```
### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| gradle-directory |  | String | False |
| gradle-project | Project name for this module in Gradle context If omitted the module name will be used. | String | False |
| gradle-jvm-args | Value for the org.gradle.jvmargs property in gradle.properties | String | False |
| native-activity | True by default. Includes the necessary files for using a native activity. Set to false if your program<br>implement its own activity class. | Boolean | False |

### Nested Elements
| Name | Description | Type | Required |
| ---- | ----------- | ---- | -------- |
| {{< task "EA.Eaconfig.Structured.StructuredFileSet" "javasourcefiles" >}}| Java files to display in Visual Studio for debugging purposes (not used for build) | {{< task "EA.Eaconfig.Structured.StructuredFileSet" >}} | False |
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "gradle-properties" >}}| Extra values that should be added to the gradle.properties file when building this project. | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | False |

## Full Syntax
```xml
<AndroidGradle gradle-directory="" gradle-project="" gradle-jvm-args="" native-activity="">
  <javasourcefiles append="" name="" optionset="" defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" if="" unless="">
    <includes />
    <excludes />
    <do />
    <group />
    <!--Structured Fileset-->
  </javasourcefiles>
  <gradle-properties if="" unless="" value="" append="">
    <do />
    <!--Text-->
  </gradle-properties>
  <echo />
  <do />
</AndroidGradle>
```
