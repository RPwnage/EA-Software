---
title: Configuration Packages
weight: 259
---

A *configuration package* is a collection of build scripts with common compiler, linker and other settings and definitions of common [Targets]( {{< ref "reference/targets/_index.md" >}} ) and it can contain common tasks.
Configuration Package name and version can be specified in [Masterconfig]( {{< ref "reference/master-configuration-file/_index.md" >}} ) file.
The default configuration package is *eaconfig* and this will be used by default if none is specified. The eaconfig package is included as part of Framework. Additional configuration packages can be layered on top of eaconfig as config extensions. This is the recommended approach rather than using your own configuration package.

<a name=""></a>
## Configuration Loading ##

To understand what needs to be in a configuration package it is important to understand how Framework finds and loads configurations.

Typically the user will provide a configuration when they run framework, for example -D:config=pc-vc-dev-debug.
This configuration name refers to a specific set of platform and compiler settings that the user wants to use when building.

The configuration settings are loaded by Framework when running the &lt;package&gt; task that is at the start of a package&#39;s build script.

The configuration, -D:config=pc-vc-dev-debug, refers to a specific build script file, pc-vc-dev-debug.xml, that exists in a configuration package.
Configuration package versions are listed in the masterconfig file like any other package.
The masterconfig must also specify a &lt;config&gt; block in the masterconfig.
The below &lt;config&gt; block indicates that eaconfig is the base configuration package and android_config and FBConfig are configuration packages that extend eaconfig.

###### the config section of a masterconfig file, indicating the order to load configuration settings ######

```xml
<config package="eaconfig"> <!-- Specifying eaconfig here is optional, if using eaconfig the 'package' attribute can be omitted. -->
 <extension>android_config</extension>
 <extension if="${config-use-FBConfig??true}">FBConfig</extension>
</config>
```
Packages like android_config extend eaconfig by adding new platforms, but typically don't change eaconfig's default behavior.
FBConfig applies frostbite specific changes to the default eaconfig configs, that is why FBConfig has a condition on it in the above example,
so that users can turn off the frostbite settings if they want to build with the original eaconfig settings.

Framework will load settings from the config packages in a specific order, which can be seen in the diagram below.
In the diagram layered boxes indicate that a file by that name is loaded from each config package where such a file exists in the order the config packages are listed in the masterconfig&#39;s extension blocks.
For example, this layering allows FBConfig to override a setting from eaconfig or android_config&#39;s config-common.xml because a config-common.xml file in eaconfig is loaded first, if it exists, then one from android_config is loaded, then the one from FBConfig.
All files in a config extension package are optional and a completely empty config extension package is entirely valid.

A simple overview of the steps preformed by the package task.![config Loading]( configloading.png )Framework will first depend on each package, which causes framework to process the package&#39;s initialize.xml file, in the order they are listed in the masterconfig&#39;s config block.
Then Framework will load a file called global/init.xml immediately after depending on the package which loads common settings before the config file is loaded.
Then Framework will search for the config file in each of the config packages, in the order they are listed in the masterconfig&#39;s config block, until it finds one that matches, for example pc-vc-dev-debug.xml.
Framework will stop searching for the config file as soon as it finds it, so this means packages listed earlier in the config block can override config files in the later packages.
After the config file is loaded platform sepcific settings are loaded, these are separated into the common settings that apply to all configs, followed by settings that apply to a specific platform, for example android-arm64-clang, followed by settings for a sepecific type of config, like a debug config.

The combine step is a part of configuration loading that takes a number of optionsets (config-options-common, config-options-library, etc.) and turns them into build types. (Program, Library, DynamicLibrary, etc.)
Code that is modifying these optionsets will need to occur before the combine step if it wants these settings to be applied properly to the build types, where as scripts that use the build types, such as for defining partial module definitions, will need to happen after the combine step.
That is why Framework allows config packages to provide a pre and post combine build script (pre-combine.xml and post-combine.xml).

Finally many configuration packages provide their own targets.
In order to keep config packages organized Framework has a convention where a file called targets/targets.xml will be loaded automatically as the last step of the config loading process.
The targets.xml file is intend to be where config package&#39;s define their custom targets.
In some cases a config package may define too many targets for a single file and our suggestion is to separate them into other files within the targets folder which are included using the &lt;include&gt; task from the targets.xml file.

<a name="ConfigurationPackage"></a>
## Structure of a Configuration Package ##

