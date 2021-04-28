---
title: Tasks
weight: 235
---

This section contains auto-generated documentation describing each task. Each
task is an xml element that may be used in *.build files or other related xml files.

## What are Tasks? ##

A task is a code module which can be executed by NAnt.
Common tasks are defined in the framework package but other packages can define there own.

Tasks appear in the build scripts as xml elements, however not all xml elements in build scripts are tasks, some are simpler elements, for example an option in an optionset.

## Defining a Task ##

Tasks can be defined in two main ways, firstly they can be defined through C# by creating a class that extends `NAnt.Core.Task` and has a Task metadata Attribute.

###### An example of a Task defined in C# ######

```C#
/// <summary>An empty task.</summary>
[TaskName("sample")]
public class SampleTask : Task {

    string _message  = null;

    /// <summary>An example attribute.</summary>
    [TaskAttribute("message", Required=false)]
    public string Message {
        get { return _message; }
        set { _message = value; }
    }
    
    /// <summary>Initializes the task where the message can be either an attribute of the task or within the body.</summary>
    protected override void InitializeTask(XmlNode taskNode) {
        if (Message != null && taskNode.InnerText.Length > 0) {
            throw new BuildException("Cannot specify a message attribute and element value, use one or the other.", Location);
        }

        // use the element body if the message attribute was not used.
        if (Message == null) {
            if (taskNode.InnerText.Length == 0) {
                throw new BuildException("Need to specify a message attribute or have an element body.", Location);
            }
            Message = Project.ExpandProperties(taskNode.InnerText);
        }
    }
    
    /// <summary>Writes the message to the console.</summary>
    protected override void ExecuteTask() {
     Console.WriteLine(Message);
    }
}
```

Custom tasks that are written in C# can be compiled into a dll and loaded by Framework.
The way this is done is by using the [TaskDef task]( {{< ref "reference/tasks/nant-core-tasks/general-tasks/taskdef-task.md" >}} ).

```xml
       <taskdef assembly="MyCustomTasks.dll">
          <sources>
              <includes name="src/*.cs"/>
          </sources>
          <references>
            <includes name="System.Drawing.dll" asis="true"/>
          </references>
        </taskdef>
```

Secondly, tasks can be defined as a fragment of build script using the{{< task "NAnt.Core.Tasks.CreateTaskTask" >}}.
However, this is a framework 2 feature and may not be supported in the future.

## How Tasks Work ##

The important methods to look for in a class are `InitializeTask`  and  `ExecuteTask` .
InitializeTask gets called during loading the of build script when the task&#39;s attributes are being initialized.
ExecuteTask gets called when the script is running and the task gets executed.

