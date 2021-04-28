---
title: Debugging Framework
weight: 34
---

This topic explains how to debug framework and related packages.

## Debugging Framework through logging ##

The first thing you should try is to get a full readable log.
Adding the -verbose flag to your command line will increase the level of details in the log.
Adding the -noparallel flag removes multithreading and can make the log easier to understand.

Some tips when reading Framework logs:
* Focus on the first error. Framework tends to cascade errors, where the 1st error will cause a bunch of other downstream errors.
* Ignore the C# callstack. Framework will sometime report a C# callstack of where the error occured. That's not overly useful as most commonly the Framework binaries are working fine but the data is incorrectly setup. Keep looking for references to .build files or Initialize.xml, or other more useful error messages

Other tips:
If you want to track when a property changes during a build you can add the traceprop argument to the command line: -traceprop:"property-name"

## How to Debug Framework in Visual Studio ##

There maybe cases where you can't get the information you need from the Framework output alone and you will want to debug the code through visual studio.

You can do this by opening the Framework solution in the root of the framework package. (in frostbite: TnT/Build/Framework/Framework.sln)

To run nant.exe with specific commandline parameters right click on the nant project (make sure it is the start-up project) and select properties.
In the debug section of the properties menu set your command line arguments and the working directory of the package you want to build.
Then add break points to the code to stop it while its running.
Even if you want to run it to completion it may be helpful to put a break point on the last line of the main method, in the nant project&#39;sConsoleRunner.csfile, otherwise the console window will close.

If you want to debug a particular framework [Task]( {{< ref "reference/tasks/_index.md" >}} ) , search for the name of the task with the visual studio search tool.
Most of the tasks, which appear as xml elements in the build scripts, have an associated C# class which extends the `NAnt.Core.Task` class.
The name of the class may be different than the xml element&#39;s name, but all the task classes have a task attribute which defines the xml name so you should be able to find it when searching.
The important methods to look for in a class are `InitializeTask`  and  `ExecuteTask` .
InitializeTask gets called during loading the of build script when the task&#39;s attributes are being initialized.
ExecuteTask gets called when the script is running and the task gets executed.

Other ways to debug:
* Most straight forward: Copy paste command line arguments from fbcli print or history view (fbenv view history) into Visual Studio and run.
* Fancy: Run fb nant or fb gensln command from fbcli but pass additional arguments: – -debug (anything after – is fowarded to nant). 
The -debug argument causes a just in time debugging dialogue to appear.
* Extra fancy: set FRAMEWORK_DEBUGONSTART=1. This sets an environment variable that causes Framework to throw a just in time debugging dialogue much like -debug. 
The advantage of this method is that it can be used to debug "hidden" Framework calls. 
There are calls to Framework done for multiple fbcli commands and for fbcli startup where the arguments are not printed to the console 
(nearly always -showmasterconfig, a command used to query the masterconfig for package version or path information). 
This method allows you to break into these when they failing.

## How to Debug Eaconfig and Build Scripts ##

Since Eaconfig is comprised almost entirely of xml files it may not be obvious how to debug problems in eaconfig or build script xml files.
Here are a couple of options that may be helpful:

 - When running nant you can provide the `-verbose` commandline option to see additional debug information.
 - Adding the{{< task "NAnt.Core.Tasks.EchoTask" >}}to build scripts (as you would use printf in C code) to output debug information to the console.<br><br><br>```<br><echo message="... your text here ..." /><br>```

## Framework Logging and Warning Levels ##

Framework allows users to configure the amount and type of log messages that are printed.
When a user is debugging a Framework related problem we suggest increasing the logging level to get more messages.

There are five logging levels that can be controlled through nant *–loglevel* commandline switch:

   *Quiet* – only errors are printed
   *Minimal* - only errors, warnings and important steps are printed
   *Normal* – errors, warnings and status
   *Detailed* – errors, warning, status, info information is printed
   *Diagnostic* -  errors, warning, status, info, and debug information is printed

An easier way to turn on the most log messages is to use the &quot;-verbose&quot; flag when calling nant.exe.
The verbose switch is equivalent to turning on Diagnostic mode. One of the benefits of this mode
is that it shows most of the command line statements called by Framework, which can be useful for checking
that the arguments are correct and appearing as expected.

{{% panel theme = "info" header = "Note:" %}}
*Detailed* outputs build settings and build steps without cluttering output with lots nant internal info.
This output level is useful for debugging build related problems.

nant . . . . -loglevel:Detailed
{{% /panel %}}
There are also four warning levels that can be controlled through nant *–warnlevel* commandline switch:

   *Quiet* – no warnings are printed
   *Normal* – warnings about situations that could affect build outcome
   *Deprecation* –usage of deprecated eaconfig/nant XML syntax or tasks.
   *Advice* – warnings about inconsistencies /errors in build scripts that unlikely to affect build outcome or that Framework can autocorrect.

## Debugging Taskdefs ##

When nant runs it will, among other things, compile and run C# extensions (known as "taskdefs"). The code can be found either in explicit .cs files or within language blocks in .build files. Thus, errors can stem from either failing to compile the code or running it.

There are a number of tricks to debug taskdef code:

* If you know the file you need to debug you can drag and drop this file into Visual Studio with the Framework solution open. In Debug configuration Visual Studio is smart enough to resolve symbols and breakpoints in these files.
* System.Diagnostics.Debugger.Launch(); - putting this in a C# file you want to debug then running your Framework command from fbcli will throw just in time debugging dialogue which will break you into the code on this line. Does not work if debugger is already attached (use .Break(); instead).
* If all else fails, good ol' printf. System.Console.WriteLine(...);.