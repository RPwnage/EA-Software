---
title: changes
weight: 48
---

This element is used to describe list of changes in this package release.

## Usage ##

Enter the element in the Manifest file and and enter changes as an inner text.


{{% panel theme = "info" header = "Note:" %}}
HTML enabled
{{% /panel %}}

{{% panel theme = "info" header = "Note:" %}}
http links are automatically converted into HREF in the{{< url PackageServer >}}UI.
{{% /panel %}}
## Example ##


```xml
<changes>
<p>
There is some cool stuff in here. You can have now have links like http://easites.ea.com/EATech/.
Links will automatically be turned into HREF tags in the UI.
</p>
</changes>.
```

##### Related Topics: #####
-  [Manifest.xml file]( {{< ref "reference/packages/manifest.xml-file/_index.md" >}} ) 
