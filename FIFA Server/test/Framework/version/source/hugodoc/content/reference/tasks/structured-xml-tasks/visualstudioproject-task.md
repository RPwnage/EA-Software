---
title: < VisualStudioProject > Task
weight: 1073
---
## Syntax
```xml
<VisualStudioProject guid="" managed="" unittest="" debug="" comment="" name="" group="" frompartial="" buildtype="">
  <ProjectFile />
  <ProjectConfig />
  <ProjectPlatform />
  <buildlayout enable="" build-type="" />
  <frompartial />
  <script order="" />
  <copylocal use-hardlink-if-possible="" />
  <buildtype name="" override="" />
  <dependencies skip-runtime-dependency="" />
  <echo />
  <do />
</VisualStudioProject>
```
## Summary ##
Define a module that represents an existing visual studio project file


### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| guid | A Visual Studio Project GUID which is inserted into the generated solution.  | String | False |
| managed | Set to true if the project is a managed project | Boolean | False |
| unittest | For unit test C# or F# projects | Boolean | False |
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
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "ProjectFile" >}}| The full path to the Visual Studio Project File | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "ProjectConfig" >}}| The Visual Studio Project Configuration name that corresponds to the current Framework ${config} value | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "ProjectPlatform" >}}| The Visual Studio Project Platform name that corresponds to the current Framework ${config} value | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.BuildLayoutElement" "buildlayout" >}}| Sets the build steps for a project | {{< task "EA.Eaconfig.Structured.BuildLayoutElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "frompartial" >}}|  | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.NantScript" "script" >}}|  | {{< task "EA.Eaconfig.Structured.NantScript" >}} | False |
| {{< task "EA.Eaconfig.Structured.CopyLocalElement" "copylocal" >}}| Applies &#39;copylocal&#39; to all dependents | {{< task "EA.Eaconfig.Structured.CopyLocalElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.BuildTypePropertyElement" "buildtype" >}}|  | {{< task "EA.Eaconfig.Structured.BuildTypePropertyElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.DependenciesPropertyElement" "dependencies" >}}| Sets the dependencies for a project | {{< task "EA.Eaconfig.Structured.DependenciesPropertyElement" >}} | False |

## Full Syntax
```xml
<VisualStudioProject guid="" managed="" unittest="" debug="" comment="" name="" group="" frompartial="" buildtype="" failonerror="" verbose="" if="" unless="">
  <ProjectFile if="" unless="" value="" append="">
    <do />
    <!--Text-->
  </ProjectFile>
  <ProjectConfig if="" unless="" value="" append="">
    <do />
    <!--Text-->
  </ProjectConfig>
  <ProjectPlatform if="" unless="" value="" append="">
    <do />
    <!--Text-->
  </ProjectPlatform>
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
</VisualStudioProject>
```