Below is the structure of a typical configuration package.
Framework expects files with certain filenames and folders and will load these automatically if it finds them.
For configuration packages that extend eaconfig all files in the config directory are optional and framework will simply load what is provided.


```
/my_config_package
/${version}
 /config
  /global
   init.xml	// The init file is loaded before loading the ${config}.xml file or any other settings from the config package
   pre-combine.xml	// Loaded before running the combine step that creates build types
   post-combine.xml	// Loaded after running the combine step that creates build types
  /platform
   config-common.xml	// contains settings that apply to all platforms in all config packages
   ${config-sytem}[-${config-processor}]-${config-compiler}.xml	// contains settings that apply to a specific platform
  /configset
   ${configset}	// contains settings that apply to a specific type of config like debug or release configs
  /targets
   targets.xml	// contains any common targets defined by the config package
  /tasks	// while not enforced by framework, most config packages place any C# tasks in a folder called tasks
  /options	// while not enforced by framework, it is suggested that GenerateOptions C# scripts be placed in a folder called options
  ${config}.xml	// if the config package provides configs they must be in the config folder
 Initialize.xml	// should be used only for information about the package, initial config loading steps should be done in global/init.xml
 my_config_package.build	// typically just contains a package task since config packages are not usually built directly.
 Manifest.xml
```
<a name="ConfigFile"></a>
## Config file format ##

Config files, like pc-vc-dev-debug.xml in eaconfig, are build scripts but all have a standard format.
All config files are required to define an optionset called &#39;configuration-definition&#39;, which indicates what platform and configset it represents.

###### An example of a configuration file ######

```xml
<project xmlns="schemas/ea/framework3.xsd">
 <optionset name="configuration-definition">
  <option name="config-name"     value="dev-debug" />
  <option name="config-system"   value="android" />
  <option name="config-compiler" value="clang" />
  <option name="config-processor" value="arm64" />

  <option name="debug"           value="on" />
 </optionset>
</project>
```
Framework takes the options config-name, config-system, config-compiler and config-processor and converts them into properties.
It will use these to know which build script contains the configuration settings by loading, platform/${config-system}[-${config-processor}]-${config-compiler}.xml, config-processor is a newer field and is not used by all configurations.

The other options that can be provided are &#39;debug&#39;, &#39;optimization&#39;, &#39;profile&#39;, &#39;retail&#39; and &#39;dll&#39;. These indicate which config set file should be loaded.
For example if debug is specified it will load configset/dev-debug.xml, if debug and dll are specified it will load dll-dev-debug.xml, if debug and optimization are specified it will load dev-debug-opt.xml.
The naming convention used for these files is an old part of framework, the dev part of the name has no real meaning but is impossible for us to remove at this point.
Typically the configset files from eaconfig are the only ones that will ever be loaded.

In addition to the optionset, any arbitrary properties can be defined in the config file and those properties will only apply to that specific config.

<a name="GenerateOptions"></a>
## GenerateOptions ##

Platform specific settings can be provided either in the platform/${config-system}[-${config-processor}]-${config-compiler}.xml file or using a C# GenerateOptions class.
There is not a clear rule for when a setting should be set via xml or via C# and often it just follows the informal convention established by other options and the desired order that options should be defined in.
GenerateOptions is executed as part of the Combine step.

GenerateOptions is a C# class that is loaded and compiled by a &lt;taskdef&gt; at some point during configuration loading.
GenerateOptions files contain a framework Task named either ${config-system}[-${config-processor}]-${config-compiler}-generate-option or GenerateOptions_${config-system}
although the first naming convention is prefered, and the file name typically corresponds to the task name rather than the class name.

Below is an excerpt from a GenerateOptions file as an example, for a better example we encourage you to look at one in eaconfig or one of the other existing configuration packages.

###### An excerpt of a generate options file ######

```csharp
.          [TaskName("kettle-clang-generate-options")]
.          public class GenerateOptions : GeneratePlatformOptions
.          {
.              protected override void SetPlatformOptions()
.              {
.                  AddOption(OptionType.link_Options, "--output \"%linkoutputname%\"");
.
.                  AddOptionIf("optimization", "on", OptionType.cc_Options, "--inline -O2);
.                  AddOptionIf("optimization", "off", OptionType.cc_Options, "-O0");
.
.                  AddOptionIf("debugsymbols", "on", OptionType.cc_Options, "--debug");
.              }
.          }
```
<a name="backwardCompatibility"></a>
## Backward Compatibility and Migration Guide ##

