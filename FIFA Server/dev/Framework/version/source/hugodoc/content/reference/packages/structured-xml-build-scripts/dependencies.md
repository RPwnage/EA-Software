---
title: How to define dependencies
weight: 204
---

Topic in this section describe how to define dependencies in Structured XML

<a name="SXMLDependencies"></a>
## Dependencies in Structured XML ##

For a list of the types of dependencies and an explanation of how dependencies work see  [Dependencies]( {{< ref "reference/packages/build-scripts/dependencies/_index.md" >}} ) .

Dependency definitions in Structured XML are entered in the  `<dependency>`  element. 


```xml
{{%include filename="/reference/packages/structured-xml-build-scripts/dependencies/dependencyelementdefinition.xml" /%}}

```
Each dependency description should be on a separate line and in one of the following forms (Note: these names are case sensitive):


```
1. [dependent-package-name]
2. [dependent-package-name]/[dependent-module-name]
3. [dependent-package-name]/[dependent-group]/[dependent-module-name]
```
 1. For &#39;test&#39; and &#39;example&#39; groups when no explicit dependency on runtime module is specified, dependency on all package modules in the &#39;runtime&#39;{{< url buildgroup >}}is added automatically.
 2. When `[dependent-group]`  is not specified, the {{< url buildgroup >}}is &#39;runtime&#39;


```xml
{{%include filename="/reference/packages/structured-xml-build-scripts/dependencies/sxmldependenciesinsamepackage.xml" /%}}

```
Below is an example of how to use conditions in dependency definitions. Use the  `<do>`  element: 


```xml

<Library name="LibraryA">
  <dependencies>
    <interface>
      EABase
      EAAssert
    </interface>
    <auto>
      rwmovie
      vp6
      <do if="'${config-compiler}' == 'vc' and '${config-system}' != 'capilano'">
        vp8
      </do>
    </auto>
  </dependencies>
  . . .
</Library>

```

##### Related Topics: #####
-  [Dependencies]( {{< ref "reference/packages/build-scripts/dependencies/_index.md" >}} ) 
-  [Define dependencies in Structured XML]( {{< ref "reference/packages/build-scripts/dependencies/dependenciesinstructuredxml.md" >}} ) 
