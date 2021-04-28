---
title: Using the Android Emulator
weight: 281
---

This topic explains some of the properties and targets used to control the android emulator

<a name="selecting_images"></a>
## Selecting an Image ##

AndroidSDK contains a couple of different android emulator images.
And there are a couple ways to select these different images.

One reason you may want to select a different image is because it uses a different processor.
We currently have an ARM and x86 image. However, in this case all you need to do is use the
appropriate config for that processor and the correct emulator will be loaded automatically.

We also have an experimental ARM image that has slightly more RAM. To select this image you need
to set the &quot;package.AndroidSDK.avdname&quot; property to &quot;standard_api17_extraram&quot;. For other future emulator
images you should set this property to the name of the image to select it.

<a name="gpu_emulation"></a>
## Enabling GPU Emulation ##

Some tests may require GPU emulation to be enabled. To do this simply set the property
&quot;package.AndroidSDK.enable-gpu-emulation&quot; to &quot;true&quot;.


{{% panel theme = "info" header = "Note:" %}}
GPU emulation is not compatible with the emulators snapshot feature, which means the
emulator will take longer to startup while you wait for it to boot.
{{% /panel %}}
<a name="disabling_restart"></a>
## Running Tests Without Restarting the Emulator ##

Since the emulator takes time to start and stop, some users who are running many tests
need the ability to deploy and run tests without restarting the emulator.

To do this the user can set the property &quot;android-runtarget-emulator-restart&quot; to &quot;false&quot;.
However, the user will then be responsible for starting and stopping the emulator themselves
which they can do by calling the targets &quot;android-testrun-initialize&quot; and &quot;android-testrun-finalize&quot;.

<a name="timeouts"></a>
## Adjusting Timeout Length ##

Since much of the android emulators startup and deployment process requires framework to execute
external programs as sub processes it is important to have timeout values to determine when these
external processes are hanging. (all timeout values are in milliseconds unless stated otherwise)

The main timeout value for android_config is &quot;android-timeout&quot;.

Alternatively the main timeout value for eaconfig is &quot;eaconfig-run-timeout&quot;.
By default this value is 1200ms and &quot;android-timeout&quot; is set to 1000 times this value.

