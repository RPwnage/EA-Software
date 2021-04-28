// Originally based on NAnt - A .NET build tool
// Copyright (C) 2003-2018 Electronic Arts Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
// 
// As a special exception, the copyright holders of this software give you 
// permission to link the assemblies with independent modules to produce 
// new assemblies, regardless of the license terms of these independent 
// modules, and to copy and distribute the resulting assemblies under terms 
// of your choice, provided that you also meet, for each linked independent 
// module, the terms and conditions of the license of that module. An 
// independent module is a module which is not derived from or based 
// on these assemblies. If you modify this software, you may extend 
// this exception to your version of the software, but you are not 
// obligated to do so. If you do not wish to do so, delete this exception 
// statement from your version. 
// 
// Electronic Arts (Frostbite.Team.CM@ea.com)

using System;
using System.Xml;
using System.Threading.Tasks;
using NAnt.Core.Threading;

namespace NAnt.Core
{
	public abstract class ParallelTaskContainer : TaskContainer
	{
		private object m_scriptInitLock = new object(); // create a single script init lock for all tasks - we don't want parallel tasks to include multiple scripts
														// at once, but calling context may already have current project's script init lock held so we can't use that

		/// <summary>
		/// Creates and Executes the embedded (child XML nodes) elements.
		/// </summary>
		/// <remarks> Skips any element defined by the host task that has an BuildElementAttribute (included filesets and special XML) defined.</remarks>
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
					ThreadUtil.RethrowThreadingException(String.Format("Error in task '{0}'", Name), Location, ex);
				}
			}
		}

		private void ExecuteOneChildTask(XmlNode childNode)
		{
			Project taskContextProject = new Project(Project, m_scriptInitLock); // parallel context
			{
				if (childNode.NodeType == XmlNodeType.Element && childNode.NamespaceURI.Equals(_xmlNode.NamespaceURI))
				{
					if (!IsPrivateXMLElement(childNode))
					{
						Task task = taskContextProject.CreateTask(childNode, this);
						task.Execute();
					}
				}
			}
		}
	}
}
