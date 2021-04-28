---
title: < MakeStyle > Task
weight: 1058
---
## Syntax
```xml
<MakeStyle transitivelink="" debug="" comment="" name="" group="" frompartial="" buildtype="">
  <MakeBuildCommand />
  <MakeRebuildCommand />
  <MakeCleanCommand />
  <MakeOutput />
  <sourcefiles append="" name="" optionset="" />
  <asmsourcefiles append="" name="" optionset="" />
  <headerfiles append="" name="" optionset="" />
  <excludedbuildfiles append="" name="" optionset="" />
  <buildlayout enable="" build-type="" />
  <frompartial />
  <script order="" />
  <copylocal use-hardlink-if-possible="" />
  <buildtype name="" override="" />
  <dependencies skip-runtime-dependency="" />
  <echo />
  <do />
</MakeStyle>
```
## Summary ##
MakeStyle modules are used to execute external build or clean commands


### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| transitivelink | Specified that any dependencies listed in this module will be propagated for transitive linking. Only use this if module is a wrapper for a static library. | Boolean | False |
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
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "MakeBuildCommand" >}}| A makestyle build command, this should contain executable OS command(s)/script | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "MakeRebuildCommand" >}}| A makestyle rebuild command, this should contain executable OS command(s)/script | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "MakeCleanCommand" >}}| A makestyle clean command, this should contain executable OS command(s)/script | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "MakeOutput" >}}| A list of expected output files (separated by semi-colon.  Only used in setting up Visual Studio vcxproj file. | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.StructuredFileSet" "sourcefiles" >}}| Adds the list of sourcefiles, does not participate directly in the<br>build but can be used in generation of projects for external build systems | {{< task "EA.Eaconfig.Structured.StructuredFileSet" >}} | False |
| {{< task "EA.Eaconfig.Structured.StructuredFileSet" "asmsourcefiles" >}}| Adds the list of asmsourcefiles, does not participate directly in the<br>build but can be used in generation of projects for external build systems | {{< task "EA.Eaconfig.Structured.StructuredFileSet" >}} | False |
| {{< task "EA.Eaconfig.Structured.StructuredFileSet" "headerfiles" >}}| Adds the list of headerfiles, does not participate directly in the<br>build but can be used in generation of projects for external build systems | {{< task "EA.Eaconfig.Structured.StructuredFileSet" >}} | False |
| {{< task "EA.Eaconfig.Structured.StructuredFileSet" "excludedbuildfiles" >}}| Adds the list of excluded build files, does not participate directly in the<br>build but can be used in generation of projects for external build systems | {{< task "EA.Eaconfig.Structured.StructuredFileSet" >}} | False |
| {{< task "EA.Eaconfig.Structured.BuildLayoutElement" "buildlayout" >}}| Sets the build steps for a project | {{< task "EA.Eaconfig.Structured.BuildLayoutElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "frompartial" >}}|  | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.NantScript" "script" >}}|  | {{< task "EA.Eaconfig.Structured.NantScript" >}} | False |
| {{< task "EA.Eaconfig.Structured.CopyLocalElement" "copylocal" >}}| Applies &#39;copylocal&#39; to all dependents | {{< task "EA.Eaconfig.Structured.CopyLocalElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.BuildTypePropertyElement" "buildtype" >}}|  | {{< task "EA.Eaconfig.Structured.BuildTypePropertyElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.DependenciesPropertyElement" "dependencies" >}}| Sets the dependencies for a project | {{< task "EA.Eaconfig.Structured.DependenciesPropertyElement" >}} | False |

## Full Syntax
```xml
<MakeStyle transitivelink="" debug="" comment="" name="" group="" frompartial="" buildtype="" failonerror="" verbose="" if="" unless="">
  <MakeBuildCommand if="" unless="" value="" append="">
    <do />
    <!--Text-->
  </MakeBuildCommand>
  <MakeRebuildCommand if="" unless="" value="" append="">
    <do />
    <!--Text-->
  </MakeRebuildCommand>
  <MakeCleanCommand if="" unless="" value="" append="">
    <do />
    <!--Text-->
  </MakeCleanCommand>
  <MakeOutput if="" unless="" value="" append="">
    <do />
    <!--Text-->
  </MakeOutput>
  <sourcefiles append="" name="" optionset="" defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" if="" unless="">
    <includes />
    <excludes />
    <do />
    <group />
    <!--Structured Fileset-->
  </sourcefiles>
  <asmsourcefiles append="" name="" optionset="" defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" if="" unless="">
    <includes />
    <excludes />
    <do />
    <group />
    <!--Structured Fileset-->
  </asmsourcefiles>
  <headerfiles append="" name="" optionset="" defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" if="" unless="">
    <includes />
    <excludes />
    <do />
    <group />
    <!--Structured Fileset-->
  </headerfiles>
  <excludedbuildfiles append="" name="" optionset="" defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" if="" unless="">
    <includes />
    <excludes />
    <do />
    <group />
    <!--Structured Fileset-->
  </excludedbuildfiles>
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
</MakeStyle>
```
