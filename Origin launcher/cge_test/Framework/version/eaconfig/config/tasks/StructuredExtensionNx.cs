// (c) Electronic Arts. All rights reserved.

using System;

using NAnt.Core;
using NAnt.Core.Attributes;

using EA.Eaconfig.Structured;

namespace EA.Eaconfig
{
	[TaskName("structured-extension-nx")]
	public class StructuredExtensionNx : PlatformExtensionBase
	{
		/// <summary>
		/// Option to setup the "Application Data Directory" in Authoring section of Visual Studio project.
		/// If this is not provided and this info is needed, the default template setup from property
		/// 'nx-default-application-data-dir-template' property will be used.
		/// </summary>
		[Property("application-data-dir")]
		public ConditionalPropertyElement ApplicationDataDir
		{
			get { return mApplicationDataDir; }
		}
		private ConditionalPropertyElement mApplicationDataDir = new ConditionalPropertyElement();

		/// <summary>
		/// File name of the output NRR file.  According to documentation it should start with
		/// [application-data-dir]\.nrr folder.  If this is not setup and this info is needed,
		/// the default template setup from property 'nx-default-nrr-file-name-template' property 
		/// will be used.
		/// </summary>
		[Property("nrr-file-name")]
		public ConditionalPropertyElement NrrFileName
		{
			get { return mNrrFileName; }
		}
		private ConditionalPropertyElement mNrrFileName = new ConditionalPropertyElement();

		/// <summary>
		/// NRO deployment path.  This path should start with [application-data-dir] folder for the
		/// nro file to be able to load in runtime.  If this is not setup and this info is needed,
		/// the default template setup from property 'nx-default-nro-deploy-path-template' property 
		/// will be used.
		/// </summary>
		[Property("nro-deploy-path")]
		public ConditionalPropertyElement NroDeployPath
		{
			get { return mNroDeployPath; }
		}
		private ConditionalPropertyElement mNroDeployPath = new ConditionalPropertyElement();

		/// <summary>
		/// Custom command line to be used for running the AuthoringTool.  If it is not setup,
		/// VSI's default command line will get used.
		/// </summary>
		[Property("authoring-user-command")]
		public ConditionalPropertyElement AuthoringUserCommand
		{
			get { return mAuthoringUserCommand; }
		}
		private ConditionalPropertyElement mAuthoringUserCommand = new ConditionalPropertyElement();

		/// <summary>
		/// Specifies custom nmeta manifest file for NX 64-bit compile.
		/// This specification will effectively creates the [group].[module].nmeta64file fileset!
		/// </summary>
		[FileSet("nmeta64file", Required = false)]
		public StructuredFileSet NMeta64File { get; set; } = new StructuredFileSet();

		/// <summary>
		/// Specifies custom nmeta manifest file for NX 32-bit compile
		/// This specification will effectively creates the [group].[module].nmeta32file fileset!
		/// </summary>
		[FileSet("nmeta32file", Required = false)]
		public StructuredFileSet NMeta32File { get; set; } = new StructuredFileSet();

		/// <summary>
		/// Provide specific option override when creating the default nmeta manifest file.  
		/// Note that if 'nmeta64file' or 'nmeta32file' is provided, that provided manifest file will 
		/// be used instead and this optionset specification will not get used!
		/// This specification will effectively creates the [group].[module].nmetaoptions optionset!
		/// Consult the file eaconfig/tasks/NxGenerateNmeta.cs to which overrides are supported!
		/// </summary>
		[OptionSet("nmetaoptions", Required = false)]
		public StructuredOptionSet NMetaOptions { get; set; } = new StructuredOptionSet();

		protected override void ExecuteTask()
		{
			string appDataDir = ApplicationDataDir.Value.TrimWhiteSpace();
			if (String.IsNullOrEmpty(appDataDir))
			{
				appDataDir = Module.Project.Properties["nx-default-application-data-dir-template"];
			}
			Module.SetModuleProperty("application-data-dir", appDataDir);

			string nrrFileName = NrrFileName.Value.TrimWhiteSpace();
			if (String.IsNullOrEmpty(nrrFileName))
			{
				nrrFileName = Module.Project.Properties["nx-default-nrr-file-name-template"];
			}
			Module.SetModuleProperty("nrr-file-name", nrrFileName);

			string nroDeployPath = NroDeployPath.Value.TrimWhiteSpace();
			if (String.IsNullOrEmpty(nroDeployPath))
			{
				nroDeployPath = Module.Project.Properties["nx-default-nro-deploy-path-template"];
			}
			Module.SetModuleProperty("nro-deploy-path", nroDeployPath);

			string authoringUserCommand = AuthoringUserCommand.Value.TrimWhiteSpace();
			if (!String.IsNullOrEmpty(authoringUserCommand))
			{
				Module.SetModuleProperty("authoring-user-command", authoringUserCommand);
			}

			if (NMeta32File != null && NMeta32File.FileItems.Count > 0)
			{
				if (NMeta32File.FileItems.Count > 1)
				{
					throw new BuildException(String.Format("The nmeta32file fileset specification for module {0} appears to contain more than one file!  It must list one file only!", Module.Name));
				}
				Module.SetModuleFileset("nmeta32file", NMeta32File);
			}

			if (NMeta64File != null && NMeta64File.FileItems.Count > 0)
			{
				if (NMeta64File.FileItems.Count > 1)
				{
					throw new BuildException(String.Format("The nmeta64file fileset specification for module {0} appears to contain more than one file!  It must list one file only!", Module.Name));
				}
				Module.SetModuleFileset("nmeta64file", NMeta64File);
			}

			if (NMetaOptions.Options.Count > 0)
			{
				Module.SetModuleOptionSet("nmetaoptions", NMetaOptions);
			}
		}
	}

}
