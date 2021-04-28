---
title: warningsaserrors
weight: 230
---

This page explains the properties that are available for controlling warnings as errors.

<a name="overview"></a>
## Overview ##

In framework builds we often treat warnings as errors in a effort to maintain high coding standards.
This option is part of the buildtype optionset and can be controlled by creating a custom buildtype.
However, sometimes there is a need to control it globally and we have added global properties for these situations.

<a name="global"></a>
## Globally Enabling/Disabling Warnings as Errors ##

Warnings as errors can be enabled/disabled globally using the boolean global property eaconfig.warningsaserrors.


```xml
<globalproperties>
 eaconfig.warningsaserrors=false
</globalproperties>
```
<a name="package-global"></a>
## Enabling/Disabling Warnings as Errors for Specific Packages Globally ##

Occassionally there is a need to disable warnings as errors because a specific package starts displaying a warning.
Often in cases where this happens it requires the package to be fixed and a new release made.
If the warning is non-critical and the package is owned by another team it might take time for a new release to be published.
In the past users would have disabled warningsaserrors globally but this would also hide warnings in other packages that the users might need to see.
So now we have a feature for setting the warningsaserror state for individual packages using global properties.

There are two global properties used to control the state of warningsaserrors.
eaconfig.warningsaserrors.packages.includes is a list of package names which are all set with warningsaserrors set to &#39;true&#39;.
eaconfig.warningsaserrors.packages.excludes is another list of packages for which warningsaserrors is set to &#39;false&#39;.

The global property &quot;eaconfig.warningsaserrors&quot; can still be used to set warningsaserrors on or off globally for all modules.
It is applied to all packages before the includes/excludes properties.
This means if you want to set warningsaserrors &#39;off&#39; in general but &#39;on&#39; for one package you can use this property and then use
the includes property with the name of the one package that needs to be enabled.

The following example is from the global properties section of a masterconfig file.
In this example the user wants warnings to be treated as errors for mypackage (the top level package) and depA (a dependency).
However, the user does not want a particular error that has started appearing in depB to fail the build.


```xml
<globalproperties>
 <globalproperty name="eaconfig.warningsaserrors.packages.includes">
  mypackage
  depA
 </globalproperty>
 <globalproperty name="eaconfig.warningsaserrors.packages.excludes">
  depB
 </globalproperty>
</globalproperties>
```
<a name="package-local"></a>
## Controlling Warnings As Errors On The Package Level ##

### Native ###

Native modules allow package level control of whether the appropriate warningsaserrors flag is passed to the compiler.

Below is an example of how to control warnings using structured xml.


```xml
<Program name="MyModule">
 <config>
  <!-- Whether or not warnings as errors should be turned on for this module -->
  <buildoptions>
   <option name="warningsaserrors" value="on"/>
  </buildoptions>
 </config>
 <sourcefiles basedir="${package.dir}/source">
  <includes name="${package.dir}/source/*.cpp" />
 </sourcefiles>
</Program>
```
### DotNet ###

Dot Net Modules allow finer control of warnings as errors on the package level.
Specific warnings can be set as errors or warnings or suppressed.

Additional info about these options can be found here: [Options]( {{< ref "reference/packages/build-scripts/dotnet-c-and-f-modules/options.md" >}} ) 

Below is an example of how to control warnings using structured xml.


```xml
<CSharpProgram name="MyModule">
 <config>
  <!-- Whether or not warnings as errors should be turned on for this module -->
  <warningsaserrors>on</warningsaserrors>
  <!-- A list of specific warnings to set as errors -->
  <warningsaserrors.list>
   217
  </warningsaserrors.list>
  <!-- A list of specific warnings that should not be treated as errors -->
  <warningsnotaserrors.list>
   219
   218
  </warningsnotaserrors.list>
  <!-- A list of specific warnings that should be suppressed and not appear at all -->
  <suppresswarnings>
   220
  </suppresswarnings>
 </config>
 <sourcefiles basedir="${package.dir}/source">
  <includes name="${package.dir}/source/*.cs" />
 </sourcefiles>
</CSharpProgram>
```
