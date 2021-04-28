---
title: < WritePackageGraphViz > Task
weight: 1221
---
## Syntax
```xml
<WritePackageGraphViz packagename="" graphfile="" graphmodules="" groupbypackages="" configurations="" dependency-filter="" exclude-dependency-filter="" find-ancestors="" dependency-color-scheme="" failonerror="" verbose="" if="" unless="">
  <excludemodules />
</WritePackageGraphViz>
```
## Summary ##
Generates &#39;dot&#39; language description of the dependency graph between modules or packages. Generated file can be opened by &quot;Graphviz&quot; tools to create an image


### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| packagename | Name of a package to use a root of generated dependency graph picture. Separate graph is generated for each configuration.<br>Package name can contain module name: valid values are &#39;package_name&#39; or &#39;package_name/module_name&#39; or &#39;package_name/group/module_name&#39;, where group is one of &#39;runtime, test, example, tool&#39;&quot;. | String | True |
| graphfile | Full path to the output file. Configguration name is added to the file name. | String | True |
| graphmodules | If &#39;true&#39; picture of dependencies between modules is generated. Otherwise dependencies between packages are used, no modules are added to the picture. Default is &#39;true&#39;. | Boolean | False |
| groupbypackages | Modules from the same package are grouped together on an image. This may complicate outline for very big projects. Only has effect when graphmodules=&#39;true&#39;, default is &#39;true&#39;. | Boolean | False |
| configurations | List of configurations to use. | String | False |
| dependency-filter | Types of dependencies to include in the generated picture. Possible values are: &#39;build&#39;, &#39;interface&#39;, &#39;link&#39;, &#39;copylocal&#39;, &#39;auto&#39;, or any combination of these, enter values space separated. Default value is: &#39;build interface link&#39;. | String | False |
| exclude-dependency-filter | Types of dependencies to exinclude from the generated picture. Possible values are: &#39;build&#39;, &#39;interface&#39;, &#39;link&#39;, &#39;copylocal&#39;, &#39;auto&#39;, or any combination of these, enter values space separated. Default value is empty. | String | False |
| find-ancestors | Graph ancestors of a given package/module indead of dependents. Default value is false. | Boolean | False |
| dependency-color-scheme | Dependency Color scheme. Standard - by dependency type, ByDependent - same color as dependent module. | DependencyColorSchemes | False |
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | Boolean | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

### Nested Elements
| Name | Description | Type | Required |
| ---- | ----------- | ---- | -------- |
| {{< task "NAnt.Core.PropertyElement" "excludemodules" >}}| List of modules to exclude | {{< task "NAnt.Core.PropertyElement" >}} | False |

## Full Syntax
```xml
<WritePackageGraphViz packagename="" graphfile="" graphmodules="" groupbypackages="" configurations="" dependency-filter="" exclude-dependency-filter="" find-ancestors="" dependency-color-scheme="" failonerror="" verbose="" if="" unless="">
  <excludemodules />
</WritePackageGraphViz>
```
