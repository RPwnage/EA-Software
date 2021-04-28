---
title: eaconfig.options-override.file
weight: 227
---

eaconfig.options-override.fileis a property that allows you to specify a custom script file for eaconfig to load to do special config/options override that you need special customization for without creating a config package yourself.

<a name="Usage"></a>
## Usage ##

Name |Always defined |Needs to be Global |Default Value |
--- |--- |--- |--- |
| eaconfig.options-override.file |  | Yes | Property does not exists by default. |

This property let you specify a nant script file for eaconfig to load during it&#39;s config initialization.  You can put any
valid nant script in there such as providing your own property initialization or override certain eaconfig&#39;s default options.


{{% panel theme = "info" header = "Note:" %}}
If you have special setup for specific configuration or platform, you can prepand the filename&#39;s extension with ${config-platform} or
${config-system} and eaconfig will load those files instead. The loading priorities are:

 1. [filename].${config-platform}.[file_ext]
 2. [filename].${config-system}.[file_ext]
 3. [filename].[file_ext]
{{% /panel %}}
<a name="Example"></a>
## Example ##

You should provide value of this property through command line as global property:


```
nant.exe ... -G:eaconfig.options-override.file=MyOptionsOverride.xml
```
Content of this file can be any valid nant script.  For example:


```xml
<project xmlns="schemas/ea/framework3.xsd">
  ...
  <!-- Turn off warning as errors except for these packages -->
  <do unless="'${package.name}' == 'PackageA' or '${package.name}' == 'PackageB'">
    <echo message="*** Turning off warnings as errors for package ${package.name}"/>
    <property name="eaconfig.warningsaserrors" value="off"/>
  </do>
  ...
</project>
```
