---
title: Configuration Package Properties
weight: 220
---

This page describes properties related to configs and common properties setup when the config package is loaded.

<a name="ConfigProperties"></a>
## Properties that indicate the Configuration (config/package.configs) ##

The properties config and package.configs indicate to framework which configs to use.
They are either specified on the command line or in config element in the masterconfig file.

property name |Description |
--- |--- |
| config | The &quot;startup configuration&quot;,<br>used in targets that operate with single configuration, like build, clean, slnruntime-generate-single-config.<br>If not set it will be set automatically to the first config in package.configs. |
| package.configs | List of configs used by targets that operate with multiple configurations, like buildall, slnruntime and others |

Configs can be provided as global properties on the command line.
The `-configs:` argument sets the value of package.configs
and is equivalent to setting package.configs on the commandline.


```
nant buildall -D:config=pc-vc-dev-debug -D:package.configs="capilano-vc-dev-debug kettle-clang-dev-debug"
```
If configs are not provided on the command line Framework will take the value from the config element of the masterconfig file.

```xml
<config default="pc-vc-dev-debug" includes="pc-vc-* kettle-* capilano-vc-*"/>
```
The value ofpackage.configsproperty is determined as:

 1. Value provided on NAnt command line.
 2. Or,Masterconfig &#39;includes&#39; attribute.
 3. Or,all configurations found in the config package.

<a name="ConfigPackageProperties"></a>
## Properties set by Configuration package ##

property name |Description |
--- |--- |
| config-system | Target system name: pc, kettle, capilano, android, osx, etc. |
| config-processor | Target processor: x86, x64, arm, etc. |
| config-compiler | Compiler name: vc, gcc, sn, mw, etc. |
| config-platform | Combination of ${config-system}-${config-processor}-${config-compiler}: capilano-vc, pc-x86-vc, kettle-clang, etc. |
| config-name | Postfix portion of configuration name: *${config-platform}-${config-compiler}-${config-name}* Eaconfig defines: *dev-debug, dev-opt, dev-profile, dev-debug-opt,<br>dll-dev-debug, dll-dev-opt, dll-dev-profile, dll-dev-debug-opt* . |
| exe-suffix | Executable filename suffix (file name extension) for a given system. |
| lib-prefix | Library filename prefix prepended to the library name for a given system. |
| lib-suffix | Library filename suffix (file name extension) for a given system. |
| secured-exe-suffix | Same as *exe-suffix*  but for secured executable files. On PS3 is  *self* . |
| Dll | Dll configurations set this property to true. |
| package.configbindir | Directory where executables and DLL files are built.<br>Default *${package.builddir}/${config}/bin*  |
| package.configbuilddir | Location for temporary build files.<br>Default *${package.builddir}/${config}/build*  |
| package.configlibdir | Directory where library files are built.<br>Default *${package.builddir}/${config}/lib*  |

<a name="CompileLinkLibProperties"></a>
## Properties to support Compile, Link, Lib build steps ##

Following properties must be set by configuration package or SDK package:

<a name="Assembly"></a>
### Assembly ###

property name |Description |
--- |--- |
| as | assembly compiler executable (initialized by toolchain package) |
| as.forcelowercasefilepaths | If true file paths (folder and file names) will be forced to lower case<br>(useful in avoiding redundant NAnt builds between VS.net and command line<br>environments in the case of capitalized	folder names when file name case is<br>unimportant).  Default is false. |
| as.nodep | If  true dependency files will not be created. |
| as.nocompile | If true object files not be created. |
| as.objfile.extension | The file extension for object files.  Default is .obj |
| as.template.commandline | The template to use when creating the command line.<br>Default is %defines% %includedirs% %options%. |
| as.template.define | The syntax template to transform ${as.defines} into compiler flags. |
| as.template.includedir | The syntax template to transform ${as.includes} into compiler flags. |
| as.template.sourcefile | The syntax template to transform a source file into compiler source file. |
| as.template.responsefile | The template used for the assembler command line when a response file is used. Default is @&quot;%responsefile%&quot;. |
| as.template.responsefile.commandline | The template for the contents of the response file. |
| as.threadcount | The total number of threads to use when compiling. Default is equal to the number of CPUs. |
| as.useresponsefile | If true a response file, containing the entire commandline, will be passed as<br>the only argument to the compiler. Default is false. |
| as.userelativepaths | - If true the working directory of the assembler will be set to the base directory of the<br>asmsources fileset. All source and output files will then be made relative to this path.<br>Default is false. |

