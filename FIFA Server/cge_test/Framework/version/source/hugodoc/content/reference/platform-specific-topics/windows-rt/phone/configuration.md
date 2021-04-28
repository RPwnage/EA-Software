---
title: Configuration
weight: 293
---

How to configure Manifest Templates

<a name="ConfiguringInputManifestOptions"></a>
## Configuring input Manifest Options ##

Most of the manifest input parameters are passed through an option set.
Options from the optionset are accessed in the template through `@Model.Options`  interface.

Default options are set in configuration package.

default optionset for For AppX Manifest is ** `config-options-appxmanifestoptions` ** :


```xml
.         <optionset name="config-options-appxmanifestoptions">
.           <option name="manifest.filename" value="package.appxmanifest"/>
.           <option name="identity.name" value="@{StrReplace(${package.name}, '_', '')}"/>
.           <option name="identity.publisher" value="EA"/>
.           <option name="identity.version" value="1.0.0.0"/>
.           <option name="properties.displayname" value="${package.name}"/>
.           <option name="properties.publisherdisplayname" value="EA"/>
.           <option name="properties.logo" value="StoreLogo.png"/>
.           <option name="properties.description" value="${package.name}"/>
.           <option name="visualelements.logo" value="Logo.png"/>
.           <option name="visualelements.smalllogo" value="SmallLogo.png"/>
.           <option name="visualelements.splashscreen" value="SplashScreen.png"/>
.           <option name="visualelements.description" value="${package.name}"/>
.           <option name="visualelements.displayname" value="${package.name}"/>
.           <option name="capabilities">
.             internetClient
.           </option>
.           <option name="capabilities" if="${winrt-privateNetworkClientServer-capability??false}">
.             ${option.value}
.             privateNetworkClientServer
.           </option>
.           <option name="devicecapabilities" value=""/>
.         </optionset>
```
Default optionset for Windows Phone Manifest is ** `config-options-wmappmanifestoptions` ** :


```xml
.         <optionset name="config-options-wmappmanifestoptions">
.           <option name="manifest.filename" value="WMAppManifest.xml"/>
.           <option name="app.title" value="@{StrReplace(${package.name}, '_', '')}"/>
.           <option name="app.publisher" value="CN=EA"/>
.           <option name="app.version" value="1.0.0.0"/>
.           <option name="app.iconpath" value="Assets\ApplicationIcon.png"/>
.           <option name="app.templateflip.smallimageuri" value="Assets\Tiles\FlipCycleTileSmall.png"/>
.           <option name="app.templateflip.backgroundimageuri" value="Assets\Tiles\FlipCycleTileMedium.png"/>
.           <option name="capabilities" value="ID_CAP_NETWORKING"/>
.           <option name="app.activationpolicy" value="Resume"/>
.         </optionset>
```
User can define custom optionset with manifest option. The name of this custom option set should be assigned to a property:

 - {{< url groupname >}} `.appxmanifestoptions` - for AppX Manifest
 - {{< url groupname >}} `.wmappmanifestoptions` - for Win Phone Manifest

Framework will merge custom optionset with default optionset using standard  `append` operation
before passing options to the generator.


{{% panel theme = "info" header = "Note:" %}}
Options in the custom option set will override options with the same name in default optionset.
{{% /panel %}}

{{% panel theme = "info" header = "Note:" %}}
For the full list of options used in the default template look in the template file ( [Dynamic Razor Templates]( {{< ref "reference/platform-specific-topics/windows-rt/phone/dynamictemplates.md" >}} ) )
{{% /panel %}}
Additional available options


```xml

  <option name="identity.processor-architecture" value="x86/x64/arm/neutral"/>
  <option name="identity.resourceid" value=""/>
  <option name="properties.framework" value="true/false"/>
  <option name="properties.content-package" value="true/false"/>
  <option name="application.startpage" value=""/>
  <option name="visualelements.toastcapable" value="true/false"/>
  <option name="visualelements.splashscreen-backgroundcolor" value=""/>
  <option name="visualelements.lockscreen-badgelogo" value=""/>
  <option name="visualelements.lockscreen-notification" value="badge / badgeAndTileText"/>
  <option name="visualelements.initialrotation">
    portrait
    landscape
    portraitFlipped
    landscapeFlipped
  </option>
  <option name="visualelements.viewstates">
    snapped
    filled
  </option>

```
<a name="ConfiguringCustomTemplate"></a>
## Configuring Custom Template ##

It is possible to use custom template file. To specify custom template file use on of the following properties:

 **AppX Manifest template:** 

 - {{< url groupname >}} `.appxmanifest.template.file`
 - `package.appxmanifest.template.file` - this property is checked if the previous property is not defined.

 **Win Phone Manifest template:** 

 - {{< url groupname >}} `.wmappmanifest.template.file`
 - `package.wmappmanifest.template.file` - this property is checked if the previous property is not defined.

