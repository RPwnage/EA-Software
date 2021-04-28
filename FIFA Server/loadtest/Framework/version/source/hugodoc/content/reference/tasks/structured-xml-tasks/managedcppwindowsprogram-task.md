---
title: < ManagedCppWindowsProgram > Task
weight: 1063
---
## Syntax
```xml
<ManagedCppWindowsProgram outputdir="" outputname="" debug="" comment="" name="" group="" frompartial="" buildtype="">
  <libraries append="" name="" optionset="" />
  <dlls append="" name="" optionset="" />
  <objectfiles append="" name="" optionset="" />
  <sourcefiles append="" name="" optionset="" />
  <asmsourcefiles append="" name="" optionset="" />
  <resourcefiles prefix="" resourcebasedir="" includedirs="" defines="" />
  <shaderfiles append="" name="" optionset="" />
  <nonembed-resourcefiles append="" name="" optionset="" />
  <natvisfiles append="" name="" optionset="" />
  <custombuildfiles basedir="" fromfileset="" failonmissing="" name="" append="" />
  <rootnamespace />
  <includedirs />
  <libdirs />
  <usingdirs />
  <headerfiles append="" name="" optionset="" />
  <comassemblies append="" name="" optionset="" />
  <assemblies append="" name="" optionset="" />
  <force-usingassemblies append="" name="" optionset="" />
  <additional-manifest-files append="" name="" optionset="" />
  <usesharedpch module="" use-shared-binary="" />
  <config>
    <buildoptions />
    <defines />
    <targetframeworkversion />
    <targetframeworkfamily />
    <warningsuppression />
    <preprocess />
    <postprocess />
    <pch enable="" pchfile="" pchheader="" pchheaderdir="" use-forced-include="" />
    <remove />
  </config>
  <buildsteps>
    <prebuild-step targetname="" nant-build-only="" />
    <postbuild-step skip-if-up-to-date="" />
    <custom-build-step before="" after="" />
    <packaging packagename="" deployassets="" copyassetswithoutsync="" />
    <run workingdir="" args="" startupprogram="" />
    <echo />
    <do />
  </buildsteps>
  <bulkbuild enable="" partial="" maxsize="" SplitByDirectories="" MinSplitFiles="" DeviateMaxSizeAllowance="" />
  <sdkreferences />
  <dotnet>
    <copylocal />
    <allowunsafe />
    <webreferences append="" />
    <echo />
    <do />
  </dotnet>
  <visualstudio>
    <pregenerate-target targetname="" nant-build-only="" />
    <excludedbuildfiles append="" name="" optionset="" />
    <pre-build-step />
    <post-build-step />
    <vcproj />
    <csproj />
    <msbuildoptions append="" />
    <extension />
    <echo />
    <do />
  </visualstudio>
  <java />
  <android-gradle gradle-directory="" gradle-project="" gradle-jvm-args="" native-activity="" />
  <platforms />
  <merge-into />
  <assemblyinfo company="" copyright="" product="" title="" description="" version="" deterministic="" />
  <buildlayout enable="" build-type="" />
  <frompartial />
  <script order="" />
  <copylocal use-hardlink-if-possible="" />
  <buildtype name="" override="" />
  <dependencies skip-runtime-dependency="" />
  <echo />
  <do />
</ManagedCppWindowsProgram>
```
## Summary ##



### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| outputdir | Overrides the default framework directory where built files are located. | String | False |
| outputname | Overrides the default name of built files. | String | False |
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
| {{< task "EA.Eaconfig.Structured.StructuredFileSet" "libraries" >}}| Adds a set of libraries  | {{< task "EA.Eaconfig.Structured.StructuredFileSet" >}} | False |
| {{< task "EA.Eaconfig.Structured.StructuredFileSet" "dlls" >}}| Adds a set of dynamic/shared libraries. DLLs are passed to the linker input. | {{< task "EA.Eaconfig.Structured.StructuredFileSet" >}} | False |
| {{< task "EA.Eaconfig.Structured.StructuredFileSet" "objectfiles" >}}| Adds the list of object files | {{< task "EA.Eaconfig.Structured.StructuredFileSet" >}} | False |
| {{< task "EA.Eaconfig.Structured.StructuredFileSet" "sourcefiles" >}}| Adds the list of source files | {{< task "EA.Eaconfig.Structured.StructuredFileSet" >}} | False |
| {{< task "EA.Eaconfig.Structured.StructuredFileSet" "asmsourcefiles" >}}| Adds a list of assembly source files | {{< task "EA.Eaconfig.Structured.StructuredFileSet" >}} | False |
| {{< task "EA.Eaconfig.Structured.ResourceFilesElement" "resourcefiles" >}}| Adds a list of resource files | {{< task "EA.Eaconfig.Structured.ResourceFilesElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.StructuredFileSet" "shaderfiles" >}}| Adds a list of shader files | {{< task "EA.Eaconfig.Structured.StructuredFileSet" >}} | False |
| {{< task "EA.Eaconfig.Structured.StructuredFileSet" "nonembed-resourcefiles" >}}| Adds a list of resource files | {{< task "EA.Eaconfig.Structured.StructuredFileSet" >}} | False |
| {{< task "EA.Eaconfig.Structured.StructuredFileSet" "natvisfiles" >}}| Adds a list of natvis files | {{< task "EA.Eaconfig.Structured.StructuredFileSet" >}} | False |
| {{< task "EA.Eaconfig.Structured.StructuredFileSetCollection" "custombuildfiles" >}}| Files with associated custombuildtools | {{< task "EA.Eaconfig.Structured.StructuredFileSetCollection" >}} | False |
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "rootnamespace" >}}| Specifies the Rootnamespace for a visual studio project | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "includedirs" >}}| Defines set of directories to search for header files. Relative paths are prepended with the package directory. | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "libdirs" >}}| Defines set of directories to search for library files. Relative paths are prepended with the package directory. | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "usingdirs" >}}| Defines set of directories to search for forced using assemblies. Relative paths are prepended with the package directory. | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.StructuredFileSet" "headerfiles" >}}| Includes a list of header files for a project | {{< task "EA.Eaconfig.Structured.StructuredFileSet" >}} | False |
| {{< task "EA.Eaconfig.Structured.StructuredFileSet" "comassemblies" >}}| A list of COM assemblies for this module | {{< task "EA.Eaconfig.Structured.StructuredFileSet" >}} | False |
| {{< task "EA.Eaconfig.Structured.StructuredFileSet" "assemblies" >}}| A list of referenced assemblies for this module | {{< task "EA.Eaconfig.Structured.StructuredFileSet" >}} | False |
| {{< task "EA.Eaconfig.Structured.StructuredFileSet" "force-usingassemblies" >}}|  | {{< task "EA.Eaconfig.Structured.StructuredFileSet" >}} | False |
| {{< task "EA.Eaconfig.Structured.StructuredFileSet" "additional-manifest-files" >}}| A list of application additional manifest files for native projects. | {{< task "EA.Eaconfig.Structured.StructuredFileSet" >}} | False |
| {{< task "EA.Eaconfig.Structured.UseSharedPchElement" "usesharedpch" >}}| Specified to use shared pch module. | {{< task "EA.Eaconfig.Structured.UseSharedPchElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.ConfigElement" "config" >}}| Sets the configuration for a project | {{< task "EA.Eaconfig.Structured.ConfigElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.BuildStepsElement" "buildsteps" >}}| Sets the build steps for a project | {{< task "EA.Eaconfig.Structured.BuildStepsElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.BulkBuildElement" "bulkbuild" >}}| Sets the bulkbuild configuration | {{< task "EA.Eaconfig.Structured.BulkBuildElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "sdkreferences" >}}| SDK references (used in MSBuild files) | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.DotNetDataElement" "dotnet" >}}| MCPP/DotNet specific data | {{< task "EA.Eaconfig.Structured.DotNetDataElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.VisualStudioDataElement" "visualstudio" >}}|  | {{< task "EA.Eaconfig.Structured.VisualStudioDataElement" >}} | False |
| {{< task "NAnt.Core.Structured.DeprecatedPropertyElement" "java" >}}|  | {{< task "NAnt.Core.Structured.DeprecatedPropertyElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.AndroidGradle" "android-gradle" >}}|  | {{< task "EA.Eaconfig.Structured.AndroidGradle" >}} | False |
| {{< task "EA.Eaconfig.Structured.PlatformExtensions" "platforms" >}}|  | {{< task "EA.Eaconfig.Structured.PlatformExtensions" >}} | False |
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "merge-into" >}}| [Deprecated] Merge module feature is being deprecated.  If you still required to use this feature, please contact Frostbite.Team.CM regarding your requirement. | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.AssemblyInfoTask" "assemblyinfo" >}}|  | {{< task "EA.Eaconfig.Structured.AssemblyInfoTask" >}} | False |
| {{< task "EA.Eaconfig.Structured.BuildLayoutElement" "buildlayout" >}}| Sets the build steps for a project | {{< task "EA.Eaconfig.Structured.BuildLayoutElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "frompartial" >}}|  | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.NantScript" "script" >}}|  | {{< task "EA.Eaconfig.Structured.NantScript" >}} | False |
| {{< task "EA.Eaconfig.Structured.CopyLocalElement" "copylocal" >}}| Applies &#39;copylocal&#39; to all dependents | {{< task "EA.Eaconfig.Structured.CopyLocalElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.BuildTypePropertyElement" "buildtype" >}}|  | {{< task "EA.Eaconfig.Structured.BuildTypePropertyElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.DependenciesPropertyElement" "dependencies" >}}| Sets the dependencies for a project | {{< task "EA.Eaconfig.Structured.DependenciesPropertyElement" >}} | False |

