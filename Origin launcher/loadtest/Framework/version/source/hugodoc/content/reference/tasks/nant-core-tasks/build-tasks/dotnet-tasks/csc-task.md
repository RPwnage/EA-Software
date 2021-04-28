---
title: < csc > Task
weight: 1167
---
## Syntax
```xml
<csc doc="" LanguageVersion="" output="" target="" debug="" targetframeworkfamily="" define="" win32icon="" win32manifest="" compiler="" resgen="" referencedirs="">
  <args />
  <references defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" />
  <resources prefix="" />
  <modules defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" />
  <sources defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" />
</csc>
```
## Summary ##
Compiles C# programs using csc, Microsoft&#39;s C# compiler.Command line switches are passed using this syntax:**&lt;arg value=&quot;&quot;/&gt;**.
See examples below to see how it can be used outside OR inside the**&lt;args&gt;**tags.



### Example ###
An example of building the proverbial &quot;Hello World&quot; program.


```xml

<project xmlns="schemas/ea/framework3.xsd">
    <csc target="exe" output="hello.exe" debug="true">
        <sources basedir="source">
            <includes name="hello.cs"/>
        </sources>
    </csc>
    <exec program="hello"/>
</project>

```
Where  `source/hello.cs`  contains:


```xml

using System;

class HelloWorld {
    static void Main() {
        Console.WriteLine("Hello, World!");
    }
}

```


### Example ###
An example of passing arguments to csc.


```xml

<project xmlns="schemas/ea/framework3.xsd">
    <csc target="exe" output="helloArgs.exe" debug="true">
        <args>
          <arg value="/nologo"/>
          <arg value="/recurse:hello.cs"/>
        </args>
    </csc>
    <exec program="helloArgs"/>
</project>

```
Where  `source/hello.cs`  contains:


```xml

using System;
class HelloWorld {
    static void Main() {
        Console.WriteLine("Hello, World!");
    }
}

```


### Example ###
An example of building an application which includes embedded resources.


```xml

<project xmlns="schemas/ea/framework3.xsd">
    <fail message="Resource file not in a sub directory." unless="@{FileExists('resources/resource.txt')}"/>
    <csc target="exe" output="restest.exe" debug="true">
        <sources>
            <includes name="restest.cs"/>
        </sources>
        <resources basedir="resources">
            <includes name="resource.txt"/>
        </resources>
    </csc>
    <exec program="restest.exe"/>
</project>

```
Where  `restest.cs`  contains:


```xml

using System;
using System.IO;
using System.Reflection;

class ResourceTest {
    static void Main() {
        Stream stream = Assembly.GetExecutingAssembly().GetManifestResourceStream("resource.txt");
        StreamReader reader = new StreamReader(stream);
        Console.WriteLine(reader.ReadToEnd());
        reader.Close();
        stream.Close();
    }
}

```
And  `resources/resource.txt`  contains:


```xml
Hello, World!
```


### Example ###
An example of building a Windows Forms application using the  `prefix`  attribute of the  `resources`  file set to prefix .resx files.


```xml

<project xmlns="schemas/ea/framework3.xsd">
    <csc target="winexe" output="About.exe" debug="false">
        <sources>
            <includes name="AboutForm.cs"/>
        </sources>
        <resources prefix="About">
            <includes name="AboutForm.resx"/>
            <includes name="App.ico"/>
        </resources>
        <references>
            <includes name="System.dll"                 asis="true"/>
            <includes name="System.Data.dll"            asis="true"/>
            <includes name="System.Xml.dll"             asis="true"/>
            <includes name="System.Windows.Forms.dll"   asis="true"/>
            <includes name="System.Drawing.dll"         asis="true"/>
        </references>
    </csc>
</project>

```



### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| doc | The name of the XML documentation file to generate.  This attribute<br>corresponds to the `/doc:` flag.  By default, documentation is not<br>generated. | String | False |
| LanguageVersion | The name of the XML documentation file to generate.  This attribute<br>corresponds to the `/doc:` flag.  By default, documentation is not<br>generated. | String | False |
| output | Output file name. | String | True |
| target | Output type ( `library`  for dll&#39;s,  `winexe`  for GUI apps,  `exe`  for console apps, or  `module`  for, well, modules). | String | True |
| debug | Generate debug output ( `true` / `false` ). Default is false. | Boolean | False |
| targetframeworkfamily | Target the build as &quot;Framework&quot; (.Net Framework), &quot;Standard&quot; (.Net Standard), or &quot;Core&quot; (.Net Core).<br>Default is &quot;Framework&quot; | String | False |
| define | Define conditional compilation symbol(s). Corresponds to  `/d[efine]:`  flag. | String | False |
| win32icon | Icon to associate with the application. Corresponds to  `/win32icon:`  flag. | String | False |
| win32manifest | Manifest file to associate with the application. Corresponds to  `/win32manifest:`  flag. | String | False |
| compiler | The full path to the C# compiler.<br>Default is to use the compiler located in the current Microsoft .Net runtime directory. | String | False |
| resgen | The full path to the resgen .Net SDK compiler. This attribute is required<br>only when compiling resources. | String | False |
| referencedirs | Path to system reference assemblies.  (Corresponds to the -lib argument in csc.exe)<br>Although the actual -lib argument list multiple directories using comma separated,<br>you can list them using &#39;,&#39; or &#39;;&#39; or &#39;\n&#39;.  Note that space is a valid path<br>character and therefore won&#39;t be used as separator characters. | String | False |
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | Boolean | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

### Nested Elements
| Name | Description | Type | Required |
| ---- | ----------- | ---- | -------- |
| {{< task "NAnt.Core.ArgumentSet" "args" >}}| A set of command line arguments. | {{< task "NAnt.Core.ArgumentSet" >}} | False |
| {{< task "NAnt.Core.FileSet" "references" >}}| Reference metadata from the specified assembly files. | {{< task "NAnt.Core.FileSet" >}} | False |
| {{< task "NAnt.DotNetTasks.ResourceFileSet" "resources" >}}| Set resources to embed. | {{< task "NAnt.DotNetTasks.ResourceFileSet" >}} | False |
| {{< task "NAnt.Core.FileSet" "modules" >}}| Link the specified modules into this assembly. | {{< task "NAnt.Core.FileSet" >}} | False |
| {{< task "NAnt.Core.FileSet" "sources" >}}| The set of source files for compilation. | {{< task "NAnt.Core.FileSet" >}} | False |

## Full Syntax
```xml
<csc doc="" LanguageVersion="" output="" target="" debug="" targetframeworkfamily="" define="" win32icon="" win32manifest="" compiler="" resgen="" referencedirs="" failonerror="" verbose="" if="" unless="">
  <args if="" unless="">
    <arg />
  </args>
  <references defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" if="" unless="">
    <includes />
    <excludes />
    <do />
    <group />
  </references>
  <resources prefix="" defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" if="" unless="">
    <includes />
    <excludes />
    <do />
    <group />
  </resources>
  <modules defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" if="" unless="">
    <includes />
    <excludes />
    <do />
    <group />
  </modules>
  <sources defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" if="" unless="">
    <includes />
    <excludes />
    <do />
    <group />
  </sources>
</csc>
```
