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
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Management.Automation;
using System.Management.Automation.Runspaces;
using System.Threading;

using Microsoft.Build.Evaluation;

using NuGet.ProjectManagement;

// use alternative names to avoid collision with NAnt types
using MSProject = Microsoft.Build.Evaluation.Project;
using MSProjectItem = Microsoft.Build.Evaluation.ProjectItem;
using MSProjectCollection = Microsoft.Build.Evaluation.ProjectCollection;

namespace NAnt.NuGet.Deprecated
{
	// pretend dte project reference, it's much faster to create this object
	// than load our sln file in visualstudio to get a real reference, this
	// object "fakes" the most commonly used interface members and field
	// from install.ps1 scripts
	public class MockDTEProjectReference
	{
		// fakes a DTE reference
		public class MockDTE
		{
			public readonly string Version; // some nuget scripts use this for compatibility checks

			public MockDTE(string dteVersion)
			{
				Version = dteVersion;
			}

			public class MockItemOperations
			{
				public void Navigate(string url) { } // some packages redirect visualstudio to their webpage once they are done installing, we just swallow these calls
			}

			public MockItemOperations ItemOperations = new MockItemOperations();
		}

		// common base, does nothing but more clear than using Object
		public interface IMockDTEItem
		{
		}

		// fake a lookup class used by COM object, seems to essentially be a string keyed dictionary
		// accessed via .Item("string") that returns null on no matching key
		public class MockDTELookup
		{
			private Dictionary<string, IMockDTEItem> m_internalDict;

			public MockDTELookup(Dictionary<string, IMockDTEItem> dict)
			{
				m_internalDict = dict;
			}

			public IMockDTEItem Item(string key)
			{
				IMockDTEItem ret = null;
				if (m_internalDict.TryGetValue(key, out ret))
				{
					return ret;
				}
				return null;
			}
		}

		// represents a folder item in solution view style hierarchy
		public class MockDTEFolderItem : IMockDTEItem
		{
			public MockDTEFolderItem()
			{
				Dictionary = new Dictionary<string, IMockDTEItem>();
			}

			public MockDTELookup ProjectItems { get { return new MockDTELookup(Dictionary); } }
			public Dictionary<string, IMockDTEItem> Dictionary;
		}

		// represents a file in solution view style hierarchy, allows manipulation of item properties
		public class MockDTEFileItem : IMockDTEItem
		{
			// emulate item.Properties - this whole stack of nested classes is so we can validly do things like the following from powershell side:
			// $property = $item.Properties.Item("CopyToOutputDirectory")
			// if ($property -ne $null) {
			//    $property.Value = 1    
			public class MockProperties
			{
				// use a property proxy object, we can't determine valid values as any valid property returns
				// a valid object because we only have access to actual metadata - so we always returns an object
				// and if code tries to manipulate it then we modify metadata accordingly
				public class PropertyProxy
				{
					public object Value
					{
						set 
						{
							string strValue = value.ToString();

							// for now treat everything is as special case
							if (m_key == "CopyToOutputDirectory" && typeof(Int32).IsInstanceOfType(value) && (int)value == 1)
							{
								strValue = "Always";
							}
							else
							{
								// throw here - trying to handle this quietly when we don't know the context is more likely
								// to lead to a quiet error than something that works
								throw new ArgumentException(String.Format("Unknown property in Nuget script '{0}'!", strValue));
							}

							if (m_projectItem.HasMetadata(m_key))
							{
								m_projectItem.Metadata.First(md => md.Name == m_key).UnevaluatedValue = strValue;
							}
							else
							{
								m_projectItem.SetMetadataValue(m_key, strValue);
							}
						}
					}

					private MSProjectItem m_projectItem;
					private string m_key;

					public PropertyProxy(string key, MSProjectItem projectItem)
					{
						m_key = key;
						m_projectItem = projectItem;
					}
				}

				private MSProjectItem m_projectItem;

				public MockProperties(MSProjectItem projectItem)
				{
					m_projectItem = projectItem;
				}

