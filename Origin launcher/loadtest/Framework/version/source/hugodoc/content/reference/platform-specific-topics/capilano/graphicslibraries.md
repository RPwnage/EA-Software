---
title: Graphics Libraries
weight: 290
---

This page explains how to toggle between the different Capilano graphics libraries.

<a name="Section1"></a>
## toggling Graphics Libraries ##

The Capilano graphics library you want to use can be selected using the global property &quot;package.CapilanoSDK.graphics-libs&quot;.

There are four sets of graphics libraries to select from: &quot;default&quot;, &quot;no&quot;, &quot;monolithic&quot;, and &quot;both&quot;.
The &quot;default&quot; setting is used when the property is not set or does not match any of the other options.
It includes the original set of libraries. The &quot;no&quot; setting can be used to specify that no graphics libraries should be included.
The &quot;monolithic&quot; setting is used to select the new set of graphics libraries.
Lastly, the &quot;both&quot; setting is for users that want to include both the default and monolithic libraries so that they can select the ones they want at compile time.

Some sets of graphics libraries and profiling tools require different files depending on if it is a debug configuration or not.
This should all be handled automatically based on the configuration you are using.
For example, building configs such as capilano-vc-dev-debug will include debug libraries and profiling tools where as the capilano-vc-dev-opt will not.

