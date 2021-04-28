---
title: autobuilduse
weight: 228
---

autobuilduseboolean property is used to control how Framework handles auto dependencies.

<a name="Usage"></a>
## Usage ##

See [auto dependency type]( {{< ref "reference/packages/build-scripts/dependencies/overview.md" >}} ) for details.

Name |Always defined |Needs to be Global |Default Value |
--- |--- |--- |--- |
| autobuilduse | ![requirements 1a]( requirements1a.gif ) | No (global writable by default). | true |

 `autobuilduse`  property is initialized to  *true* in NAnt initialization code.


{{% panel theme = "info" header = "Note:" %}}
`autobuilduse`  is global by default, this means there is no need to declare this property as global in {{< url Masterconfig >}}.
{{% /panel %}}
## Example ##

Specifying `autobuilduse` property on the nant command line:


```
nant  build -configs:pc-vc-dev-debug -D:autobuilduse=false

nant  slnruntime -configs:"pc-vc-dev-debug capilano-vc-dev-debug kettle-clang-dev-debug" -D:autobuilduse=false
```

{{% panel theme = "info" header = "Note:" %}}
`autobuilduse`  can be defined on nant command line or in {{< url Masterconfig >}}.
{{% /panel %}}

##### Related Topics: #####
-  [Overview]( {{< ref "reference/packages/build-scripts/dependencies/overview.md" >}} ) 
