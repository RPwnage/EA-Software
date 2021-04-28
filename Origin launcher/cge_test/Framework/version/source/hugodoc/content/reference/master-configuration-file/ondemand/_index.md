---
title: ondemand
weight: 248
---

This element is used to turn on ondemand package download.  Packages synchronized using this feature will be installed onto your local
workstation from some source.  Sources include [Package Server]( {{< ref "reference/package-management-tools/packageserver.md" >}} ) ,
Perforce, NuGet, etc.

<a name="Usage"></a>
## Usage ##

Enter the element in the Masterconfig file and specify &#39;true&#39; or &#39;false&#39; as an inner text.

By default ondemand functionality is turned off.

At startup, Framework will scan all [package roots]( {{< ref "reference/master-configuration-file/packageroots.md" >}} ) and identify all package releases present
under the package roots folders. When Framework needs to load a package (i.e. when{{< task "EA.FrameworkTasks.DependentTask" >}}task is invoked) and package is not present in the package roots Framework will automatically download the package into [ondemand root]( {{< ref "reference/master-configuration-file/ondemandroot.md" >}} )  if  ** `ondemand` ** is set to true.

###### Example ######

```xml
<project xmlns="schemas/ea/framework3.xsd">
 <masterversions>
 . . .
 </masterversions>
 <ondemand>true</ondemand>
</project>
```
 ** `ondemand` **  functionality can also be turned on/off using  ** `nant.ondemand` ** property on the nant command line:


```
nant -D:nant.ondemand=true
```

{{% panel theme = "info" header = "Note:" %}}
- ** `nant.ondemand` ** property on the nant command line overrides masterconfig setting.
 - Setting `nant.ondemand` in the build scripts will have no effect. This property is used only when it is specified on the<br>nant command line.
{{% /panel %}}
<a name="web_package_server"></a>
## Web Package Server ##

The web package server is the default option used for acquiring packages. Other than setting &lt;ondemand&gt; to true, no additional
configuration is required to use this package server.


{{% panel theme = "info" header = "Note:" %}}
Access to the Package Server is secured. Framework will use your Windows login credentials to login into the package server.

See  [Package Management Tools]( {{< ref "reference/package-management-tools/_index.md" >}} ) topic to learn how to set credentials for the package server when you are
on a non-Windows system or not logged into EA domain.
{{% /panel %}}
<a name="uri_package_server"></a>
## URI Package Servers ##

As alternative to the web package server, packages can be downloaded using a URI specification. To use this option, the &#39;ondemand&#39; option
must be turned on and packages in your masterconfig must be marked up with a &quot;uri&quot; attribute.

For example:

###### Uri Examples ######

```xml
<masterversions>
  <package name="NuGet.Core"     version="2.8.5"           uri="nuget-https://www.nuget.org/api/v2/" />
  <package name="jdk"            version="1.7.0_67-1-pc"   uri="p4://eaos.rws.ad.ea.com:1666/EAOS/Framework2/RL/jdk/1.7.0_67-1-pc?cl=1039469" >
    <exception propertyname="nant.platform_host">
      <condition value="osx"     version="1.7.0_67-1-osx"  uri="p4://eaos.rws.ad.ea.com:1666/EAOS/Framework2/RL/jdk/1.7.0_67-1-osx?cl=1039469"/>
    </exception>
  </package>
<masterversions>
```
###### Uri Format ######

```
<__PROTOCOL__>://<__SERVER__>(:<__PORT__>)/<__PATH__>?<__QUERY__>
```
The following protocols are supported:

 - [P4 Protocol]( {{< ref "reference/master-configuration-file/ondemand/ondemand_p4protocol.md" >}} )
 - [Nuget Protocol]( {{< ref "reference/master-configuration-file/ondemand/ondemand_nugetprotocol.md" >}} )

<a name="cleanup"></a>
## Ondemand Cleanup ##

Over time ondemand packages can accumulate in a users ondemand root.
When they update their masterconfig to a new version it will download another version of the package but the old one will remain.
When the ondemand folder starts taking up too much space on a users computer they often need to spend time
trying to figure out what packages they can delete and what ones they still need.

However, Framework has a way to make cleaning up old ondemand packages easier.
Framework creates metadata files for each package in an ondemand root and in these files stores a timestamp that indicates the last time the package was used.
Every time that an ondemand package is depended on or run as a top level package the timestamp in the metadata file will be updated.
Then when a user wants to get rid of old packages they can run the eapm &quot;prune&quot; command which will either
delete the packages or print a warning so that the user can decide which ones to delete.


{{% panel theme = "info" header = "Note:" %}}
running eapm prune will create metadata tracking files for any packages that are missing them.
{{% /panel %}}
The eapm prune command which is used to delete old packages has a few settings that users can control.
The prune command requires that users provide the path to the ondemand root they wish to clean up as the first argument.
Users can also set the threshold value, which indicates how old in days a package should be before it is delete, by default the threshold is set to 90 days.
There is also a flag &quot;-warn&quot; which indicates that you want the command to only print a warning message about what packages are old so that you can decided to delete them yourself.
If a package is missing a metadata file the eapm prune command will not be able to determine how old it is and will not delete it,
but instead will create a new metadata file with the timestamp set to the current date so that the age can begin to be tracked so it can eventually be deleted.

If for some reason you want to disable the reference date tracking for ondemand packages you can use a global property &quot;ondemand.referencetracking=false&quot; to disable this.


##### Related Topics: #####
-  [ondemandroot]( {{< ref "reference/master-configuration-file/ondemandroot.md" >}} ) 
-  [P4 Protocol]( {{< ref "reference/master-configuration-file/ondemand/ondemand_p4protocol.md" >}} ) 
-  [Nuget Protocol]( {{< ref "reference/master-configuration-file/ondemand/ondemand_nugetprotocol.md" >}} ) 
-  [Package Server]( {{< ref "reference/package-management-tools/packageserver.md" >}} ) 
-  [Package Management Tools]( {{< ref "reference/package-management-tools/_index.md" >}} ) 
