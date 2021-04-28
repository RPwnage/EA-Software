---
title: webapp
weight: 164
---

 `webapp` property sets Visual Studio project type to &#39;Web Project&#39;.
As a result additional fields are added to the project file so that visual studio recognizes
it as a web project and enables browser base debugging.

## Usage ##

##### groupname.csproj.webapp  (groupname.fsproj.webapp) #####


Setting this property to true for a C# module will create a Visual Studio web project.

Assembly references added:

   *System.Core.dll*
   *System.Configuration.dll*
   *System.EnterpriseServices.dll*
   *System.Data.DataSetExtensions.dll*
   *System.Web.dll*
   *System.Web.ApplicationServices.dll*
   *System.Web.DynamicData.dll*
   *System.Web.Entity.dll*
   *System.Web.Extensions.dll*
   *System.Web.Services.dll*
   *System.Xml.Linq.dll*
   *Microsoft.CSharp.dll*

Properties used to define webapp project:

 - {{< url groupname >}} `.csproj.webapp` - for C# modules<br><br>{{< url groupname >}} `.fsproj.webapp` - for F# modules

## Example ##


```xml
<property name="runtime.MyModule.buildtype" value="CSharpProgram" />

<property name="runtime.MyModule.webapp" value="true"/>
```
