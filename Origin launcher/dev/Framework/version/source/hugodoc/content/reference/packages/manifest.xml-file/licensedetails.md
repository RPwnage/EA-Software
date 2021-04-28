---
title: licenseDetails
weight: 68
---

This element contains License Information Block.

## Usage ##

Enter the element in the Manifest file and specify following nested elements: `containsOpenSource` ,  `licenseType` ,  `internalOnly` ,  `licenseComment` .

##### containsOpenSource #####
Boolean *true* / *false* 

If true, then the package should have license info on package server with links to the deal sheet(s).

##### licenseType #####
If EA Proprietary, should have Dev SL like: rwmath/1.15.02/EA_DEV_SOFTWARE_LICENSE.TXT.
Other license types include 3rd Party Open Source &amp; 3rd Party Commercial.

##### internalOnly #####
Boolean *true* / *false* 

If the internalOnly flag is set, then there is some reason to not allow outsourcing of this package.
Information pertaining to this flag should be added to the Licensing info page on Package Server.

##### licenseComment #####
The license Comment field is an existing field that can be reused to add extra information to the package if needed.


{{% panel theme = "info" header = "Note:" %}}
HTML enabled
{{% /panel %}}
## Example ##


```xml
<licenseDetails>
    <containsOpenSource>True</containsOpenSource>
    <licenseType>EA Proprietary</licenseType>
    <internalOnly>False</internalOnly>
    <licenseComment />
</licenseDetails>
```

##### Related Topics: #####
-  [Manifest.xml file]( {{< ref "reference/packages/manifest.xml-file/_index.md" >}} ) 
