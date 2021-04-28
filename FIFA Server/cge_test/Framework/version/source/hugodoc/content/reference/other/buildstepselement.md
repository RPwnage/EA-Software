---
title: BuildStepsElement
weight: 1005
---
## Syntax
```xml
<BuildStepsElement>
  <prebuild-step targetname="" nant-build-only="" />
  <postbuild-step skip-if-up-to-date="" />
  <custom-build-step before="" after="" />
  <packaging packagename="" deployassets="" copyassetswithoutsync="" />
  <run workingdir="" args="" startupprogram="" />
  <echo />
  <do />
</BuildStepsElement>
```
## Summary ##
BuildStepsElement


### Nested Elements
| Name | Description | Type | Required |
| ---- | ----------- | ---- | -------- |
| {{< task "EA.Eaconfig.Structured.BuildTargetElement" "prebuild-step" >}}| Sets the prebuild steps for a project | {{< task "EA.Eaconfig.Structured.BuildTargetElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.PostBuildTargetElement" "postbuild-step" >}}| Sets the postbuild steps for a project | {{< task "EA.Eaconfig.Structured.PostBuildTargetElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.CustomBuildStepElement" "custom-build-step" >}}| Defines a custom build step that may execute before or<br>after another target | {{< task "EA.Eaconfig.Structured.CustomBuildStepElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.PackagingElement" "packaging" >}}| Sets the packaging steps for a project | {{< task "EA.Eaconfig.Structured.PackagingElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.RunDataElement" "run" >}}| Sets the run steps for a project | {{< task "EA.Eaconfig.Structured.RunDataElement" >}} | False |

## Full Syntax
```xml
<BuildStepsElement>
  <prebuild-step targetname="" nant-build-only="">
    <target depends="" description="" hidden="" style="" name="" if="" unless="">
      <!--NAnt Script-->
    </target>
    <command if="" unless="" value="" append="">
      <do />
      <!--Text-->
    </command>
    <echo />
    <do />
  </prebuild-step>
  <postbuild-step skip-if-up-to-date="" targetname="" nant-build-only="">
    <target depends="" description="" hidden="" style="" name="" if="" unless="">
      <!--NAnt Script-->
    </target>
    <command if="" unless="" value="" append="">
      <do />
      <!--Text-->
    </command>
    <echo />
    <do />
  </postbuild-step>
  <custom-build-step before="" after="">
    <custom-build-tool if="" unless="" value="" append="">
      <do />
      <!--Text-->
      <!--NAnt Script-->
    </custom-build-tool>
    <custom-build-outputs if="" unless="" value="" append="">
      <do />
      <!--Text-->
    </custom-build-outputs>
    <custom-build-dependencies append="" name="" optionset="" defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" if="" unless="">
      <includes />
      <excludes />
      <do />
      <group />
      <!--Structured Fileset-->
    </custom-build-dependencies>
    <echo />
    <do />
  </custom-build-step>
  <packaging packagename="" deployassets="" copyassetswithoutsync="">
    <assetfiles if="" unless="" basedir="" fromfileset="" failonmissing="" name="" append="">
      <includes />
      <excludes />
      <do />
      <group />
    </assetfiles>
    <assetdependencies if="" unless="" value="" append="">
      <do />
      <!--Text-->
    </assetdependencies>
    <asset-configbuilddir if="" unless="" value="" append="">
      <do />
      <!--Text-->
    </asset-configbuilddir>
    <manifestfile append="" name="" optionset="" defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" if="" unless="">
      <includes />
      <excludes />
      <do />
      <group />
      <!--Structured Fileset-->
    </manifestfile>
    <echo />
    <do />
  </packaging>
  <run workingdir="" args="" startupprogram="">
    <workingdir if="" unless="" value="" append="">
      <do />
      <!--Text-->
    </workingdir>
    <args if="" unless="" value="" append="">
      <do />
      <!--Text-->
    </args>
    <startupprogram if="" unless="" value="" append="">
      <do />
      <!--Text-->
    </startupprogram>
    <echo />
    <do />
  </run>
  <echo />
  <do />
</BuildStepsElement>
```
