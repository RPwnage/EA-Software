---
title: resourcefiles
weight: 173
---

This topic describes how to add embedded and non embedded resource files for a DotNet module

## Usage ##

 - Define a{{< url fileset >}} with name {{< url groupname >}} `.resourcefiles` to set embedded resources.
 - Define a{{< url fileset >}} with name {{< url groupname >}} `.resourcefiles.notembed` to set non embedded resources.


{{% panel theme = "info" header = "Note:" %}}
If `resourcefiles` fileset is not provided explicitly Framework will add default values
using following patterns:

 - `"${package.dir}/${groupsourcedir}/**/*.ico"`

Where ${groupsourcedir} equals to

 - `${eaconfig.${module.buildgroup}.sourcedir"}/${module.name};` - for packages with modules
 - `${eaconfig.${module.buildgroup}.sourcedir"}` -for packages without modules.
{{% /panel %}}
Additional settings can affect resourcefiles build

 - {{< url groupname >}} `.resourcefiles.basedir` - this property will be used as a base directory for resource files.<br>It does not affect expansion of the &lt;includes/&gt; elements. Base directory is also used to set &lt;Link&gt; path for resource files.<br><br><br>{{% panel theme = "info" header = "Note:" %}}<br>{{< task "NAnt.Core.Tasks.FileSetTask" >}}task in Framework 3+ allows setting of separate `basedir` attribute for each &lt;includes/&gt;, which lets to set<br>different &lt;Link&gt; paths for subsets of files in the `.resourcefiles` filesets.<br>{{% /panel %}}<br><br>{{% panel theme = "info" header = "Note:" %}}<br>THere is no need to use  `.resourcefiles.basedir` property. Base directories can be set through fileset attributes.<br>This property is DEPRECATED.<br>{{% /panel %}}
 - {{< url groupname >}} `.resourcefiles.prefix` - this property is used to make logical resource name for resx resources<br><br><br>```c#<br>// Resx args<br>foreach (FileItem fileItem in Resources.ResxFiles.FileItems)<br>{<br>  string prefix = GetFormNamespace(fileItem.FileName); // try and get it from matching form<br>  string className = GetFormClassName(fileItem.FileName);<br>  if (prefix == null)<br>  {<br>    prefix = Resources.Prefix;<br>  }<br>    string tmpResourcePath = Path.ChangeExtension(fileItem.FileName, "resources");<br>    string manifestResourceName = Path.GetFileName(tmpResourcePath);<br>  if (prefix != "")<br>  {<br>    string actualFileName = Path.GetFileNameWithoutExtension(fileItem.FileName);<br>    if (className != null)<br>        manifestResourceName = manifestResourceName.Replace(actualFileName, prefix + "." + className);<br>    else<br>        manifestResourceName = manifestResourceName.Replace(actualFileName, prefix + "." + actualFileName);<br>  }<br>  string resourceoption = tmpResourcePath + "," + manifestResourceName;<br>  WriteOption(writer, "resource", resourceoption);<br>}<br>```

 - Use &#39;optionset&#39; attribute in the fileset &lt;includes&gt; element to assign different &quot;Copy To Output Directory&quot; actions to subsets of content files.<br><br>Valid action values are<br><br>  - **copy-always**<br>  - **copy-if-newer**<br>  - **do-not-copy**<br><br><br>{{% panel theme = "info" header = "Note:" %}}<br>Files with extension  ** `.settings` **  or  ** `.config` **  are assigned build action  ** `None` **  when these files are defined using  `resourcefiles` fileset.<br>When these files are defined using [contentfiles]( {{< ref "reference/packages/build-scripts/dotnet-c-and-f-modules/filesets/contentfiles.md" >}} ) fileset build action will be `Content`<br>{{% /panel %}}

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
.          <fileset name="runtime.MyModule.resourcefiles" basedir="${package.dir}/Resources">
.              <includes name="**.resx"/>
.          </fileset>
```
