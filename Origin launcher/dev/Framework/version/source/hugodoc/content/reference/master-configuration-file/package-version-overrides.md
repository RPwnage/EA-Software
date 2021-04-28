---
title: Package Version Overrides
weight: 257
---

There are various ways to change the default package version specified in main masterconfig file.  This page will discuss each method
and describe it's usage purpose.

<a name="Exceptions"></a>
## Using Exceptions ##

```xml
.            <project xmlns="schemas/ea/framework3.xsd">
.              <masterversions>
.                <package name="EAIO" version="2.03.00">
.                  <exception propertyname="config-system">
.                    <condition value="iphone" version="2.03.01"/>
.                    <condition value="android" version="2.04.00"/>
.                  </exception>
.                </package>
.              </masterversions>

.              <globalproperties>prop1</globalproperties>

.              . . . .

.            </project>
```

Users may wish to easily switch between versions of a particular package.
For example, if a different version of a package is needed when building different platforms.
This is handled in the masterconfig file using exceptions as shown above.

In the above example EAIO is set to version 2.03.00. The exception says to check the config-system property and use 2.03.01 for ios and 2.04.00 for android.

Only certain properties defined by framework and global properties are available when writing masterconfig exceptions.
If you use a property that is not defined it will silient use the default version.

Simply adding a property to &lt;globalproperties&gt; does not guarentee it will be defined if the value has not been pre-initialized in the masterconfig.
If your package or any dependent package&#39;s build script tries to spawn a command line nant.exe execution in any custom targets or
vcproj pre/post build step these scripts need to be updated to explicitly pass in the property.
Otherwise, those new nant instance will not know that property exists and the version will be reverted to default version in this new nant instance.


```xml
.             <target name="custom">
.               <exec program="${nant.location}\nant.exe" commandline="-masterconfigfile:${package.dir}\masterconfig.xml"/>
.             </target>
```
or


```xml
.             <property name="runtime.SomeModule.vcproj.post-build-step">
.               ${nant.location}\nant.exe -masterconfigfile:${package.dir}\masterconfig.xml
.             </property>
```

This type of package version override setup is mostly appropriate for changes that is going to apply to mostly everyone.  If there are isolated teams
that need to change a package to different version and not wishing to diverge a common masterconfig that is used by all teams, people should consider
using the Fragments Override feature below.

<a name="FragmentsOverride"></a>
## Using Fragments Override ##

```xml
. Content of TnT/masterconfig.xml
.
.            <project xmlns="schemas/ea/framework3.xsd">
.              <masterversions>
.                <package name="UnixClang" version="6.0.1-1"/>
.              </masterversions>
.
.              . . . .
.
.              <fragments>
.                <include name="./Code/*/masterconfig_fragment.xml">
.              <fragments>
.
.            </project>
.
. Content of TnT/Code/CM/masterconfig_fragment.xml
.
.            <project xmlns="schemas/ea/framework3.xsd">
.              <masterversions>
.                <package name="UnixClang" version="dev" allowoverride="true"/>
.              </masterversions>
.              . . . .
.            </project>

```

Overriding default version by using masterconfig fragment as shown in above example is intended for specific teams who need to 
change to different version without needing to locally diverge the main masterconfig file being used by everyone.  Good example
would be development team who actively work on a development branch while everyone else uses an officially released version. Or game
teams who need to make divergence of a specific package can change the version to a different one without modifying common masterconfig
file that is used by all teams.

One potential downside is that the system has too many fragments being loaded and each fragment trying to override and use 
different version.  So if the version update should really apply to everyone, people should consider just updating the main masterconfig
so that everyone is upgraded to the same new version instead.

Note that people need to provide the 'allowoverride' attribute and set that to true.  


