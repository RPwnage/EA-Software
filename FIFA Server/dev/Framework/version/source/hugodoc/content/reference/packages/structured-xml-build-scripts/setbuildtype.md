---
title: How to set module 'buildtype'
weight: 202
---

How to set module build type in Structured XML

<a name="SXMLSetBuildType"></a>
## Setting buildtype in native modules ##

For convenience and readability Structured XML provides tasks with predefined initial values of build types, see also [Structured XML build scripts]( {{< ref "reference/packages/structured-xml-build-scripts/_index.md#StructuredXMLTaskHierarchy" >}} ) .
Initial predefined build type values can be changed if necessary. All modules except &quot;Utility&quot;, and &quot;MakeStyle&quot; support ` *buildtype* ` attribute and ` *buildtype* ` element.


{{% panel theme = "info" header = "Note:" %}}
Structured XML allows to define custom build options without explicitly changing build type
(see [How to define custom build options in a module]( {{< ref "reference/packages/structured-xml-build-scripts/customoptions.md" >}} )  and  [buildtype and options]( {{< ref "reference/packages/build-scripts/buildtype-and-options/_index.md" >}} ) ).

Use buildtype settings for predefined buildtypes, or when you have custom buildtype shared between multiple modules.
{{% /panel %}}
###### Assign new buildtype using attribute ######

```xml

          
  <Library name="MyLibrary" buildtype="CustomLibrary">
    <sourcefiles>
    <includes name="src/**.cpp"/>
    <sourcefiles/>
    . . . .
  </Library>
            

```
XML attributes do not allow you to use conditions. When a choice between several build types is needed use the buildtype element `<buildtype name="" if="" unless=""/>` :

###### Assign new buildtype using element ######

```xml

          
  <Library name="MyLibrary">
    <buildtype name="DynamicLibrary" if="${Dll}"/>
    <buildtype name="ManagedCppAssembly" if="${config-system}==pc"/>
    <sourcefiles>
    <includes name="src/**.cpp"/>
    <sourcefiles>
    . . . .
  </Library>
            

```

{{% panel theme = "info" header = "Note:" %}}
&#39;buildtype&#39; element value overwrites &#39;buildtype&#39; attribute value.
{{% /panel %}}

{{% panel theme = "info" header = "Note:" %}}
When multiple &#39;buildtype&#39; elements are present the last build type element with condition evaluated to true will be used.
{{% /panel %}}
 ` *buildtype* `  element can be used with  ` *buildtype* ` attribute in the same module definition. Element value overrides attribute value. Both of the following definitions are equivalent:


```xml

          
  <Library name="MyLibrary" buildtype="CustomLibrary">
    <buildtype name="DynamicLibrary" if="${Dll}"/>
    <buildtype name="ManagedCppAssembly" if="${config-system}==pc"/>
    <sourcefiles>
    <includes name="src/**.cpp"/>
    <sourcefiles>
    . . . .
  </Library>
            
            
  <Library name="MyLibrary">
    <buildtype name="LibraryWithRtti" />
    <buildtype name="DynamicLibrary" if="${Dll}"/>
    <buildtype name="ManagedCppAssembly" if="${config-system}==pc"/>
    <sourcefiles>
    <includes name="src/**.cpp"/>
    <sourcefiles>
    . . . .
  </Library>
            
            

```
Because there is no difference between `<Module>`  and  `<Program>`  or  `<Library>` except initial buildtype setting, it makes sense to use appropriate task to reflect real build type. Formally the script below will work, but it is bad practice:

###### Bad practice ######

```xml

          
  <Library name="ModuleA" buildtype="Program">
   . . .
  </Library>


  <Library name="ModuleB">
    <buildtype name="Program" if="${build-as-program}"/>
   . . .
  </Library>

            

```
Examples below reflects intension better:

###### Good practice ######

```xml

          
  <Program name="ModuleA">
   . . .
  </Program>

  <Module name="ModuleB">
    <buildtype name="Library" unless="${build-as-program}"/>
    <buildtype name="Program" if="${build-as-program}"/>
   . . .
  </Module>

```
<a name="SXMLSetBuildTypeForDotNet"></a>
## Setting buildtype in DotNet modules ##

Buildtypes are rarely changed in .DotNet modules because most of the options can be set through properties.
Nevertheless, structured XML supports setting build types for DotNet modules the same way it is done for native modules.

Following predefined tasks exist for DotNet modules

 - `<CSharpLibrary>`
 - `<CSharpProgram>`
 - `<CSharpWindowsProgram>`

###### Good practice ######

```xml

          
  <BuildType name="CustomCSharpLibrary" from="CSharpLibrary">
    <option name="warningsaserrors"         value="off"/>
  </BuildType>
          
          
  <CSharpLibrary name="MyAssembly" buildtype="CustomCSharpLibrary">
   . . .
  </Program>


```

##### Related Topics: #####
-  [How to define custom build options in a module]( {{< ref "reference/packages/structured-xml-build-scripts/customoptions.md" >}} ) 
-  [buildtype and options]( {{< ref "reference/packages/build-scripts/buildtype-and-options/_index.md" >}} ) 
