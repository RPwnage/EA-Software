---
title: File Copy Steps
weight: 185
---

filecopystep allows you to setup generic file copy operations.

## Usage ##

Use `<filecopystep>` in SXML in your build script for a Utility
module to specify special file copy operations. You can specify multiple `<filecopystep>` in the module definition.


{{% panel theme = "info" header = "Note:" %}}
If you use fileset to specify the source files, any basedir specification will be ignored.  All files will be
copied flattened.

If the destination already have the indicated file (but with older timestamp) and have read only attribute, the
copying will copy over the read only file.

If the destination already have the indicated file with a newer timestamp, no copying will be done.
{{% /panel %}}
## Example ##


```xml
.          <Utility name="MyModule">
.
.              <filecopystep
.                  tofile="${package.configbindir}\test.dll"
.                  file="${package.SomePackage.dir}\test.dll"
.              />
.
.              <filecopystep todir="${package.configbindir}">
.                  <!--
.                      Note: Current implementation DOES NOT support basedir specification.
.                            All files are copied flattened.
.                  -->
.                  <fileset>
.                      <includes fromfileset="package.SomeOtherPackage.filesetname"/>
.                  </fileset>
.              </filecopystep>
.
.          </Utility>
```
