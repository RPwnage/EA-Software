---
title: contactName
weight: 51
---

This element is used to provide contact name.


{{% panel theme = "info" header = "Note:" %}}
{{< url PackageServer >}}:

 - Both contactName and contactEmail are required if you want to use a distribution list or other mailto.
 - Neither field is required if you want to use the person who uploads the package as the contact.<br><br>If it can&#39;t do either, the person that uploaded the package will be used as the contact.
{{% /panel %}}
## Usage ##

Enter the element in the Manifest file and and enter contact name as an inner text.

## Example ##


```xml
<contactName>Browes, Erin</contactName>.
```

##### Related Topics: #####
-  [Manifest.xml file]( {{< ref "reference/packages/manifest.xml-file/_index.md" >}} ) 
