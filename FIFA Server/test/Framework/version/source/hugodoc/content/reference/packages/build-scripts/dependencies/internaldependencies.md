---
title: Internal Dependencies
weight: 94
---
Framework, as of 4.02.00, has the ability to expose extra API to
&quot;friend&quot; modules/packages. This allows related components to share
internal functionality without exposing it to end users.


Protecting API With Internal Dependencies

<a name="WhatInternalDependencies"></a>
## What Are Internal Dependencies? ##

Internal dependencies are a concept tailored to C/C++ code.
Internal dependencies allow a module to differentiate its public
interface, intended for use by any end-user, from a special internal
set that is only meant for a subset of modules or packages.

<a name="WhyInternalDependencies"></a>
## Why Internal Dependencies? ##

Internal API is often more likely to evolve than public API.
Keeping it separate allows the maintainers of the package the freedom
to change these portions without inflicting breaking changes on users
of the public API.

Although there is no restriction preventing developers from putting
internal API components in the public API, separating it with a mechanism
like Framework&#39;s internal dependencies makes it much less likely that
an unintended user will depend on the code.

<a name="DeclaringInternalInterfaces"></a>
## Declaring Internal Interfaces ##

As mentioned previously, internal dependencies in Framework pertain
only to C or C++ Code. They are implemented through optional selection of
include directories. This functionality is only available through
Framework&#39;s structured XML syntax.

Internal interfaces are declared in a module&#39;s public data. As
public include directories are declared in an &lt;includedirs&gt;
element, internal interface directories are declared in an
&lt;includedirs.internal&gt; element. For example:


```xml
<project xmlns="schemas/ea/framework3.xsd">
  <publicdata packagename="Foo">
    <runtime>
      <module name="Foo1">
        <includedirs.internal>
          ${package.Foo.dir}/internal
        </includedirs.internal>
      </module>
    </runtime>
  </publicdata>
</project>
```
When a package declares an internal dependency on a module that
exports the includedirs.internal values as above, these directories
will be added to the module&#39;s include paths.

<a name="UsingInternalDependencies"></a>
## Using Internal Dependencies ##

Internal dependencies are enabled by adding the &quot;internal&quot;
attribute to the dependency group you wish to use internal API from. For
example:


```xml
<runtime>
  <Program name="Bar">
    <dependencies>
      <auto internal="true">
        Foo
      </auto>
      <auto>
        EABase
      </auto>
    </dependencies>
    <sourcefiles>
      <includes name="${package.dir}/source/*.cpp"/>
    </sourcefiles>
  </Program>
</runtime>
```
It is worth noting that you can have multiple similar dependency
groups when using Structured XML; for example you can have an auto
group in addition to an auto group with the internal flag selected. This
ensures that you are not inadvertantly using internal API from modules
you do not intend to use public API from.

Module &quot;path&quot;-style syntax is available for depending on specific
modules, for example it is possible to add only the Foo1 module from the
Foo package via the Foo/Foo1 syntax.

Internal dependencies are available for any dependency type that
implements the interface dependency flag, such as auto, interface, and
use.

