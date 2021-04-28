---
title: Build Setting Properties
weight: 219
---

The easiest way to customize build type settings is to use the build setting global properties described in this topic.

<a name="HelperProperties"></a>
## Usage ##

Build Setting properties are often used to alter buildtype flags that translate into compiler or linker options like `optimization` .
These properties should be set before build types are generated. To affect standard build types from the configuration package these properties should be set
before the configuration package is loaded, which means they should be set before the `<package>` task in a package&#39;s build script,
or in the masterconfig global properties to affect all packages in a build.

There are properties that, if set, will override options set in the build type. These properties are defined per module and don&#39;t need to be defined before
the buildtype optionsets are generated.

<a name="ChangingBuildtypeFlags"></a>
### Changing buildtype flags ###

The following properties will affect buildtype flags when set before build type is generated.  Most of these `eaconfig.[option_name]` properties control the [option_name] option flag directly.
For detail description of each option flag, please check this [options list table]( {{< ref "reference/packages/build-scripts/buildtype-and-options/createcustombuildtype.md#BuildTypeAvailableFlags" >}} ) .

 - `eaconfig.debugflags` = &quot;on&quot;/&quot;off&quot;/&quot;custom&quot;
 - `eaconfig.debugsymbols` - &quot;on&quot;/&quot;off&quot;/&quot;custom&quot;
 - `eaconfig.optimization` - &quot;on&quot;/&quot;off&quot;/&quot;custom&quot;
 - `eaconfig.usedebuglibs` - &quot;on&quot;/&quot;off&quot;/&quot;custom&quot;.
 - `package.eaconfig.usedebugfastlink` - &quot;true&quot;/&quot;false&quot;.<br><br>This property controls creation of buildtype option flag `debugfastlink` .  If this property is<br>set to true and you are using Visual Studio 2015, the buildtype option flag `debugfastlink` will be created and set to &#39;on&#39;.  See [options list table]( {{< ref "reference/packages/build-scripts/buildtype-and-options/createcustombuildtype.md#BuildTypeAvailableFlags" >}} ) for detail description of the option flag `debugfastlink` .
 - `package.eaconfig.disablecomdatfolding` - &quot;true&quot;/&quot;false&quot;.<br><br>Allows Explicit disabling of comdat folding with the -opt:noicf linker flag.<br>Comdat folding is finding functions and constants with duplicate values and merging (folding) them.<br>As a side effect builds with &quot;-OPT:noicf&quot; have improved debugging in common functions because it can be<br>unsettling to step into a different function in a different module while debugging (even if that function<br>generates the same executable code)<br>On Madden This saves ~20s link time at a cost of about 9MB of wasted runtime memory.
 - `package.eaconfig.printlinktime` - &quot;true&quot;/&quot;false&quot;.<br><br>This prints timing information about each of the linker commands and can be useful for debugging and optimizing link times.
 - `eaconfig.warnings` - &quot;on&quot;/&quot;off&quot;/&quot;custom&quot;
 - `eaconfig.warninglevel` - &quot;high&quot;/&quot;medium&quot;/&quot;pedantic&quot;
 - `eaconfig.${config-platform}.warningsuppression`
 - `eaconfig.${config-platform}.default_warning_suppression` - eaconfig can apply several sets of warning suppression options: `on` , `strict` , `custom` , `off` <br><br>This property does not have corresponding option in buildtype optionset.  It controls the following behaviour:<br><br>  - `on` - Will enable all warnings, apply default set of warning suppression options,<br>plus it will add value of property `eaconfig.${config-platform}.warningsuppression` .<br>The property is currently used to export extra warning suppression flags from SDK packages.<br>  - `strict` - Will apply smaller set of default warning suppression options.<br>  - `custom` - Will use warning suppression options defined in property `eaconfig.${config-platform}.warningsuppression` .<br>  - `off` - Will NOT enable all warnings or add any other warning related flags.
 - `eaconfig.misc` - &quot;on&quot;/&quot;off&quot;/&quot;custom&quot;
 - `eaconfig.warningsaserrors` - &quot;on&quot;/&quot;off&quot;
 - `eaconfig.rtti` - &quot;on&quot;/&quot;off&quot;
 - `eaconfig.exceptions` - &quot;on&quot;/&quot;off&quot;
 - `eaconfig.toolconfig` - &quot;on&quot;/&quot;off&quot;
 - `eaconfig.incrementallinking` - &quot;on&quot;/&quot;off&quot;
 - `eaconfig.generatemapfile` - &quot;on&quot;/&quot;off&quot;
 - `eaconfig.generatedll`
 - `eaconfig.multithreadeddynamiclib` - &quot;on&quot;/&quot;off&quot;
 - `eaconfig.managedcpp` - &quot;on&quot;/&quot;off&quot;
 - `eaconfig.banner` - &quot;on&quot;/&quot;off&quot;
 - `eaconfig.editandcontinue` - &quot;on&quot;/&quot;off&quot;
 - `eaconfig.c7debugsymbols` - &quot;on&quot;/&quot;off&quot;
 - `eaconfig.standardsdklibs` - &quot;on&quot;/&quot;off&quot;/&quot;custom&quot;
 - `eaconfig.runtimeerrorchecking` - &quot;on&quot;/&quot;off&quot;
 - `eaconfig.pcconsole` - &quot;on&quot;/&quot;off&quot;
 - `eaconfig.profile` - &quot;on&quot;/&quot;off&quot;
 - `eaconfig.uselibrarydependencyinputs` - &quot;on&quot;/&quot;off&quot;
 - `eaconfig.optimization.ltcg` - &quot;on&quot;/&quot;off&quot;
 - `eaconfig.trace` = &quot;on&quot;/&quot;off&quot;<br><br>Controls whether the TRACE define is added on C#.

<a name="OptionHelperProperties"></a>
### Helper properties to alter options ###

The following helper properties can be defined to alter settings

 - {{< url groupname >}} `.warningsuppression` adds warning suppression options to compiler input.<br><br>Instead of defining a custom build type, additional warning suppression flags for a package/module can be added using this property<br><br><br>{{% panel theme = "info" header = "Note:" %}}<br>value of this property is added to the default set defined by eaconfig (see  `eaconfig.default_warning_suppression` property).<br>{{% /panel %}}<br><br>```xml<br><Program name="MyModule"><br> <config><br>  <warningsuppression><br>   -wd4548<br>  </warningsuppression><br> </config><br></Program><br>```

