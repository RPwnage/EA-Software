---
title: Modules
weight: 40
---

This topic explains the purpose of modules in the Framework build system.

<a name="Modules"></a>
## Module ##

Modules allow a framework user to put more than one build unit into a Package&#39;s build group.
It is primarily a convenience feature that, for example, allows users to put several libraries into a single package.

To define a new Module a framework user simply includes the Module&#39;s name after the build group&#39;s name when defining or
referring to any properties specific to that module. The example below contains two modules namedLibModule and ProgramModule. Framework users must also list the names of each module
in each of a package&#39;s build groups by using the{{< url buildgroup >}}.buildmodules property. The .buildmodules property
lets Eaconfig know which modules were defined.


```xml
{{%include filename="/reference/packages/modules/modulesdefinition1.xml" /%}}

```

{{% panel theme = "info" header = "Note:" %}}
If you are still using the old style build script syntax, don&#39;t forget to list module names in the{{< url buildgroup >}}.buildmodules
property for each build group with modules.  Otherwise, they won&#39;t get built.  Using the new
Structured XML syntax, this step is done for you automatically.
{{% /panel %}}

{{% panel theme = "info" header = "Note:" %}}
When some or all of the build groups do not define modules, Framework assumes that such groups
contain a single module with name equal to the Package name.
{{% /panel %}}
Dependencies between Modules can be defined for Modules within the same Package:


```xml
<!-- New Syntax -->
<Program name="ProgramModule">
  ...
  <dependencies>
    <auto>
      ModulesExample/LibModule
      ModulesExample/LibModuleOldSyntax
    </auto>
  </dependencies>
  ...
</Program>

<!-- Old Syntax -->
<property name="runtime.ProgramModuleOldSyntax.runtime.moduledependencies">
  LibModule
  LibModuleOldSyntax
</property>
```
Dependencies between different packages are defined on a package level:


```xml
{{%include filename="/reference/packages/modules/modulesdefinitionpackagedependencies1.xml" /%}}

```
ProgramModule depends on LibModulefrom the same package, and it will depend on allruntime modules defined in the Package OtherPackageWithModules

As an example of a test module dependent on a runtime module building first:


```xml
{{%include filename="/reference/packages/modules/modulesdefinitiongroupdependencies1.xml" /%}}

```

{{% panel theme = "info" header = "Note:" %}}
It is possible to define a dependency on selected modules in another package.
For details see chapter Constraining dependencies on this page Modeling Dependencies.
{{% /panel %}}
<a name="ModuleGroupName"></a>
## Module Groupname ##

Most of the module related NAnt properties and filesets follow the following convention


```
[group].[module].xxxx
```
Where  `[group]`  means the name of the  [build group]( {{< ref "reference/packages/buildgroups.md" >}} ) ,
and `[module]` stands for the name of the module

Property  `[group].buildmodules must also be defined when convention "with modules" is used.` 

Old versions of Framework did not have any module support, i.e. they had one logical module in each group,
but no explicit module name was assigned. WE will call such packages &quot;package without modules&quot;.
Convention for property and fileset names in package without modules is:


```
[group].xxxx
```
 **To save explanations further in this documentation we will use convention `[group].[module].xxxx` , but for packages (groups) without modules
it can mean `[group].xxxx` .** 


{{% panel theme = "info" header = "Note:" %}}
Separate groups in the package can follow either convention, for example, runtime group with a single unit can use convention &quot;without modules&quot;
and test group can convention &quot;with modules&quot;
{{% /panel %}}

{{% panel theme = "primary" header = "Tip:" %}}
We recommend always use convention &quot;with modules&quot;.
{{% /panel %}}
