---
title: packageName
weight: 45
---

This element is used to define the name of the package.

Currently this field is used to drive systems external to the package server that are consuming this
information primarily: Framework and the EATech Build Farm.

 - Used to create the folder structure in the file share.
 - Used to link the package to the release.


{{% panel theme = "info" header = "Note:" %}}
- It must be unique.
 - Can have alpha numeric characters.

See more info about packages: [What Is A Framework Package?]( {{< ref "reference/packages/whatispackage.md" >}} )
{{% /panel %}}
## Usage ##

Enter the element in the Manifest file and specify package name as an inner text.


{{% panel theme = "info" header = "Note:" %}}
- Required for manifest file only packages.
 - Recommended for all packages.
{{% /panel %}}
## Example ##


```xml
<packageName>DevTrack</packageName>.
```

##### Related Topics: #####
-  [Manifest.xml file]( {{< ref "reference/packages/manifest.xml-file/_index.md" >}} ) 
-  [What Is A Framework Package?]( {{< ref "reference/packages/whatispackage.md" >}} ) 
