---
title: < script > Task
weight: 1120
---
## Syntax
```xml
<script language="" mainclass="" compile="" failonerror="" verbose="" if="" unless="">
  <references defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" />
  <code />
  <imports />
</script>
```
## Summary ##
Executes the code contained within the task.

## Remarks ##
The  `script`  element must contain a single  `code` element, which in turn
contains the script code.

A static entry point named  `ScriptMain` is required. It must have a single
Nant.Core.Project parameter.

The following namespaces are loaded by default:


 - System
 - System.Collections
 - System.Collections.Specialized
 - System.IO
 - System.Text
 - System.Text.RegularExpressions
 - NAnt.Core
The**&lt;imports&gt;**element can be used to import more namespaces; see example below.
You may need to add `<references>` to load assemblies containing those namespaces.

### Example ###
Run a C# script that sets a nant property that can be used by the rest of the build. Show examples of using
the mainclass attribute and import sub-element.


```xml
<project>
    <script mainclass="Scriptlet">
        <code>
            // If you specify the class name using the mainclass attribute, you can
            // use it like a normal class, with data members, constructors, and so on.
            Project _project;
			
            public Scriptlet(Project project)
            {
                _project = project;
            }
			
            void Run()
            {
                _project.Properties.Add("foo", "bar");
                // Example of using System.Data.DataTable
                DataTable dt = new DataTable("Table1");
                Log("dt.TableName=" + dt.TableName);
                // Since <, '&', and '>' are reserved characters in XML,
                // it'll be an error to use them without the containing CDATA element.
                if (1 < 2 && 1 > 0)
                   Log("1 < 2 && 1 > 0");
            }
			
            void Log(string msg)
            {
                NAnt.Core.Logging.Log.WriteLine(msg);
            }
            
            public static void ScriptMain(Project project) {
                Scriptlet script = new Scriptlet(project);
                script.Run();
            }
        </code>
        <references basedir="C:\WINDOWS\Microsoft.NET\Framework\v1.1.4322">
            <!-- extra assemblies to use -->
            <includes name="System.Data.dll"/>
        </references>
        <imports>
            <!-- Extra namespaces to use -->
            <import name="System.Data"/>
        </imports>
    </script>
    <fail message="Property foo not set correctly." unless="${foo} == bar"/>
</project>
```


### Example ###
Shows how to manipulate properties inside a script.


```xml
<project>
    <!-- script that adds a property (normally use the <property/> task)  -->
    <script>
        <code>
            public static void ScriptMain(Project project) {
                project.Properties["filename"] = "test.txt";
            }
        </code>
    </script>

    <!-- use the property added in the script inside some tasks -->
    <echo message="creating ${filename}"/>
    <touch file="${filename}"/>

    <!-- script that gets a property value to do something with it -->
    <script>
        <code>
            public static void ScriptMain(Project project) {
                string fileName = project.GetFullPath(project.GetPropertyValue("filename"));
                if (!File.Exists(fileName)) {
                    string msg = String.Format("File '{0}' should exist.", fileName);
                    throw new BuildException(msg);
                }
                File.Delete(fileName);
            }
        </code>
    </script>
</project>
```



### Attributes
| Attribute | Description | Type | Required |
| --------- | ----------- | ---- | -------- |
| language | The language of the script block (C# or VB). | String | False |
| mainclass | The name of the main class containing the static  `ScriptMain`  entry point. | String | False |
| compile | The flag of compiling code into a saved assembly or not. The default value is false. Given the file<br>containing the &lt;script&gt; is named package.build, the assembly will be named package.build.dll | Boolean | False |
| failonerror | Determines if task failure stops the build, or is just reported. Default is &quot;true&quot;. | Boolean | False |
| verbose | Task reports detailed build log messages.  Default is &quot;false&quot;. | Boolean | False |
| if | If true then the task will be executed; otherwise skipped. Default is &quot;true&quot;. | Boolean | False |
| unless | Opposite of if.  If false then the task will be executed; otherwise skipped. Default is &quot;false&quot;. | Boolean | False |

### Nested Elements
| Name | Description | Type | Required |
| ---- | ----------- | ---- | -------- |
| {{< task "NAnt.Core.FileSet" "references" >}}| Required assembly references to link with. | {{< task "NAnt.Core.FileSet" >}} | False |
| {{< task "NAnt.Core.XmlPropertyElement" "code" >}}| The script to execute. It&#39;s required that the script be put in a CDATA element. This is<br>because the potential use of XML reserved characters in the script. See example below. | {{< task "NAnt.Core.XmlPropertyElement" >}} | False |
| {{< task "NAnt.Core.PropertyElement" "imports" >}}| Namespaces to import. | {{< task "NAnt.Core.PropertyElement" >}} | False |

## Full Syntax
```xml
<script language="" mainclass="" compile="" failonerror="" verbose="" if="" unless="">
  <references defaultexcludes="" basedir="" failonmissing="" fromfileset="" sort="" if="" unless="">
    <includes />
    <excludes />
    <do />
    <group />
  </references>
  <code />
  <imports />
</script>
```
