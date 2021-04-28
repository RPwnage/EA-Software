---
title: Portable Solution Generation
weight: 264
---

Portable solution generation is a Framework feature that when enabled ensures that all of the paths in a solution are relative.

<a name="Usage"></a>
## Usage ##

Users often want the ability to make their Visual Studio solution files portable so they can be checked into source control and then used on other computers.

By default Framework will try to make the paths to all files relative, but in some special cases this is not possible and it switches to using absolute paths.
As an additional measure of portability you can set the property `eaconfig.generate-portable-solution=true` .
This will go to additional lengths to ensure that files are added as relative paths and will use environment varibles for SDK paths.

Name |Always defined |Needs to be Global |Default Value |
--- |--- |--- |--- |
| eaconfig.generate-portable-solution |  |  | false |

