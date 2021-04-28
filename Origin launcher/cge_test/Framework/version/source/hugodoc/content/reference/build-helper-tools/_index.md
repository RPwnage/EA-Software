---
title: Build Helper Tools
weight: 299
---

The Build Helper Tools is a collection of tools to perform number of standard actions like copying files, creating and deleting files and directories.

![package]( package.png )<a name="BuildHelperTools"></a>
## Build Helper Tools ##

Build Helper Tools in Framework can be used in place of system commands that could do similar actions.
The advantage of the Framework Tools is that they provide common cross-platform syntax.

The tools also have several features that not available in analogs provided by the system:

 - Response file with multiple files and directories can be specified to allow batch operations with single tool invocation
 - Input flags to control return codes and console output are available
 - Options to use symbolic link instead of copying files

There are  `AnyCPU`  and 32 bit versions of the tools available. Use 32 bit versions in cases when tools are expected to run under Incredibuild

NAnt properties containing tool locations are automatically set by Framework. Under mono property values prefixed with  `'mono '` 


                  Name
                 |
                  Value
                 |
--- |--- |
| nant.copy |  `${nant.location}/copy.exe`  |
| nant.copy_32 |  `${nant.location}/copy_32.exe`  |
| nant.mkdir |  `${nant.location}/mkdir.exe`  |
| nant.mkdir_32 |  `${nant.location}/mkdir_32.exe`  |
| nant.rmdir |  `${nant.location}/rmdir.exe`  |
| nant.rmdir_32 |  `${nant.location}/rmdir_32.exe`  |

 - [copy]( {{< ref "reference/build-helper-tools/copy.md" >}} )
 - [mkdir]( {{< ref "reference/build-helper-tools/mkdir.md" >}} )
 - [Rmdir]( {{< ref "reference/build-helper-tools/rmdir.md" >}} )

