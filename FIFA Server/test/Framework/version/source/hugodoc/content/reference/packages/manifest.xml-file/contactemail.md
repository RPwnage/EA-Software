---
title: contactEmail
weight: 52
---

This element is used to provide contact e-mail.

This can be an email address or a url (http://, file:// or www.)


{{% panel theme = "info" header = "Note:" %}}
{{< url PackageServer >}}:

 - Package Server will first try to match an employee, then it will use the contactName &amp; contactEmail as is (if both are filled in).
 - If it can&#39;t do either, the person that uploaded the package will be used as the contact
 - Both contactName and contactEmail are required if you want to use a distribution list or other mailto.
{{% /panel %}}
## Usage ##

Enter the element in the Manifest file and and enter contact e-mail as an inner text.

## Example ##


```xml
<contactEmail>EATechAdmin@ea.com</contactEmail>.
```

##### Related Topics: #####
-  [Manifest.xml file]( {{< ref "reference/packages/manifest.xml-file/_index.md" >}} ) 
