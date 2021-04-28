---
title: Dos and Don'ts
weight: 32
---

This section describes miscellaneous dos and donts

<a name="Section1"></a>
##  ##

Add one or more sections with content

<a name="DontUseOptionSetGetValue"></a>
### Don&#39;t use OptionSetGetValue() function ###

###### Don't use ######

```xml

<optionset name="config-options-common">
  <option name="buildset.cc.defines">
    @{OptionSetGetValue('config-options-common', 'buildset.cc.defines')}
    EA_TOOL
  </option>
</optionset>

```
###### Use following ######

```xml

<optionset name="config-options-common">
  <option name="buildset.cc.defines">
    ${option.value}
    EA_TOOL
  </option>
</optionset>

```
<a name="UseNullCoalescingOperator"></a>
### Use operator ?? ###

Don&#39;t use:


```xml

<do if="@{PropertyExists('enable-some-feature')}">
  . . . .
</do>

```
Use instead:


```xml

<do if="${enable-some-feature??false}">
  . . . .
</do>

```
Following code


```xml

<choose>
  <do if="@{PropertyExists('destination')}">
    <property name="path" value="@{PathCombine('${destination}', 'myprogram.exe')}"/>
  </do>
  <do>
    <property name="path" value="@{PathCombine('${package.configbindir}', 'myprogram.exe')}"/>
  </do>
</choose>

```
Can be replaced with


```xml

<property name="path" value="@{PathCombine('${destination??${package.configbindir}}', 'myprogram.exe')}"/>

```
<a name="UseLocalProperties"></a>
### Use Local Properties ###

Local properties execute faster,, don&#39;t pollute global context.


```xml

<property name="IsDll" value="${Dll??false}" local="true"/>

```
Local with respect to:

 - Script file
 - Target
 - Thread in parallel tasks

