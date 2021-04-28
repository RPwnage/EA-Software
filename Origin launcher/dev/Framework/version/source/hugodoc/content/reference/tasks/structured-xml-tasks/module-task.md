---
title: < module > Task
weight: 1062
---
## Syntax
```xml
<module name="" outputname="" processors="" add-defaults="" exclude-from-solution="" libnames="" buildtype="">
  <defines />
  <includedirs />
  <includedirs.internal />
  <publicdependencies />
  <publicbuilddependencies />
  <internaldependencies />
  <libdirs />
  <usingdirs />
  <libs basedir="" fromfileset="" failonmissing="" name="" append="" />
  <libs-external basedir="" fromfileset="" failonmissing="" name="" append="" />
  <dlls basedir="" fromfileset="" failonmissing="" name="" append="" />
  <dlls-external basedir="" fromfileset="" failonmissing="" name="" append="" />
  <assemblies basedir="" fromfileset="" failonmissing="" name="" append="" />
  <assemblies-external basedir="" fromfileset="" failonmissing="" name="" append="" />
  <natvisfiles basedir="" fromfileset="" failonmissing="" name="" append="" />
  <sndbs-rewrite-rules-files basedir="" fromfileset="" failonmissing="" name="" append="" />
  <programs basedir="" fromfileset="" failonmissing="" name="" append="" />
  <assetfiles defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" />
  <contentfiles defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" />
  <java.archives />
  <java.classes />
  <resourcedir />
  <java.rfile />
  <java.packagename />
  <echo />
  <do />
</module>
```
## Summary ##
A structured XML module information export specification to be used in Initialize.xml file (not to be confused with the &lt;Module&gt; task for the .build file)


### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| name | Name of the module. | String | True |
| outputname | The default output name is based on the module name.  This attribute tells Framework to remap the output name to something different. | String | False |
| processors | List of supported processors to be used in adding default include directories and libraries.Default include directories |<br>--- |<br>| &quot;${package.dir}}/include&quot; | <br>| &quot;${package.dir}}/include_${processor}&quot; | <br><br>Default libraries |<br>--- |<br>| &quot;${lib-prefix}${module}${lib-suffix}&quot; | <br>| &quot;${lib-prefix}${module}_${processor}${lib-suffix}&quot; | <br><br> | String | False |
| add-defaults | (Deprecated) If set to true default values for include directories and libraries will be added to all modules. | Boolean | False |
| exclude-from-solution | If set to true the project will not be added to the solution during solution generation.  This will only really work for utility style modules since modules with actual build dependency requirements will fail visual studio build dependency requirements. | Boolean | False |
| libnames | List of additional library names to be added to default set of libraries. Can be overwritten by &#39;libnames&#39; attribute in &#39;module&#39; task.<br>See documentation on Modules for explanation when default values are used.Default libraries |<br>--- |<br>| &quot;${lib-prefix}${module}${lib-suffix}&quot; | <br>| &quot;${lib-prefix}${libname}${lib-suffix}&quot; | <br><br> | String | False |
| buildtype | Sets the buildtype used by this module. If specified Framework will try to work out which outputs (libs, dlls, etc) to automatically export. | String | False |
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | Boolean | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

