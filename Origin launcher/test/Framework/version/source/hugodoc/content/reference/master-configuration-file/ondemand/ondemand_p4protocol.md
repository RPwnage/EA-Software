---
title: P4 Protocol
weight: 249
---

An overview of the Perforce (P4) protocol.

<a name="p4_protocol_usage_requirement"></a>
## Usage Requirement ##

To use this feature, the following must be setup: - The &quot;ondemand&quot; field in masterconfig file (or nant.ondemand property) must be set to true.
 - The package being synced in Perforce has a Manifest.xml with correct &quot;versionName&quot; field.
 - The package&#39;s Manifest.xml should also have a &quot;packageName&quot; field. (This is not made as a requirement).  If this field is provided, Framework will verify<br>the package name indicated in masterconfig against this field to validate that you indeed have the correct package.
 - The package being synced must stay at your ondemandroot.  Otherwise, the up-to-date check step will be skipped.  See the following &quot;Package Already Exists on Different PackageRoot&quot; section for more information.
 - The &quot;package&quot; XML element in masterconfig file need to have an &quot;uri&quot; attribute where the value of the attribute specify the perforce server/path/changelist to sync to:<br><br><br>```<br>p4://<__P4_SERVER__>:<__PORT__>/<__PATH__>?[head | cl=<__CHANGELIST__>]&[rooted]<br>```



<a name="p4_protocol_credentials"></a>
## Perforce Credentials ##

Framework assumes that your Perforce is setup with Windows EA Domain login credentials.
If you&#39;re on other platform, like Linux or Mac,
or using Perforce that doesn&#39;t use domain login, you will need to use eapm to setup the login info so that Framework can use it.
On command line, type &quot;eapm.exe credstore &lt;p4-server-address-and-port&gt;&quot; to setup the credentials.

By default the credential store is placed in a file called nant_credstore.dat in &quot;%USERPROFILE%\nant_credstore.dat&quot; on windows and &quot;~/nant_credstore.dat&quot; on Linux/MacOS.
But you can also generate the file to a specific location when you enter the credentials using eapm by passing a filepath argument before the server name, &quot;eapm.exe credstore &lt;filepath&gt; &lt;p4-server-address-and-port&gt;&quot;.
Or you can override the location by setting the property &quot;nant.credentialstore&quot; when invoking your build.

If Framework encounters a Perforce server it does not have stored credentials for it will try to use existing p4 tickets.  For authenticationless
servers it will also try logging in with local user name and no password.
      If those approaches fail it will prompt the user for credentials.
      This prompt will timeout after a small amount of time in order not to stall automated builds.


{{% panel theme = "info" header = "Note:" %}}
If you&#39;re using eapm to setup the login password, please test it and make sure that it works. Too many unsuccessful login attempts
could get your account locked out.
{{% /panel %}}

{{% panel theme = "info" header = "Note:" %}}
If you need to setup perforce credentials on a build farm environment where you need to have the same credentials set on many machines,
you can do this by adding them to the credential store on one machine and then copying the credential store file to each of the other machines.
{{% /panel %}}
<a name="p4_protocol_proxy"></a>
## Perforce Hosts and Proxies ##

Some perforce hosts have proxies setup for it to improve performance.  Framework will attempt to match up with your proxy by using
the information in the P4ProtocolOptimalProxyMap.xml file. This file is stored on perforce server dice-p4-one.dice.ad.ea.com:2001 at
//fbstream/dev-cm/TnT/Code/External/EA/Framework/Framework/data/P4ProtocolOptimalProxyMap.xml. Framework will attempt to read the most
up-to-date version of this file from this location. If this fails it will use a local version of the file stored in Framework&#39;s /data/
directory. If you notice that this file needs to be updated, please contact{{< url SupportEmailLink >}}to have this file updated.

<a name="URI_Protocol_head_vs_changelist"></a>
## Sync to Head vs Changelist ##

In the uri specification mentioned above, you have the option to sync to head (using the ?head query) or sync to a specific
changelist (using the ?cl=__changelist__ query).  Using a specific changelist (recommended option for deployment) allows you to
be consistent and make sure that Framework will always sync to a specific changelist.  However, if you&#39;re doing active development
and want the package to be always synced to head, you can use the &quot;head&quot; option.  Be careful that using the &quot;head&quot; option will
cause Framework to always perform a sync to head on every build.

By default if neither &quot;head&quot; or &quot;cl=&quot; query is provided, Framework will assume &quot;head&quot; as a default.

Once package is synced.  On future build, Framework will check the specified changelist to see if package is already up-to-date or
or if an update sync is needed.  This step is done by examining the p4protocol.sync file that Framework leave behind at the
package folder after the sync.  See below &quot;p4protocol.sync File&quot; section for more info on this file.

<a name="URI_Protocol_rooted_query"></a>
## The &quot;rooted&quot; query ##

In the uri specification mentioned above, you have the option to use the &quot;rooted&quot; query.  If this entry is provided in the query,
Framework will automatically append the package name and package version to the path.  This option is a handy shortcut to allow
user to just update the version field in the masterconfig file and not need to update the uri field as well during version update.

###### Example ######

```
The following 'package' element:
<package name="UnixCrossTools" version="1.00.03"  uri="p4://dice-p4-one.dice.ad.ea.com:2001/SDK?rooted&amp;head"/>
will be equivalent to:
<package name="UnixCrossTools" version="1.00.03"  uri="p4://dice-p4-one.dice.ad.ea.com:2001/SDK/UnixCrossTools/1.00.03?head"/>
```
<a name="URI_Protocol_package_already_Exists"></a>
## For Package Already Exists on Different PackageRoot ##

It is important to re-iterate that for the auto up-to-date check against indicated changelist to work, the package must be located in
the ondemandroot folder.  So if you changed your ondemandroot or if the package already exists in another package root path aside from
the ondemandroot folder, Framework will skip this up-to-date check and use the existing package from another package root.

<a name="URI_Protocol_p4protocol_sync_file"></a>
## Notice a p4protocol.sync File ##

During the download on demand from perforce process, Framework will save out a file named p4protocol.sync in your synced package folder.
This file is used by Framework to keep track of your last Perforce sync changelist information for this package.  So if you delete this
file or modified this file, it can affect Framework&#39;s download on demand from perforce operation.  If you check in this file to perforce,
Framework will fail your build after the package sync.  You can setup Perforce&#39;s P4IGNORE environment to automatically igore this
file.  Please consult Perforce&#39;s documentation on how to set this up.

