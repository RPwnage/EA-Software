---
title: PackagingElement
weight: 1024
---
## Syntax
```xml
<PackagingElement packagename="" deployassets="" copyassetswithoutsync="">
  <assetfiles basedir="" fromfileset="" failonmissing="" name="" append="" />
  <assetdependencies />
  <asset-configbuilddir />
  <manifestfile append="" name="" optionset="" />
  <echo />
  <do />
</PackagingElement>
```
## Summary ##
PackagingElement


### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| packagename | Sets or gets the package name | String | False |
| deployassets | If true assetfiles are deployed/packaged according to platform requirements. Default is true for programs. | Boolean | False |
| copyassetswithoutsync | Specify assets files are copied only and do not perform asset directory synchronization. Default is false for programs. | Boolean | False |

### Nested Elements
| Name | Description | Type | Required |
| ---- | ----------- | ---- | -------- |
| {{< task "EA.Eaconfig.Structured.StructuredFileSetCollection" "assetfiles" >}}| Asset files defined by the module.<br>The asset files directory will be synchronized to match this fileset, therefore only one assetfiles fileset can be defined per package.<br>To attach asset files to multiple modules use the assetfiles &#39;fromfileset&#39; attribute. | {{< task "EA.Eaconfig.Structured.StructuredFileSetCollection" >}} | False |
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "assetdependencies" >}}| Sets or gets the asset dependencies | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" "asset-configbuilddir" >}}| Sets or gets the asset-configbuilddir | {{< task "EA.Eaconfig.Structured.ConditionalPropertyElement" >}} | False |
| {{< task "EA.Eaconfig.Structured.StructuredFileSet" "manifestfile" >}}| Gets the manifest file | {{< task "EA.Eaconfig.Structured.StructuredFileSet" >}} | False |

## Full Syntax
```xml
<PackagingElement packagename="" deployassets="" copyassetswithoutsync="">
  <assetfiles if="" unless="" basedir="" fromfileset="" failonmissing="" name="" append="">
    <includes />
    <excludes />
    <do />
    <group />
  </assetfiles>
  <assetdependencies if="" unless="" value="" append="">
    <do />
    <!--Text-->
  </assetdependencies>
  <asset-configbuilddir if="" unless="" value="" append="">
    <do />
    <!--Text-->
  </asset-configbuilddir>
  <manifestfile append="" name="" optionset="" defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" if="" unless="">
    <includes />
    <excludes />
    <do />
    <group />
    <!--Structured Fileset-->
  </manifestfile>
  <echo />
  <do />
</PackagingElement>
```
