---
title: 5 - Distributing A Package
weight: 19
---

This pages builds upon  [this HelloWorldLibrary]( {{< ref "user-guides/writing-a-package/dependingonotherpackages.md" >}} )  example.

Once you&#39;ve build a library or tool you wish to distribute you may consider uploading to  [Package 
	  	Server](http://packages.ea.com)  for use by other teams.

Framework contains a tool in its bin directory called eapm.exe, which is responsible for certain package management tasks like this. To create a package you can run the command &quot;eapm package&quot; in the root folder, next the manifest.xml file, to create a zip file of the package. Then run the &quot;eapm post &lt;path to zipfile&gt;&quot; command to post the newly created zip file to the package server.

Running these commands on the HelloWorldLibrary looks like this:


```
[doc-demo] C:\HelloWorldLibrary\1.00.00>eapm package
Zipping 6 files to HelloWorldLibrary-1.00.00.zip

[doc-demo] C:\HelloWorldLibrary\1.00.00>eapm post HelloWorldLibrary-1.00.00.zip
```
By default the package target will simply zip everything in the folder. Certain files can be excluded by adding a .packageignore file to the root of your package.


```
# an example .packageignore file

# excluding all files with a particular extension
**.tmp

# excluding all files in a directory
temp/**

# including a specific file in a directory that has been excluded
!temp/file.txt
```
