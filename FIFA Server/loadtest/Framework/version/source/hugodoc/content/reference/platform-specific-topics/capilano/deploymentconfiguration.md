---
title: Deployment Configuration
weight: 288
---

This topic describes how to configure push and pull deployment options in generated Visual Studio projects

<a name="Section1"></a>
## Deployment ##

Framework provides configuration elements for layout and deployment options. To set these configuration elements use  `<capilano>` structured extensions:


```xml

<Program name="Example">
 <sourcefiles>
    . . . .
    . . . .
              
 <platforms>
  <capilano>
    <layout isolateconfigurations="true">
      <layoutdir>$(Platform)\${config}\Layout\</layoutdir>
    </layout>
    <deployment
      enable="true"
      removeextrafiles="true"
      deploymode="Pull">
      <pullmapping>
        <path source="C:\src" target="src" include="*.exe" exclude="*.pdb"/>
        <path source="D:\FOO" target="FOO"/>
        <excludepath source="D:\EXCLUDE_PATH"/>
      </pullmapping>
    </deployment>
  </capilano>
</platforms>
          

```
 `<layout>`  element provides two configuration options:

 - **isolateconfigurations=true/false**
 - **layoutdir** - alolws to change default setting for layout directory

 `<deployment>`  element allows to configure  `push`  or  `pull` deployment, mapping file for pull deployment and other options:

 - **enable=true/false** - enable or disable deployment
 - **removeextrafiles=true/false** - remove extra files from target.
 - **deploymode=&quot;pull/push**  - use  `push`  or  `pull` deployment
 - **pullmappingfile** - define content of the pull mapping file or define path to the external pull mapping file.
 - **pulltempfolder** - define temp folder for pull deployment.

For pull deployment external pull mapping file can be defined


```xml

<capilano>
      
   . . . .
      
  <deployment
    enable="true"
    removeextrafiles="true"
    deploymode="Pull">
    <pullmapping   pullmappingfile="${package.dir}\pullmapping.txt"/>
  </deployment>
</capilano>
          
    
```
Or content of the mapping file can be defined. File is automatically created in this case:


```xml

<capilano>
      
   . . . .
      
  <deployment
    enable="true"
    removeextrafiles="true"
    deploymode="Pull">
    <pullmapping>
      <path source="C:\src" target="src" include="*.exe" exclude="*.pdb"/>
      <path source="D:\FOO" target="FOO"/>
      <excludepath source="D:\EXCLUDE_PATH"/>
    </pullmapping>
  </deployment>
</capilano>
          
    
```
See XDK documentation for more information about pull and push deployment.

