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

using NAnt.Core;


namespace EA.Eaconfig.Structured
{
	public abstract class BuildGroupBaseTask : TaskContainer
	{
		private const string GROUP_NAME_PROPERTY = "__private_structured_buildgroup";
		private string m_existingGroupNameValue = null;

		public static string GetGroupName(Project project)
		{
			string val = project.Properties[GROUP_NAME_PROPERTY];
			return (val ?? "runtime");
		}

		public string GroupName { get; } = "runtime";

		protected BuildGroupBaseTask(string group) : base()
		{
			GroupName = group;
		}
		protected override void ExecuteTask()
		{
			try
			{
				if (Project.Properties.Contains(GROUP_NAME_PROPERTY))
				{
					// If a value currently exists, it means this build group task is somehow being nested (most likely through
					// dependent usage and the dependent package's initialize.xml also as a build group task). So we cache
					// the currently value before executing the task and restore this value after task execution.
					m_existingGroupNameValue = Project.Properties[GROUP_NAME_PROPERTY];
				}
				else
				{
					m_existingGroupNameValue = null;
				}
				Project.Properties[GROUP_NAME_PROPERTY] = GroupName;
				base.ExecuteTask();
			}
			finally
			{
				if (m_existingGroupNameValue == null)
				{
					Project.Properties.Remove(GROUP_NAME_PROPERTY);
				}
				else
				{
					Project.Properties[GROUP_NAME_PROPERTY] = m_existingGroupNameValue;
				}
			}
		}
	}
}
