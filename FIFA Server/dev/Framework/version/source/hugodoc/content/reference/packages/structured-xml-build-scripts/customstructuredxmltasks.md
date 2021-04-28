---
title: How to write custom Structured XML tasks
weight: 206
---

Creating custom Structured XML tasks to change functionality or extend XML syntax

<a name="CustomStructuredTasks"></a>
## Custom Structured Tasks ##

It is possible to derive new tasks from Framework Structured XML tasks like &#39;Module&#39; to add new XML elements:

 - In a derived task define additional XML elements like in any other Framework  [Task]( {{< ref "reference/tasks/_index.md" >}} ) .
 - Override `protected virtual void SetupModule()` method to setup your data

<a name="Example"></a>
## Example ##

Create custom Structured task to support ispc compiler. Standard module task can be used to define &#39;custombuildfiles&quot; element with &#39;ispc-options&#39;,
but using custom task creates nicer XML syntax.


```c#
.          [TaskName("LibraryWithISPC")]
.          public class FrostbiteLibraryTask : ModuleTask
.          {
.              public FrostbiteCommonTask(string baseTask) : base("Library")
.              {
.              }
.
.              [FileSet("ispcfiles", Required = false)]
.              public StructuredFileSet ISPCFiles
.              {
.                 get { return _ispcfiles; }
.              }
.              private StructuredFileSet _ispciles = new StructuredFileSet();

.              protected override void SetupModule()
.              {
.                  base.SetupModule();
.
.                  GetModuleFileset("custombuildfiles").IncludeWithBaseDir(ISPCFiles, optionSetName: "ispc-options");
.              }
.          }
```
