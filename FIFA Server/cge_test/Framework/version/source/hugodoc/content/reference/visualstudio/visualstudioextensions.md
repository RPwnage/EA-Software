---
title: Visual Studio Extensions
weight: 268
---

The page describes how to extend and override the visual studio extensions used when generating visual studio project files.

<a name="overview"></a>
## Overview ##

Visual Studio Extensions are used for altering the solution generation process to allow adding configuration specific xml elements to project files.
Extensions are usually added to configuration packages for configurations that require them, but they can also be overriden in other packages.

Extensions will affect the solution generation process for all of the modules involved in the build.
There is another for adding msbuild options to project files that can be set on specific modules and custom build types.
(For more info see: [Visual Studio Custom Project File Elements]( {{< ref "reference/visualstudio/vsconfigbuildoptions.md" >}} ) )

<a name="ProjectFileExtension"></a>
## Project Level Extensions ##

To add a Visual Studio extension (for vcxproj / csproj files), you need to create a C# class that extends &quot;IMCPPVisualStudioExtension&quot; (for vcxproj file) or &quot;IDotNetVisualStudioExtension&quot;
(for csproj file).  You also need to add the TaskName attribute field above the class.
Then in your build script you would need to add a taskdef task for building the script into an assembly.
You also need to add a property with the name &quot;${config-platform}-visualstudio-extension&quot; or &quot;{{< url groupname >}}.visualstudio-extension&quot; and in this property list the names of the extensions based on their &quot;TaskName&quot; field.
Using the property &quot;${config-platform}-visualstudio-extension&quot; will apply the extension to all projects/modules for a specific platform.
Using the property &quot;{{< url groupname >}}.visualstudio-extension&quot; will apply the extension to a specific module.

Here are some examples of how to write a visual studio extension.
For reference, interface IMCPPVisualStudioExtension is defined in framework here: source\EA.Tasks\EA.Config\Backends\VisualStudio\IVisualStudioExtension.cs

###### Example C# Task ######

```csharp
using System;
using NAnt.Core;
using NAnt.Core.Attributes;
using EA.Eaconfig.Core;

namespace EA.Eaconfig
{
    [TaskName("my-custom-capilano-vc-extension")]
    public class My_Custom_VisualStudioExtension : IMCPPVisualStudioExtension
    {
        // update the vcproj files with the compiler/linker override
        public override void WriteExtensionToolProperties(IXmlWriter writer)
        {
            writer.WriteNonEmptyElementString("MyCustomMsBuildOption", "value");
        }
    }
}
```
###### Example Build Script Using SXML Style ######

```xml
<taskdef assembly="my_extension.dll">
  <sources>
    <includes name="${package.dir}/scripts/MyCustomVisualStudioExtension.cs"/>
  </sources>
</taskdef>

<Program name="mymodule">
   <sourcefiles>
     <includes name="source/*.cpp"/>
   </sourcefiles>
   <visualstudio>
     <extension>
       my-custom-vc-extension
     </extension>
   </visualstudio>
</Program>
```
###### Example Build Script ######

```xml
<taskdef assembly="my_extension.dll">
  <sources>
    <includes name="${package.dir}/scripts/MyCustomVisualStudioExtension.cs"/>
  </sources>
</taskdef>

<property name="capilano-vc-visualstudio-extension">
  ${property.value}
  my-custom-capilano-vc-extension
</property>
```
IMCPPVisualStudioExtension provides several functions that can be overriden to write text to various parts of the project file.
Here is a list of each of the functions, followed by a visual representation of where they get added to the project file.

###### Example of each of the functions of IMCPPVisualStudioExtension ######

