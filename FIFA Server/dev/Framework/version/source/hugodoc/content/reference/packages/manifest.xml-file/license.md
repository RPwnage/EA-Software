---
title: license
weight: 67
---

This element is used to describe license type.

Each package on the EA Package Server has a license which describes the restrictions on using that package.


{{% panel theme = "info" header = "Note:" %}}
These licenses are still under review and may change in the future. Questions about licenses and open source software should be directed to somebody
{{% /panel %}}

{{% panel theme = "info" header = "Note:" %}}
Partially deprecated
{{% /panel %}}
## Usage ##

Enter the element in the Manifest file and and enter license type string as an inner text.

Must be one of the defined values
( [LicenseAndRestrictions](http://packages.ea.com/LicenseAndRestrictions.aspx) )

##### Commercial Software #####
The package contains commercial software or checks to make sure commercial software is installed.
As long as users have valid licenses for the commercial software you will have no problems using this package in games or tools.

##### EA Proprietary (Internal Use Only) #####
The package is custom code written by EA.
It can be used by any EA game or tool but cannot be shipped to external developers without special permission from the package owners.

##### EA Proprietary (External Use Allowed) #####
Much like the above license but this package can be used by external developers. This license is often used by binary distributions.

##### Approved Open Source for Games #####
The package contains open source code that has been approved by EA legal for use in games.
The license comment must link to the EA Legal database that documents this specific instance.

##### Approved Open Source for Tools #####
The package contains open source code that has has been approved by EA legal for tool.


{{% panel theme = "warning" header = "Warning:" %}}
You cannot use this package in games without permission from EA legal.
{{% /panel %}}
##### Pending Open Source (Not Licensed for Games) #####
The package contains open source code that is pending approval by EA legal.


{{% panel theme = "warning" header = "Warning:" %}}
You cannot use this package in games with out permission from EA legal.
{{% /panel %}}
##### Unknown #####
The package has not set it&#39;s license. Assume that you cannot use this package and then beat the package author to update the license.

## Example ##


```xml
<license>Commercial Software</license>.
```

##### Related Topics: #####
-  [Manifest.xml file]( {{< ref "reference/packages/manifest.xml-file/_index.md" >}} ) 
