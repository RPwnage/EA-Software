---
title: bulkbuild
weight: 223
---

bulkbuildboolean property is used to control globally whether to perform bulk builds or loose file builds.

<a name="ConfigPropertyUsage"></a>
## Usage ##

Name |Always defined |Needs to be Global |Default Value |
--- |--- |--- |--- |
| bulkbuild | ![requirements 1a]( requirements1a.gif ) | No (global writable by default). | true |

 `bulkbuild`  property is initialized to  *true*  in eaconfig in config\global\init.xmlfile.


{{% panel theme = "info" header = "Note:" %}}
`bulkbuild`  is global by default, this means there is no need to declare this property as global in {{< url Masterconfig >}}.
{{% /panel %}}

{{% panel theme = "info" header = "Note:" %}}
Unlike standard global properties `bulkbuild` property remains writable when propagated to dependent packages.
{{% /panel %}}
## Example ##

Specifying `bulkbuild` property on the nant command line:


```
nant  build -configs:pc-vc-dev-debug -D:bulkbuild=false

nant  slnruntime -configs:"pc-vc-dev-debug capilano-vc-dev-debug kettle-clang-dev-debug" -D:bulkbuild=false
```

{{% panel theme = "info" header = "Note:" %}}
`bulkbuild`  property can be defined on nant command line, in {{< url Masterconfig >}}, or in the package build script.
In latter case property value will be propagated to dependents of given package.
{{% /panel %}}

##### Related Topics: #####
-  [Bulk Build]( {{< ref "reference/packages/build-scripts/native-c/c++-modules/bulk-build/_index.md" >}} ) 
