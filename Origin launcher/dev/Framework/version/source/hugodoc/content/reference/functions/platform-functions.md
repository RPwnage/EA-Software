---
title: Platform Functions
weight: 1014
---
## Summary ##
Collection of string manipulation routines.


## Overview ##
| Function | Summary |
| -------- | ------- |
| [String PlatformIsUnix()]({{< relref "#string-platformisunix" >}}) | Returns &#39;true&#39; if the current platform is Linux. |
| [String PlatformIsWindows()]({{< relref "#string-platformiswindows" >}}) | Returns &#39;true&#39; if the current platform is Windows. |
| [String PlatformIsOSX()]({{< relref "#string-platformisosx" >}}) | Returns &#39;true&#39; if the current platform is OSX. |
| [String PlatformPlatform()]({{< relref "#string-platformplatform" >}}) | Returns the Operating System we are running on. |
| [String IsMonoRuntime()]({{< relref "#string-ismonoruntime" >}}) | Returns &#39;true&#39; if running under mono. |
| [String RuntimeVersion()]({{< relref "#string-runtimeversion" >}}) | Returns current .NET runtime version. |
## Detailed overview ##
#### String PlatformIsUnix() ####
##### Summary #####
Returns &#39;true&#39; if the current platform is Linux.

###### Parameter:  project ######


###### Return Value: ######
&#39;true&#39; if the current platform is Linux, otherwise &#39;false&#39;.




#### String PlatformIsWindows() ####
##### Summary #####
Returns &#39;true&#39; if the current platform is Windows.

###### Parameter:  project ######


###### Return Value: ######
&#39;true&#39; if the current platform is Windows, otherwise &#39;false&#39;.




#### String PlatformIsOSX() ####
##### Summary #####
Returns &#39;true&#39; if the current platform is OSX.

###### Parameter:  project ######


###### Return Value: ######
&#39;true&#39; if the current platform is OSX, otherwise &#39;false&#39;.




#### String PlatformPlatform() ####
##### Summary #####
Returns the Operating System we are running on.

###### Return Value: ######
name of the platform. Supported values are: Windows, Unix, OSX, Xbox, Unknown. 




#### String IsMonoRuntime() ####
##### Summary #####
Returns &#39;true&#39; if running under mono.

###### Parameter:  project ######


###### Return Value: ######
&#39;true&#39; if running under mono, otherwise &#39;false&#39;.




#### String RuntimeVersion() ####
##### Summary #####
Returns current .NET runtime version.

###### Parameter:  project ######


###### Return Value: ######
Current .NET runtime version.




