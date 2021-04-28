---
title: Master Configuration File
weight: 242
---

Master Configuration Files define the versions of the packages,
the{{< url ConfigurationPackage >}}, and a number of other configuration parameters that will be used in a build.

<a name="MasterconfigSyntax"></a>
## Master Configuration File ##

Master configuration files are written in XML. They are the global configuration authority for the build and include version numbers of all the packages that can be used.
The information included in the masterconfig file includes:

 - Where to look for installed Packages on your machine
 - The version of each package to build against
 - The location of the build output
 - The [Configuration Packages]( {{< ref "reference/configurationpackage.md" >}} ) to use
 - Whether to download and install missing packages automatically (See [ondemand]( {{< ref "reference/master-configuration-file/ondemand/_index.md" >}} ) section for more info).
 - Declare Global Properties, define default Configurations and several other options.

{{% panel theme = "info" header = "Note:" %}}
A Masterconfig File must be [well-formed XML]( {{< ref "framework-101/xml.md" >}} )  with  ** `<project xmlns="schemas/ea/framework3.xsd">` ** as the root (document) element. In spite of this, it is not a NAnt script; As a result, NAnt script elements
(like `<if>` ,  `<do>` , and others) cannot be used in Masterconfig Files.
{{% /panel %}}
 - ** `<project xmlns="schemas/ea/framework3.xsd">` ** <br><br>  - [masterversions]( {{< ref "reference/master-configuration-file/masterversions/_index.md" >}} ) - required<br>  - [packageroots]( {{< ref "reference/master-configuration-file/packageroots.md" >}} )<br>  - [buildroot]( {{< ref "reference/master-configuration-file/buildroot.md" >}} )<br>  - [ondemand]( {{< ref "reference/master-configuration-file/ondemand/_index.md" >}} )<br>  - [ondemandroot]( {{< ref "reference/master-configuration-file/ondemandroot.md" >}} )<br>  - [globalproperties]( {{< ref "reference/master-configuration-file/globalproperties.md" >}} )<br>  - [globaldefines]( {{< ref "reference/master-configuration-file/globaldefines.md" >}} )<br>  - [Warnings]( {{< ref "reference/master-configuration-file/warnings.md" >}} )<br>  - [Deprecations]( {{< ref "reference/master-configuration-file/deprecations.md" >}} )<br>  - [config]( {{< ref "reference/master-configuration-file/config.md" >}} ) - required<br>  - [fragments]( {{< ref "reference/master-configuration-file/fragments.md" >}} )

{{% panel theme = "info" header = "Note:" %}}
It is not necessary for a package itself to contain a masterconfig file.  This masterconfig file can be provided through nant.exe command line option "-masterconfigfile:".
It only load the top package's masterconfig.xml if the nant.exe command line does not provide one.  Once the initial masterconfig
file is loaded for the top package, this file will be the only masterconfig being used throughout.  If this top level package needs to load a 
dependent package and the dependent package also provided a masterconfig file, that dependent package's masterconfig will be ignored.

However, this normal behaviour can be broken when user write build script that explicitly use `<exec>` task or write module post build step
that explicitly launch a new nant.exe process without passing in current masterconfig being used.
{{% /panel %}}

<a name="Flattened"></a>
## Flattened packages ##

Some package users don't want their packages to have a version information (ie no version folder and no version information in manifest.xml).
For example, a large engine that has a single version of each package checked in version control may consider the version folder unnecessary.
Framework allows users to do this with a feature called flattened packages.
In the masterconfig file, use the keyword &quot;flattened&quot; as the version number for any packages that are flattened with no version information.


```xml
.            <project xmlns="schemas/ea/framework3.xsd">
.              <masterversions>
.                <package name="PackageB" version="flattened"/>
.                . . . .
.              </masterversions>
.            </project>
```

<a name="sample"></a>
## Sample ##

Here is a sample master config file containing the essentials one would need to build one of the example packages.

###### A sample master config file ######

```xml
<project xmlns="schemas/ea/framework3.xsd">
  <masterversions>
    <!-- Compiler & SDK packages-->
    <package name="kettlesdk"                version="5.508.021"     />
    <package name="CapilanoSDK"              version="180200-2"       />
    <package name="WindowsSDK" 		   version="10.0.16299"  />
  </masterversions>

  <!-- Option to download packages specified in the above list if not found  -->
  <ondemand>true</ondemand>

  <!-- A folder name within the built package where auto-generated and temp files will be placed -->
  <buildroot>build</buildroot>

  <!-- Directories to look for the above packages in -->
  <packageroots>
    <packageroot>${nant.location}\..\..\..</packageroot>
    <packageroot>.\..\..</packageroot>
  </packageroots>

  <!-- The platforms to build for unless specified on the commandline -->
  <config default="pc-vc-dev-debug"/>
</project>
```
