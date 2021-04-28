---
title: Nuget Protocol
weight: 250
---

An overview of the Nuget protocol (NOTE: This protocol is currently a beta feature.)

<a name="nuget_protocol_usage_requirement"></a>
## Usage Requirement ##

To use this feature, the following must be setup: - The &quot;ondemand&quot; field in masterconfig file (or nant.ondemand property) must be set to true.
 - The &quot;package&quot; XML element in masterconfig file need to have an &quot;uri&quot; attribute specifying the NuGet server, prefixed with &quot;nuget-&quot; in order<br>to indicate to Framework this is a NuGet protocol package.<br><br>###### Example NuGet Uri ######<br><br>```xml<br><package name="log4net" version="2.0.3" uri="nuget-https://www.nuget.org/api/v2/"/><br>```<br>Once your masterconfig is set up this way can just dependent on these packages the same way you would any other Framework package.<br><br>###### Example NuGet Depedency ######<br><br>```xml<br><CSharpProgram name="MyModul><br>  <copylocal>true</copylocal><br>  <dependencies><br>    <auto><br>     log4net<br>    </auto><br>  </dependencies><br></CSharpProgram><br>```<br>Note NuGet packages names should use the canocial package name of the NuGet package not the display name. The canonical name is the name you would<br>use to install the package via Package Manager Console and not the name you would see on the NuGet package page. These names are often the same.



<a name="nuget_protocol_automatic_versions"></a>
## Automatic Versions ##

When downloading a NuGet package Framework will automatically determine automatic versions to use for dependent NuGet packages even if you
do not specify a version for the package in your masterconfig. If multiple Nuget packages require different versions of a package you will
need to explicitly specify a version in the masterconfig that satisfys the version constraints of both packages.

<a name="nuget_protocol_post_install_scripts"></a>
## NuGet Post-Install Scripts ##

NuGet packages often feature PowerShell initialization and install scripts. By default Framework will try to run these after solution
generation however this may or may not be undesirable. For a desirable example a package may add MSBuild targets to your project file
that are required to use the NuGet package. For an undesirable example the package might modify your app.config file and stomp over
your changes. In general you want these scripts to run but if not they can be disabled using the following properties:
```
package.<nuget-package-name>.suppress-nuget-init=true
package.<nuget-package-name>.suppress-nuget-install=true
```
It is recommended you set these in the global properties section of your masterconfig. Note that these properties do not automatically
suppress dependencies of this package.

NuGet install scripts expect to run within a Visual Studio context and can perform any arbitrary actions within that context. Unfortunately
actually creating this context is prohibitively slow as it requires starting a Visual Studio instance, loading your solution and then
waiting for all projects to be initialized. Instead Framework creates a &quot;fake&quot; context that satisfies most of the requirements of NuGet install
scripts and is cheap to construct. However if your NuGet packages&#39; install scripts are throwing errors and script suppress is not an option,
please send a Support request to #frostbite-ew slack channel for assistance.

<a name="nuget_protocol_submitting_packages"></a>
## Submitting NuGet Packages To Source Control ##

If you don&#39;t wish to use NuGet but would like instead to have these packages in your source control, the packages created in your
ondemand root are full Framework packages and can used as is.

