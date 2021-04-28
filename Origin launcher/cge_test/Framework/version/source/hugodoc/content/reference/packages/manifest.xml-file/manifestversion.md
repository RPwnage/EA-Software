---
title: manifestVersion
weight: 44
---

This element is used to describe manifest file XML schema revision.

Currently this field is used to drive systems external to the package server that are consuming this
information primarily: Framework and the EATech Build Farm.

## Usage ##

Enter the element in the Manifest file and specify one of the following values as an inner text.

##### 1 #####
Manifest file XML schema revision 1

##### 2 #####
Manifest file XML schema revision 2

## Example ##


```xml
<manifestVersion>2</manifestVersion>.
```

##### Related Topics: #####
-  [Manifest.xml file]( {{< ref "reference/packages/manifest.xml-file/_index.md" >}} ) 
