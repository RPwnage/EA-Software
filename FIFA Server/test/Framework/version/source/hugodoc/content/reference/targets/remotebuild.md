---
title: RemoteBuild
weight: 212
---

Topics in this section describe how to build generated Xcode project from Windows computer.

<a name="RemoteBuild"></a>
## Remote build from PC ##

It is possible to invoke Xcode project generation and Xcode build on a MAC computer remotely from your Windows PC. To do that you will need [RemoteBuild](http://eamobiletech.eamobile.ad.ea.com/EAMTHelp/dokuwiki/doku.php?id=eamt:packages:remotebuild) package. This package will sync your files to MAC, invoke specified nant targets and transfer results back to your PC. The whole process is quite simple.


{{% panel theme = "info" header = "Note:" %}}
Xcode project generation can be performed on PC or on MAC. With Projectizer 2 there is little difference in speed between MAC and PC.
{{% /panel %}}
The easiest way to use RemoteBuild is to use its build file via the Nant framework.


```
nant remotebuild -D:config=iphone-clang-dev-debug
```
The remotebuild Nant target is not supported for all platforms/configurations, but its build script will tell you so.

Be aware that this command won’t do anything useful. The remotebuild.commandline property must be specified through Nant with a set of steps to execute. Here’s a command sample to do a full rebuild and transfer the resulting build in zip format on the PC:


```
nant remotebuild -D:config=iphone-clang-dev-debug -D:remotebuild.commandline="-f -r -p l -x -k zip -t"
```
Or using long argument names:


```
nant remotebuild -D:config=iphone-clang-dev-debug -D:remotebuild.commandline="--fullClean --rsync --projectize local --xcodebuild --package zip --transfer"
```
RemoteBuild can be used to easily transfer packages to a Mac computer. Simply specify a masterconfig file and packages in its masterversions tag relevant to the current config will be transferred:


```
nant remotebuild -D:config=iphone-clang-dev-debug -masterconfigfile:masterconfig.xml -D:remotebuild.commandline="-r"
```

{{% panel theme = "info" header = "Note:" %}}
The masterconfig file doesn’t need to be specified if it is the one in the current directory or the project directory if the &quot;projectPath&quot; argument is specified.
{{% /panel %}}
For more information see [RemoteBuild](http://packages.ea.com/Package.aspx?ID=1679) package on the package server.

