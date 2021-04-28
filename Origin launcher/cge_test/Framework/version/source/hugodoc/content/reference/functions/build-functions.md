---
title: Build Functions
weight: 1001
---
## Summary ##
Collection of utility functions usually used in build, run and other targets.


## Overview ##
| Function | Summary |
| -------- | ------- |
| [String GetModuleOutputDir(String type,String packageName)]({{< relref "#string-getmoduleoutputdir-string-type-string-packagename" >}}) | Returns output directory for a given module. For programs and DLLs it is usually &#39;bin&#39; directory, for libraries &#39;lib&#39;.<br>This function takes into account output mapping. |
| [String GetModuleOutputDirEx(String type,String packageName,String moduleName,String moduleGroup,String buildType)]({{< relref "#string-getmoduleoutputdirex-string-type-string-packagename-string-modulename-string-modulegroup-string-buildtype" >}}) | Returns output directory for a given module. For programs and DLLs it is usually &#39;bin&#39; directory, for libraries &#39;lib&#39;.<br>This function takes into account output mapping. NOTE. In the context of a .build file set property &#39;eaconfig.build.group&#39;<br>to use this function for groups other than runtime. |
| [String GetModuleOutputName(String type,String packageName,String moduleName,String moduleGroup,String buildType)]({{< relref "#string-getmoduleoutputname-string-type-string-packagename-string-modulename-string-modulegroup-string-buildtype" >}}) | Returns outputname for a module. Takes into account output mapping. |
| [String GetModuleGroupnames(String groups)]({{< relref "#string-getmodulegroupnames-string-groups" >}}) | Returns list of module groupmanes for modules defined in package loaded into the current Project. |
| [String ModuleNameFromGroupname(String groupname)]({{< relref "#string-modulenamefromgroupname-string-groupname" >}}) | Returns module name based on the group name |
| [String HasUsableBuildGraph(String configurations,String groups)]({{< relref "#string-hasusablebuildgraph-string-configurations-string-groups" >}}) | Returns true if there is usable build graph |
| [String RetrieveConfigInfo(String configuration,String config_component)]({{< relref "#string-retrieveconfiginfo-string-configuration-string-config_component" >}}) | Return a specific config component for a specifc full config name from top level build graph. |
## Detailed overview ##
#### String GetModuleOutputDir(String type,String packageName) ####
##### Summary #####
Returns output directory for a given module. For programs and DLLs it is usually &#39;bin&#39; directory, for libraries &#39;lib&#39;.
This function takes into account output mapping.

###### Parameter:  project ######


###### Parameter:  type ######
type can be either &quot;lib&quot; or &quot;bin&quot;

###### Parameter:  packageName ######
name of the package

###### Return Value: ######
module output directory




#### String GetModuleOutputDirEx(String type,String packageName,String moduleName,String moduleGroup,String buildType) ####
##### Summary #####
Returns output directory for a given module. For programs and DLLs it is usually &#39;bin&#39; directory, for libraries &#39;lib&#39;.
This function takes into account output mapping. NOTE. In the context of a .build file set property &#39;eaconfig.build.group&#39;
to use this function for groups other than runtime.

###### Parameter:  project ######


###### Parameter:  type ######
type can be either &quot;lib&quot; or &quot;bin&quot;

###### Parameter:  packageName ######
name of the package

###### Parameter:  moduleName ######
name of the module

###### Parameter:  moduleGroup ######
Optional - group of the module e.g. runtime, test. If not provided Framework will attempt to deduce it from context.

###### Parameter:  buildType ######
Optional - build type of the module e.g. Program, Library.

###### Return Value: ######
module output directory




#### String GetModuleOutputName(String type,String packageName,String moduleName,String moduleGroup,String buildType) ####
##### Summary #####
Returns outputname for a module. Takes into account output mapping.

###### Parameter:  project ######


###### Parameter:  type ######
Can be either &quot;lib&quot; or &quot;bin&quot;

###### Parameter:  packageName ######
name of a package

###### Parameter:  moduleName ######
name of a module in the package

###### Parameter:  moduleGroup ######
Optional - group of the module e.g. runtime, test. If not provided Framework will attempt to deduce it from context.

###### Parameter:  buildType ######
Optional - build type of the module e.g. Program, Library.

###### Return Value: ######
outputname




#### String GetModuleGroupnames(String groups) ####
##### Summary #####
Returns list of module groupmanes for modules defined in package loaded into the current Project.

###### Parameter:  project ######


###### Parameter:  groups ######
list of groups to examine. I.e. &#39;runtime&#39;, &#39;test&#39;, ...

###### Return Value: ######
module groupnames, new line separated.




#### String ModuleNameFromGroupname(String groupname) ####
##### Summary #####
Returns module name based on the group name

###### Parameter:  project ######


###### Parameter:  groupname ######
I.e. &#39;${module}, runtime.${module}&#39;, &#39;test.${module}&#39;, ...

###### Return Value: ######
module name.




#### String HasUsableBuildGraph(String configurations,String groups) ####
##### Summary #####
Returns true if there is usable build graph

###### Parameter:  project ######


###### Parameter:  configurations ######
list of configurations

###### Parameter:  groups ######
list of group names

###### Return Value: ######
Returns true if there is usable build graph.




#### String RetrieveConfigInfo(String configuration,String config_component) ####
##### Summary #####
Return a specific config component for a specifc full config name from top level build graph.

###### Parameter:  project ######


###### Parameter:  configuration ######
The full config name (such as pc64-vc-debug)

###### Parameter:  config_component ######
The config component (supported values are: config-system, config-compiler, config-platform, config-name, config-processor)

###### Return Value: ######
Return a specific config component for a specifc full config name or an empty string if not found.  NOTE that this function assumes that build graph is already constructed, otherwise, it will just return empty string.




