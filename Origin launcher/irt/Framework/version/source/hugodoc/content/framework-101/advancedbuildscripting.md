---
title: More Build Scripting
weight: 12
---

So far you have learned to write simple modules in a package build script.
Build script syntax is much more flexible than that and can basically be used as its own programming language.
This page will highlight some of the other features of Framework build scripts that you will encounter.

<a name="Properties"></a>
## Properties ##

Properties are a very fundamental part of Framework and are used all over the place.
Properties are named values defined in build script or by Framework that are stored in a dictionary within Framework.
All properties are stored within Framework as strings. But it is possible to use functions to compare number values and perform math operations on properties.


```xml
<property name='myprop' value='somevalue'/>

<echo message='${myprop}'/>
```
Above is an example of how you can declare and then use a property in build scripts.


```
nant build -configs:pc-vc-dev-debug -D:myprop=somevalue
```
Another way to declare properties is to pass them in on the command line or declare them in the global properties section of the masterconfig file.
Properties that are declared this way are often used for switching certain features on and off.

<a name="Targets"></a>
## Targets ##

In the previous sections we have used predefined targets like the build and gensln targets.
However, it is possible for you to declare your own targets in build script.


```xml
<target name='mytarget'/>
 <echo message='helloworld'/>
</target>

<call target='mytarget'/>
```
Above you can see a target being defined in a build script.
Inside the target is arbitrary NAnt script tasks, in this case an echo task that prints helloworld.
You can run the target either by using the target on the nant command line or you can call a target within build script using the call task.