<a name="CPP"></a>
### C/C++ ###

property name |Description |
--- |--- |
| cc | Compiler executable (usually initialized by toolchain package) |
| cc.forcelowercasefilepaths | If true file paths (folder and file names) will be<br>forced to lower case (useful in avoiding redundant NAnt builds<br>between VS.net and command line environments in the case of capitalized<br>folder names when file name case is unimportant).  Default is false. |
| cc.nocompile | If true object files not be created. |
| cc.nodep | If true dependency files will not be created. |
| cc.objfile.extension | The file extension for object files.  Default is .obj. |
| cc.parallelcompiler | If true, compilation will occur on parallel threads if possible<br>Default is false. |
| cc.template.define | The syntax template to transform ${cc.defines} into compiler flags. |
| cc.template.commandline | The template to use when creating the command line.<br>Default is %defines% %includedirs% %options%. |
| cc.template.includedir | The syntax template to transform ${cc.includedirs} into compiler flags. |
| cc.template.sourcefile | The syntax template to transform a source file into compiler source file. |
| cc.template.responsefile | The template used for the compiler command line when a response file is used. Default is @&quot;%responsefile%&quot;. |
| cc.template.responsefile.commandline | The template for the contents of the response file. |
| cc.template.usingdir | The syntax template to transform ${cc.usingdirs} into compiler flags. |
| cc.threadcount | The total number of threads to use when compiling. Default is equal to the number of CPUs. |
| cc.userelativepaths | If true the working directory of the compiler will be set to the base directory of the<br>sources fileset. All source and output files will then be made relative to this path.<br>Default is false. |
| cc.useresponsefile | If true a response file, containing the entire commandline, will be passed as the only<br>argument to the compiler. Default is false. |
| cc.usingdirs | The syntax template to transform${cc.usingdirs}into compiler flags. |

<a name="Linker"></a>
### Linker ###

property name |Description |
--- |--- |
| link | Linker executable, usually defined by toolchain package. |
| link.forcelowercasefilepaths | If true file paths (folder and file names) will be<br>forced to lower case (useful in avoiding redundant NAnt builds<br>between VS.net and command line environments in the case of capitalized<br>folder names when file name case is unimportant).  Default is false. |
| link.postlink.commandline | The command line to use when running the post link program. |
| link.postlink.program | The path to program to run after linking.  If not defined no post link process will be run. |
| link.template.commandline | The template to use when creating the command line.<br>Default is %options% %librarydirs% %libraryfiles% %objectfiles%. |
| link.template.librarydir | The syntax template to transform the ${link.librarydirs} property into linker flags. |
| link.template.libraryfile | The syntax template to transform the libraries file set into linker flags. |
| link.template.objectfile | The syntax template to transform the objects file set into linker flags. |
| link.template.responsefile | The template used for the linker command line when a response file is used. Default is @&quot;%responsefile%&quot;. |
| link.template.responsefile.commandline | The template for the contents of the response file. |
| link.postlink.commandline | The command line to use when running the post link program. |
| link.postlink.program | The path to program to run after linking.  If not defined no post link process will be run. |
| link.postlink.workingdir | The directory the post link program will run in.  Defaults to the program&#39;s directory. |
| link.userelativepaths | If true the working directory of the linker will be set to the outputdir.<br>All object files will then be made relative to this path. Default is false. |
| link.useresponsefile | If true a response file, containing the entire commandline,<br>will be passed as the only argument to the linker. Default is false. |

<a name="Librarian"></a>
### Librarian ###

property name |Description |
--- |--- |
| lib | Librarian executable, usually set by toolchain package |
| lib.forcelowercasefilepaths | If true file paths (folder and file names) will be<br>forced to lower case (useful in avoiding redundant NAnt builds<br>between VS.net and command line environments in the case of capitalized<br>folder names when file name case is unimportant).  Default is false. |
| lib.template.objectfile | The syntax template to transform the objects file set into librarian flags. |
| lib.template.responsefile | The template used for the librarian command line when a response file is used. Default is @&quot;%responsefile%&quot;. |
| lib.template.responsefile.commandline | The template for the contents of the response file. |
| lib.userelativepaths | If true the working directory of the archiver will be set to the outputdir.<br>All object files will then be made relative to this path. Default is false. |
| lib.useresponsefile | If true a response file, containing the entire commandline, will be passed as<br>the only argument to the archiver. Default is false. |

