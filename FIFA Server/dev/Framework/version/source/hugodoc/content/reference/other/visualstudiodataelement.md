---
title: VisualStudioDataElement
weight: 1075
---
## Syntax
```xml
<VisualStudioDataElement>
  <pregenerate-target targetname="" nant-build-only="" />
  <excludedbuildfiles append="" name="" optionset="" />
  <enableunmanageddebugging />
  <msbuildoptions append="" />
  <projecttypeguids />
  <extension />
  <echo />
  <do />
</VisualStudioDataElement>
```
### Nested Elements
| Name | Description | Type | Required |
| ---- | ----------- | ---- | -------- |
| {{< task "EA.Eaconfig.Structured.BuildTargetElement" "pregenerate-target" >}}|  | {{< task "EA.Eaconfig.Structured.BuildTargetElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.StructuredFileSet" "excludedbuildfiles" >}}|  | {{< task "EA.Eaconfig.Structured.StructuredFileSet" >}} | False |
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "enableunmanageddebugging" >}}|  | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.StructuredOptionSet" "msbuildoptions" >}}|  | {{< task "EA.Eaconfig.Structured.StructuredOptionSet" >}} | False |
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "projecttypeguids" >}}|  | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "extension" >}}|  | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | False |

## Full Syntax
```xml
<VisualStudioDataElement>
  <pregenerate-target targetname="" nant-build-only="">
    <target depends="" description="" hidden="" style="" name="" if="" unless="">
      <!--NAnt Script-->
    </target>
    <command if="" unless="" value="" append="">
      <do />
      <!--Text-->
    </command>
    <echo />
    <do />
  </pregenerate-target>
  <excludedbuildfiles append="" name="" optionset="" defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" if="" unless="">
    <includes />
    <excludes />
    <do />
    <group />
    <!--Structured Fileset-->
  </excludedbuildfiles>
  <enableunmanageddebugging if="" unless="" value="" append="">
    <do />
    <!--Text-->
  </enableunmanageddebugging>
  <msbuildoptions append="" fromoptionset="">
    <option />
    <!--Structured Optionset-->
  </msbuildoptions>
  <projecttypeguids if="" unless="" value="" append="">
    <do />
    <!--Text-->
  </projecttypeguids>
  <extension if="" unless="" value="" append="">
    <do />
    <!--Text-->
  </extension>
  <echo />
  <do />
</VisualStudioDataElement>
```
