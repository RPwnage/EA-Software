---
title: NAnt Functions
weight: 1010
---
## Summary ##
Collection of NAnt Project routines.


## Overview ##
| Function | Summary |
| -------- | ------- |
| [String NAntIsParallel()]({{< relref "#string-nantisparallel" >}}) | Tests whether Framework (NAnt) is running in parallel mode. |
| [String GetLogFilePaths()]({{< relref "#string-getlogfilepaths" >}}) | Gets log file name paths. |
## Detailed overview ##
#### String NAntIsParallel() ####
##### Summary #####
Tests whether Framework (NAnt) is running in parallel mode.

###### Parameter:  project ######


###### Return Value: ######
Returns true if Framework is in parallel mode (default).

##### Remarks #####
Parallel mode can be switched off by NAnt command line parameter -noparallel.




#### String GetLogFilePaths() ####
##### Summary #####
Gets log file name paths.

###### Parameter:  project ######


###### Return Value: ######
Returns new line separated list of log file names or an empty string when log is not redirected to a file




