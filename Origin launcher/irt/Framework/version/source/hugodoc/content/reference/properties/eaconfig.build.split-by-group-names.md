---
title: eaconfig.build.split-by-group-names
weight: 226
---

eaconfig.build.split-by-group-namesboolean property is used to control how whether generation task
(currently applies to Visual Studio only) splits generated solutions bu buildgroups &#39;runtime&#39;, &#39;test&#39;, &#39;example&#39;, &#39;tool&#39;.

<a name="Usage"></a>
## Usage ##

See [targets]( {{< ref "reference/targets/targetreference.md#VisualStudioSlnTargets" >}} ) .

Name |Always defined |Needs to be Global |Default Value |
--- |--- |--- |--- |
| eaconfig.build.split-by-group-names |  |  | does not exist. Different solution generation targets use different default values. Targets generating solutions for multiple groups set default to &#39;true&#39;, otherwise it is false. |

## Example ##

Specifying `autobuilduse` property on the nant command line:


```
nant  slntest -configs:"pc-vc-dev-debug capilano-vc-dev-debug kettle-clang-dev-debug" -D:eaconfig.build.split-by-group-names=true
```

##### Related Topics: #####
- {{< task "EA.Eaconfig.Backends.BackendGenerateTask" >}}
