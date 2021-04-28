---
title: Create Custom BuildType
weight: 86
---

Sections in this topic describe how to create and modify custom buildtypes.

<a name="CreateCustomBuildtype"></a>
## Creating a Custom Buildtype ##

There are cases when an extra compiler flag needs to be set, and there is no helper eaconfig property to do so.
To create a custom build type in the package build file do it the same way eaconfig does it internally.

It is also possible to remove or substitute existing flags using NAnt StrReplace() function.
However, many options are generated in the GenerateBuildOptionset task, and thus can not be changed in the custom option set definition.
What to do in this case is described in the next section.

In Structured XML custom build options can be set directly in the module definition, see [How to set module 'buildtype' in Structured XML]( {{< ref "reference/packages/structured-xml-build-scripts/setbuildtype.md" >}} ) and [How to define custom build options in Structured XML]( {{< ref "reference/packages/structured-xml-build-scripts/customoptions.md" >}} ) 

<a name="BuildTypeTaskSXML"></a>
### BuildType Task ###

Defining custom buildtypes is cumbersome and verbose. Structured Xml layer introduces{{< task "EA.Eaconfig.Structured.BuildTypeTask" >}}task that makes custom buildtype definition simpler.
It can be used in traditional, non structured Framework build scripts.

Traditional custom buildtype definition above can be done using `BuildType` task:


```xml

       <BuildType name ="Program_ForceInclude"  from="Program">
         <option name="exceptions" value="on">
         <option name="buildset.cc.options">
           ${option.value}
           /FI ${package.dir}\source\TestIncludes\Func2.cpp
         </option>
       </BuildType>

```
There are number of options that affect generation of compiler or linker switches. These options can be changed in the custom build type.
For example, to switch off link time code generation you can change the option optimization.ltcg to off:


```xml

       <BuildType name ="Program_LTCG_off"  from="Program">
         <option name="optimization.ltcg" value="off" />
       </BuildType>

```

{{% panel theme = "info" header = "Note:" %}}
`BuildType`  task uses buildtype name for the base optionset, while  `GenerateBuildOptionset` task uses
proto optionset name.
{{% /panel %}}
### How to define custom build type using traditional Framework syntax. ###

In older build scripts you may see custom build types defined in this way.
Please do not use this style in new build scripts because the BuildType task provides all of the same features but is much simpler to read and write.

In your package build file define new option set derived from one of eaconfig standard sets, and add additional options.


```xml
<optionset name="config-options-program-fi" fromoptionset="config-options-program">
  <option name="buildset.name"          value="Program_ForceInclude"/>
  <option name="buildset.cc.options">
    ${option.value}
    /FI ${package.dir}\source\TestIncludes\Func2.cpp
  </option>
</optionset>
<task name="GenerateBuildOptionset" configsetname="config-options-program-fi"/>

<property name="runtime.buildtype" value="Program_ForceInclude"/>
```

{{% panel theme = "info" header = "Note:" %}}
To make sure that we do not loose options already defined in the option set use **${option.value}** which will add existing value for this option.
{{% /panel %}}
There are number of options that affect generation of compiler or linker switches. These options can be changed in the custom build type.
For example, to switch off link time code generation you can change the option optimization.ltcg to off:


```xml
<optionset name="config-options-program-LTCG-off" fromoptionset="config-options-program">
  <option name="buildset.name" value="Program_LTCG_off"/>
  <option name="optimization.ltcg"       value="off" />
</optionset>
<task name="GenerateBuildOptionset" configsetname="config-options-program-LTCG-off"/>
```
<a name="BuildTypeTask"></a>
## BuildType Task ##

Defining custom buildtypes is cumbersome and verbose. Structured Xml layer introduces{{< task "EA.Eaconfig.Structured.BuildTypeTask" >}}task that makes custom buildtype definition simpler.
It can be used in traditional, non structured Framework build scripts.

Traditional custom buildtype definition above can be done using  `BuildType` task:


