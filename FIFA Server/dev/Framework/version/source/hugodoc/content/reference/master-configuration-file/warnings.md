---
title: warnings
weight: 254
---

Warnings section allows to customize whether specific Framework warnings are enabled or disabled and whether they
are thrown as an error or not regardless of the global warning and warning-as-error level.

<a name="usage"></a>
## Usage ##

A **\<warnings>** section can be added under the root element of masterconfig to override
warnings for specific settings. Note that specific warning settings from command line will override the masterconfig settings. The **\<warnings>** section can contain a number of **\<warning>** elements with the following attributes: 

- **id** - required, the numeric code of the warning to be manipulated.
- **enabled** - optional, true or false. If true forces the warning to be enabled, if false forces it to be disabled.
- **as-error** - optional, true or false. If false forces the warning to not be thrown as an error. If true forces<br>the warning to be thrown as an error and also **forces it to be enabled regardless of any other setting** .

<a name="examples"></a>
## Examples ##

Here is an example of how to use warnigs section:

```xml
<warnings>
 <warning id="1001" enabled="false"/> <!-- hides this warning even if it would be normally reported, stops is from being thrown as error also -->
 <warning id="1002" as-error="false"/> <!-- stops this warning from being thrown as an error when warning-as-level would normally cause this, instead it just reported in output -->
 <warning id="1003" enabled="true"/> <!-- enables this warning even if it's above the selected warning level -->
 <warning id="1004" enabled="false" as-error="true"/> <!-- this warning will always be thrown as an error, note this means enabled="false" has no effect -->
</warnings>
```
