---
title: Asset Deployment Setup
weight: 198
---

This topic discuss how to setup the build script to do asset deployment for different platforms and some unique behaviour
for different platforms.

<a name="Usage"></a>
## Usage ##

To turn on the asset deployment support in build script, you need to set the following deployassets flag to true.

###### In Structured XML ######

```xml
<Program name="[ModuleName]">
  ...
  <buildsteps>
    <packaging deployassets="true">
    ...
    </packaging>
  </buildsteps>
</Program>
```
After the above flag is turned on, you need to actually specify the asset files to deploy. To do so, you setup the following
assetfiles fileset:

###### In Structured XML ######

```xml
<Program name="[ModuleName]">
  ...
  <buildsteps>
    <packaging deployassets="true">
      <assetfiles basedir="${package.dir}/source_assets">
        <includes name="**"/>
      </assetfiles>
    </packaging>
  </buildsteps>
</Program>
```
By default, after the build, the indicated asset files will be copied over to the following folder:

 - PC, Unix, Kettle: ${package.configbindir}\[{group}]\assets
 - Capilano: ${package.builddir}\[ModuleName]\[platform]\Layout\Image\Loose
 - iPhone: ${package.configbindir}\[${group}]\${module_output_name}.app
 - OSX: ${package.configbindir}\[${group}]\${module_output_name}.app\Contents\Resources
 - Android: asset files are added to the APK file, which is just a zip file. Inside the APK file the asset files are added in a folder named &quot;assets&quot;.

<a name="AlteringOutputDirectory"></a>
## Altering The Output Directory Behaviour ##

If Framework&#39;s default asset output location isn&#39;t what you desired, it can be changed using the asset-configbuilddir property.
This property can be setup like so:

###### In Structured XML ######

```xml
<Program name="[ModuleName]">
  ...
  <buildsteps>
    <packaging deployassets="true">
      <assetfiles basedir="${package.dir}/source_assets">
        <includes name="**"/>
      </assetfiles>
      <asset-configbuilddir value="testdata"/>
    </packaging>
  </buildsteps>
</Program>
```
This property may be set either as a full path or as a relative path. If a relative path is used the output path will be as follows for each platform:

 - PC, Unix, Kettle: ${package.configbindir}\[${group}]\${${group}.${module}.asset-configbuilddir}
 - Capilano: ${package.builddir}\[ModuleName]\[platform]\Layout\Image\Loose\${${group}.${module}.asset-configbuilddir}
 - iPhone: ${package.configbindir}\[${group}]\${module}.app\${${group}.${module}.asset-configbuilddir}
 - OSX (plain nant build): ${package.configbindir}\[${group}]\${${group}.${module}.asset-configbuilddir}
 - OSX (build using XcodeProjectizer): ${package.configbindir}\[${group}]\${module}.app\Contents\Resources\${${group}.${module}.asset-configbuilddir}
 - Android: This property currently has no influence on android builds.

<a name="DependentAssets"></a>
## Asset Files From Dependent Packages ##

If you have dependent packages that export asset files and you need them added to your module, you can use the assetdependencies property to list the packages like so:

###### In Structured XML ######

```xml
<Program name="[ModuleName]">
  ...
  <dependencies>
    <auto>
      TestPackage1
      TestPackage2
    </auto>
  </dependencies>
  ...
  <buildsteps>
    <packaging deployassets="true">
      <assetdependencies value="TestPackage1 TestPackage2"/>
      <assetfiles basedir="${package.dir}/source_assets">
        <includes name="**"/>
      </assetfiles>
    </packaging>
  </buildsteps>
</Program>
```
However, it is important to remember that your dependent packages need to declare the assetfiles fileset to export out in the intialize.xml
in order for this feature to work.  The following in an example:

###### Initialize.xml for TestPackage1 In Structured XML ######

```xml
<publicdata packagename="TestPackage1">
  <runtime>
    <module name="TestLib1">
      <assetfiles basedir="${package.TestPackage1.dir}">
        <includes name="${package.TestPackage1.dir}/assets/sxml-testlib1-data/*.txt"/>
      </assetfiles>
    </module>
  </runtime>
</publicdata>
```
<a name="AlteringAssetSynchronization"></a>
## Altering The Default Assset Synchronization Behviour ##

By default, the output asset files directory is synchronized with the assetfiles fileset so that asset files only need
to be copied if there are changes. This also means that any files in the output directory that are not listed in the
fileset will be deleted.

However, this default behaviour is not always desirable and Framework offer two ways for you to modify this behaviour:

 - Use{{< url groupname >}}.additional-assetfiles fileset to tell Framework to exclude synchronization for the indicated files.<br><br>The following is an example:<br><br><br>```xml<br><fileset name="[group].[ModuleName].additional-assetfiles"<br>            failonerror="false"<br>            if="${config-system} == iphone"><br>  <includes name="${package.configbindir}\TestProgram.app\test.nib" asis="true"/><br></fileset><br>```<br>Note that you cannot use wildcard to set up this fileset.  If your asset files are generated in a [preprocess step]( {{< ref "reference/packages/build-scripts/pre/post-process-steps/moduleprocess.md" >}} ) , you can update this fileset in your<br>preprocess step.
 - Tell Framework to just copy but skip the synchronization behaviour completely.<br><br>You can do this by specifying the SXML packaging element&#39;s copyassetswithoutsync attribute and set it to true or set<br>the property{{< url groupname >}}.CopyAssetsWithoutSync to true in old style build script.  The following are some<br>examples:<br><br>###### In Structured XML ######<br><br>```xml<br><Program name="[ModuleName]"><br>  ...<br>  <buildsteps><br>    <packaging deployassets="true" copyassetswithoutsync="true"><br>      <assetfiles basedir="${package.dir}/source_assets"><br>        <includes name="**"/><br>      </assetfiles><br>    </packaging><br>  </buildsteps><br></Program><br>```


##### Related Topics: #####
-  [assetfiles]( {{< ref "reference/packages/build-scripts/native-c/c++-modules/filesets/assetfiles.md" >}} ) 
-  [assetdependencies]( {{< ref "reference/packages/build-scripts/native-c/c++-modules/properties/assetdependencies.md" >}} ) 