```csharp
using System;
using System.Collections.Generic;
using NAnt.Core;
using NAnt.Core.Tasks;
using NAnt.Core.Attributes;
using EA.Eaconfig.Core;

namespace EA.Eaconfig
{
 [TaskName("my-custom-vc-extension")]
 public class My_Custom_VisualStudioExtension : IMCPPVisualStudioExtension
 {
  // Added near the beginning of the project file, before the 'Globals' section
  public override void WriteExtensionProjectProperties(IXmlWriter writer)
  {
   // each of these functions gets executed once per config, so it is necessary to use a do once block to prevent duplicates.
   // the key used by each do once block can be anything but must be unqiue to each block.
   string doOncekey = string.Format("{0}.ExtensionProjectProperties.do-once", Module.GroupName);
   DoOnce.Execute(doOncekey, () =>
   {
    writer.WriteNonEmptyElementString("MyExtensionProjectProperties", "value");
   });
  }

  // by adding strings to the dictionary they get added to the 'Globals' PropertyGroup
  public override void ProjectGlobals(IDictionary<string, string> projectGlobals)
  {
   string doOncekey = string.Format("{0}.projectGlobals.do-once", Module.GroupName);
   DoOnce.Execute(doOncekey, () =>
   {
    projectGlobals.Add("myGlobal", "value");
   });
  }
		
  // by adding strings to the dictionary they get added to the 'Configurations' PropertyGroup
  public override void ProjectConfiguration(IDictionary<string, string> projectConfigurations)
  {
   string doOncekey = string.Format("{0}.projectConfigurations.do-once", Module.GroupName);
   DoOnce.Execute(doOncekey, () =>
   {
    projectConfigurations.Add("myConfiguration", "value");
   });
  }

  // writes to the tool section of the project file where settings for tools like ClCompile and Link are defined.
  public override void WriteExtensionTool(IXmlWriter writer)
  {
   string doOncekey = string.Format("{0}.ExtensionTool.do-once", Module.GroupName);
   DoOnce.Execute(doOncekey, () =>
   {
    writer.WriteStartElement("MyTool");
    writer.WriteNonEmptyElementString("ToolOption", "value");
    writer.WriteEndElement();
   });
  }

  // writes to the property group after the tool section
  public override void WriteExtensionToolProperties(IXmlWriter writer)
  {
   string doOncekey = string.Format("{0}.ExtensionToolProperties.do-once", Module.GroupName);
   DoOnce.Execute(doOncekey, () =>
   {
    writer.WriteNonEmptyElementString("MyWriteExtensionToolProperties", "value");
   });
  }

  // writes to the section of the after just after the item group where source files are listed
  public override void WriteExtensionItems(IXmlWriter writer)
  {
   string doOncekey = string.Format("{0}.ExtensionItems.do-once", Module.GroupName);
   DoOnce.Execute(doOncekey, () =>
   {
    writer.WriteStartElement("ItemGroup");
    writer.WriteStartElement("CustomTool");
    writer.WriteAttributeString("Include", "mycustomfile.dll");
    writer.WriteEndElement();
    writer.WriteEndElement();
   });
  }

  // writes to the file after the msbuild targets have been loaded, presumably so that users can override msbuild settings
  public override void WriteMsBuildOverrides(IXmlWriter writer)
  {
   string doOncekey = string.Format("{0}.MsBuildOverrides.do-once", Module.GroupName);
   DoOnce.Execute(doOncekey, () =>
   {
    writer.WriteNonEmptyElementString("MyWriteMsBuildOverrides", "value");
   });
  }

  // adds entires to the module.vcxproj.user file
  public override void UserData(IDictionary<string, string> projectGlobals)
  {
   string doOncekey = string.Format("{0}.UserData.do-once", Module.GroupName);
   DoOnce.Execute(doOncekey, () =>
   {
    projectGlobals.Add("myUserData", "value");
   });
  }
 }
}
```
###### A visual representation of where each of the functions writes to in a project file ######

