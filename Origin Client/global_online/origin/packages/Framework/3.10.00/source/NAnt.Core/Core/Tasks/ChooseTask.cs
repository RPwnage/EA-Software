// NAnt - A .NET build tool
// Copyright (C) 2001-2002 Gerry Shaw
//
// Kosta Arvanitis (karvanitis@ea.com)

using System;
using System.Collections.Generic;
using System.Xml;

using NAnt.Core.Attributes;

namespace NAnt.Core.Tasks 
{
    /// <summary>
    /// The <c>choose</c> task is used in conjuction with the <c>do</c> task to express multiple conditional 
    /// statements.
    /// </summary>
    /// <remarks>
    ///   <para>
    ///   The code of the first, and only the first, <c>do</c> task whose expression evaluates to true is executed.
    ///   </para>
    /// </remarks>
    /// <include file='Examples/Choose/IfElse.example' path='example'/>
    /// <include file='Examples/Choose/IfElseIfElse.example' path='example'/>
    [TaskName("choose")]
    public class ChooseTask : Task 
    {
        private readonly TaskCollection DoTaskList = new TaskCollection();
        

#if BPROFILE
		/// <summary>Return additional info identifying the task to the BProfiler.</summary>
		public override string BProfileAdditionalInfo 
		{
			get	{
				if (this._xmlNode != null) {
					return this._xmlNode.OuterXml.Substring(0, Math.Min(20, this._xmlNode.OuterXml.Length));
				}
				return base.BProfileAdditionalInfo;
			}
		}
#endif
        [XmlElement("do", "NAnt.Core.Tasks.DoTask", AllowMultiple = true)]
        protected override void InitializeTask(XmlNode elementNode) {
            foreach (XmlNode node in elementNode) {
                if (node is XmlElement) {
                    XmlElement element = (XmlElement) node;
                    if (element.Name == "do") {
                        DoTask task = new DoTask();
                        task.Project = Project;
                        task.Initialize(element);

                        DoTaskList.Add(task);
                    } 
                    else {
                        string msg = String.Format("Illegal Xml element '{0}' inside choose element.", element.Name);
                        throw new BuildException(msg, Location);
                    }
                }
            }
        }
        
        protected override void ExecuteTask() {
            foreach(DoTask task in DoTaskList) {
                if (task.IfDefined && !task.UnlessDefined) {
                    task.Execute();
                    break;
                }
            }
        }
    }
}
