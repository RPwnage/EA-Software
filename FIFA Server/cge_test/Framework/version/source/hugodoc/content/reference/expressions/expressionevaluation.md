---
title: Expression Evaluation
weight: 240
---

This topic describes how expressions are evaluated

<a name="Section1"></a>
## Expression Evaluation ##

Previous topic describes expression syntax. Here we will explain how and when expressions are evaluated in Framework scripts.

During regular property string expansion expressions are **NOT** evaluated. For example:


```xml

    <property name="left" value="10"/>
    <property name="right" value="7"/>

    <property name="compare" value="${left} GT ${right}"/>     NOTE: value of compare is "10 GT 7", it is not "true"

```
When property string is **expanded** , values of other properties are substituted and functions are invoked but expression is not evaluated.

Expressions **ARE**  evaluated in  *if*  and  *unless* attributes:


```xml

    <property name="left" value="10"/>
    <property name="right" value="7"/>
              
    <do if="${left} GT ${right}">
              
      <echo>
             ${left} GT ${right} evaluates to "true"
      <echo>
               
    </do>

```
Expression can also be evaluated using function `@{Evaluate()}` 


```xml

    <property name="left" value="10"/>
    <property name="right" value="7"/>

    <property name="compare-string" value="${left} GT ${right}"/>     NOTE: value of compare is "10 GT 7", it is not "true"
              
    <property name="compare-result" value="@{Evaluate('${left} GT ${right}')"/>
              
    <echo>
        compare-string = ${compare-string}
                  
        compare-result = ${compare-result}
    </echo>

```
Printed output will be:


```

        compare-string = 10 GT 7
                  
        compare-result = true

```

##### Related Topics: #####
-  [Expression Definitions]( {{< ref "reference/expressions/expressiondefinitions.md" >}} ) 
-  [?? Operator]( {{< ref "reference/expressions/nullcoalescingoperator.md" >}} ) 