```xml
<BuildType name ="Program_ForceInclude"  from="Program">
  <option name="exceptions" value="on">
  <option name="buildset.cc.options">
    ${option.value}
    /FI ${package.dir}\source\TestIncludes\Func2.cpp
  </option>
</BuildType>
```

{{% panel theme = "info" header = "Note:" %}}
`BuildType`  task uses buildtype name for the base optionset, while  `GenerateBuildOptionset` task uses
proto optionset name.
{{% /panel %}}
<a name="RemovingChangingOptions"></a>
## Removing or changing options in custom build type ##

Many of the standard options are added to the optionset during execution of the task GenerateBuildOptionset.
To change or remove such options, the operation should be performed **after** GenerateBuildOptionset task is completed.
Here are some examples of removing standard &#39;--fno-rtti&quot; option in GCC configuration in a custom build type:

This first example uses structured xml and is the recommended way to remove options from a custom build type.
For more information about the structured xml method of removing options see: [SXML Change Existing Build Options]( {{< ref "reference/packages/structured-xml-build-scripts/customoptions.md#SXMLChangeExistingBuildOptions" >}} ) .


```xml

<BuildType name="LibraryExtra" from="Library">
  <remove>
    <cc.options if="${config-compiler} == gcc">
      -fno-rtti
    </cc.options>
  </remove>
</BuildType>
     
```
Another older, less standard way of removing options by modifying the option set directly.


```xml

                 <!-- Create extra build types which simply removes some compiler arguments -->
                 <BuildType name="LibraryExtra" from="Library"/>

                 <!-- IMPORTANT: Need to do this StrReplace AFTER the above BuildType task execution. -->
                 <optionset name="LibraryExtra">
                   <option name="cc.options" if="${config-compiler} == 'gcc'">
                     @{StrReplace(${option.value}, '-fno-rtti', '')}
                   </option>
                 </optionset>

                 <property name="runtime.buildtype" value="LibraryExtra" />

```

{{% panel theme = "info" header = "Note:" %}}
The name of options changes in the generated Optionset. **buildset** prefix is removed from option name,
instead of using **buildset.cc.options**  you need to use  **cc.options** .
{{% /panel %}}
Framework introduces new set of helper properties that can be used to remove options without introducing new custom buildtype in the build script:

 - {{< url groupname >}} `.remove.defines`
 - {{< url groupname >}} `.remove.cc.options`
 - {{< url groupname >}} `.remove.as.options`
 - {{< url groupname >}} `.remove.link.option`
 - {{< url groupname >}} `.remove.lib.options`

Framework will search each define or option given in the above properties and remove it from the list of defines or options in the corresponding tool
(compiler, linker, or librarian).


{{% panel theme = "info" header = "Note:" %}}
Each define or option must be on a separate line
{{% /panel %}}
<a name="BuildTypeAvailableFlags"></a>
## Available flags and options ##

Compiler, linker, librarian options can be added using following option names:

name | |
--- |--- |
| buildset.cc.options | C/C++ compiler options |
| buildset.as.options | Assembly compiler options |
| buildset.link.options | Linker options |
| buildset.lib.options | librarian options |
| buildset.csc.options | C# compiler options |

Following flags can be set to on/off states