### Nested Elements
| Name | Description | Type | Required |
| ---- | ----------- | ---- | -------- |
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "defines" >}}| Set preprocessor defines exported by this module | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "includedirs" >}}| Set include directories exported by this module. Empty element will result in default include directories added. | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | False |
| {{< task "NAnt.Core.Structured.DeprecatedPropertyElement" "includedirs.internal" >}}| Set internal include directories exported by this module. | {{< task "NAnt.Core.Structured.DeprecatedPropertyElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "publicdependencies" >}}| Declare packages or modules that modules which have an interface dependency on this module will also need to depend on. | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "publicbuilddependencies" >}}| Declare packages or modules that modules which have a build dependency on this module will also need to depend on. | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | False |
| {{< task "NAnt.Core.Structured.DeprecatedPropertyElement" "internaldependencies" >}}| Declare packages or modules that modules which have an internal dependency on this module will also need to internally depend on. | {{< task "NAnt.Core.Structured.DeprecatedPropertyElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "libdirs" >}}| Set library directories exported by this module | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "usingdirs" >}}| Set &#39;using&#39; (/FU) directories exported by this module | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.StructuredFileSetCollection" "libs" >}}| Set libraries exported by this module. If &lt;libs/&gt; is present and empty, default library values are added. | {{< task "EA.Eaconfig.Structured.StructuredFileSetCollection" >}} | False |
| {{< task "EA.Eaconfig.Structured.StructuredFileSetCollection" "libs-external" >}}| Set libraries (external to current package) exported by this module. | {{< task "EA.Eaconfig.Structured.StructuredFileSetCollection" >}} | False |
| {{< task "EA.Eaconfig.Structured.StructuredFileSetCollection" "dlls" >}}| Set dynamic libraries exported by this module. If &lt;dlls/&gt; is present and empty, default dynamic library values are added. | {{< task "EA.Eaconfig.Structured.StructuredFileSetCollection" >}} | False |
| {{< task "EA.Eaconfig.Structured.StructuredFileSetCollection" "dlls-external" >}}| Set dynamic libraries (external to current package) exported by this module. | {{< task "EA.Eaconfig.Structured.StructuredFileSetCollection" >}} | False |
| {{< task "EA.Eaconfig.Structured.StructuredFileSetCollection" "assemblies" >}}| Set assemblies exported by this module. If &lt;assemblies/&gt; is present and empty, default assembly values are added. | {{< task "EA.Eaconfig.Structured.StructuredFileSetCollection" >}} | False |
| {{< task "EA.Eaconfig.Structured.StructuredFileSetCollection" "assemblies-external" >}}| Set assemblies (external to current package) exported by this module. | {{< task "EA.Eaconfig.Structured.StructuredFileSetCollection" >}} | False |
| {{< task "EA.Eaconfig.Structured.StructuredFileSetCollection" "natvisfiles" >}}| Set natvis files exported by this module. | {{< task "EA.Eaconfig.Structured.StructuredFileSetCollection" >}} | False |
| {{< task "EA.Eaconfig.Structured.StructuredFileSetCollection" "sndbs-rewrite-rules-files" >}}| Set full path to rewrite rules ini files that provide computed-include-rules for SN-DBS. See SN-DBS help documentation regarding &quot;Customizing dependency discovery.&quot; | {{< task "EA.Eaconfig.Structured.StructuredFileSetCollection" >}} | False |
| {{< task "EA.Eaconfig.Structured.StructuredFileSetCollection" "programs" >}}| Set programs exported by this module. Unlike the above filesets, if &lt;programs/&gt; is present and empty, nothing will be added. | {{< task "EA.Eaconfig.Structured.StructuredFileSetCollection" >}} | False |
| {{< task "NAnt.Core.FileSet" "assetfiles" >}}| Set assetfiles exported by this module. | {{< task "NAnt.Core.FileSet" >}} | False |
| {{< task "NAnt.Core.FileSet" "contentfiles" >}}| Set content files exported by this module. | {{< task "NAnt.Core.FileSet" >}} | False |
| {{< task "NAnt.Core.Structured.DeprecatedPropertyElement" "java.archives" >}}| set Java archive (.jar) files exported by this module. This is currently used on Android only. . | {{< task "NAnt.Core.Structured.DeprecatedPropertyElement" >}} | False |
| {{< task "NAnt.Core.Structured.DeprecatedPropertyElement" "java.classes" >}}| set Java class files (compiled .java files) exported by this module. This is currently used on Android only. . | {{< task "NAnt.Core.Structured.DeprecatedPropertyElement" >}} | False |
| {{< task "NAnt.Core.Structured.DeprecatedPropertyElement" "resourcedir" >}}| Set Android resource directories exported by this module. This is often used in Android Extension Libraries  | {{< task "NAnt.Core.Structured.DeprecatedPropertyElement" >}} | False |
| {{< task "NAnt.Core.Structured.DeprecatedPropertyElement" "java.rfile" >}}|  | {{< task "NAnt.Core.Structured.DeprecatedPropertyElement" >}} | False |
| {{< task "NAnt.Core.Structured.DeprecatedPropertyElement" "java.packagename" >}}|  | {{< task "NAnt.Core.Structured.DeprecatedPropertyElement" >}} | False |

## Full Syntax
```xml
<module name="" outputname="" processors="" add-defaults="" exclude-from-solution="" libnames="" buildtype="" failonerror="" verbose="" if="" unless="">
  <defines if="" unless="" value="" append="">
    <do />
    <!--Text-->
  </defines>
  <includedirs if="" unless="" value="" append="">
    <do />
    <!--Text-->
  </includedirs>
  <includedirs.internal />
  <publicdependencies if="" unless="" value="" append="">
    <do />
    <!--Text-->
  </publicdependencies>
  <publicbuilddependencies if="" unless="" value="" append="">
    <do />
    <!--Text-->
  </publicbuilddependencies>
  <internaldependencies />
  <libdirs if="" unless="" value="" append="">
    <do />
    <!--Text-->
  </libdirs>
  <usingdirs if="" unless="" value="" append="">
    <do />
    <!--Text-->
  </usingdirs>
  <libs if="" unless="" basedir="" fromfileset="" failonmissing="" name="" append="">
    <includes />
    <excludes />
    <do />
    <group />
  </libs>
  <libs-external if="" unless="" basedir="" fromfileset="" failonmissing="" name="" append="">
    <includes />
    <excludes />
    <do />
    <group />
  </libs-external>
  <dlls if="" unless="" basedir="" fromfileset="" failonmissing="" name="" append="">
    <includes />
    <excludes />
    <do />
    <group />
  </dlls>
  <dlls-external if="" unless="" basedir="" fromfileset="" failonmissing="" name="" append="">
    <includes />
    <excludes />
    <do />
    <group />
  </dlls-external>
  <assemblies if="" unless="" basedir="" fromfileset="" failonmissing="" name="" append="">
    <includes />
    <excludes />
    <do />
    <group />
  </assemblies>
  <assemblies-external if="" unless="" basedir="" fromfileset="" failonmissing="" name="" append="">
    <includes />
    <excludes />
    <do />
    <group />
  </assemblies-external>
  <natvisfiles if="" unless="" basedir="" fromfileset="" failonmissing="" name="" append="">
    <includes />
    <excludes />
    <do />
    <group />
  </natvisfiles>
  <sndbs-rewrite-rules-files if="" unless="" basedir="" fromfileset="" failonmissing="" name="" append="">
    <includes />
    <excludes />
    <do />
    <group />
  </sndbs-rewrite-rules-files>
  <programs if="" unless="" basedir="" fromfileset="" failonmissing="" name="" append="">
    <includes />
    <excludes />
    <do />
    <group />
  </programs>
  <assetfiles defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" if="" unless="">
    <includes />
    <excludes />
    <do />
    <group />
  </assetfiles>
  <contentfiles defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" if="" unless="">
    <includes />
    <excludes />
    <do />
    <group />
  </contentfiles>
  <java.archives />
  <java.classes />
  <resourcedir />
  <java.rfile />
  <java.packagename />
  <echo />
  <do />
</module>
```
