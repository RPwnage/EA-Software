---
title: nant.transitive
weight: 229
---

nant.transitiveboolean property is used to control transitive dependency propagation mode.

<a name="NantTransitivePropertyUsage"></a>
## Usage ##

See [Transitive Dependency Propagation]( {{< ref "reference/packages/build-scripts/dependencies/transitivedependencypropagation.md" >}} ) for details about transitive dependency propagation in Framework.

name |Always defined |Needs to be Global |Default Value |
--- |--- |--- |--- |
| nant.transitive | ![requirements 1a]( requirements1a.gif ) | No (global writable by default). | true |

 `nant.transitive`  property is initialized to  *true*  in {{< url Framework >}}initialization code.

In majority of cases transitive dependency propagation does not create any problems when building legacy{{< url Framework2 >}}projects.
In case you want to switch it off: set property on nant command line or in masterconfig global properties.


{{% panel theme = "info" header = "Note:" %}}
`nant.transitive`  is global by default, this means there is no need to declare this property as global in {{< url Masterconfig >}}.
{{% /panel %}}
## Example ##

Specifying `nant.transitive` property on the nant command line:


```
nant  build -configs:pc-vc-dev-debug -D:nant.transitive=false

nant  slnruntime -configs:"pc-vc-dev-debug capilano-vc-dev-debug kettle-clang-dev-debug" -D:nant.transitive=false
```

{{% panel theme = "info" header = "Note:" %}}
`nant.transitive`  can be defined on nant command line or in {{< url Masterconfig >}}. Property is used by &lt;create-build-graph&gt;
task which is always executed in the context of the top level package. Defining this property in dependent package build scripts will likely have no effect.
{{% /panel %}}

##### Related Topics: #####
-  [Transitive Dependency Propagation]( {{< ref "reference/packages/build-scripts/dependencies/transitivedependencypropagation.md" >}} ) 
