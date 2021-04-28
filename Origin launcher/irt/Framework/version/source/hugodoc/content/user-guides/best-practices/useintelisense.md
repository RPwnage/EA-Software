---
title: Use Intellisense
weight: 27
---

Use intellisense to avoid input errors and write build scripts efficiently.

<a name="BPUseIntelisense"></a>
## Use Intellisense ##

 - Add namespace to build files:<br><br><br>```xml<br><br><project xmlns="schemas/ea/framework3.xsd"><br><br>```
 - Run nant target to install intellisense:<br><br><br>```xml<br><br>nant install-visualstudio-intellisense<br><br>```


{{% panel theme = "info" header = "Note:" %}}
Intellisense schema is computed dynamically by install-visualstudio-intellisense target. Custom tasks from your build scripts will be added.
{{% /panel %}}
![Intellisense Example]( intellisenseexample.png )