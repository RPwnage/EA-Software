---
title: How to write Structured XML scripts
weight: 200
---

This section provides general advice on writing structured XML scripts

<a name="HowToStart"></a>
## How to Start ##

 - Use  [Intellisense]( {{< ref "framework-101/intellisense.md" >}} ) . It is especially effective with structured XML
 - Start with adding  `<project xmlns="schemas/ea/framework3.xsd">` and `<package name="">` elements. These elements are the same in traditional input and in structured XML.
 - Try to avoid adding script outside of structured elements if possible.


{{% panel theme = "info" header = "Note:" %}}
Structured XML allows conditional inclusion/exclusion of most structured elements using `<do>` element.
For more details see [How to include elements conditionally]( {{< ref "reference/packages/structured-xml-build-scripts/howtoaddconditions.md" >}} )
{{% /panel %}}
<a name="NativeModules"></a>
## Native Modules ##

Use one of the following elements to describe native modules: `<Library>` , `<Program>` , `<DynamicLibraryLibrary>` , `<WindowsProgram>` , `<ManagedCPPAssembly>` , `<ManagedCPPProgram>` , `<ManagedCPPWindowsProgram>` .

For more information see  [How to set module 'buildtype']( {{< ref "reference/packages/structured-xml-build-scripts/setbuildtype.md" >}} )  and  [Structured XML build scripts]( {{< ref "reference/packages/structured-xml-build-scripts/_index.md#StructuredXMLTaskHierarchy" >}} ) 

Here is example of Native library module definition in structured XML:


```xml

  <project default="build" xmlns="schemas/ea/framework3.xsd">

    <package  name="FreeType"/>

    <Library name="FreeType" buildtype="CLibrary">
      <config>
        <buildoptions>
          <option name="buildset.cc.options" if="${config-compiler}==vc">
            ${option.value}
            -W3
          </option>
        </buildoptions>
        <warningsuppression>
          <do if="${config-compiler}==vc">
            -wd4255
          </do>
          <do if="${config-compiler}==clang">
            -Wno-empty-body
          </do>
        </warningsuppression>
        <defines>
          FT2_BUILD_LIBRARY
          FB_FT_BUILD
        </defines>
      </config>
      <includedirs>
        ${package.dir}/include
      </includedirs>
      <headerfiles>
        <includes name="**.h"/>
      </headerfiles>
      <sourcefiles>
        <includes name="src/**.c"/>
      </sourcefiles>
      <do if="${config-compiler}==vc">
        <buildsteps>
          <do if="${config-system}==pc">
            <postbuild-step>
              <command>
                @echo PC: Example of conditionally including Structured XML elements.
              </command>
            </postbuild-step>
          </do>
          <do if="${config-system}==pc64">
            <postbuild-step>
              <command>
                @echo PC64: Example of conditionally including Structured XML elements.
              </command>
            </postbuild-step>
          </do>
        </buildsteps>
      </do>
    </Library>

  </project>

```
<a name="DotNetModules"></a>
## DotNet Modules ##

Use one of the following elements to describe DotNet modules: `<CSharpLibrary>` , `<CSharpProgram>` , `<CSharpWindowsProgram>`

Here is an example of C# assembly  module definition in structured XML:


```xml

  <project default="build" xmlns="schemas/ea/framework3.xsd">

    <package  name="Example"/>
  
    <CSharpLibrary name="ExampleAssembly">
      <rootnamespace>EA.ExampleAssembly</rootnamespace>
      <dependencies>
        <build>
          DependentPackage
          DependentPackageA/Module1
        </build>
      </dependencies>
      <assemblies>
        <includes name="${package.dir}/bin/log4net.dll"/>
        <includes name="WindowsBase.dll" asis="true"/>
      </assemblies>
      <sourcefiles>
        <includes name="**.cs"/>
        <includes name="**.xaml"/>
      </sourcefiles>
      <resourcefiles>
        <includes name="Resources/**.fx"/>
      </resourcefiles>
      <nonembed-resourcefiles>
        <includes name="Resources/**.*"/>
      </nonembed-resourcefiles>
    </CSharpLibrary>
  </project>

```
<a name="UtilityModules"></a>
## Utility Modules ##

 [Utility modules]( {{< ref "reference/packages/build-scripts/utility-modules/_index.md" >}} )  allow to define  [Pre/Post Build Steps]( {{< ref "reference/packages/build-scripts/native-c/c++-modules/prepostbuildsteps.md" >}} ) , [Custom Build Steps]( {{< ref "reference/packages/build-scripts/native-c/c++-modules/custombuildsteps.md" >}} ) ,
and [Custom Build Steps on Individual Files]( {{< ref "reference/packages/build-scripts/native-c/c++-modules/custom-build-files.md" >}} ) .

Example below demonstrates custom build steps in Utility module:


```xml

  <project default="build" xmlns="schemas/ea/framework3.xsd">

    <package  name="CompanionApp"/>

    <Utility name="CompanionApp">
      <custombuildfiles>
        <includes name="html/*.html" optionset="companionapp-html"/>
        <includes name="html/*.css" optionset="companionapp-html"/>
        <includes name="html/*.js" optionset="companionapp-html"/>
        <includes name="*.bat" optionset="companionapp-launcher"/>
      </custombuildfiles>
      <buildsteps>
        <prebuild-step>
          <command>
            @echo Creating CompanionApp folder
            md "${package.configbuilddir}\CompanionApp\html"
            set ERRORLEVEL=0
          </command>
        </prebuild-step>
      </buildsteps>
    </Utility>
  
    <optionset name="companionapp-html">
      <option name="name" value="buildinfo"/>
      <option name="description" value='Copying CompanionApp file: "%filename%%fileext%"...'/>
      <option name="build.tasks"            value="buildinfo"/>
      <option name="buildinfo.command"   value="copy  %(fullpath) ${package.configbuilddir}\CompanionApp\html\%filename%%fileext%"/>
      <option name="outputs">
        ${package.configbuilddir}\CompanionApp\html\%filename%%fileext%
      </option>
    </optionset>
              
    <optionset name="companionapp-html">
      <option name="name" value="buildinfo"/>
      <option name="description" value='Copying CompanionApp file: "%filename%%fileext%"...'/>
      <option name="build.tasks"            value="buildinfo"/>
      <option name="buildinfo.command"   value="copy  %(fullpath) ${package.configbuilddir}\CompanionApp\html\%filename%%fileext%"/>
      <option name="outputs">
        ${package.configbuilddir}\CompanionApp\html\%filename%%fileext%
      </option>
    </optionset>

  </project>

```
<a name="MakeStyleModules"></a>
## MakeStyle Modules ##

 [Make Style modules]( {{< ref "reference/packages/build-scripts/make-style-modules/_index.md" >}} ) are used to execute external build or clean commands.


```xml

<MakeStyle name="MyModule">
  <headerfiles>
    <includes name="include/**.h"/>
  </headerfiles>
  <sourcefiles>
    <includes name="source/**.c"/>
  </sourcefiles>
  <MakeBuildCommand>
    @echo Executing runtime.MyModule.MakeBuildCommand
    @echo inline const char *GetConfigTestString() { return "${config}"; } > output.txt
    @echo Finished runtime.MyModule.MakeBuildCommand
  </MakeBuildCommand>
</MakeStyle>

```