```xml
<project xmlns="schemas/ea/framework3.xsd">
 <!-- Set default startup configuration -->
 <PropertyGroup>
  <Configuration/>
  <Platform/>
 </PropertyGroup>
 <!--
 *************************************************************
 ***   WriteExtensionProjectProperties(IXmlWriter writer)  ***
 *************************************************************
 -->
 <ItemGroup Label="ProjectConfigurations">
  <ProjectConfiguration>
   <Configuration/>
   <Platform/>
  </ProjectConfiguration>
 </ItemGroup>
 <PropertyGroup Label="Globals">
  <!--
  ********************************************************************
  ***   ProjectGlobals(IDictionary<string, string> projectGlobals) ***
  ********************************************************************
  -->
  <ProjectGuid/>
  <ProjectName/>
 </PropertyGroup>
 <Import Project="$(VCTargetsPath)\Microsoft.Cpp.Default.props" />
 <PropertyGroup Label="Configuration">
  <!--
  ********************************************************************************
  ***  ProjectConfiguration(IDictionary<string, string> ConfigurationElements) ***
  ********************************************************************************
  -->
  <ConfigurationType/>
  <PlatformToolset/>
 </PropertyGroup>
 <Import Project="$(VCTargetsPath)\Microsoft.Cpp.props" />
 <ImportGroup Label="PropertySheets">
  <Import Project="$(UserRootDir)\Microsoft.Cpp.$(Platform).user.props" Condition="exists('$(UserRootDir)\Microsoft.Cpp.$(Platform).user.props')" />
 </ImportGroup>
 <ItemDefinitionGroup>
  <Midl />
  <ClCompile/>
  <Link>
  <!--
  ******************************************************************************************
  ***   WriteExtensionLinkerToolProperties(IDictionary<string, string> toolProperties)   ***
  ******************************************************************************************
  *******************************************************
  ***   WriteExtensionLinkerTool(IXmlWriter writer)   ***
  *******************************************************
  -->
  </Link>
  <PreBuildEvent/>
  <PostBuildEvent/>
  <!--
  *************************************************
  ***   WriteExtensionTool(IXmlWriter writer)   ***
  *************************************************
  -->
 </ItemDefinitionGroup>
 <PropertyGroup>
  <OutDir/>
  <IntDir/>
  <TargetName/>
  <TargetExt/>
  <!--
  *********************************************************
  ***   WriteExtensionToolProperties(IXmlWriter writer) ***
  *********************************************************
  *********************************************************
  ***   SetVCPPDirectories(VCPPDirectories directories) ***
  *********************************************************
  -->
  <ExecutablePath/>
  <IncludePath/>
  <ReferencePath>$(ReferencePath)</ReferencePath>
  <LibraryPath>$(LibraryPath)</LibraryPath>
  <SourcePath>$(SourcePath)</SourcePath>
 </PropertyGroup>
 <ItemGroup Label="ProjectReferences">
 </ItemGroup>
 <ItemGroup>
  Files
 </ItemGroup>
 <!--
 ************************************************
 ***   WriteExtensionItems(IXmlWriter writer) ***
 ************************************************
 -->
 <Import Project="$(VCTargetsPath)/Microsoft.Cpp.targets" />
 <!--
 **************************************************
 ***   WriteMsBuildOverrides(IXmlWriter writer) ***
 **************************************************
 -->
 <ProjectExtensions/>
</Project>
```
<a name="SolutionExtension"></a>
## Solution Level Extensions ##

To add a Visual Studio extension for solution (sln file) level (such as writing special task that create
custom files parallel to the generated sln file), you need to create a C# class that extends &quot;IVisualStudioSolutionExtension&quot;.
You also need to add the TaskName attribute field above the class.
Then in your build script you would need to add a taskdef task for building the script into an assembly.
You also need to add a property with the name &quot;visualstudio-solution-extensions&quot; and in this property list the names of the extensions based on their &quot;TaskName&quot; field.

At present, only &quot;PostGenerate()&quot; override is supported.  If your team have specific need to inject script to the sln file generation, please contact{{< url SupportEmailLink >}}to discuss your need.

###### Example C# Task ######

```csharp
using System.IO;
using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Attributes;
using EA.Eaconfig.Core;

namespace ResharperSupport
{
 [TaskName("resharper-support")]
 public class ResharperSupportTask : IVisualStudioSolutionExtension
 {
  public override void PostGenerate()
  {
   string resharperTemplate = Project.Properties.GetPropertyOrDefault("resharper-dotsettings-template", null);

   if (string.IsNullOrEmpty(resharperTemplate))
   {
    throw new BuildException("ERROR: The resharper-support task expects the property 'resharper-dotsettings-template' to be set and points to the full path of the settings file template.");
   }

   resharperTemplate = PathNormalizer.Normalize(resharperTemplate);

   if (!File.Exists(resharperTemplate))
   {
    throw new BuildException("ERROR: The resharper-dotsettings-template '{0}' does not exists.", resharperTemplate);
   }

   string destinationPath = Path.Combine(SolutionGenerator.Interface.OutputDir.Path, SolutionGenerator.Interface.Name + ".sln.DotSettings");

   string filecontent = File.ReadAllText(resharperTemplate);
   filecontent = filecontent.Replace("%dotsettings_full_path%", destinationPath);
   File.WriteAllText(destinationPath,filecontent);
  }
 }
}
```
###### Example Build Script ######

```xml
<taskdef assembly="${package.YOUR_TEAM_CONFIG_PACKAGE.builddir}/ResharperSupport/ResharperSupport.dll">
    <sources>
        <includes name="scripts/*.cs"/>
    </sources>
</taskdef>
<property name="resharper-dotsettings-template" value="${package.YOUR_TEAM_CONFIG_PACKAGE.dir}/data/Resharper.DotSettings.Template"/>
<property name="visualstudio-solution-extensions">
    ${property.value}
    resharper-support
</property>
```
