---
title: Defines in Framework
weight: 35
---

This topic explains how defines are set and what is the scope of definitions in Framework.

<a name="Section1"></a>
## Everything you wanted to know about Framework defines, but were afraid to ask ##

There are some misconceptions about how defines work in Framework and this page seeks to
clarify how these are managed and hopefully solve the issues you are facing.

<a name="Caveats"></a>
## Caveats ##

Please note that Framework performs no intelligent merging of defines.
If you have a define listed multiple times it is up to the compiler that is
invoked how these are handled, which may include generating a warning or failing
with an error.

<a name="ScopeOfDefinitions"></a>
## Scope of Definitions ##

Most of the problems surround defines are related to their scope.  i.e.
It’s easy to be confused about what the scope of a define is depending on how it is set.
Often the scope is smaller than what people expect which can lead to problems.
So, let’s look at the scope that defines apply to.  There are really three different
scopes that you need to think about in most cases.

<a name="LibraryScope"></a>
### &quot;Library&quot; Scoped ###

Often a define only needs to be set when compiling a specific library.
For example there may be a feature that needs to be compiled into a library depending on a define.
In this case the .build file that describes how to build the library can add a definition.


```xml

  <Library name="libjpeg" buildtype="CLibrary">
     <!-- SNIP -->
      <config>
          <defines>
              LIBJPEG_EA_SUPPORT_ENABLED=${LIBJPEG_EA_SUPPORT_ENABLED}
          </defines>
      </config>
  </Library>

```
Or using non-structured XML:


```xml

  <property name="runtime.libjpeg.defines">
      ${property.value}
      LIBJPEG_EA_SUPPORT_ENABLED=${LIBJPEG_EA_SUPPORT_ENABLED}
  </property

```

{{% panel theme = "danger" header = "Important:" %}}
**Remember, this define is only set when building the specified module** .
It is **not** defined when another package includes a header file belonging to this library.
So, if the define appears in a public header, it is likely that it will be out of sync with the definition used
when the library was built.
{{% /panel %}}
<a name="DependencyScope"></a>
### &quot;Dependency&quot; Scoped ###

In the case of a define that appears in a public header, it is usually important for the define to be set
properly for any module that includes the header.  As mentioned above, adding the definition to the module/library
does not ensure other libraries using public headers get the same definition.
To ensure that packages that depend on a particular package get the correct defines,
the dependent package needs to &quot;export&quot; defines in its [Initialize.xml]( {{< ref "reference/packages/initialize.xml-file/propertydefines.md" >}} ) file.

Here’s an example taken from job_manager’s Initialize.xml:


```xml

  <do if="${package.job_manager.allow_profiling??false}">
      <property name="package.job_manager.defines">
          ${property.value}
          JOB_MANAGER_ALLOW_PROFILING
      </property>
  </do>

```
Or using structured XML:


```xml

  <property name="JobManagerBuildType" value="Library" />
  <property name="JobManagerBuildType" value="DynamicLibrary" if="${Dll??false}" />

  <publicdata packagename="job_manager">
      <runtime>
          <module name="job_manager" buildtype="${JobManagerBuildType}">
              <!-- Exporting define for the module -->
              <defines if="${package.job_manager.allow_profiling??false}">
                  JOB_MANAGER_ALLOW_PROFILING
              </defines>
              . . .
          </module>
      </runtime>
  </publicdata>

```
This code defines JOB_MANAGER_ALLOW_PROFILING for any packages that directly depends on job_manager when
the package.job_manager.allow_profiling property exists.


{{% panel theme = "danger" header = "Important:" %}}
Caveat: The snippet conditionally defines JOB_MANAGER_ALLOW_PROFILING for packages that depend on job_manager. **But, it doesn’t actually make sure it is set for the job_manager library** .
job_manager’s build file also needs to set a module define to ensure JOB_MANAGER_ALLOW_PROFILING is
defined during compilation of the library.
{{% /panel %}}
<a name="GloballyScope"></a>
### &quot;Globally&quot; Scoped ###

Sometimes a define should be set for every .cpp that gets compiled.
In that case you need a define to be set globally. There are a couple ways that global defines can be set.
Traditionally global defines had to be set within config packages.
That made it somewhat difficult to set defines because it required modifying a config package.

However there are newer (and simpler!) ways to set global defines.  It is now possible to set global defines within a masterconfig file.
Basically you just need to add the define to a [<globaldefines>]( {{< ref "reference/master-configuration-file/globaldefines.md" >}} ) section.

Here’s an example:


```xml

  <globaldefines>
      BOOLEAN_DEFINE
      BASIC_DEFINE=1

      <if condition="'${config-system}'=='android'">
          ANDROID_DEFINE=5
      </if>

      <if condition="true">
          <if condition="false">
              NESTED_CONDITION_1=3
          </if>
          <if condition="true">
              NESTED_CONDITION_2=4
          </if>
      </if>

      <if packages="EAThread">
          EATHREAD_ONLY_DEFINE=5
      </if>

  </globaldefines>

  <globaldotnetdefines>
      BOOLEAN_DOTNET_DEFINE
  </globaldotnetdefines>

```
From this example you can see that the masterconfig file can allow you to specify global defines
as well as defines for a specific package.  When you set a define for a package in the masterconfig,
it is set only for modules within the package (i.e. it is a &quot;library scoped&quot; define).

