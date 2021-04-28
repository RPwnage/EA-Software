---
title: Visual Studio 2015 Android Debugging
weight: 283
---

Debugging for Android platform using VS2015. VS2015 can be used to debug native Android code on devices and ships with its own X86 emulator.

<a name="Android Solution"></a>
## Debugging Android Solutiion ##

VS2015 supports Android native debugging &quot;out of the box&quot;. When using Framework to generate your solution file all necessary debugging settings should be
automatically set up for you. Each of the executable modules in your packages will have a Shared Library project and an Android project in the solution.
For example a Program module named &quot;MyModule&quot; will have a &quot;MyModule&quot; project (MyModule.vcxproj) and a &quot;MyModule.Packaging&quot; project (MyModule.Packaging.androidproj).
The .Packaging project handles the java build if applicable as well as the deployment and program entry. You should select this project as your startup project to
debug.

<a name="Activity Arguments"></a>
## Passing Activity Start Arguments ##

VS2015 does not explicitly support passing launch arguments to Activity Manager but it is possible by modifying the Launch Activity property in the properties
page for a packaging project. Visual Studio starts the Activity by calling &quot;am start&quot; inside of adb shell in the following manner:


```
am start -D -n [Package Name]/[Launch Activity]
```
This can be exploited by appending launch arguments to the Launch Activity for example:


```
android.app.NativeActivity -a android.intent.action.MAIN --ei myIntArg 1 --es myStringArg 'test'
```
<a name="APK Debugging"></a>
## Loose APK Debugging ##

Visual Studio can be used to debug an any arbitrary application as long as you have the following:

 - .apk file to be deployed and run
 - versions of the shared library (.so) files you wish to debug that have debug symbols (.so files in .apk typically have symbols stripped before packaging)
 - sources file for the shared library

<a name="Dummy Project"></a>
### Setting up a Visual Studio Dummy Project ###

 - Create a new “Basic Application (Android)” project in Visual Studio 2015
 - You can delete all of the default files in the project except for the AndroidManifest file
 - Either update the AndroidManifest file for your application, or replace it with your own version of the AndroidManifest file<br><br>  - If the AndroidManifest file doesn’t match the package and activity names it expects it will fail during deployment saying that it deployed the apk but can’t find the application on the device
 - Make sure the platform, in the drop down box next to the run button, is set to ARM if you are going to be testing with an ARM device<br><br>  - If it is not set to ARM the device will not appear to be connected<br>  - If you are testing with an x86 emulator make sure it is set to x86<br>  - Make sure that your APK file is built with the matching platform config, or else you may get errors saying the APK is built against an old version of the API.
 - Right click on the Project in the solution explorer and select properties
 - In the General Tab<br><br>  - Adjust the “Target API Level” field if needed<br>  - set the &quot;Android Package File (APK) Name&quot; to match the name of your APK file without extension.<br>  - set the &quot;Output Directory&quot; to the bin directory where the APK is located.
 - In the Debugging Tab<br><br>  - &quot;Debugger to launch&quot; should be set to &quot;Android Debugger&quot;<br>  - &quot;Debugger Type&quot; should be set to &quot;Native only&quot;<br>  - If a device is connected it should appear in the “Debug Target” field<br><br>   - If not make sure that it is connected, that the platform drop box is on ARM if the device is an ARM device, and that usb debugging is enabled on the device<br>  - “Package to Launch” should be set to the full path to the .apk file.<br>  - “Launch Activity” should be set to match the activity name of your application listed in the AndroidManfiest file.<br>  - “Additional Symbol Search Paths” can contain a list of paths and needs to include the path to a version of the ‘.so’ file that contains debug symbols (this should match the output directory field set previously).
 - In the ANT tab it is recommended to clear any of the fields that are already set. This way if you accidently try to build the sample project it will fail instead of overwriting your APK file.

### Debugging Your Application ###

 - Drag and drop any source files you want to debug into the Visual Studio window and set your breakpoints
 - Run the application through visual studio with debugging by pressing F5 or the start button icon on the toolbar.
 - If it asks to rebuild the application choose not to.
 - The application will be deployed to the device and will then begin to run, the break points should be hit and you should have many of the typical Visual Studio debugging features like the watch window and the ability to hover over variables.
 - If you need to bring up a logcat window to see the device output you can use the tool bar menus: Tools &gt; Android Tools &gt; Logcat.