				public PropertyProxy Item(string key)
				{
					return new PropertyProxy(key, m_projectItem);
				}
			}

			public MockProperties Properties { get { return new MockProperties(m_projectItem); } }

			private MSProjectItem m_projectItem;

			public MockDTEFileItem(MSProjectItem projectItem)
			{
				m_projectItem = projectItem;
			}
		}

		public class MockProperty
		{
			private ProjectProperty property;

			public object Value
			{
				get
				{
					return property.UnevaluatedValue;
				}
			}

			public MockProperty(ProjectProperty prop)
			{
				property = prop;
			}
		}

		public class MockPropertiesCollection
		{
			private Dictionary<string, MockProperty> collection;

			public MockPropertiesCollection(MSProject project)
			{
				collection = project.Properties.ToDictionary(x => x.Name, x => new MockProperty(x));
			}

			public MockProperty Item(string name)
			{
				return collection[name];
			}
		}

		public MockDTELookup ProjectItems { get { return BuildMockDTEItemTree(); } }

		public readonly string FullName;
		public readonly MSProject MSProject;
		public readonly MockDTE DTE;
		public MockPropertiesCollection Properties;

		public MockDTEProjectReference(string projectFileName, MSProject msProject, string vsVersion)
		{
			FullName = projectFileName;
			MSProject = msProject;
			Properties = new MockPropertiesCollection(msProject);
			DTE = new MockDTE(vsVersion);
		}

		public void Save() // we perform our own save using msbuild object
		{
		}

		private MockDTELookup BuildMockDTEItemTree()
		{
			// used to build up project.ProjectItems

			// project.ProjectItems looks to be a tree of objects that correspond to the "Solution Explorer" view in VS,
			// unfortunately the MSProject object we have doesn't have a corresponding object so we have to do some path
			// shenanigans to emulate the object i.e
			//
			// /folder1				project.ProjectItems.Items("folder1");
			//		/folder11		project.ProjectItems.Items("folder1").ProjectItems.Items("folder11");
			//		/folder12		project.ProjectItems.Items("folder1").ProjectItems.Items("folder12");
			//			/file121.e	project.ProjectItems.Items("folder1").ProjectItems.Items("folder12").ProjectItems.Items("file121.e");
			// /folder2				project.ProjectItems.Items("folder2");
			//		/folder21		project.ProjectItems.Items("folder2").ProjectItems.Items("folder21");
			//			/file211.e	project.ProjectItems.Items("folder2").ProjectItems.Items("folder21").ProjectItems.Items("file211.e");
			//

			// build up dictionary of project items, this is not strictly one to one with DTE version, but we only need stuff nuget packages
			// care about which to date, seems to be content files map the "Solution Explorer" i.e Link path of these items to their references
			Dictionary<string, IMockDTEItem> topLevelDict = new Dictionary<string, IMockDTEItem>();
			IEnumerable<MSProjectItem> contentItems = MSProject.Items.Where(i => i.ItemType == "Content");
			foreach (MSProjectItem content in contentItems)
			{
				Dictionary<string, IMockDTEItem> currentFolderLevel = topLevelDict;
				string itemPath = content.Metadata.First(md => md.Name == "Link").EvaluatedValue;
				IEnumerable<string> pathComponents = itemPath.Split(new[] { Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar });
				foreach (string folderComponent in pathComponents.Take(pathComponents.Count() - 1))
				{
					IMockDTEItem folderItem = null;
					if (!currentFolderLevel.TryGetValue(folderComponent, out folderItem))
					{
						folderItem = new MockDTEFolderItem();
						currentFolderLevel.Add(folderComponent, folderItem);
					}						
					currentFolderLevel = ((MockDTEFolderItem)folderItem).Dictionary;
				}

				// hack, line should be:
				// currentFolderLevel.Add(pathComponents.Last(), new MockDTEFileItem(content));
				// to prevent duplicate items being added but right now we have duplciate items,
				// this is because multiple nuget packages export the same content file. Visual 
				// Studio gets way with this bceause it knows which content files do not need
				// to be added to the project file unfortuantely we don't have this info until
				// we upgrade to nuget 3.0
				currentFolderLevel[pathComponents.Last()] = new MockDTEFileItem(content);
				
			}
			return new MockDTELookup(topLevelDict);
		}
	}

