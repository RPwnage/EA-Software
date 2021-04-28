---
title: Options
weight: 175
---

Setting compiler options for DotNet module

<a name="Section1"></a>

<a name="Defines"></a>
### Defines ###

##### groupname.defines #####
Use this property to specify additional defines.

Each define should be on a separate line

{{< url buildtype >}} option  ** `csc.defines` ** ( ** `fsc.defines` ** for F#) can also be used to set defines.
Values from option and property are merged.


{{% panel theme = "info" header = "Note:" %}}
Framework adds &#39;EA_DOTNET2&#39; define, and in debug mode it will add &#39;TRACE DEBUG&#39; defines.

To disable automatic addition of defines use property:

 **{{< url groupname >}} `.nodefaultdefines` ** = true
{{% /panel %}}
See also: [Everything you wanted to know about Framework defines, but were afraid to ask]( {{< ref "user-guides/everythingaboutdefines.md" >}} ) .

<a name="Warningsaserrors"></a>
### Warningsaserrors ###

##### groupname.warningsaserrors #####
Use this boolean property to enable warningsaserrors ( `/warnaserror` )

If property does not exist Framework will look for corresponding option in the{{< url buildtype >}}optionset
( `csc.warningsaserrors`  or  `fsc.warningsaserrors` for F#).

##### groupname.warningsaserrors.list #####
Use this property to enable specific warnings as errors: `/warnaserror:${[group].[module].warningsaserrors.list}` 

If property does not exist Framework will look for corresponding option in the{{< url buildtype >}}optionset
( `csc.warningsaserrors.list`  or  `fsc.warningsaserrors.list` for F#).


{{% panel theme = "info" header = "Note:" %}}
Specifying a list of specific warnings as errors will automatically disable the warningsaserror setting for the module
{{% /panel %}}
<a name="Warningsnotaserrors"></a>
### Warningsnotaserrors ###

##### groupname.warningsnotaserrors.list #####
Disables specific warnings as errors but they will still appear as warnings: `/warnaserror-:${[group].[module].warningsnotaserrors.list}` 


{{% panel theme = "info" header = "Note:" %}}
Warningsnotaserrors will only have an effect if warningsaserrors is turned on
{{% /panel %}}
<a name="Suppresswarnings"></a>
### Suppresswarnings ###

##### groupname.suppresswarnings #####
Use boolean property to suppress specific warnings: `/nowarn:${[group].[module].suppresswarnings}` 

If property does not exist Framework will look for corresponding option in the{{< url buildtype >}}optionset
( `csc.suppresswarnings`  or  `fsc.suppresswarnings` for F#).

<a name="Debug"></a>
### Debug ###

##### groupname.debug #####
Use this property to set &#39;Debug Info Level&#39;: **none** ,  **full** ,  **pdb-only** . `/debug:${[group].[module].debug}` 

If property does not exist Framework will look for corresponding option in the{{< url buildtype >}}optionset
( `csc.debug`  or  `fsc.debug` for F#).


{{% panel theme = "info" header = "Note:" %}}
`[group].[module].debug`  property only used when buildtype optionset has option  `debugflags='on'`
{{% /panel %}}
<a name="Keyfile"></a>
### Keyfile ###

##### groupname.keyfile #####
Use this property to set keyfile location: `/keyfile:${[group].[module].keyfile}` 

If property does not exist Framework will look for corresponding option in the{{< url buildtype >}}optionset
( `csc.keyfile`  or  `fsc.keyfile` for F#).

<a name="Platform"></a>
### Platform ###

##### groupname.platform #####
Use this property to specify target platform: `/platform:${[group].[module].platform}` 

When `platform` property is not specified Framework will use value ** `anycpu` ** for assemblies, unless property

 ** `eaconfig.use-exact-dotnet-platform` or**  **{{< url groupname >}} `.use-exact-dotnet-platform` ** 

is set to true or Module is an executable.

 When module is executable or  `'use-exact-dotnet-platform=true'` Framework will use `${config-processor}`  property:

 - **x86**  ==&gt;  `/platform:x86`
 - **x64**  ==&gt;  `/platform:x64`
 - **arm**  ==&gt;  `/platform:ARM`

When `${config-processor}` property is not specified Framework
will use `${config-system}` property:

 - **pc**  ==&gt;  `/platform:x86`
 - **pc64**  ==&gt;  `/platform:x64`
 - **any other case**  ==&gt;  `/platform:anycpu`


{{% panel theme = "info" header = "Note:" %}}
If property [group].[module].platform does not exist Framework will look for corresponding option
in the{{< url buildtype >}} optionset ( `csc.platform`  or  `fsc.platform` for F#).
{{% /panel %}}
<a name="Allowunsafe"></a>
### Allowunsafe ###

##### groupname.allowunsafe #####
Use this boolean property to set allowunsafe option: `/unsafe` 

If property does not exist Framework will look for corresponding option in the{{< url buildtype >}}optionset
( `csc.allowunsafe`  or  `fsc.allowunsafe` for F#).

<a name="AdditionalArgs"></a>
### Additional Args ###

##### groupname.csc-args  (.fsc-args for F#) #####
Use this property to pass additional options to compiler.

