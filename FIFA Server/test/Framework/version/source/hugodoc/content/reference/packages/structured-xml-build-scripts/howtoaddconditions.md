---
title: How to include elements conditionally
weight: 201
---

This topic describes how to include or exclude Structured XML elements conditionally.

<a name="HowToIncludeElementsConditionally"></a>
## How to include Structured XML elements conditionally ##

Many Structured XML elements have  `if`  and  `unless` attributes that can be used to include modules conditionally.

For example, module definition can be included conditionally:


```xml

  <project default="build" xmlns="schemas/ea/framework3.xsd">

    <package  name="Example"/>

    <Library name="MyLibrary" buildtype="CLibrary">
      . . . .
    </Library>

    <ManagedCPPAssembly name="MyAssembly" if="${config-system}==pc OR ${config-system}==pc64">
      . . . .
    </ManagedCPPAssembly>

  </project>


```
Or same result can be achieved using  `<do>` element:


```xml

  <project default="build" xmlns="schemas/ea/framework3.xsd">

    <package  name="Example"/>

    <tests>

      <Library name="MyLibrary" buildtype="CLibrary">
        . . . .
      </Library>

      <ManagedCPPAssembly name="MyAssembly" if="${config-system}==pc OR ${config-system}==pc64">
        . . . .
      </ManagedCPPAssembly>

      <do if="${config-system}==pc OR ${config-system}==pc64">

        <ManagedCPPAssembly name="MySecongAssembly" >
          . . . .
        </ManagedCPPAssembly>

      </do>

    </tests>

  </project>

```
Similar patterns can be applied to other elements.

Changing buildtype using `if` :


```xml

  <Library name="MyLibrary">
    <buildtype name="DynamicLibrary" if="${Dll}"/>
    <buildtype name="ManagedCppAssembly" if="${config-system}==pc"/>
    <sourcefiles>
    <includes name="src/**.cpp"/>
    <sourcefiles>
    . . . .
  </Library>

```
Or  using `<do>` :


```xml

  <Library name="MyLibrary">
    <do if="${Dll}">
        <buildtype name="DynamicLibrary"/>
    </do>
    <buildtype name="ManagedCppAssembly" if="${config-system}==pc"/>
    <sourcefiles>
    <includes name="src/**.cpp"/>
    <sourcefiles>
    . . . .
  </Library>

```
Some elements do not have `if`  and  `unless` attributes.
Using `<do>` is the only option.


{{% panel theme = "info" header = "Note:" %}}
Use [Intellisense]( {{< ref "framework-101/intellisense.md" >}} )  or  [Structured XML Tasks]( {{< ref "reference/tasks/structured-xml-tasks/_index.md" >}} ) to find what attributes and elements are supported in each case.
{{% /panel %}}

```xml

    <Library . . .
              
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


```

##### Related Topics: #####
-  [Installing Intellisense for Build Files]( {{< ref "framework-101/intellisense.md" >}} ) 
-  [Structured XML Tasks]( {{< ref "reference/tasks/structured-xml-tasks/_index.md" >}} ) 