	public sealed class NugetVSPowerShellEnv
	{
		private readonly Dictionary<string, MSProject> m_loadedProjects = new Dictionary<string, MSProject>();

		private readonly string m_solutionName;
		private readonly string m_visualStudioVersion;

		// runspace and instance are lazy initialize as we probably won't need them
		private Runspace m_runSpace;
		private PowerShell m_psInstance;

#if NANT_NUGET_DEBUG_MODE
		private EnvDTE.DTE m_dte = null; // do not access directly, use GetDTEInstance()
#endif

		public NugetVSPowerShellEnv(string solutionFileName, string visualStudioVersion)
		{
			m_solutionName = solutionFileName;
			m_visualStudioVersion = visualStudioVersion;
		}

		public void CallInitScript(string initScript, string installPath, string toolsPath, INuGetProjectContext package)
		{
			CallScript(initScript, installPath, toolsPath, package, null, null); // init scripts don't reference a project
		}

		public void CallInstallScript(string installScript, string installPath, string toolsPath, INuGetProjectContext package, string projectFileName)
		{
			// looks like a lot of post install scripts expect GlobalProjectCollection to have been pre-initialized
			// with the project in question (by VisualStudio), load it ourselves and cache the reference so we can
			// accumulate changes
			if (!m_loadedProjects.TryGetValue(projectFileName, out MSProject openProject))
			{
				MSProjectCollection.GlobalProjectCollection.SetGlobalProperty("VisualStudioVersion", m_visualStudioVersion); // on clean install of vs this doesn't get set implicitly and breaks certain project files - set explicitly here
				MSProjectCollection.GlobalProjectCollection.SetGlobalProperty("FullPath", Path.GetDirectoryName(projectFileName));
				openProject = MSProjectCollection.GlobalProjectCollection.LoadProject(projectFileName);
				m_loadedProjects.Add(projectFileName, openProject);
			}

			// get project object
			object projectObject = null;
			object dteObject = null;

#if !NANT_NUGET_DEBUG_MODE
			MockDTEProjectReference mockProjectReference = new MockDTEProjectReference(projectFileName, openProject, m_visualStudioVersion);
			projectObject = mockProjectReference;
			dteObject = mockProjectReference.DTE;
#else
			// slow mode, initialize visualstudio (if it isn't already), and get project com object
			bool successfulProjectEnumeration = false;
			while (!successfulProjectEnumeration)
			{
				try
				{
					EnvDTE.DTE dte = GetDTEInstance();
					projectObject = dte.Solution.Projects.Cast<EnvDTE.Project>().FirstOrDefault(proj => PathNormalizer.Normalize(proj.FileName) == projectFileName);
					dteObject = dte;
					successfulProjectEnumeration = true;
				}
				catch (COMException comException)
				{
					if (!comException.Message.Contains("RPC_E_SERVERCALL_RETRYLATER")) // microsoft suggest way to fix this is a bit more involved: https://msdn.microsoft.com/en-us/library/ms228772.aspx
					{
						throw;
					}
				}
			}
			if (projectObject == null)
			{
				throw new BuildException(String.Format("Cannot find project target {0} for Nuget post-install!", projectFileName));
			}
#endif

			// call script
			CallScript(installScript, installPath, toolsPath, package, projectObject, dteObject);
		}

