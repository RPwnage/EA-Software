---
title: ondemandroot
weight: 251
---

This element is used to specify the location where packages are installed when automatically downloaded from the [Package Server]( {{< ref "reference/package-management-tools/packageserver.md" >}} ) .


{{% panel theme = "info" header = "Note:" %}}
This element can be overridden by environment variable FRAMEWORK_ONDEMAND_ROOT.  If this environment variable is specified, content of this environment variable will
override all ondemandroot settings.
{{% /panel %}}
<a name="Usage"></a>
## Usage ##

Enter the element in the Masterconfig file and specify the directory path as an inner text.

 - If  `<ondemandroot>`  is not specified, the default location is rooted where the running Framework package is located.
 - `<ondemandroot>` can contain full or relative path.
 - When a relative path is specified it is computed relative to the masterconfig file location.
 - `<ondemandroot>`  can contain &#39;if&#39; conditional attribute.  The conditional string can use a limited set of  [System Properties]( {{< ref "reference/properties/systemproperties.md" >}} )  or  [Functions]( {{< ref "reference/functions/_index.md" >}} ) .  If you have multiple ondemandroot with &#39;if&#39; condition evaluated to true, you will receive a build exception.


{{% panel theme = "info" header = "Note:" %}}
By default ondemand functionality is turned off. Use  [ondemand]( {{< ref "reference/master-configuration-file/ondemand/_index.md" >}} )  element to turn it on.
{{% /panel %}}
## Example ##


```xml
.        <project xmlns="schemas/ea/framework3.xsd">

.            <masterversions>
.              . . . . . .
.            </masterversions>
.
.            <ondemand>true</ondemand>
.            <ondemandroot>.\..\OnDemand</ondemandroot>
.            <ondemandroot if="${nant.platform_host}==osx or ${nant.platform_host}==unix">~/packages/ondemand</ondemandroot>
.            <ondemandroot if="${nant.platform_host}==windows">${nant.drive}\packages\ondemand</ondemandroot>
.
.        </project>
```

##### Related Topics: #####
-  [ondemand]( {{< ref "reference/master-configuration-file/ondemand/_index.md" >}} ) 
