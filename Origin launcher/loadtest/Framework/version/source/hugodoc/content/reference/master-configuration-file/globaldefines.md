---
title: globaldefines
weight: 252
---

Global defines allow users to specify precompiler defines in their masterconfig
file that are applied globally to the project they are building.

<a name="usage"></a>
## Usage ##

There are two ways to declare global defines. The recommended way is to use the ** `<globaldefines>` or `<globaldotnetdefines>` ** element under the ** `<project xmlns="schemas/ea/framework3.xsd">` ** element. Alternatively, users can set the global property `global.defines` or `global.dotnet.defines` in the ** `<globalproperties>` ** section of their masterconfig file.

Global defines can be defined conditionally just like global properties.
The ** `<if>` ** tag can be used inside ** `<globaldefines>` or `<globaldotnetdefines>` ** with a condition attribute.
Also, just like with global properties, conditions can be nested.

Global define conditions are evaluated at the end of the package task
which means that they are evaluated after the configuration package is loaded.

In addition to being able to specify conditions, users can also specify individual
packages that the define should be applied to. One way to do this is to use the `packages` attribute of the ** `<if>` ** element, which may contain the name of one package or a list of packages.
The other way is to use global properties that follow the naming convention `global.defines.[packagename]` or `global.dotnet.defines.[packagename]` .

Please note that Framework performs no intelligent merging of global defines.
If you have a define listed multiple times it is up to the compiler that is
invoked how these are handled, which may include generating a warning or failing
with an error.

<a name="examples"></a>
## Examples ##

Here is an example of how to use the global defines section.


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
Here is an example of how to use global.defines in the global properties section.


```xml
<globalproperties>
              
 <globalproperty name="global.defines">
  BOOLEAN_DEFINE
  BASIC_DEFINE=1
 </globalproperty>
              	
 <globalproperty name="global.defines.EAThread">
  EATHREAD_ONLY_DEFINE=5
 </globalproperty>
              	
 <globalproperty name="global.dotnet.defines">
  BOOLEAN_DOTNET_DEFINE
 </globalproperty>
              	
</globalproperties>
```

##### Related Topics: #####
-  [Everything you wanted to know about Framework defines, but were afraid to ask]( {{< ref "user-guides/everythingaboutdefines.md" >}} ) 