		public void Close()
		{
			// close runspace
			if (m_runSpace != null)
			{
				m_runSpace.Close();
			}

			// save changes to project objects
			foreach (MSProject loadedMSBuildProject in m_loadedProjects.Values)
			{
				loadedMSBuildProject.Save();
			}

#if NANT_NUGET_DEBUG_MODE
			// close solution
			if (m_dte != null)
			{
				bool successfulQuit = false;
				while (!successfulQuit)
				{
					try
					{
						m_dte.Quit(); // close visualstudio
						successfulQuit = true;
					}
					catch (COMException comException)
					{
						if (!comException.Message.Contains("RPC_E_SERVERCALL_RETRYLATER")) // microsoft suggest way to fix this is a bit more involved: https://msdn.microsoft.com/en-us/library/ms228772.aspx
						{
							throw;
						}
					}
				}
			}
#endif
		}

		private void CallScript(string script, string installPath, string toolsPath, INuGetProjectContext package, object project, object dte)
		{
			// create powershell instance, if hasn't been created yet
			if (m_runSpace == null)
			{
				InitialSessionState initial = InitialSessionState.CreateDefault();
				initial.AuthorizationManager = new AuthorizationManager("MyShellId"); // avoids restriction errors
				Runspace runspace = RunspaceFactory.CreateRunspace(new PretendPackageConsoleHost(Thread.CurrentThread.CurrentCulture, Thread.CurrentThread.CurrentUICulture), initial);
				runspace.Open();
				m_runSpace = runspace;
			}
			// create a single ps instance to run all scripts in, we require persistence between inits and installs
			if (m_psInstance == null)
			{
				PowerShell instance = PowerShell.Create(); 
				instance.Runspace = m_runSpace;
				m_psInstance = instance;
			}

			// using the session state proxy is the only way to pass .net object through to powershell
			// reliably, all the permutations of .AddParameter and .Parameters.Add only really work
			// for command objects
			m_runSpace.SessionStateProxy.SetVariable("__installPath", installPath);
			m_runSpace.SessionStateProxy.SetVariable("__toolsPath", toolsPath);
			m_runSpace.SessionStateProxy.SetVariable("__package", package); // TODO in latest version we always pass this as null which is techinically wrong but it's only reference in one case we've discovered where we don't need it anyway
			m_runSpace.SessionStateProxy.SetVariable("__project", project);

			// this is an undocumented variable that some scripts rely on
			m_runSpace.SessionStateProxy.SetVariable("dte", dte);

			// & is a short hand for Invoke-Expression cmdlet - we invoke a command that invokes our
			// script as powershell api doesn't offer a good way to pass parameters to scripts that
			// declare Params block, so we pass variables from our session in script
			string command = "& " + script + " $__installPath $__toolsPath $__package $__project";

			// invoke our script command
			m_psInstance.AddScript(command);
			m_psInstance.Invoke();
			m_psInstance.Commands.Clear();

			if (m_psInstance.Streams.Error.Any())
			{
				ErrorRecord firstError = m_psInstance.Streams.Error.First();
				throw new Exception
				(
					String.Format
					(
						"Error at nuget script {0}({1},{2})",
						firstError.InvocationInfo.ScriptName,
						firstError.InvocationInfo.ScriptLineNumber,
						firstError.InvocationInfo.OffsetInLine
					),
					firstError.Exception
				);
			}

			// make sure we don't leak our variables
			m_runSpace.SessionStateProxy.SetVariable("__installPath", null);
			m_runSpace.SessionStateProxy.SetVariable("__toolsPath", null);
			m_runSpace.SessionStateProxy.SetVariable("__package", null);
			m_runSpace.SessionStateProxy.SetVariable("__project", null);
			m_runSpace.SessionStateProxy.SetVariable("dte", null);
		}

#if NANT_NUGET_DEBUG_MODE
		private EnvDTE.DTE GetDTEInstance()
		{
			if (m_dte == null)
			{
				// determine type from vs version
				string progId = String.Format("VisualStudio.DTE.{0}", m_visualStudioVersion);
				Type type = Type.GetTypeFromProgID(progId);

				// open visual studio instance and laod solution
				m_dte = (EnvDTE.DTE)System.Activator.CreateInstance(type);
				m_dte.MainWindow.Visible = false;
				m_dte.Solution.Open(m_solutionName);
			}
			return m_dte;
		}
#endif
	}
}
