---
title: System Properties
weight: 217
---

System Properties are Properties that describe the NAnt system and are setup for each NAnt project so they are available in all build scripts.
These properties mostly describe the global NAnt environment or the the system environment.

<a name="NantProperties"></a>
## NAnt Properties ##

property name |is readonly |Description |
--- |--- |--- |
| nant.location | (readonly) | Location of nant executable |
| nant.drive | (readonly) | On Windows, it returns Drive letter (ex. C:) or UNC Path (ex. \\Network\ShareDirve) of currently running nant.exe.<br>On Windows Subsystem for Linux, it returns Windows drive mount (ex. /mnt/c) of currently running nant.exe.<br>On OSX or Unix, it just returns the home folder (ex. /Users/[usrid] on OSX or /home/[usrid] on Linux).<br>This property is added in Framework version 3.31.00. |
| nant.version | (readonly) | nant version string |
| nant.platform_host | (readonly) | Current host machine&#39;s platform.  Valid values are &#39;windows&#39;, &#39;osx&#39;, &#39;unix&#39;, &#39;xbox&#39; and &#39;unknown&#39;.  This property is added in Framework version 3.31.00. |
| nant.verbose | (readonly) | Verbose log mode, true/false.  |
| nant.endline | (readonly) | Convenience property  to use in scripts. ${nant.endline} = &quot;\n&quot; |
| nant.env.path.separator | (readonly) | Convenience property contains platform specific path separator to use in the PATH environment variable.<br>On Windows *${nant.env.path.separator} = &quot;;&quot;*  , on MAC/Unix  *${nant.env.path.separator} = &quot;:&quot;*  |
| nant.project.buildroot | (readonly) | Build root defined in Masterconfig file or through nant command line option   -buildroot:. |
| nant.project.temproot | (readonly) | Folder inside build root where Framework stores temporary files like compiled on the fly task assembly Dlls, etc. |
| nant.project.masterconfigfile | (readonly) | Path to [Masterconfig]( {{< ref "reference/master-configuration-file/_index.md" >}} ) file. |
| nant.project.buildfile | (readonly) | Current buildfile |
| nant.project.default | (readonly) | Default target |
| target.name |  | name of the currently executing target |
| mastertarget.name |  | name of the top level target. When script is loaded out of any target<br>execution context this property is empty. |
| cmdtargets.name |  | Space separated list of targets passed on the nant command line,<br>or default target from the top level build script if command<br>line does not contain targets.<br>empty string if neither command line nor default target specified. |
| nant.commandline.properties | (readonly) | This is an **optionset** containing all properties specified on the nant command line. |
| nant.project.packageroots | (readonly) | Package roots directories specified in Masterconfig. [See Masterconfig Reference]( {{< ref "reference/master-configuration-file/_index.md" >}} ) . |
| nant.usetoplevelversion |  | If &#39;true&#39;, this property disables the error thrown when the top level package&#39;s version does not match the version listed in the masterconfig file.<br>The build should use the current version in place of the masterconfig version when calling the GetMasterVersion function. |
| nant.loglevel | (readonly) | Returns the current nant log level as a string (ie. normal, minimal, quiet, detailed, diagnostic) |
| nant.loglevelnum | (readonly) | Returns the current nant log level as an integer |
| nant.warnlevel | (readonly) | Returns the current nant warning level as a string (ie. normal, minimal, quiet, detailed, diagnostic) |
| nant.warnlevelnum | (readonly) | Returns the current nant warning level as an integer |
| nant.postbuildcopylocal |  | When set to true this will append a post-build step to each module that is involved in a copy-local dependency to ensure it is deployed to the destination directories of its parent modules. This is to complement the default behavior where the parent modules need to be built in order to have these dependencies copied. |
| nant.active-config-extensions |  | A list of all active config packages. Can be useful if you want to loop over all of the config packages to include a file from each. |

<a name="SystemProperties"></a>
## System Info Properties ##

These properties describe information about the system environment of the machine where NAnt is being run.

property name |is readonly |Description |
--- |--- |--- |
| sys.os |  | Operating system version string. |
| sys.os.version |  | Operating system version. |
| sys.os.platform |  | Operating system platform ID. |
| sys.os.folder.temp |  | The temporary directory. |
| sys.os.folder.system |  | The System directory. |
| sys.clr.version |  | Common Language Runtime (DotNet Virtual Machine) version number. |
| sys.localnetworkaddress |  | The first discovered network address of the host machine. Returns an empty string if we couldn't get network information. |
| sys.localnetworkaddresses |  | The network addresses of the host machine. Returns an empty string if we couldn't get network information. |
| sys.env.* |  | Environment variables, stored both in upper case or their original case.(e.g., sys.env.Path or sys.env.PATH).<br>Does not include variables with special characters that are not valid property names.<br>It is recommend to use the `@{GetEnvironmentVariable('variableName')}` function in build script instead<br>since this function is case insensitive and can handle special characters in environment variable names. |

<a name="NantToolPathProperties"></a>
## NAnt Tool Properties ##

These properties provide the full paths to some [Build Helper Tools]( {{< ref "reference/build-helper-tools/_index.md" >}} ) that are shipped with NAnt.


{{% panel theme = "info" header = "Note:" %}}
When running under mono the path stored in these properties is prepended with &#39;mono&#39; like this: *mono &quot;C:\packages\Framework\3.16.00\bin\copy_32.exe&quot;*
{{% /panel %}}
property name |is readonly |Description |
--- |--- |--- |
| nant.copy | (readonly) | Path to the [nant copy tool]( {{< ref "reference/build-helper-tools/copy.md" >}} ) . |
| nant.copy_32 | (readonly) | Path to the 32 bit version of copy tool [nant copy tool]( {{< ref "reference/build-helper-tools/copy.md" >}} ) . |
| nant.mkdir | (readonly) | Path to the [nant mkdir tool]( {{< ref "reference/build-helper-tools/mkdir.md" >}} ) . |
| nant.mkdir_32 | (readonly) | Path to the 32 bit version of [nant mkdir tool]( {{< ref "reference/build-helper-tools/mkdir.md" >}} ) . |
| nant.rmdir | (readonly) | Path to the [nant rmdir tool]( {{< ref "reference/build-helper-tools/rmdir.md" >}} ) . |
| nant.rmdir_32 | (readonly) | Path to the 32 bit version of [nant rmdir tool]( {{< ref "reference/build-helper-tools/rmdir.md" >}} ) . |

<a name="UnCommonProperties"></a>
## Rarely Used NAnt Properties ##

These properties are setup by NAnt but are not commonly used.

property name |is readonly |Description |
--- |--- |--- |
| nant.project.name | (readonly) | Name of the Package |
| nant.tasks.[task name] |  | For each loaded into NAnt C# task property is created |
| nant.onsuccess |  | Name of the target that will be executed after successful NAnt run. Default is Null (Property not defined). |
| nant.onfailure |  | Name of the target that will be executed after failed NAnt run. Default is Null (Property not defined). |
| nant.continueonfail |  | Continue execution after failure |
| package.all |  | List of descriptive names of all the packages this Build File depends on. |
| nant.project.basedir | (readonly) | Current project base directory (location of the build script == Package directory for currently processed Package) |

