---
title: < RegexMatch > Task
weight: 1118
---
## Syntax
```xml
<RegexMatch Input="" Pattern="" Properties="" failonerror="" verbose="" if="" unless="" />
```
## Summary ##
A task that execute the RegularExpression&#39;s Match function.**NOTE**This task is new in Framework version 3.28.00.  So if you use this task in your build script,
you need to make sure that your project don&#39;t need to be build by older version of Framework.



## Remarks ##
This task executes the System.Text.RegularExpressions.Regex.Match .Net function for
the given input and pattern.  The match result will be sent to the properties provided.
The first property provided will always store the match result.  If a match is not
found, the properties provided will be set to empty string.

### Example ###
Simple example using RegExMatch task.


```xml

<project xmlns="schemas/ea/framework3.xsd">
    <property name="TestString">
        Ubuntu clang version 3.4-1ubuntu3 (tags/RELEASE_34/final) (based on LLVM 3.4)
        Target: x86_64-pc-linux-gnu
        Thread model: posix
    </property>

    <RegexMatch
        Input="${TestString}"
        Pattern="clang version (\d)\.(\d)"
        Properties="ClangVersionString;ClangMajorVer;ClangMinorVer"
    />

    <fail
        unless="'${ClangVersionString}' == 'clang version 3.4'"
        message="Unexpected ClangVersionString result."
    />

    <fail
        unless="'${ClangMajorVer}' == '3'"
        message="Unexpected ClangMajorVer result."
    />

    <fail
        unless="'${ClangMinorVer}' == '4'"
        message="Unexpected ClangMinorVer result."
    />

    <RegexMatch
        Input="${TestString}"
        Pattern="Expecting no match (\d)\.(\d)\.(\d)"
        Properties="MatchResult"
    />

    <fail
        unless="@{StrIsEmpty('${MatchResult}')}"
        message="Expecting empty string MatchResult for no match."
    />

</project>

```



### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| Input | The input string. | String | True |
| Pattern | The search pattern. | String | True |
| Properties | The output property names (separated by semi-colon &#39;;&#39;) to store the match output. | String | True |
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | Boolean | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

## Full Syntax
```xml
<RegexMatch Input="" Pattern="" Properties="" failonerror="" verbose="" if="" unless="" />
```
