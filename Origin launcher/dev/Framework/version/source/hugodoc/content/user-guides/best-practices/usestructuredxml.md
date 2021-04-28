---
title: Use Structured XML
weight: 24
---

Use structured XML syntax when creating new packages, convert existing packages to structured XML.

<a name="UseStructuredXML"></a>
## Use Structured XML input ##

 [Structured XML]( {{< ref "reference/packages/structured-xml-build-scripts/_index.md" >}} ) is a collection of NAnt tasks that convert XML input into traditional Framework properties, filesets, and optionsets

Advantages of Structured XML

 - Simplifies input
 - Intellisense helps to avoid errors and find available inputs
 - Has more built in verification of input data

<a name="StructuredXMLTypes"></a>
## How to define different buildtypes in SXML ##

<a name="SXMLNative"></a>
### &lt;Module&gt; task for native input. ###

Specify module name and buildtype:

![Native Module Definition]( nativemoduledefinition.png )<a name="SXMLPredefinedTypes"></a>
### There is number of elements with predefined Native SXML buildtypes ###

See full list of predefined types  [Structured XML build scripts]( {{< ref "reference/packages/structured-xml-build-scripts/_index.md#StructuredXMLTaskHierarchy" >}} )  and list of available build types  [buildtype and options]( {{< ref "reference/packages/build-scripts/buildtype-and-options/_index.md" >}} ) . 

![PredefinedSXMLTypes]( predefinedsxmltypes.png )Buildtype can be explicitly changed:


```xml

<Library name="XXXX">
   <buildtype name="DynamicLibrary" if="${Dll}"/>
   . . . .
   </Library>

```
<a name="SXMLUtility"></a>
### Utility SXML types ###

You can define various utility modules using SXML (for more info see  [Structured XML build scripts]( {{< ref "reference/packages/structured-xml-build-scripts/_index.md#StructuredXMLTaskHierarchy" >}} )  in reference section. 

![UtilitySXMLTypes]( utilitysxmltypes.png )<a name="SXMLDotNetPython"></a>
### .Net and Python are also supported. ###


```xml

 <CSharpLibrary name="XXXX" >
 <CSharpProgram name="XXXX" >
 <CSharpWindowsProgram name="XXXX" >
               
 <PythonProgram name="XXXX" >

```
See reference section [buildtype and options]( {{< ref "reference/packages/build-scripts/buildtype-and-options/_index.md" >}} ) for the full list of available build types.

![CSharp Example]( csharpexample.png )<a name="SXMLGroups"></a>
### Test, Example, Tool, Runtime groups ###

In SXML different groups (tool, example, test) can be conveniently defined inside corresponding XML elements:

For more info see  [BuildGroups]( {{< ref "reference/packages/buildgroups.md" >}} ) 

![Test Example Tool Runtime]( testexampletoolruntime.png )<a name="StructuredXMLBuildsettings"></a>
## Structured XML provides easier way to customize buildoptions ##

<a name="SXMLCustomConfigOptions"></a>
### Custom configuration options in SXML ###

For more information see [How to define custom build options in a module]( {{< ref "reference/packages/structured-xml-build-scripts/customoptions.md" >}} ) 

![Custom Cofiguration Options]( customcofigurationoptions.png )<a name="SXMLRemoveOptions"></a>
### Replacing and removing options from a standard optionset is easier in SXML ###

For more information see [How to define custom build options in a module]( {{< ref "reference/packages/structured-xml-build-scripts/customoptions.md" >}} ) and [Create Custom BuildType]( {{< ref "reference/packages/build-scripts/buildtype-and-options/createcustombuildtype.md" >}} ) 

![Remove Options]( removeoptions.png )<a name="SXMLBuildType"></a>
### Use &#39;BuildType&#39; task to create new custom optionset to re-use  it in multiple modules ###

For more information see [How to define custom build options in a module]( {{< ref "reference/packages/structured-xml-build-scripts/customoptions.md" >}} ) and [Create Custom BuildType]( {{< ref "reference/packages/build-scripts/buildtype-and-options/createcustombuildtype.md" >}} ) 

![Build Type Task]( buildtypetask.png )<a name="StructuredXMLPartialModules"></a>
## Partial Modules ##

Partial modules can provide definitions of data that are can be shared between multiple modules that are derived from a partial module.
     Partial modules can be defined in a game configuration package, or can be exported by a package Initialize.xml file.
For example Partial modules are commonly used in Frostbite and are defined in FbConfig for things like extension modules that are mostly the same.

See also  [Partial Modules]( {{< ref "reference/packages/build-scripts/partialmodules.md" >}} ) 

![Partial Module Use]( partialmoduleuse.png )![Partial Module Define]( partialmoduledefine.png )<a name="SXMLPartialModuleAndOtherExtensions"></a>
### Partial modules and other extensions ###

 - Partial modules available for Native and .Net SXML
 - Partial modules can be defined in Initialize.xml to allow other packages use them
 - In case Partial Modules functionality is not enough, new nant task can be inherited from Module nant task to extend structured XML.

