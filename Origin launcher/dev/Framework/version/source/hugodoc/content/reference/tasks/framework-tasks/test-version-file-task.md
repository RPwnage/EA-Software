---
title: < test-version-file > Task
weight: 1176
---
## Syntax
```xml
<test-version-file packagename="" targetversion="" packagedir="" failonerror="" verbose="" if="" unless="" />
```
## Summary ##
Verifies package version info in version.h file.

## Remarks ##
This task should be called by a package target.

package task will verify that version information in the version.h file corresponds to the package version.
This verification will apply to the packages that have &#39;include&#39; directory to export header files.
Verification process will check that:
* file &#39;version.h&#39; or &#39;package_version.h&quot; exists
* version information inside &#39;version.h&#39; file corresponds to the package version.
            
Task assumes  that version information in the version.h file complies with the following convention:
#define_VERSION_MAJOR   1
#define_VERSION_MINOR   2
#define_VERSION_PATCH   3
            
Where is usually a package name but verification ignores content of NOTE: This task will also check the Manifest.xml to make sure that it has a &lt;versionName&gt; entry and that
the version info matches as well.  If the entry is missing or incorrect, it will throw an error.

The task declares the following properties:

Property |Description |
--- |--- |
| ${test-version-file.packagename} | The name of the package. | 
| ${test-version-file.targetversion} | The version number of the package | 
| ${test-version-file.packagedir} | package directory: **path** to the package | 




### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| packagename | The name of a package comes from the directory name if the package is in the packages directory.  Use this attribute to name a package that lives outside the packages directory. | String | True |
| targetversion | The version of the package in development. | String | True |
| packagedir | The name of a package comes from the directory name if the package is in the packages directory.  Use this attribute to name a package that lives outside the packages directory. | String | True |
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | Boolean | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

## Full Syntax
```xml
<test-version-file packagename="" targetversion="" packagedir="" failonerror="" verbose="" if="" unless="" />
```
