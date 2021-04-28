---
title: deprecations
weight: 254
---

Deprecations section allows to customize whether specific Framework deprecation messages are enabled or disabled.

<a name="usage"></a>
## Usage ##

A **\<deprecations>** section can be added under the root element of masterconfig to override
deprecation messages for specific settings. Note that specific deprecation settings from command line will override the masterconfig settings. The **\<deprecations>** section can contain a number of **\<deprecation>** elements with the following attributes: 

- **id** - required, the numeric code of the deprecation to be manipulated.
- **enabled** - optional, true or false. If true forces the deprecation message to be enabled, if false forces it to be disabled.

<a name="examples"></a>
## Examples ##

Here is an example of how to use deprecations section:

```xml
<deprecations>
 <deprecation id="0019" enabled="false"/> <!-- hides deprecation message about sysinfo task is no longer needed. -->
</deprecations>
```
