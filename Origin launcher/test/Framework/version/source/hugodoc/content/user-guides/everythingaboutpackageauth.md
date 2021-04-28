---
title: Package Authentication
weight: 35
---

This topic explains everything someone would need to know about package authentication.

<a name="Section1"></a>

## Package Downloads in Framework ##
When Framework is unable to find a package in one of the package roots listed in the masterconfig it will try to download it ondemand.
Ondemand package can be download from either the package server, perforce or NuGet.
Which service a package is downloaded from is determined by how the package is listed in the masterconfig.
By default a package is downloaded from the web package server.
If a URI field is specified and a perforce path is provided it will try to download that package from perforce.
If a URI field is specified and the NuGet url is provided it will try to download that package from NuGet.
Downloading packages from Perforce requires user authentication and thus we have created a system for managing user credentials.

<a name="Section2"></a>

## Setting up Perforce Credentials ##
Perforce credentials are stored in a file know as the credential store, or credstore for short.
To add new credentials to the credstore you can use the command "eapm credstore <server>" and you will be prompted to enter a user name and password.
There is also a GUI credential manager located in the framework bin directory, just click on CredentialManager.exe.

It is important to realize that credentials need to be created under the same account that Framework runs under.
This can be especially important on build machines that run under the system account, in that case the credentials should also be entered under the system account.

If the credentials you have entered are not working you can clear your credential store simply by deleting the credstore file.
The credstore file is located in the current users home directory in a file called nant_credstore.dat 
(%USERPROFILE%\nant_credstore_keys.dat on Windows, ~/nant_credstore_keys.dat on MacOS or Linux)

<a name="Section3"></a>

## Perforce Authentication Fallback ##
When Framework attempts to connect to perforce, using the credentials in the credential store, 
it will try various ways to connect if the first method fails.

Firstly, it will try to connect to the perforce server with the credentials in the credential store file.
If that fails it will attempt to find the credentials in the perforce tickets file.
If all of those fail it will try one last time without a password in the hopes that the server doesn't require password authentication.

If all attempts fail it will prompt the user to enter their credentials. 
On windows the prompt will be a GUI window and on linux it will be on the command line.
The prompt will timeout after a short time unless it detects user input. 
The new credentials that the user provides will be put in the credential store so they can be used for the next run.

<a name="Section4"></a>

## NuGet Package Downloading ##
When Framework decides to download a NuGet package it will check to see if that package exists on the user's machine and if it needs to be updated.
Each NuGet package contains a marker file called "nugetinstall.marker" in the root of the package.

Framework will do a full reinstall of the package if either the marker file is invalid or the install version number in the marker file has changed.

There is a second number in the marker file, the package version, if this number is different it means that the NuGet contents of the package are fine
but the Framework files need to be regenerated. When it regenerates the package it will start by deleting the marker file to ensure that if Framework
fails in anyway during package regeneration it will simply to a full reinstall of the package next time. It will then delete everything in the package
other than the contents of the NuGet package folder and regenerate the Framework files.
