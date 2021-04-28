---
title: T4 text templates (.tt files)
weight: 178
---

This topic describes how to add T4 text templates to the generated projects.

<a name="Section1"></a>
## Usage ##

T4 text template file can be added to the `resourcefiles`  or to the  `sourcefiles` fileset:


```xml

  <project  xmlns="schemas/ea/framework3.xsd">

    <package name="Example"/>

    <CSharpProgram name="TestT4Template">
      <sourcefiles>
        <includes name="source\**.cs"/>
      </sourcefiles>
      <resourcefiles>
        <includes name="source\**.config"/>
        <includes name="source\**.tt"/>
      </resourcefiles>
    </CSharpProgram>

  </project>


```
Corresponding generated file is added to the same location as the template file. Framework parses template to find extension of the generated file, value of the `output extension=".xx"` is used:


```

<#@ template debug="false" hostspecific="true" language="C#" #>
<#@ output extension=".cs" #>

```
When extension is `.cs`  or  `.vb`  the generated file action is set to  `Compile` , otherwise action is set to content.
Below is a fragment of generated project file:


```xml

    <Compile Include="..\..\source\TextTemplate1.cs">
      <AutoGen>True</AutoGen>
      <DesignTime>True</DesignTime>
      <DependentUpon>TextTemplate1.tt</DependentUpon>
      <Link>source\TextTemplate1.cs</Link>
    </Compile>
    <None Include="..\..\source\TextTemplate1.tt">
      <Generator>TextTemplatingFileGenerator</Generator>
      <LastGenOutput>TextTemplate1.cs</LastGenOutput>
      <Link>source\TextTemplate1.tt</Link>
    </None>

. . . .

  <ItemGroup>
    <Service Include="{508349B6-6B84-4DF5-91F0-309BEEBAD82D}" />
  </ItemGroup>


```

{{% panel theme = "info" header = "Note:" %}}
Framework also checks `hostspecific` attribute and adds service entry if it is set to true.
{{% /panel %}}

{{% panel theme = "warning" header = "Warning:" %}}
File generation from the template requires opening solution and edditing/saving template file. Building generated solution without saving tempplatye file in Visual Studio IDE
does not result in the the regeneration of target file from the template.
{{% /panel %}}
The action applied to generated from the template file can be configured through the build scrtipt if automatic assignment in Framework does not produce desired result:


```xml

  <project  xmlns="schemas/ea/framework3.xsd">

    <package name="Example"/>

    <optionset name="template-action">
      <option name="visual-studio-tool" value="None"/>
    </optionset>

    <CSharpProgram name="TestT4Template">
      <sourcefiles>
        <includes name="source\**.cs"/>
      </sourcefiles>
      <resourcefiles>
        <includes name="source\**.config"/>
        <includes name="source\**.tt" optionset="template-action"/>
      </resourcefiles>
    </CSharpProgram>

  </project>

```
Now action  `None` is assigned to the generated file:


```xml

    <None Include="..\..\source\TextTemplate1.cs">
      <AutoGen>True</AutoGen>
      <DesignTime>True</DesignTime>
      <DependentUpon>TextTemplate1.tt</DependentUpon>
      <Link>source\TextTemplate1.cs</Link>
    </None>
    <None Include="..\..\source\TextTemplate1.tt">
      <Generator>TextTemplatingFileGenerator</Generator>
      <LastGenOutput>TextTemplate1.cs</LastGenOutput>
      <Link>source\TextTemplate1.tt</Link>
    </None>

. . . .

  <ItemGroup>
    <Service Include="{508349B6-6B84-4DF5-91F0-309BEEBAD82D}" />
  </ItemGroup>


```
It is also possible to add additional meta elements to the generated file item in the project file. Use option  `visual-studio-tool-properties` :


```xml

  <project  xmlns="schemas/ea/framework3.xsd">

    <package name="Example"/>

    <optionset name="template-action">
      <option name="visual-studio-tool" value="None"/>
      <option name="visual-studio-tool-properties">
        AdditionaOptionOne=AdditionalValue
        OtherOption=OtherValue
      </option>
    </optionset>

    <CSharpProgram name="TestT4Template">
      <sourcefiles>
        <includes name="source\**.cs"/>
      </sourcefiles>
      <resourcefiles>
        <includes name="source\**.config"/>
        <includes name="source\**.tt" optionset="template-action"/>
      </resourcefiles>
    </CSharpProgram>

  </project>

```
Now action `None` is assigned to the generated file:


```xml

    <None Include="..\..\source\TextTemplate1.cs">
      <AutoGen>True</AutoGen>
      <DesignTime>True</DesignTime>
      <DependentUpon>TextTemplate1.tt</DependentUpon>
      <AdditionaOptionOne>AdditionalValue</AdditionaOptionOne>
      <OtherOption>OtherValue</OtherOption>
      <Link>source\TextTemplate1.cs</Link>
    </None>
    <None Include="..\..\source\TextTemplate1.tt">
      <Generator>TextTemplatingFileGenerator</Generator>
      <LastGenOutput>TextTemplate1.cs</LastGenOutput>
      <Link>source\TextTemplate1.tt</Link>
    </None>

. . . .

  <ItemGroup>
    <Service Include="{508349B6-6B84-4DF5-91F0-309BEEBAD82D}" />
  </ItemGroup>


```
