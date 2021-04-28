---
title: Properties
weight: 216
---

The topics in this section define the various NAnt properties that affect NAnt or targets execution,
or affect how configuration package loading.

## NAnt (system) properties ##

System Properties are the Properties used explicitly by the NAnt code: [System Properties]( {{< ref "reference/properties/systemproperties.md" >}} ) .

## Build / Config Packages Related Properties ##

 - [config]( {{< ref "reference/properties/configurationproperties.md#ConfigProperties" >}} ) - name of configuration to use (build) in single config targets.
 - [package.configs]( {{< ref "reference/properties/configurationproperties.md#ConfigProperties" >}} ) - set of configuration names to use in multi-config targets
 - [bulkbuild]( {{< ref "reference/properties/bulkbuild_property.md" >}} ) - boolean property is used to control globally whether to perform bulk builds or loose file builds.
 - [nant.transitive]( {{< ref "reference/properties/nant.transitive_property.md" >}} ) - boolean property is used to control transitive dependency propagation in NAnt.
 - [autobuilduse]( {{< ref "reference/properties/autobuilduse_property.md" >}} ) - boolean property is used to turn &#39;on&#39; or &#39;off&#39; autodependencies.
 - [eaconfig.build.split-by-group-names]( {{< ref "reference/properties/eaconfig.build.split-by-group-names.md" >}} ) - boolean property affects Visual Studio Solution generation.
 - [Eaconfig.build-MP]( {{< ref "reference/properties/eaconfig.build-mp.md" >}} ) - boolean or integer property to turn on /MP switch using Visual Studio&#39;s compiler.
 - [eaconfig.options-override.file]( {{< ref "reference/properties/eaconfig.options-override.file.md" >}} ) - Property specifying a file to be loaded by eaconfig to do configuration override.
 - [Build Group Warnings]( {{< ref "reference/properties/buildgroupwarnings.md" >}} ) - Properties to warn/fail when a user tries to build a package using the wrong group type

## Properties that affect buildtype ##

For properties that affect build types, please go to the [Build Setting Properties]( {{< ref "reference/properties/buildsettingproperties.md#ChangingBuildtypeFlags" >}} ) page for the list.

## Platform / SDK Related Properties ##

 - [Android Platform Related Properties]( {{< ref "reference/properties/android_platform_related_properties.md" >}} )
 - [OSX / IOS Platforms Related Properties]( {{< ref "reference/properties/ios_osx_platforms_related_properties.md" >}} )
 - [Windows Platform Related Properties]( {{< ref "reference/properties/windows_platforms_related_properties.md" >}} )

