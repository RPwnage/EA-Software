---
title: Environment Functions
weight: 1006
---
## Summary ##
Collection functions for inquiring about aspects of the environment, such as looking up environment variable values.


## Overview ##
| Function | Summary |
| -------- | ------- |
| [String GetEnvironmentVariable(String variableName)]({{< relref "#string-getenvironmentvariable-string-variablename" >}}) | Returns the value of a specified Environment variable. Equivalent to using the property sys.env.(name of environment variable). |
## Detailed overview ##
#### String GetEnvironmentVariable(String variableName) ####
##### Summary #####
Returns the value of a specified Environment variable. Equivalent to using the property sys.env.(name of environment variable).

###### Parameter:  project ######


###### Parameter:  variableName ######
The name of the environment variable to return, environment variable names are case insensitive on windows platforms.

###### Return Value: ######
The value of the environment variable.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <echo message="Path: @{GetEnvironmentVariable('PATH')}" />
</project>

```





