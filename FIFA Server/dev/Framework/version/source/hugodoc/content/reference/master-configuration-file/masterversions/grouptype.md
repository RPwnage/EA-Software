---
title: grouptype
weight: 245
---

Grouptype element is used to apply settings to groups of packages.

<a name="Usage"></a>
## Usage ##

 ** `<grouptype>` **  element can appear under  ** `<masterversions>` ** element. Grouptype can not be nested inside another grouptype.

##### grouptype #####
 **Attributes:** 

   ** `name` ** (required)- name of the grouptype. The name of the grouptype is added to the buildroot for all packages in this grouptype.<br><br>Name of the grouptype can also be used in exceptions to assign all packages in the given grouptype to another grouptype.<br><br><br>{{% panel theme = "warning" header = "Warning:" %}}<br>Following characters can not be used in grouptype name:<br><br> `", \, /, :, *, ?, ", <, >, |` <br><br>grouptype name can not start or end with space or dot (.) characters.<br>{{% /panel %}}
   ** `autobuildclean` ** (optional)- value can be &#39;true&#39; or &#39;false&#39;. Default is &#39;true&#39;. When autobuildclean=false all packages in the grouptype are considered already built.<br>Framework will not try to build or clean it, but will use libraries and other data exported by the package in [Initialize.xml file]( {{< ref "reference/packages/initialize.xml-file/_index.md" >}} ) <br><br><br>{{% panel theme = "info" header = "Note:" %}}<br>[autobuildclean]( {{< ref "reference/master-configuration-file/masterversions/package.md#AutobuildClean" >}} )  attribute specified in the  [package]( {{< ref "reference/master-configuration-file/masterversions/package.md" >}} ) element overrides grouptype setting.<br>{{% /panel %}}<br><br>{{% panel theme = "info" header = "Note:" %}}<br>autobuildclean setting can be overridden by nant command line option  ** `-forceautobuild` **<br>{{% /panel %}}
   ** `outputname-map-options` **  (optional)- sets  [output mapping options]( {{< ref "reference/packages/redefinebuildoutputpaths.md" >}} ) for packages defined in this grouptype.

 **Nested elements** 

   ** [package]( {{< ref "reference/master-configuration-file/masterversions/package.md" >}} ) ** - packages in this grouptype.<br><br><br>{{% panel theme = "info" header = "Note:" %}}<br>List of packages can be empty and assigned using  [grouptype exception]( {{< ref "#GroupTypeException" >}} ) .<br>{{% /panel %}}
   ** [exception]( {{< ref "#GroupTypeException" >}} ) ** - exception element allows to assign packages in this grouptype to another grouptype.<br><br><br>{{% panel theme = "info" header = "Note:" %}}<br>Multiple exception elements are allowed.<br>{{% /panel %}}

## Example ##

Grouptypes can be used to organize build folder. Build output from different packages goes into grouptype subfolders:


```xml
.         <masterversions>
.           <package  name="eaconfig"                   version="2.06.00"    />
.
.           <grouptype name="build">
.             <package  name="AndroidSDK"    version="9-ndk-r6b-2"      />
.           </grouptype>
.           <grouptype name="deprecated">
.             <package  name="rwcore"  version="1.04.02" />
.           </grouptype>
.           <grouptype name="docs">
.             <package  name="Doxygen"            version="1.5.6"   />
.             <package  name="GraphViz"           version="2.6.0"   />
.           </grouptype>
.           <grouptype name="packages">
.             <package  name="coreallocator"                   version="1.03.07" />
.             <package  name="EAAssert"                        version="1.03.00" />
.             <package  name="EABase"                          version="2.00.29" />
.           </grouptype>
.           <grouptype name="tests">
.             <package  name="benchmarkenvironment"  version="3.17.00" />
.           </grouptype>
.         </masterversions>
```
Grouptypes can be used to define prebuild packages:


```xml
.         <masterversions>

.           <package name="cocos2dx"                    version="dev"/>
.
.           <grouptype name="Prebuilt" autobuildclean="false">
.             <package name="coreallocator"               version="1.03.07"/>
.             <package name="EAAssert"                    version="1.02.00"/>
.             <package name="EABase"                      version="2.00.28"/>
.             <package name="EACallstack"                 version="1.12.06"/>
.             <package name="EACOM"                       version="2.06.05"/>
.             . . . . . . .
.           </grouptype>

.          </masterversions>
```
<a name="GroupTypeException"></a>
## GroupType Exceptions ##

 ** `<exception>` **  element can appear under  ** `<grouptype>` ** element.
Exception element defines property name that will be used in conditions.

##### exception #####
 **Attributes:** 

   ** `propertyname` ** (required)- name of the property that is used to evaluate conditions.

 **Nested elements** 

exception can contain one or more conditions

   ** `condition` ** - condition contains three attributes:<br><br>  - ** `value` ** value is compared to the value of property defined by &#39;propertyname&#39; attribute in the &#39;exception&#39; element, and if values match the condition is applied.<br>  - ** `name` ** - new grouptype name.<br><br>If grouptype with new name exists packages from the given group assigned to the new group. All other settings from the new group will be assigned to packages.<br><br>If grouptype with new name does not exist - the name of the given group is changed.<br>  - ** `autobuildclean` ** - change autobuildclean setting for given group.<br><br><br>{{% panel theme = "info" header = "Note:" %}}<br>This attribute is taken into account only when new group name is same as given group name, or if there is no existing group which name matches new name<br><br>If new name points to the another existing group - settings from that group will be applied.<br>{{% /panel %}}<br><br><br>{{% panel theme = "info" header = "Note:" %}}<br>Multiple condition elements are allowed in the exception. The first condition that evaluates to true is used.<br>{{% /panel %}}<br><br>{{% panel theme = "info" header = "Note:" %}}<br>Exceptions are evaluated recursively. I.e. when exception points to another group, exceptions in that group are evaluated as well.<br>{{% /panel %}}

## Example ##

Exception allows to move packages between groups:


```xml
.         <masterversions>

.           <package name="cocos2dx"                    version="dev"/>
.
.           <grouptype name="Prebuilt" autobuildclean="false">
.             <exception propertyname="use-live-build">
.               <condition value="true" name="LiveBuild" />
.             </exception>
.             <package name="coreallocator"               version="1.03.07"/>
.             <package name="EAAssert"                    version="1.02.00"/>
.             <package name="EABase"                      version="2.00.28"/>
.             <package name="EACallstack"                 version="1.12.06"/>
.             <package name="EACOM"                       version="2.06.05"/>
.             . . . . . . .
.           </grouptype>

.           <grouptype name="LiveBuild" autobuildclean="true"/>

.         </masterversions>
```
Exception exception can be used to change group name. There is no need to define grouptype &#39;AlternativeGroupName&#39; if only name and no other parameters need to be changed.


```xml
.         <masterversions>

.           <grouptype name="MyGroupName" >
.             <exception propertyname="change-my-groupname">
.               <condition value="true" name="AlternativeGroupName" />
.             </exception>
.             <package name="EAAssert"                    version="1.02.00"/>
.             <package name="EABase"                      version="2.00.28"/>
.           </grouptype>

.         </masterversions>
```
Exception exception can be used to change autobuildclean attribute.


```xml
.         <masterversions>

.           <grouptype name="MyGroupName" >
.             <exception propertyname="use-prebuild-packages">
.               <condition value="true" name="MyGroupName" autobuildclean="false" />
.             </exception>
.             <package name="EAAssert"                    version="1.02.00"/>
.             <package name="EABase"                      version="2.00.28"/>
.           </grouptype>

.         </masterversions>
```
