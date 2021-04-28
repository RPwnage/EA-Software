---
title: Eaconfig.build-MP
weight: 225
---

eaconfig.build-MPboolean or integer property is used to control the Visual Studio compiler parallel build feature.

<a name="Usage"></a>
## Usage ##

Name |Always defined |Needs to be Global |Default Value |
--- |--- |--- |--- |
| eaconfig.build-MP | ![requirements 1a]( requirements1a.gif ) |  | true |

This property only affects Visual Studio build and allows you to specify the number of threads being used to do build.
If this property is not specified, the default value will be used, which is that parallel compilation is enabled with
no limit to the number of threads being used.  If you want to disable this feature, create a global property called
eaconfig.build-MP and set the value to false.  If you want to limit the number of parallel compilation threads, set
this property&#39;s value to an integer corresponding to that number.

<a name="Example"></a>
## Example ##

You can set this as a global property in your project masterconfig.  The following will set the build to use -MP6 switch


```xml
<project xmlns="schemas/ea/framework3.xsd">
  ...
  <globalproperties>
    eaconfig.build-MP=6
  </globalproperties>
  ...
</project>
```
The following will disable parallel compilation:


```xml
<project xmlns="schemas/ea/framework3.xsd">
  ...
  <globalproperties>
    eaconfig.build-MP=false
  </globalproperties>
  ...
</project>
```
