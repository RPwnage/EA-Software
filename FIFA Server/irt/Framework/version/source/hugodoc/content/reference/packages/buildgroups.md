---
title: BuildGroups
weight: 41
---

Groups are the way for Framework to put additional example, test, or tool build units
into a{{< url package >}}and have a standard way to build these groups selectively.

<a name="BuildGroups"></a>
## Build Groups ##

There are four build Groups supported by Framework

Group |Purpose |Targets |
--- |--- |--- |
| runtime | &quot;main&quot; build unit library, program or Dll. | build, slnruntime, run, ...  |
| example | Example code. Demonstrates how to use runtime code  | example-build, slnexample, example-run, ...  |
| test | Test code. Tests Package runtime code. | test-build, slntest, test-run ...  |
| tool | Tool code. Usually tools are additional executables that are used in a build process or for other helper purposes. | tool-build, slntool, tool-run...  |

By default Structured XML modules defined under the `<project xmlns="schemas/ea/framework3.xsd">`  tag are assigned to the  ` *runtime* ` {{< url buildgroup >}}. To assign modules to other groups put them inside one of the following tags:

 - `<tests>`  - for  ` *test* ` group.
 - `<tools>`  - for  ` *tool* ` group
 - `<examples>`  - for  ` *example* ` group
 - `<runtime>`  - for  ` *runtime* ` group.<br>This is optional. Modules are assigned to runtime group by default


```xml

<project xmlns="schemas/ea/framework3.xsd">

  <package name="Example"/>

  <Library name="LibA">
    <!-- LibA is placed into 'runtime' group-->
    . . . . .
  </Library>

  <runtime>
    <Library name="LibA">
      <!-- LibB is placed into 'runtime' group-->
      . . . . .
    </Library>
  </runtime>

  <Library name="LibC">
    <!-- LibC is placed into 'runtime' group-->
    . . . . .
  </Library>

  <tests>

    <Program name="MyTest">
      <!-- MyTest program is placed into 'test' group-->
      . . . . .
    </Program>

    <Library name="TestLib">
      <!-- TestLib is placed into 'test' group-->
      . . . . .
    </Library>

  </tests>

  <examples>

    <Program name="MyExample">
      <!-- MyExample program is placed into 'example' group -->
      . . . . .
    </Program>

  </examples>

</project>

```

{{% panel theme = "info" header = "Note:" %}}
When modules are assigned to &#39;test&#39;, &#39;tool&#39;, or &#39;example&#39; group
dependencies to all runtime modules in this package are added automatically if module has no explicit dependencies defined.
See [How to define dependencies]( {{< ref "reference/packages/structured-xml-build-scripts/dependencies.md" >}} ) for info about dependencies.

To restrict dependencies to particular &#39;runtime&#39; modules define these dependencies explicitly.

To remove all dependencies to runtime define empty dependency definition.
{{% /panel %}}
![Build Groups 1]( build_groups_1.png )If, for example, *test*  (or  *runtime* ) group has builddependency on
another package this dependency is only for *runtime* group in that dependent package:

![Build Groups 2]( build_groups_2.png )
{{% panel theme = "info" header = "Note:" %}}
Build dependency is defined between packages, not between groups, but is only applies to **runtime** group.
{{% /panel %}}
