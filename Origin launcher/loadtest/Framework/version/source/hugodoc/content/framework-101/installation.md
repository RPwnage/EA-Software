---
title: Installation Instructions
weight: 5
---

This page explains how to install Framework as well as its system requirements.

<a name="ObtainingFramework"></a>
## Obtaining Framework ##

Framework can be downloaded from the {{< url PackageServer >}} or sync&#39;ed from our {{< url PerforceDepot >}}on the Frostbite Perforce server in //packages/Framework.
It is also included with Frostbite in TnT/Bin/Framework.


{{% panel theme = "info" header = "Note:" %}}
- Access to the Package Server is secured and only accessible to those on the [PROGRAMMERS @ EA](https://dlmanager.ea.com:4443/DLManager/Groups/Properties?objectType=Group&identity=4fe49273-e7da-41f9-a077-58e79b7aaf9b) mailing list.  Framework will use your Windows login credentials to login into the package server.
- See [Package Management Tools]( {{< ref "reference/package-management-tools/_index.md" >}} ) topic to learn how to set credentials for the package server when you are on a non-Windows system or not logged into EA domain.
- The installer for Framework is now deprecated and no longer supported.  To install framework please obtain the .zip version of the [Framework Package](https://packages.ea.com/pkg/framework) from the package server and unzip it to a location of your choice.
{{% /panel %}}


{{% panel theme = "warning" header = "Warning:" %}}
- Unix/Linux systems may have an open sourcenant preinstalled and its location is in PATH, make sure that you are using the Framework version of nant.
{{% /panel %}}
<a name="InstallFrameworkRequirements"></a>
## Requirements ##

On Windows:

 - [.NET Framework 4.7.2](https://dotnet.microsoft.com/download/dotnet-framework/net472)
 - [.NET Core 3.1](https://dotnet.microsoft.com/download/dotnet-core/3.1)

On Unix and Macintosh:

 - A recent version of {{< url Mono >}} installed. Framework was tested with Mono 6.10 on MacOS and Linux (Ubuntu 18.04).

{{% panel theme = "warning" header = "Warning:" %}}
- Many Unix/Linux systems have older version of Mono preinstalled. Make sure you have minimal required version.
{{% /panel %}}

<a name="Ondemand"></a>
## Ondemand Downloads ##

When working with Framework you will use a masterconfig file which will list all of the packages you are using, which versions and some information about where to look for the packages.
If the package is not found within one of the package root locations on your machine, as specified in the masterconfig, it may try to download the package ondemand, provided that the masterconfig package has enabled ondemand downloads.
Ondemand packages are downloaded to the ondemand root, which is also specified by the masterconfig file.

Ondemand packages can be downloaded from various sources.
The default source is the{{< url PackageServer >}}.
Packages can also be downloaded from perforce if the package has a p4 uri attribute declared that specifies the server address, perforce path and changelist.
Nuget packages can also be obtain ondemand and Framework will convert them to Framework packages.
These features will be converted in more detail when we talk about the masterconfig file.

Framework itself can also be obtained ondemand. If the -autoversion parameter is passed to nant.exe on the command line the executing version of Framework will
read the masterconfig and if the version of Framework list in the masterconfig is not the same as the executing instance of Framework it will ondemand download that version.
It will then run the ondemand downloaded version of Framework and pass through all of its command line arguments to this second instance.
