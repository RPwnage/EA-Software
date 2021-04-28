---
title: Optimize Incremental Dll Builds in Visual Studio
weight: 265
---

Avoiding unnecessary rebuiklds of Dll projects in Visual Studio

<a name="Section1"></a>
## Optimization for incremental dll builds in Visual Studio ##

Framework can inject into generated Visual Studio project custom MSBuild task and MSBuild target
that prevents cpp dlls from relinking if their dependent modules haven’t changed their interface.
This should significantly reduce rebuild times on dll configurations.

MSBuild task MaybePreserveImportLibraryTimestamp is used to compare two outputted libraries in a timestamp insensitive maner to determine
if we can fake that an import library is actually not changed shortcurciting msbuild rebuild of dependent dlls.

This functionality is disabled by default. To enable it use property &#39;visualstudio.enable-preserve-importlibrary-timestamp&#39;, or to enable it on per-module basis
use property [group].[module].enable-preserve-importlibrary-timestamp

 - `visualstudio.enable-preserve-importlibrary-timestamp`  - Set this property to  **true** to enable incremental dll build optimization for all modules.
 - {{< url groupname >}} `.enable-preserve-importlibrary-timestamp`  - - Set this property to  **true** to enable incremental dll build optimization for given module.

To supress log output from MaybePreserveImportLibraryTimestamptask use boolean property `visualstudio.custom.msbuildtasks.suppress-logging` . Default value is  **false** .


{{% panel theme = "info" header = "Note:" %}}
Shortcutting rebuilds may affect execution of postbuild steps because MSBuild invokes post build steps only if build step was executed.
{{% /panel %}}
