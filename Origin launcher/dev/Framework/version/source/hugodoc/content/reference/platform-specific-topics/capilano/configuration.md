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

default optionset for For MicrosoftGame.config Manifest is ** `config-options-gameconfigoptions` ** :

Here is an example of options that can be set via the optionset. 
See Microsoft's docs for a full description of what each option in the MicrosoftGame.config is for.

```xml
	<optionset name="config-options-gameconfigoptions">
		<option name="manifest.filename" value="MicrosoftGame.config"/>
		<option name="identity.name" value="${package.name}"/>
		<option name="identity.publisher" value="Electronic Arts Inc."/>
		<option name="identity.version" value="1.0.0.0"/>
		<option name="executable.id" value="${package.name}"/>
		<option name="title.id" value="1111"/>
		<option name="store.id" value="2222"/>
		<option name="msaappid.id" value="3333"/>
		<option name="title.gameosversion" value="11.22.33.44"/>
		<option name="title.persistentlocalstorage" value="1234"/>
		<option name="title.msafulltrust" value="true1"/>
		<option name="title.requiresxboxlive" value="true2"/>
		<option name="title.relatedproducts">
			rp1
			rp2
			rp3
		</option>
		<option name="title.allowedproduct">
			ap1
			ap2
			ap3
		</option>
		<option name="title.dbgport">
			3232
			3233
			3244
		</option>
		<option name="title.xboxonemaxmemory" value="xbmax"/>
		<option name="title.xboxonexmaxmemory" value="xbmax_x"/>
		<option name="title.gamedvrsystemcomponent" value="asdf"/>
		<option name="title.blockbroadcast" value="b1"/>
		<option name="title.blockgamedvr" value="b2"/>
		<option name="shellvisuals.storelogo" value="StoreLogo.png"/>
		<option name="shellvisuals.square150x150logo" value="Square150x150Logo.png"/>
		<option name="shellvisuals.square150x150logo" value="Square150x150Logo.png"/>
		<option name="shellvisuals.square44x44logo" value="Square44x44Logo.png"/>
		<option name="shellvisuals.description" value="${package.name}"/>
		<option name="shellvisuals.displayname" value="${package.name}"/>
		<option name="shellvisuals.foregroundtext" value="${gameconfig.shellvisuals.foregroundtext}" if="@{PropertyExists('gameconfig.shellvisuals.foregroundtext')}"/>
		<option name="shellvisuals.backgroundcolor" value="${gameconfig.shellvisuals.backgroundcolor}" if="@{PropertyExists('gameconfig.shellvisuals.backgroundcolor')}"/>
	</optionset>
```

User can define custom optionset with manifest option. The name of this custom option set should be assigned to a property:

 - {{< url groupname >}} `.gameconfigoptions` - for MicrosoftGame.config Manifest

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

 **MicrosoftGame.config template:** 

 - {{< url groupname >}} `.gameconfig.template.file`
 - `package.gameconfig.template.file` - this property is checked if the previous property is not defined.
