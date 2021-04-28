---
title: Setting a Visual Studio Startup Project
weight: 262
---

When you run or debug a program in Visual Studio you need to indicate which project to run,
especially if there are multiple executable projects in a solution.
To do this you can use a property for controlling which project will be the startup project in a visualstudio solution.

<a name="BuildEnvPropertyUsage"></a>
## Usage ##

Name |Always defined |Needs to be Global |Default Value |
--- |--- |--- |--- |
| package.sln.startupproject |  | No |  |

## Example ##

Set the module &quot;mymodule&quot; as the startup project.
When the user regenerates their visual studio solution this module will be set as the startup project in visual studio.


```

<property name="package.sln.startupproject" value="mymodule"/>

```
