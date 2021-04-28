---
title: Functions
weight: 237
---

This section contains auto-generated documentation explaining functions available
in build scripts and related xml files.

<a name="What"></a>
## Usage ##

Functions can be used as the value of any attribute in an xml build script.
Below are a couple examples of how to use functions:


```xml
<echo message="@{MathPI()}"/>
<echo message="@{StrToLower('Hello World')}"/>
<echo message="@{FileExists('${package.builddir}\${cfg}\bin\project.exe')}"/>
```
<a name="defining"></a>
## Defining a Function ##

In framework, sets of functions are grouped in classes that extend `NAnt.Core.FunctionClassBase` and have a FunctionClass attribute with the name of the set.
Any function defined within a function class can be called within a build script as long as it has the Function attribute and its first parameter is of type Project.
Also, the function must be static, return a string and all parameters other than project attribute must be convertible to strings.
The project parameter gets set automatically and does not appear when calling the function in the build scripts.

When adding custom functions you can either create a new function class by providing a unique name, or add functions to an existing function class by using the name of an existing function class within the function class attribute.
Function class names are primarily for documentation purposes and do not change how the function is called from within a build script.


{{% panel theme = "info" header = "Note:" %}}
Function names need not be unique, however, functions with the same name are differentiated by the number of parameters, regardless of types.
{{% /panel %}}
###### A fragment of the string functions class ######

```C#
/// <summary>Collection of string manipulation routines.</summary>
[FunctionClass("String Functions")]
public class StringFunctions : FunctionClassBase
{
 /// <summary>Gets the number of characters in a string.</summary>
    [Function()]
 public static string StrLen(Project project, string strA)
 {
  return strA.Length.ToString();
 }
}
```