## Full Syntax
```xml
<ManagedCppWindowsProgram outputdir="" outputname="" debug="" comment="" name="" group="" frompartial="" buildtype="" failonerror="" verbose="" if="" unless="">
  <libraries append="" name="" optionset="" defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" if="" unless="">
    <includes />
    <excludes />
    <do />
    <group />
    <!--Structured Fileset-->
  </libraries>
  <dlls append="" name="" optionset="" defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" if="" unless="">
    <includes />
    <excludes />
    <do />
    <group />
    <!--Structured Fileset-->
  </dlls>
  <objectfiles append="" name="" optionset="" defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" if="" unless="">
    <includes />
    <excludes />
    <do />
    <group />
    <!--Structured Fileset-->
  </objectfiles>
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
  <resourcefiles prefix="" resourcebasedir="" includedirs="" defines="" append="" name="" optionset="" defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" if="" unless="">
    <includes />
    <excludes />
    <do />
    <group />
    <!--Structured Fileset-->
  </resourcefiles>
  <shaderfiles append="" name="" optionset="" defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" if="" unless="">
    <includes />
    <excludes />
    <do />
    <group />
    <!--Structured Fileset-->
  </shaderfiles>
  <nonembed-resourcefiles append="" name="" optionset="" defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" if="" unless="">
    <includes />
    <excludes />
    <do />
    <group />
    <!--Structured Fileset-->
  </nonembed-resourcefiles>
  <natvisfiles append="" name="" optionset="" defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" if="" unless="">
    <includes />
    <excludes />
    <do />
    <group />
    <!--Structured Fileset-->
  </natvisfiles>
  <custombuildfiles if="" unless="" basedir="" fromfileset="" failonmissing="" name="" append="">
    <includes />
    <excludes />
    <do />
    <group />
  </custombuildfiles>
  <rootnamespace if="" unless="" value="" append="">
    <do />
    <!--Text-->
  </rootnamespace>
  <includedirs if="" unless="" value="" append="">
    <do />
    <!--Text-->
  </includedirs>
  <libdirs if="" unless="" value="" append="">
    <do />
    <!--Text-->
  </libdirs>
  <usingdirs if="" unless="" value="" append="">
    <do />
    <!--Text-->
  </usingdirs>
  <headerfiles append="" name="" optionset="" defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" if="" unless="">
    <includes />
    <excludes />
    <do />
    <group />
    <!--Structured Fileset-->
  </headerfiles>
  <comassemblies append="" name="" optionset="" defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" if="" unless="">
    <includes />
    <excludes />
    <do />
    <group />
    <!--Structured Fileset-->
  </comassemblies>
  <assemblies append="" name="" optionset="" defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" if="" unless="">
    <includes />
    <excludes />
    <do />
    <group />
    <!--Structured Fileset-->
  </assemblies>
  <force-usingassemblies append="" name="" optionset="" defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" if="" unless="">
    <includes />
    <excludes />
    <do />
    <group />
    <!--Structured Fileset-->
  </force-usingassemblies>
  <additional-manifest-files append="" name="" optionset="" defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" if="" unless="">
    <includes />
    <excludes />
    <do />
    <group />
    <!--Structured Fileset-->
  </additional-manifest-files>
  <usesharedpch module="" use-shared-binary="" if="" unless="" />
  <config>
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
  </config>
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
  <bulkbuild enable="" partial="" maxsize="" SplitByDirectories="" MinSplitFiles="" DeviateMaxSizeAllowance="">
    <loosefiles append="" name="" optionset="" defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" if="" unless="">
      <includes />
      <excludes />
      <do />
      <group />
      <!--Structured Fileset-->
    </loosefiles>
    <sourcefiles name="" append="" optionset="" defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" if="" unless="">
      <name />
      <includes />
      <excludes />
      <do />
      <group />
      <!--Structured Fileset-->
    </sourcefiles>
    <manualsourcefiles append="" name="" optionset="" defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" if="" unless="">
      <includes />
      <excludes />
      <do />
      <group />
      <!--Structured Fileset-->
    </manualsourcefiles>
    <echo />
    <do />
  </bulkbuild>
  <sdkreferences if="" unless="" value="" append="">
    <do />
    <!--Text-->
  </sdkreferences>
  <dotnet>
    <copylocal if="" unless="" value="" append="">
      <do />
      <!--Text-->
    </copylocal>
    <allowunsafe if="" unless="" value="" append="">
      <do />
      <!--Text-->
    </allowunsafe>
    <webreferences append="" fromoptionset="">
      <option />
      <!--Structured Optionset-->
    </webreferences>
    <echo />
    <do />
  </dotnet>
  <visualstudio>
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
    <pre-build-step if="" unless="" value="" append="">
      <do />
      <!--Text-->
    </pre-build-step>
    <post-build-step if="" unless="" value="" append="">
      <do />
      <!--Text-->
    </post-build-step>
    <vcproj>
      <pre-link-step if="" unless="" value="" append="">
        <do />
        <!--Text-->
      </pre-link-step>
      <input-resource-manifests append="" name="" optionset="" defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" if="" unless="">
        <includes />
        <excludes />
        <do />
        <group />
        <!--Structured Fileset-->
      </input-resource-manifests>
      <additional-manifest-files append="" name="" optionset="" defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" if="" unless="">
        <includes />
        <excludes />
        <do />
        <group />
        <!--Structured Fileset-->
      </additional-manifest-files>
      <echo />
      <do />
    </vcproj>
    <csproj>
      <link.resourcefiles.nonembed if="" unless="" value="" append="">
        <do />
        <!--Text-->
      </link.resourcefiles.nonembed>
      <echo />
      <do />
    </csproj>
    <msbuildoptions append="" fromoptionset="">
      <option />
      <!--Structured Optionset-->
    </msbuildoptions>
    <extension if="" unless="" value="" append="">
      <do />
      <!--Text-->
    </extension>
    <echo />
    <do />
  </visualstudio>
  <java />
  <android-gradle gradle-directory="" gradle-project="" gradle-jvm-args="" native-activity="">
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
  </android-gradle>
  <platforms if="" unless="">
    <failonerror />
    <verbose />
  </platforms>
  <merge-into if="" unless="" value="" append="">
    <do />
    <!--Text-->
  </merge-into>
  <assemblyinfo company="" copyright="" product="" title="" description="" version="" deterministic="">
    <echo />
    <do />
  </assemblyinfo>
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
</ManagedCppWindowsProgram>
```