Prior to the addition of config extensions the process of config package loading was quite complicated.
Originally there was only eaconfig but over time various new features were added to extend eaconfig.
Config extensions are intended to unify these various extension features into a single, easy to understand, mechanism.
Until people have transitioned to config extensions there is some backward compatibility provided
but considering that the cost of transitioning to config extensions is quite low we would like users to make the switch soon after taking Framework 8.

<a name="backwardCompatibility2"></a>
## Backward Compatibility and Migration Guide (Platform Config Packages) ##

The first mechanism for extending the config packages was called platform config packages.
This feature was created because we did not want to have all the configs defined in a single package,
in some cases we were required by platform first parties like Sony and Microsoft to keep the configurations for certain platforms hidden from most users.

The way platform config packages would work is that they would first search for the config in eaconfig and if they could not find it they would
take the first field in the config and append the string _config to the end and looks for the config in a package with that name.
(for example, android-clang-dev-debug would use android_config)

As soon as you begin using the &lt;extension&gt; element in your masterconfig framework will stop looking for configs in the platform configuration package.
Instead it will look for the config file in eaconfig and then in each of the config extensions listed in the masterconfig until it finds it.

Some small modifications need to be made to make Platform config packages work as config extensions.
The config extension version of the platform config package will still work as a platform config package when it is not used as a config extension.

The main change is that a config extension&#39;s Initialize.xml file is loaded right away,
whereas before it was loaded after loading the config file and was used to manually load the platform/${config-system}[-${config-processor}]-${config-compiler}.xml file.
To handle this Framework provides a property, framework-new-config-system, to indicate that the config extension feature is being used.
The contents of the Initialize.xml file that are related to config loading can be moved into a pre-combine.xml file or split up into the other appropriate config extension files.
To ensure the package still works as a platform config package the build scripts should still be included from the Initialize.xml file but using the new property so framework can skip these when the package is used as an extension.

###### A platform config package's Initialize.xml that works as both a platform config package and a config extension package ######

```xml
<project xmlns="schemas/ea/framework3.xsd">
 <do unless="${framework-new-config-system??false}">
  <include file="${package.android_config.dir}/config/platform/android-${config-compiler}.xml"/>
  <include file="${package.android_config.dir}/config/platform/android-targets.xml"/>
 </do>
</project>
```
<a name="backwardCompatibility3"></a>
## Backward Compatibility and Migration Guide (Extra Config Dirs) ##

Extra config dirs was a feature that was added so that users could easily extend existing eaconfig or platform configs without diverging those packages.
It was also a way that they could implement new configs.

The way this feature worked is that an attribute could be added to the config element to specify a directory which contained additional config files and platform override settings.
For example, in the below example if the user tried to build with a config called android-extra-debug that was not defined in eaconfig or android_config,
framework would look in ${package.GamePackage.dir}/config for a file called android-extra-debug.xml and use that as the config file.
Also before running the combine step it would load a file called ${package.GamePackage.dir}/config/config-overrides/overrides.xml to provide additional options.


```xml
<config ... extra-config-dir="${package.GamePackage.dir}/config" ... />
```
If the masterconfig file is shared by multiple teams the extra-config-dir can be overridden using the &lt;gameconfig&gt; element in a masterconfig fragment.


```xml
<gameconfig ... extra-config-dir="${package.GamePackage.dir}/config" ... />
```
Once you start using config extensions framework will no longer look for config files in the extra config dir, although it will continue to load the overrides.xml file.

Converting an extra config dir to a config extension should be very easy, simply remove the extra-config-dir attribute from the masterconfig, move the configs into a package structure, and if you have an override.xml file that can be converted into a pre-combine.xml file.

<a name="backwardCompatibility4"></a>
## Backward Compatibility and Migration Guide (eaconfig.options-override.file) ##

Options override files were another feature that worked pretty much the same as the extra config dir overrides file.

The way this feature worked is that you would provide a global property in the masterconfig file that indicated the name of a file that was relative to the masterconfig file.
Then immediately after loading the extra config dir overrides.xml file and before running the combine step, framework would try to load one of three build script files.
In the case of the example below these three files would be:
overrides/game.${config-platform}.xml, overrides/game.${config-system}.xml or overrides/game.xml


```xml
eaconfig.options.override.file=overrides/game.xml
```
Converting these to a config extension is also very easy. Simply remove the global property from the masterconfig and move these into a package structure and put these steps in a pre-combine.xml file.


##### Related Topics: #####
-  [Master Configuration File]( {{< ref "reference/master-configuration-file/_index.md" >}} ) 
