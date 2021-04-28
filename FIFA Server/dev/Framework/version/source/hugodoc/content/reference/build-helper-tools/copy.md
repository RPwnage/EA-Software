---
title: copy
weight: 300
---

copy files

<a name="NAntCopyToolUsage"></a>
## Usage ##

to get help run &quot;copy.exe -h&quot;


```
Usage: copy [-esvh...] src dst [src1 dst1 ...] [@responsefile]

-e  Do not return error code
-s  Silent
-v  Print version (logo)
-r  recursively copy directories
-o  overwrite readonly files (default)
-n  do not overwrite readonly files
-t  copy only newer files
-l  use hardlink if possible
-f  copy empty directories
-i  print list of copied files
-w  print warnings
-m  print summary
-y  ignore missing input (do not return error)
@[filename] Responsefile can contain list of source and destination,
            each path on a separate line.
file name portion of the source  can contain patterns '*' and '?' .
```

{{% panel theme = "info" header = "Note:" %}}
No options are allowed in response files
{{% /panel %}}
