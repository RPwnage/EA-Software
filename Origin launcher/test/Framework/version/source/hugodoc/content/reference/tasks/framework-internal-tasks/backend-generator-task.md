---
title: < backend-generator > Task
weight: 1219
---
## Syntax
```xml
<backend-generator generator-name="" startup-config="" configurations="" generate-single-config="" split-by-group-names="" solution-name="" enable-sndbs="" top-modules="" excluded-root-modules="" failonerror="" verbose="" if="" unless="" />
```
### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| generator-name |  | String | True |
| startup-config |  | String | True |
| configurations |  | String | True |
| generate-single-config |  | Boolean | False |
| split-by-group-names |  | Boolean | False |
| solution-name | The Name of the Solution to Generate. | String | False |
| enable-sndbs |  | Boolean | False |
| top-modules | A whitespace delimited list of module names. If provided, overrides the default modules slngenerator will use a starting point when searching the graph for modules to include in the solution. | String | False |
| excluded-root-modules | A whitespace delimited list of module names. The solution generator will not include projects that are dependencies of these modules - can be used to exclude subgraphs when searching graph for modules to include in the solution. | String | False |
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | Boolean | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

## Full Syntax
```xml
<backend-generator generator-name="" startup-config="" configurations="" generate-single-config="" split-by-group-names="" solution-name="" enable-sndbs="" top-modules="" excluded-root-modules="" failonerror="" verbose="" if="" unless="" />
```
