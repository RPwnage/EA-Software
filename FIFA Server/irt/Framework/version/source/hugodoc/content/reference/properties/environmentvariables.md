---
title: Environment Variables
weight: 221
---

This page explains the various properties related to Environment variables and other ways of working with environment variables in Framework.

<a name="Section1"></a>
## Getting Environment Variables in Build Scripts ##

There are two ways to get the value of an environment variable in a build script.
The recommended way is to use the build script function `@{GetEnvironmentVariable('VariableName')}` ,
because this method will properly handle special characters in environment variable names and be case insensitive on windows and case sensitive on unix.

There is also a property `sys.env.[Variable Name]` which contains the values of the environment variables.
Like all properties, this property is case sensitive, but all environment variables can be accessed using their original case or all uppper case characters.
And since property names cannot contain certain special characters, such as brackets, any environment variable containing these characters will not exist as a property.


{{% panel theme = "info" header = "Note:" %}}
Up until Framework 7.14.00 all sys.* properties would only exist after executing the &lt;sysinfo&gt; task.
However, they are now automatically defined on NAnt start up and the sysinfo task is now deprecated and no longer does anything.
If your build scrips no longer need to be compatibile with Framework versions 7.14.00 and older you can remove &lt;sysinfo&gt; task.
{{% /panel %}}
<a name="BuildEnv"></a>
## build.env.* ##

The property `build.env.*` is used to set environment variables for when the build toolchain executables are invoked.
This is most commonly done by SDK packages.


```

<property name="build.env.PATH" value="${package.VisualStudio.appdir}\Common7\IDE"/>
<property name="build.env.SystemRoot" value="@{RegistryGetValue('LocalMachine', 'SOFTWARE\Microsoft\Windows NT\CurrentVersion', 'PathName')}"/>

```
<a name="ExecEnv"></a>
## exec.env.* ##

The property `exec.env.*`  is used to control environment variables that are set when the  `<exec>` task invokes a subprocess.

