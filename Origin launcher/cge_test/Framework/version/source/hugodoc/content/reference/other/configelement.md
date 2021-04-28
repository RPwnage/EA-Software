---
title: ConfigElement
weight: 1014
---
## Syntax
```xml
<ConfigElement>
  <buildoptions />
  <defines />
  <targetframeworkversion />
  <targetframeworkfamily />
  <warningsuppression />
  <preprocess />
  <postprocess />
  <pch enable="" pchfile="" pchheader="" pchheaderdir="" use-forced-include="" />
  <remove />
</ConfigElement>
```
## Summary ##
Sets various attributes for a config


### Nested Elements
| Name | Description | Type | Required |
| ---- | ----------- | ---- | -------- |
| {{< task "EA.Eaconfig.Structured.BuildTypeElement" "buildoptions" >}}| Gets the build options for this config. | {{< task "EA.Eaconfig.Structured.BuildTypeElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "defines" >}}| Gets the macros defined for this config | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | False |
| {{< task "NAnt.Core.Structured.DeprecatedPropertyElement" "targetframeworkversion" >}}| Set target framework version for managed modules (4.0, 4.5, ...) | {{< task "NAnt.Core.Structured.DeprecatedPropertyElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "targetframeworkfamily" >}}| Set target framework family for managed modules (framework, standard, core) | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "warningsuppression" >}}| Gets the warning suppression property for this config | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "preprocess" >}}| Preprocess step can be C# task derived from AbstractModuleProcessorTask class or a target. Multiple preprocess steps are supported | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "postprocess" >}}| Preprocess step can be C# task derived from AbstractModuleProcessorTask class or a target. Multiple preprocess steps are supported | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.PrecompiledHeadersElement" "pch" >}}| Set up precompiled headers.  NOTE: To use this option, you still need to list a source file and specify that file with &#39;create-precompiled-header&#39; optionset. | {{< task "EA.Eaconfig.Structured.PrecompiledHeadersElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.RemoveBuildOptions" "remove" >}}| Define options to remove from the final optionset | {{< task "EA.Eaconfig.Structured.RemoveBuildOptions" >}} | False |

## Full Syntax
```xml
<ConfigElement>
  <buildoptions if="" unless="" fromoptionset="">
    <option />
  </buildoptions>
  <defines if="" unless="" value="" append="">
    <do />
    <!--Text-->
  </defines>
  <targetframeworkversion />
  <targetframeworkfamily if="" unless="" value="" append="">
    <do />
    <!--Text-->
  </targetframeworkfamily>
  <warningsuppression if="" unless="" value="" append="">
    <do />
    <!--Text-->
  </warningsuppression>
  <preprocess if="" unless="" value="" append="">
    <do />
    <!--Text-->
  </preprocess>
  <postprocess if="" unless="" value="" append="">
    <do />
    <!--Text-->
  </postprocess>
  <pch enable="" pchfile="" pchheader="" pchheaderdir="" use-forced-include="" if="" unless="" />
  <remove>
    <defines if="" unless="" value="" append="">
      <do />
      <!--Text-->
    </defines>
    <cc.options if="" unless="" value="" append="">
      <do />
      <!--Text-->
    </cc.options>
    <as.options if="" unless="" value="" append="">
      <do />
      <!--Text-->
    </as.options>
    <link.options if="" unless="" value="" append="">
      <do />
      <!--Text-->
    </link.options>
    <lib.options if="" unless="" value="" append="">
      <do />
      <!--Text-->
    </lib.options>
    <echo />
    <do />
  </remove>
</ConfigElement>
```
