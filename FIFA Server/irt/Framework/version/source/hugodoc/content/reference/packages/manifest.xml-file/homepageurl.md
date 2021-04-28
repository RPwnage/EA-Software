---
title: homePageUrl
weight: 55
---

This element is used to describe package home page URL.


{{% panel theme = "info" header = "Note:" %}}
http links are automatically converted into HREF in the{{< url PackageServer >}}UI.

Accepts http://, file:// or www.
{{% /panel %}}
## Usage ##

Enter the element in the Manifest file and and enter owning team name as an inner text.

## Example ##


```xml
<homePageUrl> http://easites.ea.com/eatech/webservices </homePageUrl>.
```

##### Related Topics: #####
-  [Manifest.xml file]( {{< ref "reference/packages/manifest.xml-file/_index.md" >}} ) 
