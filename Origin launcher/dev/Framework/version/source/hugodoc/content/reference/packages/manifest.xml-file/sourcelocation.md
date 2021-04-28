---
title: sourceLocation
weight: 57
---

This element is used to describe the path or depot for the source files.
For packages that are physically stored on package server, we can&#39;t use the downloadURL tag.
In the case of a &quot;dev&quot; release to package server, the downloadURL and the sourceLoction would be the EAOS depot path.

## Usage ##

Enter the element in the Manifest file and and enter source location as an inner text.

## Example ##


```xml
<sourceLocation> //EAOS/rwmath/DL/rwmath/dev </sourceLocation>.
```

##### Related Topics: #####
-  [Manifest.xml file]( {{< ref "reference/packages/manifest.xml-file/_index.md" >}} ) 
