---
title: Upgrading To Framework 8
weight: 1
---

This page details some of the larger changes between Framework 8 and previous versions. 

## Android Support ##

It will be supported in a subsequent patch. If you need to support Android it is recommended you remain on Framework 7 if the patch is not yet available. Android is *NOT* supported in Framework 8 initial release.

## The *eaconfig* and *VisualStudio* Packages ##

The eaconfig and VisualStudio packages are now included as part of the Framework package. Setting versions for these packages in the masterconfig will have no effect. For backwards compatbility in version checks the eaconfig version is set to the same as the Framework version. The VisualStudio package version is based on Visual Studio version control properties. See [Setting Visual Studio Version]( {{< ref "reference/visualstudio/setting-visual-studio-version.md" >}} ) for more information. These packages are still initialized with all the standard properties (such as *package.eaconfig.dir*) to allow backwards compatbility with packages that reference files within them.

## Warning System ##

Framework 8 introduces finer control over warning and deprecation levels and also adds and removes some warnings. Warning ids have been preserved to have the same meaning but some have been removed due to no longer being relevant. Most notably warning W2001 (DeprecatedWarning) is no longer a valid warning code - deprecations have been split out into their own category (DXXXX codes). The *nant.warningsuppression* property is still supported but is recommended that you convert this to using the [warnings]( {{< ref "reference/master-configuration-file/warnings.md" >}} ) section in your masterconfig and double check that all warnings suppressed are still relevant.

## Configuration Packages ##

#### kettle_config, capilano_config, ios_config and osx_config Consolidation ####

In addition to eaconfig being include in Framework, the kettle_config, capilano_config, ios_config and osx_config have been consolidated into it. These packages no longer conceptually exist so their properties cannot be referenced. These packages can be removed from masterconfig and version checks should be updated if needed to use eaconfig or Framework version.

#### Configuration Extensions vs Base Configuration Packages ####

Using a base configuration package other than eaconfig is supported in Framework 8 but not recommended. Customizations you wish to make to global configuration are best achieved through a configuration extension. Details of how to use configuration extensions can be found in the [Configuration Package]( {{< ref "reference/configurationpackage.md" >}} ) section. If you wish to continue using a custom configuration package some change may be necessary due to changes in configuration package structure - especially if you were relying on including internal files of eaconfig.

## Breaking Changes ##

This section details changes that are most likely to crop up during migration to Framework 8. This is not an exhaustive list - please refer to Framework [change log]( {{< ref "introduction/changelog.md" >}} ) for full details.

#### eaconfig Targets Are All Included By Default ####

In Framework 7, compatible versions of eaconfig had default targets that were included or excluded based on the presences of the following optionset before the *\<package>* task was run:

```xml
  <optionset name="config.targetoverrides">
    <option name="run" value="include" /> <!-- include a target that is disabled by default -->
    <option name="build" value="exclude" /> <!-- exclude a target that is enabled by default (usually so it can be overridden) -->
  </optionset>
```

In Framework 8's eaconfig *all* targets are included by default (config.targetoverrides is still supported for excluding targets but this is not the recommended way) but with the *allowoverride="true"* attribute set. If you wish to override a default target you can declare a target with the same name after the *\<package>* task in your build file and set the *override="true"* attribute.

#### The \<group>.buildtype Property ####

##### No Longer Defaults To 'Program' #####

This change only applies to legacy Framework module properties, however unlike the below item under this heading this is *always* treated as an error, rather than warning as error. If using structured XML syntax (recommended!) this change is handled automatically. When defining the modules for a package each listed module must also have a corresponding build type property set. See example below:

```xml
  <property name="runtime.buildmodules" value="MyLibraryModule MyProgramModule" />

  <!-- the following could be omitted for Framework 7 and would generally result in correct behaviour, however in Framework 8 it is an error to omit these -->
  <property name="runtime.MyLibraryModule.buildtype" value="Library"/>
  <property name="runtime.MyProgramModule.buildtype" value="Program"/>
```

Note while upgrading this error is encountered more frequently for modules that were *NOT* intended to be declared, rather than modules that are supposed to be present having only been partially declared. Pay close attention to surrounding context to determine which is the case. For example the *.buildmodules* property may contain several modules whose full initialization (and setting of *.buildtype*) is conditional - adding these modules to *.buildmodules*  should also follow the same condition.

##### Warning 5013 \<group>.buildtype Specified #####

This is only a warning but may be promoted to an error based on warning-as-error settings. Framework 8 adds extra validation around the *\<group>.buildtype* property. Note this is distinct from the *\<group>.\<modulename>.buildtype* property. For every module listed in *\<group>.buildmodules* you should have a corresponding *\<group>.\<modulename>.buildtype* property. The *\<group>.buildtype* property is only required if the package doesn't specify any modules but still has buildable code (though it is recommended to update your package to specify modules, preferrably with structured XML). Note structured XML tasks handle all the property settings for you and none of these properties need to be set explicitly if using structured XML. If you encounter this warning, you should:

- **If you are using structured XML** you can delete these properties as they are not required.

```xml
  <!-- below two lines are unnecessary and should be removed, the <Library> task handles this automatically -->
  <property name="runtime.buildtype" value="Library"/>
  <property name="runtime.MyLibraryModule.buildtype" value="Library"/>

  <Library name="MyLibraryModule>
    ...
  </Library>
```

- **If you have \<group>.buildmodules defined and converting to structured XML isn't an option** make sure you have *\<group>.\<modulename>.buildtype* property defined per module. Note in the exact case of having one module in the group *and that module has the same name as the package* Framework will automatically convert the property for you - but it will still throw the warning. This is to help with upgrades but you are encouraged to fix these warnings when possible.

```xml
  <property name="runtime.buildmodules" value="MyLibrary" />

  <property name="runtime.buildtype" value="Library"/> <!-- this is incorrect and should be removed -->
  <property name="runtime.MyLibrary.buildtype" value="Library"/> <!-- this is the correct way to set buildtype when modules are listed if not using structured XML -->
```

#### Combine.xml No Longer Exists ####

The file *config/global/combine.xml* from the eaconfig package no longer exists - typically custom config package included this file to run the combine step for configuration. This file is no longer need because the combine step is run automatically. You can remove includes of this file but make sure surrounding includes are being process in the correct order. See [Configuration Loading]( {{< ref "reference/configurationpackage.md#configuration-loading" >}} ) for more details.

#### \<taskdef> May Required Explicit References ####

Before Framework 8 \<taskdef> and \<script> tasks automatically referenced all assemblies currently loaded in the current AppDomain. This was fragile and error prone because correct compliation became dependent on Framework internal details and potentially the order in which other taskdefs were loaded. In Framework 8 only specific subset of "well known" assemblies are automatically referenced:

- EA.Tasks.dll 
- NAnt.Core.dll
- NAnt.Tasks.dll
- System.Core.dll
- System.dll
- System.Runtime.dll 
- System.Xml.dll

If you encounter compile error in \<taskdef> and \<script> tasks when upgrading to Framework 8 it is likely because any other references must now be listed explicitly. See [\<taskdef> Task]( {{< ref "reference/tasks/nant-core-tasks/general-tasks/taskdef-task.md" >}} ) for more information.

The old reference loading mechanism can still be enabled with global property *framework.script-compiler.use-legacy-references* set to true but this is recommended only as a short term workaround. Enabling this also automatically references the following assemblies which were loaded at Framework 7 start up but are not in Framework 8:

- Microsoft.Build.dll
- Microsoft.Build.Framework.dll
- Microsoft.Build.Utilities.Core.dll