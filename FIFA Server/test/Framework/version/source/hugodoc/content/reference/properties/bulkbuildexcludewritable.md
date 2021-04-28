---
title: bulkbuild.exclude-writable
weight: 224
---

bulkbuild.exclude-writableboolean property is used to control globally whether writable (readonly flag not set) are included bulk builds or loose file builds.

<a name="PropertyUsage"></a>
## Usage ##

Name |Always defined |Needs to be Global |Default Value |
--- |--- |--- |--- |
| bulkbuild.exclude-writable |  | ![requirements 1a]( requirements1a.gif ) | true |

 `bulkbuild.exclude-writable` property controls whether files that are writable and are not located under buildroot folder are automatically excluded from bulkbuild filesets..

## Example ##

Specifying `bulkbuild.exclude-writable` property on the nant command line:


```
nant  build -configs:pc-vc-dev-debug -G:bulkbuild.exclude-writable=false

nant  slnruntime -configs:"pc-vc-dev-debug capilano-vc-dev-debug kettle-clang-dev-debug" -G:bulkbuild.exclude-writable=false
```

{{% panel theme = "info" header = "Note:" %}}
`bulkbuild.exclude-writable`  property can be defined on nant command line, in {{< url Masterconfig >}}, or in the package build script.
In latter case property value will affect only files in the given package.
{{% /panel %}}

##### Related Topics: #####
-  [bulkbuild.exclude-writable]( {{< ref "reference/packages/build-scripts/native-c/c++-modules/bulk-build/bulkbuildexcludewritable.md" >}} ) 
