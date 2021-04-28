---
title: Installing Intellisense for Build Files
weight: 6
---

Intellisense can make working with build scripts much easier and is a recommended first step for anyone setting up Framework on their machine.

<a name="Section1"></a>
## What is Intellisense? ##

Intellisense is a visual studio feature that provides auto-completion and realtime code verification.
As soon as you start typing a keyword visual studio will provide a list of valid suggestions.
If you type an incorrect keyword it will be underlined and provide a warning message.

We have been able to provide Intellisense support for our custom NAnt build file xml format.
To make this possible we created a framework [Task]( {{< ref "reference/tasks/_index.md" >}} ) that generates an xml schema describing all of the tasks that framework knows about.
The schema was generated using reflection to analyze the source code of each task and by reading the xml comments in the source code.

![intellisense]( intellisense.png )<a name="Section2"></a>
## Installing Intellisense for Build Files ##

In order for visual studio to start providing you with intellisense features you must first run a framework target that generates the schema and copies it to the visual studio install directory.

To install intellisense navigate to the directory of your project and run the following NAnt target:


```
nant install-visualstudio-intellisense
```
In Frostbite there is an fbcli command that can be run to generate the schema file:


```
fb gen_nant_intellisense
```

{{% panel theme = "info" header = "Note:" %}}
Running the installer target from the package directory of a project should automatically allow it to detect all of the custom tasks associated with that project and add them to the schema.
{{% /panel %}}
Once intellisense has been installed you can enable it by opening any xml file in visual studio and adding the schema&#39;s namespace to the root xml node.

###### An xml file with the schema files namespace specified ######

```xml
<project xmlns="schemas/ea/framework3.xsd">
 <!-- ... -->
</project>
```
<a name="Section3"></a>
## Fixing Visual Studio File Association for .build files ##

You may find that Visual Studio is not recognizing .build files as XML files and thus don&#39;t get intellisense.
This is easy to fix in Visual Studio by going to `Tools > Options > Text Editor > File Extension` .
Then add an entry to the list to associate the build extension with xml files.

![file association]( file_association.png )