---
title: config
weight: 255
---

Config element is used to specify the configuration package's name, the default configuration name, and a list of included configurations.

<a name="Usage"></a>
## Usage ##
`<config>`  element must be defined under `<project xmlns="schemas/ea/framework3.xsd">` element in the masterconfig file.

##### config #####
Attributes:

**`package`** (optional) - Name of the [configuration package]( {{< ref "reference/configurationpackage.md" >}} ) . Package names are case sensitive. If not specified the package *eaconfig* is used.
**`default`** (optional) - Name of the default configuration. When nant is invoked without explicitly specifying the [config]( {{< ref "reference/properties/configurationproperties.md#ConfigProperties" >}} ) property the default configuration is used. If the default configuration is not set, the first configuration name found in the [configuration package]( {{< ref "reference/configurationpackage.md" >}} )  is used as the default  [config]( {{< ref "reference/properties/configurationproperties.md#ConfigProperties" >}} )
**`includes`** (optional) - List of the allowed configuration names. Wild card  *  is allowed at the end of a name:  `pc-* capilano-vc-*` .<br><br>This feature is deprecated and we are phasing it out because it is confusing to most users. We recommend that you stop using this feature.<br>This list of names is used in multiconfiguration targets like solution generation targets (&#39;slnruntime&#39;).<br>This list can be overridden by  [package.configs]( {{< ref "reference/properties/configurationproperties.md#ConfigProperties" >}} )  property.
**`excludes`** (optional) - List of configuration names to exclude from the full list found in configuration package. We do not recommend using this element.<br><br>This feature is deprecated and we are phasing it out because it is confusing to most users. We recommend that you stop using this feature.<br>Wild card *****  is allowed at the end of a name:  `pc-* capilano-vc-*` .<br>{{% panel theme = "warning" header = "Warning:" %}}<br>Either  `includes`  or  `excludes`  element is allowed. When both are present Framework will use  `excludes` and ignore `includes` .<br>{{% /panel %}}
**`extra-config-dir`** (optional) - Provides a way for users to list a directory with config files and build scripts that can override default config settings.<br><br>This feature is deprecated and we are phasing it out as we move toward using config extensions.<br>For details and a migration guide see: [Configuration Packages]( {{< ref "reference/configurationpackage.md" >}} ) <br>This list of names is used in multiconfiguration targets like solution generation targets (&#39;slnruntime&#39;).<br>This list can be overridden by [package.configs]( {{< ref "reference/properties/configurationproperties.md#ConfigProperties" >}} ) property.

## Example ##

Config can specify [configuration package]( {{< ref "reference/configurationpackage.md" >}} ) name.

```xml
<project xmlns="schemas/ea/framework3.xsd">
  ... 
  <config package="eaconfig"/>
</project>
```

Additional configuration packages that override eaconfig can be provided. For details see [Configuration Packages]( {{< ref "reference/configurationpackage.md" >}} ) .

```xml
<project xmlns="schemas/ea/framework3.xsd">
  ...
  <config package="eaconfig">
    <extension>android_config</extension>
    <extension if="${config-use-FBConfig??true}">FBConfig</extension>
  </config>
</project>
```
The tag in example above tells NAnt that:

 - The configuration package is eaconfig.
 - The default configuration is pc-vc-dev-debug.
 - It's assumed that the config package has a subfolder called config (e.g. eaconfig/config) containing the configuration files.
 - Folder config has pc-vc-dev-debug.xml.

