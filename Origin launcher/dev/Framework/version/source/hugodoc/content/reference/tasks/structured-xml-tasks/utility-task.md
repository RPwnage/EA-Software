---
title: < Utility > Task
weight: 1070
---
## Syntax
```xml
<Utility transitivelink="" outputdir="" debug="" comment="" name="" group="" frompartial="" buildtype="">
  <custombuildfiles basedir="" fromfileset="" failonmissing="" name="" append="" />
  <excludedbuildfiles append="" name="" optionset="" />
  <buildsteps>
    <prebuild-step targetname="" nant-build-only="" />
    <postbuild-step skip-if-up-to-date="" />
    <custom-build-step before="" after="" />
    <packaging packagename="" deployassets="" copyassetswithoutsync="" />
    <run workingdir="" args="" startupprogram="" />
    <echo />
    <do />
  </buildsteps>
  <filecopystep file="" tofile="" todir="" />
  <config />
  <buildlayout enable="" build-type="" />
  <frompartial />
  <script order="" />
  <copylocal use-hardlink-if-possible="" />
  <buildtype name="" override="" />
  <dependencies skip-runtime-dependency="" />
  <echo />
  <do />
</Utility>
```
## Summary ##



### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| transitivelink | Specified that any dependencies listed in this module will be propagated for transitive linking. Only use this if module is a wrapper for a static library. | Boolean | False |
| outputdir | Setup the location where &lt;copylocal&gt; of dependencies should copy the files to. | String | False |
| debug |  | Boolean | False |
| comment |  | String | False |
| name | The name of this module. | String | True |
| group | The name of the build group this module is a part of. The default is &#39;runtime&#39;. | String | False |
| frompartial |  | String | False |
| buildtype | Used to explicitly state the build type. By default<br>structured XML determines the build type from the structured XML tag.<br>This field is used to override that value. | String | False |
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | Boolean | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

### Nested Elements
| Name | Description | Type | Required |
| ---- | ----------- | ---- | -------- |
| {{< task "EA.Eaconfig.Structured.StructuredFileSetCollection" "custombuildfiles" >}}| Files with associated custombuildtools | {{< task "EA.Eaconfig.Structured.StructuredFileSetCollection" >}} | False |
| {{< task "EA.Eaconfig.Structured.StructuredFileSet" "excludedbuildfiles" >}}| Files that are part of the project but are excluded from the build. | {{< task "EA.Eaconfig.Structured.StructuredFileSet" >}} | False |
| {{< task "EA.Eaconfig.Structured.BuildStepsElement" "buildsteps" >}}| Sets the build steps for a project | {{< task "EA.Eaconfig.Structured.BuildStepsElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.UtilityTask+FileCopyStepCollection" "filecopystep" >}}| Set up special step to do file copies. | {{< task "EA.Eaconfig.Structured.UtilityTask+FileCopyStepCollection" >}} | False |
| {{< task "EA.Eaconfig.Structured.UtilityTask+UtilityConfigElement" "config" >}}| Sets the configuration for a project | {{< task "EA.Eaconfig.Structured.UtilityTask+UtilityConfigElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.BuildLayoutElement" "buildlayout" >}}| Sets the build steps for a project | {{< task "EA.Eaconfig.Structured.BuildLayoutElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "frompartial" >}}|  | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.NantScript" "script" >}}|  | {{< task "EA.Eaconfig.Structured.NantScript" >}} | False |
| {{< task "EA.Eaconfig.Structured.CopyLocalElement" "copylocal" >}}| Applies &#39;copylocal&#39; to all dependents | {{< task "EA.Eaconfig.Structured.CopyLocalElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.BuildTypePropertyElement" "buildtype" >}}|  | {{< task "EA.Eaconfig.Structured.BuildTypePropertyElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.DependenciesPropertyElement" "dependencies" >}}| Sets the dependencies for a project | {{< task "EA.Eaconfig.Structured.DependenciesPropertyElement" >}} | False |

## Full Syntax
```xml
<Utility transitivelink="" outputdir="" debug="" comment="" name="" group="" frompartial="" buildtype="" failonerror="" verbose="" if="" unless="">
  <custombuildfiles if="" unless="" basedir="" fromfileset="" failonmissing="" name="" append="">
    <includes />
    <excludes />
    <do />
    <group />
  </custombuildfiles>
  <excludedbuildfiles append="" name="" optionset="" defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" if="" unless="">
    <includes />
    <excludes />
    <do />
    <group />
    <!--Structured Fileset-->
  </excludedbuildfiles>
  <buildsteps>
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
  </buildsteps>
  <filecopystep file="" tofile="" todir="" if="" unless="">
    <file />
    <tofile />
    <todir />
    <fileset />
  </filecopystep>
  <config>
    <preprocess if="" unless="" value="" append="">
      <do />
      <!--Text-->
    </preprocess>
    <postprocess if="" unless="" value="" append="">
      <do />
      <!--Text-->
    </postprocess>
    <echo />
    <do />
  </config>
  <buildlayout enable="" build-type="">
    <tags append="" fromoptionset="">
      <option />
      <!--Structured Optionset-->
    </tags>
    <echo />
    <do />
  </buildlayout>
  <frompartial if="" unless="" value="" append="">
    <do />
    <!--Text-->
  </frompartial>
  <script order="" />
  <copylocal use-hardlink-if-possible="" if="" unless="" value="" append="">
    <do />
    <!--Text-->
  </copylocal>
  <buildtype name="" override="" if="" unless="" />
  <dependencies skip-runtime-dependency="">
    <auto interface="" link="" copylocal="" internal="" if="" unless="" value="" append="">
      <do />
      <!--Text-->
    </auto>
    <use interface="" link="" copylocal="" internal="" if="" unless="" value="" append="">
      <do />
      <!--Text-->
    </use>
    <build interface="" link="" copylocal="" internal="" if="" unless="" value="" append="">
      <do />
      <!--Text-->
    </build>
    <interface interface="" link="" copylocal="" internal="" if="" unless="" value="" append="">
      <do />
      <!--Text-->
    </interface>
    <link interface="" link="" copylocal="" internal="" if="" unless="" value="" append="">
      <do />
      <!--Text-->
    </link>
    <echo />
    <do />
  </dependencies>
  <echo />
  <do />
</Utility>
```