name |initial value |Description |
--- |--- |--- |
| debugflags | off | This option is set to &#39;on&#39; for debug and debug-opt configs. |
| debugsymbols | on |  - `on` - Enables emission of debug symbols.  Examples build options includes:<br><br>  - For MSVC compiler option: -Zo (if optimization is also on), -Zi (if editandcontinue is off) or -ZI (if editandcontinue is on), -Fd&quot;[pdbfile]&quot;<br><br>For MSVC linker option: -debug (or -debug:FASTLINK if debugfastlink is on), -PDB:[pdbfile], and -ASSEMBLYDEBUG (if managedcpp is on)<br>  - For C#: /debug:full<br>  - For GCC/Clang: -g for compiler option<br> - `off` - None of the above build options are not set with the exception of:<br><br>  - For C#: /debug:pdbonly<br><br>This means aside from not emit debug symbols option to compiler, it also disable linker&#39;s debug symbol collation functionality (ie -debug to link.exe), if it exists.<br>The caveat here is that if a prebuilt library, or other component with custom debug symbol settings, has symbols they will not be incorporated into the final build.<br> - `custom`  - The build will use values from these options:  `debugsymbols.custom.cc` ,  `debugsymbols.custom.link` , and  `debugsymbols.custom.lib` with the exception that in C# build, /debug:full will also be added.<br><br> |
| debugfastlink | [not defined] | This is only used in Visual Studio 2015 and if debugsymbols is on.  If turned on, linker option -debug:FASTLINK will be used, otherwise, linker option will just be -debug. |
| disable_reference_optimization | off | Comdat folding is finding functions and constants with duplicate values and merging (folding) them.<br>As a side effect builds with &quot;-OPT:noicf&quot; have improved debugging in common functions because it can be<br>unsettling to step into a different function in a different module while debugging (even if that function<br>generates the same executable code) |
| print_link_timings | off | This prints timing information about each of the linker commands and can be useful for debugging and optimizing link times. |
| optimization | on | This option is set to &#39;off&#39; for debug config.  When the flag is turned on, compiler/linker will have extra command line options added to build with optimization on.  User can override the setting by assigning &#39;custom&#39; as value and then define options: &#39;optimization.custom.cc&#39;, &#39;optimization.custom.link&#39;, and &#39;optimization.custom.lib&#39;. |
| optimization.ltcg | [not defined] | This option is not defined by default and only set up user config setup.<br>Used by MSVC to set up build options for link time code generation.<br>If user setup it up and set the value to &#39;on&#39;, it will add link option -LTCG.<br>If people need specific LTCG mode, they will need to setup build option `optimization.ltcg.mode` with instrument, optimize, or update. |
| retailflags | off | This option is set to &#39;on&#39; for retail configs.  If this option is set to &#39;on&#39;, preprocessor defines EA_RETAIL=1 will be added to compiler option. |
| usedebuglibs | off | This option is set to &#39;on&#39; for debug and debug-opt configs.  Use debug system/SDK libraries if &quot;standardsdklibs&quot; option is &quot;on&quot;, otherwise, uses regular libs.  User can set the value as &#39;custom&#39;, however, it will only accept value in option &#39;usedebuglibs.custom.cc&#39; for compiler arguments.  It will do nothing for link options.  If user need to define custom set of libraries, consult the &#39;standardsdklibs&#39; option. |
| warnings | on |  - `on` - enables all warnings and suppresses some default subset.<br> - `off` - does not set any warnings related compiler options.<br> - `custom`  - will use values from the current optionset:  `warnings.custom.cc`  and  `warnings.custom.link`<br><br> |
| warninglevel | high | Acceptable values can be &#39;high&#39;, &#39;medium&#39;, or &#39;pedantic&#39;. |
| misc | on |  - `on` - Setup some miscellaneous compiler options. Slightly different for different compiler.<br><br>  - `VC Compiler` - Set compile Only switch (-c) and enforce standard C++ for scoping (-Zc:forScope)<br>  - `Unix Clang/GCC Compiler` - Set compile Only switch (-c), allocate even uninitialized global in data section (-fno-common), and put output entirely on one line (-fmessage-length=0)<br> - `off` - Don&#39;t add these compiler options.<br> - `custom`  - Use only values defined in option  `misc.custom.cc` .<br><br> |
| warningsaserrors | on | Turns on or off the appropriate compiler switch for compiling with warnings as errors. |
| rtti | off | Turns on or off the appropriate compiler switch for compiling with run time type information. |
| exceptions | off | Turns on or off the appropriate compiler/linker switch for building with exception handling. |
| toolconfig | off | This flag is turned on under all eaconfig&#39;s tool configs.  It&#39;s main impact is that different compiler<br>option is used when building with exception.  When this flag is on, it uses /EHa, otherwise, /EHsc will be used. |
| incrementallinking | off | Turns on or off adding the appropriate linker switch for incremental linking. |
| generatemapfile | on | Turns on or off adding the appropriate linker switch for generating map file. |
| generatedll | off | Turns on or off adding the appropriate linker switch for doing dll build. |
| multithreadeddynamiclib | off | A Visual Studio compiler settings.  This flag is automatically turned on if the following conditions are true: |
| managedcpp | off | Setting for Microsoft Visual C++ Compiler. Sets the build options for building for Managed C++ code. |
| banner | off | Setting for Microsoft Visual C++ Compiler.  Default is off (adding -nologo compiler/linker/librarian option) to help suppress the start up copyright message of the compiler/linker/librarian tool. |
| editandcontinue | off | Setting for Microsoft Visual C++ Compiler.  Setup &quot;Debug Information Format&quot; /ZI compiler switch for &quot;Program Database for Edit and Continue&quot; |
| c7debugsymbols | off | Setting for Microsoft Visual C++ Compiler.  Setup &quot;Debug Information Format&quot; /Z7 compiler switch for C7-style embedded debug information. |
| standardsdklibs | on |  - `on`  - Setup to build with the standard set of SDK libraries defined by property  `platform.sdklibs.regular`  or  `platform.sdklibs.debug` ,<br> - `off` - Don&#39;t add any standard SDK libs<br> - `custom`  - Use only values defined in option  `standardsdklibs.custom.link` .<br><br> |
| runtimeerrorchecking | off | Setting for Microsoft Visual C++ Compiler.  Setup Runtime Error Checking compiler options. |
| pcconsole | on |  - `on` - Setup pc link option for console subsystem build (-subsystem:console)<br> - `off` - Setup pc link option for building windows application (-subsystem:windows)<br><br> |
| profile | off | Option set to &#39;on&#39; for profile configs.  In most cases this option has no effect. Users can detect the<br>presence of this option for adding profiling functionality to their builds should they wish. |
| clanguage | off | If turned on, will setup compiler / linker options to build the code a C code. |
| uselibrarydependencyinputs | off | A setting for Visual Studio solution generation to set the &quot;Use Library Dependency Inputs&quot; setting |
| iteratorboundschecking | off | Enable iterator bounds checking for Visual C++ builds |
| debugflags.custom.cc |  | See &#39;debugflags&#39; for more info. |
| debugsymbols.custom.cc |  | See &#39;debugsymbols&#39; for more info. |
| debugsymbols.custom.link |  | See &#39;debugsymbols&#39; for more info. |
| misc.custom.cc |  | See &#39;misc&#39; for more info. |
| usedebuglibs.custom.cc |  | See &#39;usedebuglibs&#39; for more info. |
| standardsdklibs.custom.link |  | See &#39;standardsdklibs&#39; for more info. |
| warnings.custom.cc |  | See &#39;warnings&#39; for more info. |
| warnings.custom.link |  | See &#39;warnings&#39; for more info. |
| enable.strict.aliasing | off | Explicitly enables or disables strict aliasing optimizations for compilers that support it. Disabling strict aliasing, in this case, means passing the &quot;-fno-strict-aliasing&quot; parameter, for example for GCC or Clang. |
| stripallsymbols | off | This option is turned on for opt and retail configs.  When stripallsymbols=&#39;on&#39; gcc and clang configurations add linker &quot;-s&quot; option. |
| disable_reference_optimization | off | This option is turned on for debug config.  Used in VC configurations and add linker &quot;-opt:noref&quot; option if turned on and add linker &quot;-opt:ref&quot; option if turned off. |
| cc.cpp11 | on | &#39;on&#39; - turn on C++11 support. Default is on for CPP build types, and off for C build types |
| disable-crt-secure-warnings | on | It is used by Microsoft configs.  If turned on, it adds &quot;_CRT_SECURE_NO_WARNINGS&quot; define to the compiler option. |


##### Related Topics: #####
-  [How to set module 'buildtype' in Structured XML]( {{< ref "reference/packages/structured-xml-build-scripts/setbuildtype.md" >}} ) 
-  [How to define custom build options in Structured XML]( {{< ref "reference/packages/structured-xml-build-scripts/customoptions.md" >}} ) 
