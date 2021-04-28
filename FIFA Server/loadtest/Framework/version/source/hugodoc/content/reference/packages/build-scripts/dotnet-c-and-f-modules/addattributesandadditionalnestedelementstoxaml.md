---
title: Add attributes and nested elements to Xaml files
weight: 177
---

Framework allows to define additinal attributes and nested elements for xaml files in generated Visual Studio projects

<a name="Section1"></a>
## How to add elements to xaml tool ##

To add attributes or sub elements to xaml tool in generated Visual Studio project define an optionset with following options:

 - **visual-studio-tool-attributes** - Additional attributes for the tool XML element.<br>Each attribute should be defined on a separate line using format `Name=Value` .
 - **visual-studio-tool-properties** - Additional nested elements (properties) for the tool XML element.<br>Each property should be defined on a separate line using format `Name=Value` .

## Example ##


```xml

<optionset name="custom-design-time-attributes">
  <option name="visual-studio-tool-properties">
    ContainsDesignTimeResources=true
  </option>
  <option name="visual-studio-tool-attributes">
    Condition='$(DesignTime)'=='true' OR ('$(SolutionPath)'!='' AND Exists('$(SolutionPath)') AND '$(BuildingInsideVisualStudio)'!='true' AND '$(BuildingInsideExpressionBlend)'!='true')
  </option>
</optionset>
  
<CSharpLibrary name="MyAssembly">
  <sourcefiles>
    <includes name="source/**.cs"/>
    <includes name="source/**.xaml" optionset="custom-design-time-attributes"/>
  </sourcefiles>
</CSharpLibrary>

```
