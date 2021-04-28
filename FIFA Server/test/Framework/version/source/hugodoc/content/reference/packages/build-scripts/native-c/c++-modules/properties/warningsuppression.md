---
title: warningsuppression
weight: 133
---

Warning suppression property lets to set additional compiler warning suppression options

<a name="WarningsupressionUsage"></a>
## Usage ##

Define SXML `<config><warningsuppression>`  element or a property {{< url groupname >}} `.warningsuppression` in your build script.
The value of this property can contain compiler specific warning suppression options.

It is recommended to define each option on a separate line.

## Example ##


```xml

    <project default="build" xmlns="schemas/ea/framework3.xsd">

      <package name="HelloWorldLibrary"/>

      <Library name="Hello">
        <config>
          <warningsuppression>
            <do if="${config-compiler}==vc">
              -wd4350
              -wd4254
            </do>
          </warningsuppression>
        </config>
        <includedirs>
          include
        </includedirs>
        <sourcefiles>
          <includes name="source/hello/*.cpp" />
        </sourcefiles>
      </Library>

```

##### Related Topics: #####
-  [warningsupression and other options can also be set through custom buildoptionset]( {{< ref "reference/packages/build-scripts/buildtype-and-options/createcustombuildtype.md" >}} ) 
-  [How to define custom build options in a module]( {{< ref "reference/packages/structured-xml-build-scripts/customoptions.md" >}} ) 
