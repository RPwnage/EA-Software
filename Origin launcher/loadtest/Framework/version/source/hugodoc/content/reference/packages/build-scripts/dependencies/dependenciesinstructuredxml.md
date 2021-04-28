---
title: Define dependencies in Structured XML
weight: 95
---

Dependencies definition in structured XML

<a name="DependenciesInStructuredXmlUsage"></a>
## Usage ##

Specifying dependencies in structured XML is simpler than in traditional Framework syntax.
All different cases: dependency on the whole package, dependency on a module, various additional attributes
are specified in the same way:


```xml
{{%include filename="/reference/packages/build-scripts/dependencies/dependenciesinstructuredxml/basicstructuredxmldependencydefinition.xml" /%}}

```
More formal description of a dependency definitions in Structured XML:


```xml
{{%include filename="/reference/packages/build-scripts/dependencies/dependenciesinstructuredxml/dependencyelementdefinition.xml" /%}}

```
Each dependency description should be on a separate line. One of the following forms can be used


```
[dependent-package-name]
[dependent-package-name]/[dependent-module-name]
[dependent-package-name]/[dependent-group]/[dependent-module-name]
```
There are five preset types: `auto` ,  `use` ,  `build` , `interface` , and  `link` .

Attributes `interface`  and  `link` can be used to alter flags of predefined types.
For example:


```xml
<build interface="false"  link="true">
    EASTL
</build>
```
 `copylocal` attribute can be assigned on dependency elements


```xml
{{%include filename="/reference/packages/build-scripts/dependencies/dependenciesinstructuredxml/structuredxmlcopylocalexample.xml" /%}}

```
See more examples here  [How to define dependencies in structured XML]( {{< ref "reference/packages/structured-xml-build-scripts/dependencies.md" >}} ) 


##### Related Topics: #####
-  [How to define dependencies in structured XML]( {{< ref "reference/packages/structured-xml-build-scripts/dependencies.md" >}} ) 
