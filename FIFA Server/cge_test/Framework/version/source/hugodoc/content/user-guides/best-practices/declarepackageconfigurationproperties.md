---
title: Declare package configuration properties
weight: 31
---

Package configuration properties can be registered with automatic help system

<a name="Section1"></a>
## Declaring properties that affect package configuration ##

Some packages use Framework properties to configure different modes the package can be built in.

Public property declaration task lets users of the package to discover what these properties are.

For example, EAThread build script uses following properties:


```xml

    <project default="build" xmlns="schemas/ea/framework3.xsd">

        <package name="EAThread" initializeself="true"/>

        <!-- Options -->
        <!-- To enable options put (e.g.) <globalproperties>EAThreadSPUEnabled</globalproperties>  -->
        <!-- in your masterconfig file and use -D:EAThreadSPUEnabled=1 on the nant command line.   -->

        <!-- If enabled then the SPU EAThread code is compiled.                                    -->
        <!-- <property name="EAThreadSPUEnabled" value="1" unless="@{PropertyExists('EAThreadSPUEnabled')}" /> -->

        <!-- If enabled, then the native PS3 sys_semaphore API is used to implement semaphores,    -->
        <!-- else the previous EAThread custom implementation is used. At one time the native      -->
        <!-- PS3 sys_semaphore API didn't exist and we only had the custom implementation.         -->
        <!-- This option is has been superseded by EAThread.EATHREAD_USE_NATIVE_PS3_SEMAPHORE.
             Use that property instead. -->
        <!-- <property name="EAThreadUseNativePS3Semaphore" value="0" unless="@{PropertyExists('EAThreadUseNativePS3Semaphore')}" /> -->

        <!-- Controls the warnings-as-errors compiler setting. -->
        <!-- <property name="EATECH_WARNINGSASERRORS" value="0" unless="@{PropertyExists('EATECH_WARNINGSASERRORS')}" /> -->

```
Without reading comments in build file for every package it&#39;s impossible to know which properties can be used to configure various packages in the build. `<declare-public-properties>` task provides a way to declare such configuration properties and users of the package can print all declarations using
standard eaconfig target `print-property-declarations` .


```xml

<project default="build" xmlns="schemas/ea/framework3.xsd">

    <package name="EAThread" initializeself="true"/>

  <declare-public-properties>
    <property name="EAThreadSPUEnabled">
      To enable options put (e.g.) <globalproperties>EAThreadSPUEnabled</globalproperties>
      in your masterconfig file and use -D:EAThreadSPUEnabled=1 on the nant command line.

      If enabled then the SPU EAThread code is compiled.
    </property>
    <property name="EAThreadUseNativePS3Semaphore">
      If enabled, then the native PS3 sys_semaphore API is used to implement semaphores,
      else the previous EAThread custom implementation is used. At one time the native
      PS3 sys_semaphore API didn't exist and we only had the custom implementation.
      This option is has been superseded by EAThread.EATHREAD_USE_NATIVE_PS3_SEMAPHORE.
      Use that property instead.
    </property>
    <property name="EATECH_WARNINGSASERRORS" description="Controls the warnings-as-errors compiler setting."/>
  </declare-public-properties>

```
Running target  `print-property-declarations` will output following information:


```

  nant print-property-declarations -configs:"pc-vc-dev-debug"

            [package] EAThread-dev (pc-vc-dev-debug)  print-property-declarations
            [create-build-graph] runtime [Packages 5, modules 2, active modules 2]  (215 millisec)

        Public Property Declarations:

        EATECH_WARNINGSASERRORS   [EAThread]

              Controls the warnings-as-errors compiler setting.

        EAThreadSPUEnabled   [EAThread]

              To enable options put (e.g.) <globalproperties>EAThreadSPUEnabled</globalproperties>
              in your masterconfig file and use -D:EAThreadSPUEnabled=1 on the nant command line.
              If enabled then the SPU EAThread code is compiled.

        EAThreadUseNativePS3Semaphore   [EAThread]

              If enabled, then the native PS3 sys_semaphore API is used to implement semaphores,
              else the previous EAThread custom implementation is used. At one time the native
              PS3 sys_semaphore API didn't exist and we only had the custom implementation.
              This option is has been superseded by EAThread.EATHREAD_USE_NATIVE_PS3_SEMAPHORE.
              Use that property instead.
             [nant] BUILD SUCCEEDED (00:00:00)


```
