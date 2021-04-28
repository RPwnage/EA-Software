---
title: < CSharpLibrary > Task
weight: 1050
---
## Syntax
```xml
<CSharpLibrary outputdir="" outputname="" workflow="" unittest="" webapp="" debug="" comment="" name="" group="" frompartial="" buildtype="">
  <rootnamespace />
  <applicationmanifest />
  <appdesignerfolder />
  <disablevshosting />
  <importmsbuildprojects />
  <runpostbuildevent />
  <applicationicon />
  <generatedoc />
  <keyfile />
  <copylocal-dependencies />
  <sourcefiles append="" name="" optionset="" />
  <assemblies usedefaultassemblies="" />
  <dlls append="" name="" optionset="" />
  <resourcefiles prefix="" resourcebasedir="" includedirs="" defines="" />
  <custombuildfiles basedir="" fromfileset="" failonmissing="" name="" append="" />
  <nonembed-resourcefiles append="" name="" optionset="" />
  <contentfiles append="" name="" optionset="" />
  <webreferences append="" />
  <modules append="" name="" optionset="" />
  <comassemblies append="" name="" optionset="" />
  <config>
    <buildoptions />
    <defines />
    <additionaloptions />
    <copylocal />
    <platform />
    <allowunsafe />
    <targetframeworkversion />
    <languageversion />
    <usewpf />
    <usewindowsforms />
    <targetframeworkfamily />
    <generateserializationassemblies />
    <suppresswarnings />
    <warningsaserrors.list />
    <warningsaserrors />
    <warningsnotaserrors.list />
    <remove />
    <preprocess />
    <postprocess />
    <echo />
    <do />
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
  <visualstudio>
    <pregenerate-target targetname="" nant-build-only="" />
    <excludedbuildfiles append="" name="" optionset="" />
    <enableunmanageddebugging />
    <msbuildoptions append="" />
    <projecttypeguids />
    <extension />
    <echo />
    <do />
  </visualstudio>
  <platforms />
  <nugetreferences append="" name="" optionset="" />
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
</CSharpLibrary>
```
## Summary ##
A Module buildable as a C Sharp library.


### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| outputdir | Overrides the default framework directory where built files are located. | String | False |
| outputname | Overrides the default name of built files. | String | False |
| workflow | Is this a Workflow module. | Boolean | False |
| unittest | Is this a UnitTest module. | Boolean | False |
| webapp | Indicates this is a web application project and enables web debugging. | Boolean | False |
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
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "rootnamespace" >}}| Specifies the Rootnamespace for a visual studio project | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "applicationmanifest" >}}| Specifies the location of the Application manifest | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "appdesignerfolder" >}}| Specifies the name of the App designer folder | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "disablevshosting" >}}| used to enable/disable Visual Studio hosting process during debugging | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "importmsbuildprojects" >}}| Additional MSBuild project imports | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "runpostbuildevent" >}}| Postbuild event run condition | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "applicationicon" >}}| The location of the Application Icon file | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "generatedoc" >}}| property enables/disables generation of XML documentation files | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "keyfile" >}}| Key File | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "copylocal-dependencies" >}}|  | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.StructuredFileSet" "sourcefiles" >}}| Adds the list of sourcefiles | {{< task "EA.Eaconfig.Structured.StructuredFileSet" >}} | False |
| {{< task "EA.Eaconfig.Structured.DotNetAssembliesFileset" "assemblies" >}}| A list of referenced assemblies for this module | {{< task "EA.Eaconfig.Structured.DotNetAssembliesFileset" >}} | False |
| {{< task "EA.Eaconfig.Structured.StructuredFileSet" "dlls" >}}| Adds a set of native dynamic/shared libraries. These do not affect build but be copied<br>to module output directory if copy local is enabled. | {{< task "EA.Eaconfig.Structured.StructuredFileSet" >}} | False |
| {{< task "EA.Eaconfig.Structured.ResourceFilesElement" "resourcefiles" >}}| Adds a list of resource files | {{< task "EA.Eaconfig.Structured.ResourceFilesElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.StructuredFileSetCollection" "custombuildfiles" >}}| Files with associated custombuildtools | {{< task "EA.Eaconfig.Structured.StructuredFileSetCollection" >}} | False |
| {{< task "EA.Eaconfig.Structured.StructuredFileSet" "nonembed-resourcefiles" >}}| Adds a list of resource files | {{< task "EA.Eaconfig.Structured.StructuredFileSet" >}} | False |
| {{< task "EA.Eaconfig.Structured.StructuredFileSet" "contentfiles" >}}| Adds a list of &#39;Content&#39; files | {{< task "EA.Eaconfig.Structured.StructuredFileSet" >}} | False |
| {{< task "EA.Eaconfig.Structured.StructuredOptionSet" "webreferences" >}}| A list of webreferences for this module | {{< task "EA.Eaconfig.Structured.StructuredOptionSet" >}} | False |
| {{< task "EA.Eaconfig.Structured.StructuredFileSet" "modules" >}}| Adds the list of modules | {{< task "EA.Eaconfig.Structured.StructuredFileSet" >}} | False |
| {{< task "EA.Eaconfig.Structured.StructuredFileSet" "comassemblies" >}}| A list of COM assemblies for this module | {{< task "EA.Eaconfig.Structured.StructuredFileSet" >}} | False |
| {{< task "EA.Eaconfig.Structured.DotNetModuleTask+DotNetConfigElement" "config" >}}| Sets the configuration for a project | {{< task "EA.Eaconfig.Structured.DotNetModuleTask+DotNetConfigElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.BuildStepsElement" "buildsteps" >}}| Sets the build steps for a project | {{< task "EA.Eaconfig.Structured.BuildStepsElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.DotNetModuleTask+VisualStudioDataElement" "visualstudio" >}}|  | {{< task "EA.Eaconfig.Structured.DotNetModuleTask+VisualStudioDataElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.PlatformExtensions" "platforms" >}}|  | {{< task "EA.Eaconfig.Structured.PlatformExtensions" >}} | False |
| {{< task "EA.Eaconfig.Structured.StructuredFileSet" "nugetreferences" >}}|  | {{< task "EA.Eaconfig.Structured.StructuredFileSet" >}} | False |
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
<CSharpLibrary outputdir="" outputname="" workflow="" unittest="" webapp="" debug="" comment="" name="" group="" frompartial="" buildtype="" failonerror="" verbose="" if="" unless="">
  <rootnamespace if="" unless="" value="" append="">
    <do />
    <!--Text-->
  </rootnamespace>
  <applicationmanifest if="" unless="" value="" append="">
    <do />
    <!--Text-->
  </applicationmanifest>
  <appdesignerfolder if="" unless="" value="" append="">
    <do />
    <!--Text-->
  </appdesignerfolder>
  <disablevshosting if="" unless="" value="" append="">
    <do />
    <!--Text-->
  </disablevshosting>
  <importmsbuildprojects if="" unless="" value="" append="">
    <do />
    <!--Text-->
  </importmsbuildprojects>
  <runpostbuildevent if="" unless="" value="" append="">
    <do />
    <!--Text-->
  </runpostbuildevent>
  <applicationicon if="" unless="" value="" append="">
    <do />
    <!--Text-->
  </applicationicon>
  <generatedoc if="" unless="" value="" append="">
    <do />
    <!--Text-->
  </generatedoc>
  <keyfile if="" unless="" value="" append="">
    <do />
    <!--Text-->
  </keyfile>
  <copylocal-dependencies if="" unless="" value="" append="">
    <do />
    <!--Text-->
  </copylocal-dependencies>
  <sourcefiles append="" name="" optionset="" defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" if="" unless="">
    <includes />
    <excludes />
    <do />
    <group />
    <!--Structured Fileset-->
  </sourcefiles>
  <assemblies usedefaultassemblies="" append="" name="" optionset="" defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" if="" unless="">
    <includes />
    <excludes />
    <do />
    <group />
    <!--Structured Fileset-->
  </assemblies>
  <dlls append="" name="" optionset="" defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" if="" unless="">
    <includes />
    <excludes />
    <do />
    <group />
    <!--Structured Fileset-->
  </dlls>
  <resourcefiles prefix="" resourcebasedir="" includedirs="" defines="" append="" name="" optionset="" defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" if="" unless="">
    <includes />
    <excludes />
    <do />
    <group />
    <!--Structured Fileset-->
  </resourcefiles>
  <custombuildfiles if="" unless="" basedir="" fromfileset="" failonmissing="" name="" append="">
    <includes />
    <excludes />
    <do />
    <group />
  </custombuildfiles>
  <nonembed-resourcefiles append="" name="" optionset="" defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" if="" unless="">
    <includes />
    <excludes />
    <do />
    <group />
    <!--Structured Fileset-->
  </nonembed-resourcefiles>
  <contentfiles append="" name="" optionset="" defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" if="" unless="">
    <includes />
    <excludes />
    <do />
    <group />
    <!--Structured Fileset-->
  </contentfiles>
  <webreferences append="" fromoptionset="">
    <option />
    <!--Structured Optionset-->
  </webreferences>
  <modules append="" name="" optionset="" defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" if="" unless="">
    <includes />
    <excludes />
    <do />
    <group />
    <!--Structured Fileset-->
  </modules>
  <comassemblies append="" name="" optionset="" defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" if="" unless="">
    <includes />
    <excludes />
    <do />
    <group />
    <!--Structured Fileset-->
  </comassemblies>
  <config>
    <buildoptions if="" unless="" fromoptionset="">
      <option />
    </buildoptions>
    <defines if="" unless="" value="" append="">
      <do />
      <!--Text-->
    </defines>
    <additionaloptions if="" unless="" value="" append="">
      <do />
      <!--Text-->
    </additionaloptions>
    <copylocal if="" unless="" value="" append="">
      <do />
      <!--Text-->
    </copylocal>
    <platform if="" unless="" value="" append="">
      <do />
      <!--Text-->
    </platform>
    <allowunsafe if="" unless="" value="" append="">
      <do />
      <!--Text-->
    </allowunsafe>
    <targetframeworkversion />
    <languageversion if="" unless="" value="" append="">
      <do />
      <!--Text-->
    </languageversion>
    <usewpf if="" unless="" value="" append="">
      <do />
      <!--Text-->
    </usewpf>
    <usewindowsforms if="" unless="" value="" append="">
      <do />
      <!--Text-->
    </usewindowsforms>
    <targetframeworkfamily if="" unless="" value="" append="">
      <do />
      <!--Text-->
    </targetframeworkfamily>
    <generateserializationassemblies if="" unless="" value="" append="">
      <do />
      <!--Text-->
    </generateserializationassemblies>
    <suppresswarnings if="" unless="" value="" append="">
      <do />
      <!--Text-->
    </suppresswarnings>
    <warningsaserrors.list if="" unless="" value="" append="">
      <do />
      <!--Text-->
    </warningsaserrors.list>
    <warningsaserrors if="" unless="" value="" append="">
      <do />
      <!--Text-->
    </warningsaserrors>
    <warningsnotaserrors.list if="" unless="" value="" append="">
      <do />
      <!--Text-->
    </warningsnotaserrors.list>
    <remove>
      <defines if="" unless="" value="" append="">
        <do />
        <!--Text-->
      </defines>
      <options if="" unless="" value="" append="">
        <do />
        <!--Text-->
      </options>
      <echo />
      <do />
    </remove>
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
  </visualstudio>
  <platforms if="" unless="">
    <failonerror />
    <verbose />
  </platforms>
  <nugetreferences append="" name="" optionset="" defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" if="" unless="">
    <includes />
    <excludes />
    <do />
    <group />
    <!--Structured Fileset-->
  </nugetreferences>
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
</CSharpLibrary>
```
