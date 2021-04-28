---
title: Build Config Settings Related Questions
weight: 305
---

This page lists frequently asked questions related to modifying the default build settings provided by the standard config packages (such as adding custom defines, adding compiler switches, changing compiler switches, etc) .

<a name="DetectDebugConfig"></a>
## Is there a recommended way for me to detect if the current config is a debug config? ##

Many people attempt to detect debug config by looking for the &#39;debug&#39; keyword in a config name. This method is not that reliable as many teams have their own custom config name
definition which might not have that keyword in the config name. A more reliable way to test for debug configs is to test for the following condition:


```xml
<do if="@{OptionSetGetValue('config-options-common','debugflags')} == 'on'">
. . .
</do>
```
<a name="AddingCustomOptions"></a>
## How Do I Add Custom Defines/Compiler/Linker Options? ##

This can be done by either creating a custom build type (if you have to change a lot of modules in the exact same manner) or
by specifying this custom option directly in your module definition (if the change is only applicable to a single module).

To create a custom define in a custom build type that can be used by multiple modules, please see [Creating Custom Buildtype]( {{< ref "reference/packages/build-scripts/buildtype-and-options/createcustombuildtype.md#CreateCustomBuildtype" >}} )  section and  [How to create custom buildtype optionset]( {{< ref "reference/packages/structured-xml-build-scripts/customoptions.md#CustomBuildType" >}} ) section for more info.

To create a custom define for a specific module, please see [Setting custom build options in a module]( {{< ref "reference/packages/structured-xml-build-scripts/customoptions.md#SXMLSetCustomBuildOptions" >}} ) section for more info.

<a name="RemovingReplacingKnownCompilerSwitch"></a>
## How Do I Remove/Replace Known Compiler Switch? ##

Removing or replacing an existing compiler options is a bit more complicated if you want to do it at the Custom BuildType level.
Because the full list of defines, compile, and link options haven&#39;t been constructed yet when you provided your Custom BuildType,
you will have to do the removal / replacement after the BuildType task is completed.  Please see [Removing or changing options in custom build type]( {{< ref "reference/packages/build-scripts/buildtype-and-options/createcustombuildtype.md#RemovingChangingOptions" >}} ) section for more info.
page for further details.

Making changes at a module definition level is much simpler as syntax have been added to help you remove existing options.  Please
see the [Changing existing build options in a module]( {{< ref "reference/packages/structured-xml-build-scripts/customoptions.md#SXMLChangeExistingBuildOptions" >}} ) section for more info.

<a name="AvailableOptionFlags"></a>
## When I define custom BuildType, what is the available list of option flags that eaconfig support? ##

When defining your custom BuildType, aside from modifying the &quot;buildset.cc.options&quot;, etc. directly, a number of flags are provided
to help you set the compiler/linker options independent of platforms so that you don&#39;t need to worry about finding the appropriate switch for each platform.
Please go to this link [Available flags and options]( {{< ref "reference/packages/build-scripts/buildtype-and-options/createcustombuildtype.md#BuildTypeAvailableFlags" >}} ) to get the full list of flags that are available.

<a name="HowToSetCustomWarningsOptimizations"></a>
## How do I provide my own custom warnings/optimization options? ##

Some of the build option flags that Framework supported (such as optimization and warnings) allow you to further customize the
exact set of switches you like to build with. Instead of setting those flags to &#39;on&#39; or &#39;off&#39;, you set them as &#39;custom&#39; and then provide your own compiler/linker switches
using the option names &#39;&lt;flag_name&gt;.custom.cc&#39; and &#39;&lt;flag_name&gt;.custom.link&#39;. For example:


```xml
<BuildType name="MyProgramBuildType" from="Program">

  <!-- Setting optimization to custom and further define your own switches. -->
  <option name="optimization" value="custom" if="${config-compiler}==vc"/>
  <option name="optimization.custom.cc" if="${config-compiler}==vc and @{OptionSetGetValue('config-options-common','debugflags')} == 'on'">
    /O2
    /Oi
    /GL
    /GS
    /Gy
  </option>
  <option name="optimization.custom.cc" if="${config-compiler}==vc and @{OptionSetGetValue('config-options-common','debugflags')} != 'on'">
    /Od
    /Gm
    /GS
  </option>
  <option name="optimization.custom.link" if="${config-compiler}==vc and @{OptionSetGetValue('config-options-common','debugflags')} == 'on'">
    /OPT:REF
    /OPT:ICF
    /LTCG
    /DYNAMICBASE
  </option>

  <!-- Setting warnings to custom and further define your own switches. -->
  <option name="warnings" if="${config-compiler}==vc" value="custom"/>
  <option name="warnings.custom.cc" if="${config-compiler}==vc" value="/W4"/>

  . . .

</BuildType>
```
<a name="ChangingBuildOptionsInSmallSetOfFiles"></a>
## Can I change build options for a small set of files in my module? ##

You can change the compiler setting for just a small subset of files in your module by providing a custom optionset for those files during the include statement.  Here&#39;s an example:


```xml
<BuildType name="NoWarningsOptionset" from="Library">
  <option name="warnings" value="off"/>
</BuildType>
. . .
<Library name="MyLibrary">
  . . .
  <sourcefiles>
    <includes name="${package.YOUR_PACKAGE_NAME.dir}/source/library/*.cpp">
    <includes name="${package.configbuilddir}/ThirdPartyAutoGeneratedFiles/*.cpp" optionset="NoWarningsOptionset">
  </sourcefiles>
  . . .
</Library>
```
<a name="HowToBuildWithOwnCompiler"></a>
## How do I specify my own compiler for a specific set of files? ##

Your build project sometimes may require use of special compiler to build a special set of files to create new obj / new cpp
files to add to your module.  You do this by listing those special files under &quot;custombuildfiles&quot; fileset instead of
&quot;sourcefiles&quot; fileset in your module definition, then assign a custom build option set for those specific files.
More details on how to do this can be found in the [Custom Build Steps on Individual Files]( {{< ref "reference/packages/build-scripts/native-c/c++-modules/custom-build-files.md" >}} ) page.

<a name="CopyLocalPdbFiles"></a>
## How do I setup copylocal to copy PDB files along with DLL files? ##

Copylocal will copy the files listed in a fileset called &quot;dlls&quot; in the initialize.xml file.
To ensure that PDB files are copied as well, add the PDB files to the same fileset.

###### Structured XML Example ######

```xml
<publicdata packagename="tempdep">
  <module name="templibrary">
    <dlls>
      <includes name="${package.tempdep.builddir}/${config}/bin/templibrary${dll-suffix}" />
      <includes name="${package.tempdep.builddir}/${config}/bin/templibrary.pdb" />
    </dlls>
  </module>
</publicdata>
```
###### Original Syntax Example ######

```xml
<fileset name="package.tempdep.dlls">
    <includes name="${package.tempdep.builddir}/${config}/bin/templibrary${dll-suffix}" />
    <includes name="${package.tempdep.builddir}/${config}/bin/templibrary.pdb" />
</fileset>
```

##### Related Topics: #####
-  [How to create custom buildtype optionset]( {{< ref "reference/packages/structured-xml-build-scripts/customoptions.md#CustomBuildType" >}} ) 
-  [Setting custom build options in a module]( {{< ref "reference/packages/structured-xml-build-scripts/customoptions.md#SXMLSetCustomBuildOptions" >}} ) 
