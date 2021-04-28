---
title: package
weight: 244
---

Package element is used to specify package version that will be used by Framework and autobuildclean status.

<a name="Usage"></a>
## Usage ##

`<package>` element can appear under `<masterversions>` element,
or inside [grouptype]( {{< ref "reference/master-configuration-file/masterversions/grouptype.md" >}} ) element.

The `<package>` element defines which package versions should be used in a build.
NAnt will look for packages in the folders defined in `<packageroots>` and subfolders of those folders.
Only packages that are required for the build (passed as parameter to the{{< task "EA.FrameworkTasks.DependentTask" >}} task) are searched for.

If a required package is missing NAnt will print an error message, listing versions of the required package found on a local hard drive and on a package
server and file that declares dependency on a missing package:


```
C:\packages\eaconfig\1.30.01\config\targets\target-build.xml(22,5): Failed to download dependent package 'EAThread-1.14.11' or one of its dependents from the Package Server:
   Package 'EAThread-1.14.11' not found on package server.
If package 'EAThread-1.14.11' already exists in your package roots, make sure it is either under its version folder, or it has a <version> tag inside its manifest.
'EAThread-1.14.11' package is referenced in:
   'C:\packages\EAText\1.05.05\EAText.build'.
Following versions of the package EAThread are found in package roots:
  EAThread-1.15.00: C:\packages\EAThread\1.15.00
  EAThread-1.14.07: C:\packages\EAThread\1.14.07
Following versions of the EAThread package were found on the package server:
   EAThread-1.15.00 (Official): http://packages.worldwide.ea.com/release.aspx?id=9998
   EAThread-1.14.07 (Official): http://packages.worldwide.ea.com/release.aspx?id=9686
   EAThread-1.14.04 (Official): http://packages.worldwide.ea.com/release.aspx?id=8708
   EAThread-1.14.03 (Official): http://packages.worldwide.ea.com/release.aspx?id=8660
   EAThread-1.14.02 (Official): http://packages.worldwide.ea.com/release.aspx?id=8350
```

### package ###
**Attributes:** 

* **`name`** (required) - name of the package. Package names are case sensitive.
* **`version`** (required) - package version. Package version can be altered using  [version exceptions]( {{< ref "#PackageVersionException" >}} )
* **`uri`** (optional) - Package download URI. At present, this URI protocol only support downloading from perforce using this syntax:<br>
p4://&lt;_P4_SERVER_&gt;:&lt;_PORT_&gt;/&lt;_P4_PATH_TO_PACKAGE_&gt;?&lt;head | cl=&lt;_CHANGELIST_&gt;&gt;<br>
Please see the [ondemand]( {{< ref "reference/master-configuration-file/ondemand/_index.md#uri_package_server" >}} ) page for more information.
* **`autobuildclean`** (optional) - value can be true or false. Default is true. When autobuildclean=false package is considered already built. Framework will not try to build or clean it, but will use libraries and other data exported by the package in [Initialize.xml file]( {{< ref "reference/packages/initialize.xml-file/_index.md" >}} ).<br/>
Note: autobuildclean setting can be overridden by nant command line option  `-forceautobuild`
* **`autobuildtarget`** (optional) - name of the target executed when package is used as a dependent. Default autobuild target is &#39;build&#39;,<br>unless project element in package build script specified different default value (see [build scripts overview]( {{< ref "reference/packages/build-scripts/_index.md" >}} ) ).
* **`autocleantarget`** (optional) - name of the target executed by Framework &#39;clean&#39; target when package is used as a dependent. Default autoclean target is &#39;clean&#39;.
* **`allowoverride`** (optional) - Default is false.  Setting it to true allows a masterconfig fragment to override the package&#39;s version number (otherwise, if there is a conflict, you will receive a build error.)<br>See example below.

**Nested elements** 

* [exception]( {{< ref "#PackageVersionException" >}} ) - exception element allows to redefine value of the [package version]( {{< ref "#PackageVersion" >}} ) based on conditions (see [Package version Exception]( {{< ref "#PackageVersionException" >}} ) ). <br/> Note: Multiple exception elements are allowed.

## Example ##

### Basic Example ###

```xml
<project xmlns="schemas/ea/framework3.xsd">
    <masterversions>
        <package name="eaconfig"              version="2.04.03"          />
        <package name="VisualStudio"          version="10.0.40219-2-sp1" />
        <package name="ps3sdk"                version="253.001-lite"     />
    </masterversions>
</project>
```
An example for using the &#39;allowoverride&#39; attribute in fragments.  See  [fragments]( {{< ref "reference/master-configuration-file/fragments.md" >}} )  page for more information on fragments.

### Example with masterconfig fragments and 'allowoverride' usage ###

```xml
Main masterconfig file: ./main_masterconfig.xml
<project xmlns="schemas/ea/framework3.xsd">
    <masterversions>
        <package name="TestPackage"           version="1.00.00"          />
    </masterversions>
  
    <fragments>
        <include name="./MasterConfigOverrides/*.xml"/>
    </fragments>
   
</project>

Masterconfig file fragment: ./MasterConfigOverrides/DevOverrides.xml
<project xmlns="schemas/ea/framework3.xsd">
    <masterversions>
        <package name="TestPackage"           version="dev"      allowoverride="true"      />
    </masterversions>
</project>

During build, you will receive the following message informing you the version change:

> nant.exe -masterconfigfile:main_masterconfig.xml
Fragment 'MasterConfigOverrides\DevOverrides.xml' is changing package 'TestPackage' from version '1.00.00' to 'dev'.
```
<a name="PackageVersionException"></a>
## Package Version Exception ##

`<exception>` element can appear under `<package>` element. Exception element defines property name that will be used in conditions.

### exception ###
**Attributes:** 

* **`propertyname`** (required)- name of the property that is used to evaluate conditions.

**Nested elements** 

exception can contain one or more conditions

* **`condition`** - condition contains two attributes:
  * **`value`** value is compared to the value of property defined by &#39;propertyname&#39; attribute in the &#39;exception&#39; element, and if values match the condition is applied.
  * **`version`** Package version is set to the value of this attribute when condition is satisfied.<br/>
  Note: Multiple condition elements are allowed in the exception. The first condition that evaluates to true is used.

## Example ##

### Using multiple exceptions ###

```xml
<package name="VisualStudio" version="10.0.40219-1-sp1" >
    <exception propertyname="config-system">
        <condition value="winrt" version="11.0.0-pr1" />
    </exception>
    <exception propertyname="vs2008">
        <condition value="true" version="9.0.30729-5-sp1" />
    </exception>
</package>
```
