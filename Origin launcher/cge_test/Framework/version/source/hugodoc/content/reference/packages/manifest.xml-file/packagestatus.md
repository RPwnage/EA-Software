---
title: packageStatus
weight: 63
---

This element is used to describe status of the package.


{{% panel theme = "info" header = "Note:" %}}
This is the status of the package, not the release.
{{% /panel %}}
## Usage ##

Enter the element in the Manifest file and and enter status string as an inner text.

Status must be one of the defined values
( [Status](http://packages.ea.com/Status.aspx) )

##### Official #####
This is the official release of the package. Whenever possible use the official release.

##### Accepted #####
This release is in use. You should feel comfortable using it but if possible move to the official release instead.

##### Prerelease #####
This is a prerelease or beta release.

##### Deprecated #####
The release is not recommended but still works. You should upgrade to the official release as soon as possible.

##### Broken #####
The release does not work as expected. Releases that are completely broken should be deleted.
Releases with bugs should be left on the server as other people may be depending on them. Indicate what is broken in the status comment.

##### Unknown #####
The default status for all releases.

## Example ##


```xml
<packageStatus>Official</packageStatus>.
```

##### Related Topics: #####
-  [Manifest.xml file]( {{< ref "reference/packages/manifest.xml-file/_index.md" >}} ) 
