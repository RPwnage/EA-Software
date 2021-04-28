---
title: contentfiles
weight: 171
---

This topic describes how to set contentfiles in Visual Studio Project

## Usage ##

Define a{{< url fileset >}} with name {{< url groupname >}} `.contentfiles` in your build script.

To assign Content action use property

 - Property{{< url groupname >}} `.contentfiles.action` will set contentfiles action for all contentfiles in the project.
 - Use &#39;optionset&#39; attribute in the fileset &lt;includes&gt; element to assign different &quot;Copy To Output Directory&quot; actions to subsets of content files.
 - To save relative directory structure while copying local, include the full path in the name attribute. Framework will remove the basedir attribute value from the name attribute value to determine the relative path.<br><br><br>
 
```xml
  <fileset name="runtime.MyModule.contentfiles" >
    <includes name="Content/*.xml"  optionset="copy-if-newer" basedir="${package.dir}/Content" />
    <includes name="Content/Subfolder/*.xml"  optionset="copy-if-newer"  basedir="${package.dir}/Content/Subfolder"/>
  </fileset>
```

To save directory structure (this will copy files to the destination folder/Subfolder directory)

```xml
  <fileset name="runtime.MyModule.contentfiles" >
    <includes name="${package.dir}/Content/Subfolder/*.xml"  optionset="copy-if-newer"  basedir="${package.dir}/Content"/>
  </fileset>
```

Valid action values are

 - **copy-always**
 - **copy-if-newer**
 - **do-not-copy**

For .Net Visual Studio projects control to set  *&#39;Generator&#39;*  type for  *&#39;*.settings&#39;*  file type using optionset:


```xml

  <CSharpLibrary name="Example">
    <sourcefiles>
      <includes name="source/**.cs"/>
    </sourcefiles>
    <resourcefiles>
      <includes name="source/**.settings" optionset="settings-generator"/>
    </resourcefiles>
  </CSharpLibrary>

  <optionset name="settings-generator">
    <option name="Generator" value="PublicSettingsSingleFileGenerator"/>
  </optionset>

```
## Example ##


```xml
.          <property name="runtime.MyModule.buildtype" value="CSharpProgram" />
.
.          <fileset name="runtime.MyModule.contentfiles" >
.              <includes name="Content/**.xml"  optionset="copy-if-newer"/>
.          </fileset>
```
