---
title: Python modules
weight: 195
---

This topic describes how to work with Python modules.

<a name="what"></a>
## What are python modules for? ##

Python modules have been added to Framework to allow users to generate
visual studio python solutions. Since python is an interpreted language
native NAnt builds are not supported.

<a name="pytools"></a>
## Python and Visual Studio ##

Python integration for visual studio is achieved using an open source
extension called pytools (pytools.codeplex.com). Pytools provides
intellisense and debugging as well as a number of other visual studio
features that are already available in other languages like C#. When
a python project is run visual studio simply calls a user specified
python interpreter (activepython, cpython, Ironpython, etc.).


{{% panel theme = "danger" header = "Important:" %}}
For now, You must install pytools separately from the official
website (pytools.codeplex.com) for these new visual studio features
to be available.
{{% /panel %}}
Setting up an interpreter in visual studio is done separately from
creating a python module. To do so open visual studio and go to
&quot;Tools-&gt;Options-&gt;Python Tools-&gt;Interpreter Options&quot; to setup a new
interpreter. Then the interpreter can be reused for each new
project you make.

<a name="usage"></a>
## Usage ##

To generate a python solution define a python module in your build script
as you would for any other type of module.

The sourcefiles fileset should contain the paths to all of your python files.
The contentfiles fileset should contain all of the paths to non python files,
any that are not run by the python interpreter. The startupfile property is
used to specify which python file will be executed first and the projecthome
property is used to specify the path which other paths are relative to.

Since documentation of pytools is rather limited it is still a little unclear
what the remaining properties are for however since they are changeable through
the visual studio GUI we have decided to add them in case of future need.

<a name="structured"></a>
## Structured Xml Example ##


```xml
<PythonProgram name="MyPythonModule" startupfile="Program.py" projecthome="../../../source">
<sourcefiles basedir="${package.dir}\source">
<includes name="${package.dir}\source\*.py"/>
</sourcefiles>

<contentfiles basedir="${package.dir}\content">
<includes name="${package.dir}\content\*.txt"/>
</contentfiles>

<searchpaths>../content</searchpaths>
</PythonProgram>
```

##### Related Topics: #####
- {{< task "EA.Eaconfig.Structured.PythonProgramTask" >}}
