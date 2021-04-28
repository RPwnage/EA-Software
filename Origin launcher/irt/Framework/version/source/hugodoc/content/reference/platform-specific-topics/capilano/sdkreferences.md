---
title: Adding Extension APIs
weight: 289
---

This topic describes how to use the sdkreferences property to add Extension APIs for Capilano

<a name="overview"></a>
## Overview ##

The CapilanoSDK comes with several Extension APIs.
Extension APIs can be found in the SDK package under &quot;installed/xdk/Extensions/&quot;
These can be added to a module using the sdkreferences property.

Below is an example of how you can use sdkreferences to add Extension APIs using structured xml.
Notice the name used for the Extension API in the example, the name matches the directory where it is found in the SDK package.
For example, &quot;installed/xdk/Extensions/Microsoft.Kinect API/8.0&quot;.
If the name is incorrect you should get a build failure.


```xml
<Program name="somemodule">
  <sdkreferences>
    Microsoft.Kinect API, Version=8.0
    Xbox Services API, Version=8.0
  </sdkreferences>
  <sourcefiles basedir="${package.dir}/source">
    <includes name="${package.dir}/source/*.cpp" />
  </sourcefiles>
</Program>
```
