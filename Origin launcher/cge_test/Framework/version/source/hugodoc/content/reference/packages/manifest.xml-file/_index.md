---
title: Manifest.xml file
weight: 42
---

Manifest.xmlfile serves serves several purposes.

 - Declares the Package as {{< url Framework2 >}} ({{< url Framework3 >}}) compliant.
 - Provides information about build options<br><br>  - Is Package buildable?<br>  - Supported platforms<br>  - Supported build targets ( *build, test-build, ..., slnruntime ...* )<br>  - Compatibility with other packages
 - Provides descriptive information about the Package which will be displayed by the{{< url PackageServer >}}.

<a name="ManifestFile"></a>
## Manifest file ##

The package manifest is a file used to provide additional information about the package.
The Framework tools make use of the manifest to provide additional information to the user about the package.
The manifest is required for *Framework 2* style packages, but is optional for 1.x ones.


{{% panel theme = "info" header = "Note:" %}}
By convention, the manifest file is called Manifest.xml and located in the{{< url PackageDirectory >}}.
{{% /panel %}}
### Required manifest elements ###

 - [buildable]( {{< ref "reference/packages/manifest.xml-file/buildable.md" >}} )
 - [manifestVersion]( {{< ref "reference/packages/manifest.xml-file/manifestversion.md" >}} ) - only required if version 2 or higher elements are present in the manifest.

### Optional manifest elements ###

Descriptive elements

 - [packageName]( {{< ref "reference/packages/manifest.xml-file/packagename.md" >}} )
 - [versionName]( {{< ref "reference/packages/manifest.xml-file/versionname.md" >}} )
 - [summary]( {{< ref "reference/packages/manifest.xml-file/summary.md" >}} )
 - [changes]( {{< ref "reference/packages/manifest.xml-file/changes.md" >}} )
 - [tags]( {{< ref "reference/packages/manifest.xml-file/tags.md" >}} )
 - [relationship]( {{< ref "reference/packages/manifest.xml-file/relationship.md" >}} )

Contact Info

 - [contactName]( {{< ref "reference/packages/manifest.xml-file/contactname.md" >}} )
 - [contactEmail]( {{< ref "reference/packages/manifest.xml-file/contactemail.md" >}} )
 - [owningTeam]( {{< ref "reference/packages/manifest.xml-file/owningteam.md" >}} )
 - [support]( {{< ref "reference/packages/manifest.xml-file/support.md" >}} )

Resource Locations and info

 - [homePageUrl]( {{< ref "reference/packages/manifest.xml-file/homepageurl.md" >}} )
 - [community]( {{< ref "reference/packages/manifest.xml-file/community.md" >}} )
 - [sourceLocation]( {{< ref "reference/packages/manifest.xml-file/sourcelocation.md" >}} )
 - [downloadLink]( {{< ref "reference/packages/manifest.xml-file/downloadlink.md" >}} )
 - [downloadSize]( {{< ref "reference/packages/manifest.xml-file/downloadsize.md" >}} )
 - [fileNameExtension]( {{< ref "reference/packages/manifest.xml-file/filenameextension.md" >}} )

Package Status

 - [status]( {{< ref "reference/packages/manifest.xml-file/status.md" >}} )
 - [statusComment]( {{< ref "reference/packages/manifest.xml-file/statuscomment.md" >}} )
 - [packageStatus]( {{< ref "reference/packages/manifest.xml-file/packagestatus.md" >}} )
 - [isSupported]( {{< ref "reference/packages/manifest.xml-file/issupported.md" >}} )
 - [driftVersion]( {{< ref "reference/packages/manifest.xml-file/driftversion.md" >}} )

License

 - [license]( {{< ref "reference/packages/manifest.xml-file/license.md" >}} )
 - [licenseDetails]( {{< ref "reference/packages/manifest.xml-file/licensedetails.md" >}} )

Version compatibility with other packages

 - [compatibility]( {{< ref "reference/packages/manifest.xml-file/compatibility.md" >}} )

Deprecated

 - [description]( {{< ref "reference/packages/manifest.xml-file/description.md" >}} )
 - [categories]( {{< ref "reference/packages/manifest.xml-file/categories.md" >}} )

## Examples ##


```xml
{{%include filename="/reference/packages/manifest.xml-file/_index/manifestintroexample.xml" /%}}

```


###### Full Manifest Example ######

```xml
{{%include filename="/reference/packages/manifest.xml-file/_index/fullmanifestexample.xml" /%}}

```
