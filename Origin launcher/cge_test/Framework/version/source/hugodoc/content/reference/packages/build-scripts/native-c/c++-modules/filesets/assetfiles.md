---
title: assetfiles
weight: 138
---

assetfiles fileset defines deployable assets

## Usage ##

Use `<assetfiles>`  in SXML or define {{< url fileset >}} with name {{< url groupname >}} `.assetfiles` in your build script.

Asset files deployment may use paths relative to `fileset`  or  `include` elements base directory.

To control whether assets should be deployed/packaged use SXML element `deployassets`  or property {{< url groupname >}} `.deployassets` .

The output asset files directory is synchronized with the assetfiles fileset so that asset files only need to be copied if there are changes.
This means that any files in the output directory that are not listed in the fileset will be deleted.
If a user needs to add a file to this directory that is excluded from the synchronization process so it wont be deleted they should add
the path to the output file to the fileset `package.${package.name}.additional-assetfiles` .


{{% panel theme = "warning" header = "Warning:" %}}
The asset file directory will be synchronized with the files described by this fileset.
This means that if you have two modules in the same package with different asset files the asset files folder will be synchronized with the files from the last module built.
It is recommended that you use a single fileset that is shared across all of your modules.
{{% /panel %}}
## Controlling Output Directory ##

The location where the asset files get copied, varies by platform. The platform defaults are as follows:

 - PC, Unix, Kettle: ${package.configbindir}\[{group}]\assets
 - Capilano: ...\Loose
 - iPhone: ${package.configbindir}\[${group}]\${module_output_name}.app
 - OSX: ${package.configbindir}\[${group}]\${module_output_name}.app\Contents\Resources
 - Android: asset files are added to the APK file, which is just a zip file. Inside the APK file the asset files are added in a folder named &quot;assets&quot;.

The default asset output directory can be changed using the property{{< url groupname >}} `.asset-configbuilddir` .
The property may be set either as a full path or as a relative path. If a relative path is used the output path will be as follows for each platform:

 - PC, Unix, Kettle: ${package.configbindir}\[${group}]\${${group}.${module}.asset-configbuilddir}
 - Capilano: ...\Loose\${${group}.${module}.asset-configbuilddir}
 - iPhone: ${package.configbindir}\[${group}]\${module}.app\${${group}.${module}.asset-configbuilddir}
 - OSX (plain nant build): ${package.configbindir}\[${group}]\${${group}.${module}.asset-configbuilddir}
 - OSX (build using XcodeProjectizer): ${package.configbindir}\[${group}]\${module}.app\Contents\Resources\${${group}.${module}.asset-configbuilddir}
 - Android: This property currently has no influence on android builds.

## Example ##

Below is an example of how to use asset files in a package with multiple modules.
If only a single module defines asset files then you can include the asset files directly within that module&#39;s assetfiles tags.


```xml

<project default="build" xmlns="schemas/ea/framework3.xsd">

  <package name="HelloWorldLibrary"/>

  <!-- When more than one module in a package must contain asset files
   they need to be added to a single shared fileset, like this. -->
  <fileset name="my-asset-files" basedir="${package.dir}/data">
    <includes name="*.ini"/>
    <includes name="Assets/Fonts/*.otf"/>
    <includes name="Assets/Fonts/*.ttf"/>
    <includes name="Assets/Images/*.jpg"/>
    <includes name="Assets/Images/*.png"/>
    <includes name="Assets/Video/*.mp4"/>
    <includes name="Assets/Video/avc1/*.mp4" if="${config-system} == 'winrt'"/>
    <includes name="Assets/Video/*.wmv" if="${config-system} == 'winprt'"/>
  </fileset>

  <Program name="Hello">
    <includedirs>
      include
    </includedirs>
    <sourcefiles>
      <includes name="source/hello/*.cpp" />
    </sourcefiles>
    <buildsteps>
      <packaging deployassets="true">
        <assetfiles fromfileset="my-asset-files"/>
      </packaging>
    </buildsteps>
  </Program>

  <Program name="SecondHello">
    <sourcefiles>
      <includes name="source/secondhello/*.cpp" />
    </sourcefiles>
    <buildsteps>
      <packaging deployassets="true">
        <assetfiles fromfileset="my-asset-files"/>
      </packaging>
    </buildsteps>
  </Program>
</project>
          
```

##### Related Topics: #####
-  [assetdependencies]( {{< ref "reference/packages/build-scripts/native-c/c++-modules/properties/assetdependencies.md" >}} ) 
