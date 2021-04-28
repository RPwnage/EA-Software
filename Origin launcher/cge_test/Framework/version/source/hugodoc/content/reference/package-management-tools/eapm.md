---
title: eapm
weight: 295
---

EA Package Manager tool (eapm) is a command line Package Management tool that is shipped with Framework.

<a name="Framework3eapm"></a>
## What is eapm? ##

EA Package Manager tool (eapm) is a command line tool that is shipped along with Framework in the same directory as nant.exe.
Under the hood it shares a lot of the same code with Framework but is specifically focused on interacting with the package server
and performing operations on ondemand packages.
If you use the NAnt Command Prompt on the Start Menu or have NAnt in your path this tool will also be in your path.

The most common use of eapm is to post packages to the{{< url PackageServer >}}, install packages, or display package info.

![EAPM]( eapm.jpg )One way to get started with eapm is to use the help command to list all of the available commands:


```
eapm help
```
You can also find out more about a specific command:


```
eapm help post
```
<a name="eapmpackage"></a>
## eapm package ##

The package command is used for turning a package into a zip file that can be posted to the{{< url PackageServer >}}.
The command must be run in the root of the package, the directory that contains your manifest and build files.

The command will automatically update the revision field in the manifest file to match version if it doesn&#39;t already, if it does you should submit this change to the manifest to perforce.

By default the package command will include all files in the zip file, but if you need to exclude certain files from packaging you can put a file called .packageignore in the root of your package. The format of this file is similar to a .p4ignore or .gitignore file but slightly different as it is based on the framework fileset wildcard syntax.


```
# an example .packageignore file

# excluding all files with a particular extension
**.tmp

# excluding all files in a directory
temp/**

# including a specific file in a directory that has been excluded
!temp/file.txt
```

```
eapm package
```
<a name="eapmpost"></a>
## eapm post ##

The post command is used for posting packages to the{{< url PackageServer >}}.
In order to post the package the package must be in a zip file.
You can create a zip file for a Framework package using the eapm package command.


```
eapm post mypackage-1.00.00.zip
```
<a name="eapminstall"></a>
## eapm install ##

The install command can be used to install a package from the{{< url PackageServer >}}.
This can be done if you want to download a package on the command line without going to the package server website in a browser.

<a name="eapmprune"></a>
## eapm prune ##

The prune command is a way to get rid of old ondemand packages that you haven&#39;t used in a while.
Framework keeps track of when ondemand packages were last used and packages older than a certain date can be deleted with this command.

By default packages that have not been used in the last 90 days are deleted. This threshold can be adjusted with the -threshold argument.
You can also choose to only list the packages without deleting them by using the -warn argument.


```
eapm prune D:/packages/ondemand
```
<a name="eapmlatest"></a>
## eapm latest ##

This command can be used to quick check what the latest version of a package is on the package server.


```
eapm latest CapilanoSDK
```
