using System;
using System.Xml;
using System.Collections.Generic;
using System.Threading.Tasks;

using NAnt.Core.Attributes;
using NAnt.Core.Reflection;
using NAnt.Core.Threading;

namespace NAnt.Core
{

    public abstract class ParallelTaskContainer : TaskContainer
    {

#if BPROFILE
		/// <summary>Return additional info identifying the task to the BProfiler.</summary>
		public override string BProfileAdditionalInfo 
		{
			get {
				if (this._xmlNode != null) {
					return this._xmlNode.OuterXml.Substring(0, Math.Min(20, this._xmlNode.OuterXml.Length));
				}
				return base.BProfileAdditionalInfo;
			}
		}
#endif


        /// <summary>
        /// Creates and Executes the embedded (child xml nodes) elements.
        /// </summary>
        /// <remarks> Skips any element defined by the host task that has an BuildElementAttribute (included filesets and special xml) defined.</remarks>
        protected override void ExecuteChildTasks()
        {
            if (Project.NoParallelNant)
            {
                foreach (XmlNode childNode in _xmlNode.ChildNodes)
                {
                    ExecuteOneChildTask(childNode);
                }
            }
            else
            {
                try
                {
                    Parallel.For(0, _xmlNode.ChildNodes.Count, j =>
                    {
                        ExecuteOneChildTask(_xmlNode.ChildNodes[j]);
                    });
                }
                catch (Exception ex)
                {
                    ThreadUtil.RethrowThreadingException(String.Format("Error in task '{0}'", this.Name), Location, ex);
                }
            }
        }

        private void ExecuteOneChildTask(XmlNode childNode)
        {
            if (childNode.NodeType == XmlNodeType.Element && childNode.NamespaceURI.Equals(_xmlNode.NamespaceURI))
            {
                if (!IsPrivateXMLElement(childNode))
                {
                    Task task = CreateChildTask(childNode);
                    // we should assume null tasks are because of incomplete metadata about the xml.
                    if (task != null)
                    {
                        task.Parent = this;
                        task.Execute();
                    }
                }
            }
        }



    }
}
