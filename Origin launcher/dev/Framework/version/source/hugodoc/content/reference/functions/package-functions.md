---
title: Package Functions
weight: 1012
---
## Summary ##
Collection of NAnt Project routines.


## Overview ##
| Function | Summary |
| -------- | ------- |
| [String PackageGetMasterversion(String packageName)]({{< relref "#string-packagegetmasterversion-string-packagename" >}}) | Returns package masterversion declared in masterconfig file. This function does not perform dependent task. |
| [String PackageGetMasterDir(String packageName)]({{< relref "#string-packagegetmasterdir-string-packagename" >}}) | Returns package master release directory. |
| [String PackageGetMasterDirOrEmpty(String packageName)]({{< relref "#string-packagegetmasterdirorempty-string-packagename" >}}) | Returns package master release directory. |
| [String GetPackageRoot(String packageName)]({{< relref "#string-getpackageroot-string-packagename" >}}) |  |
## Detailed overview ##
#### String PackageGetMasterversion(String packageName) ####
##### Summary #####
Returns package masterversion declared in masterconfig file. This function does not perform dependent task.

###### Parameter:  project ######


###### Parameter:  packageName ######


###### Return Value: ######
masterversion string.

##### Remarks #####
Throws error if package is not not declared in masterconfig file.




#### String PackageGetMasterDir(String packageName) ####
##### Summary #####
Returns package master release directory.

###### Parameter:  project ######


###### Parameter:  packageName ######


###### Return Value: ######
package directory string.

##### Remarks #####
Throws error if package is not not declared in masterconfig file or failed to install.




#### String PackageGetMasterDirOrEmpty(String packageName) ####
##### Summary #####
Returns package master release directory.

###### Parameter:  project ######


###### Parameter:  packageName ######


###### Return Value: ######
package directory string.

##### Remarks #####
returns empty string if package is not not declared in masterconfig file or is not installed.




#### String GetPackageRoot(String packageName) ####
##### Summary #####





