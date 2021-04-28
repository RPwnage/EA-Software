// (c) Electronic Arts. All rights reserved.

using System;
using System.Text;
using System.Xml;

using NAnt.Core;
using NAnt.Core.Attributes;

using EA.Eaconfig.Structured;

namespace EA.Eaconfig
{
	[TaskName("structured-extension-xbsx", XmlSchema = false)]
	class StructuredExtensionXbsx : PlatformExtensionBase
	{
		/// <summary>
		/// image builder options.
		/// </summary>
		[Property("remoteIPaddress")]
		public ConditionalPropertyElement RemoteIPAddress
		{
			get { return _remoteIPAddress; }
		}
		private ConditionalPropertyElement _remoteIPAddress = new ConditionalPropertyElement();

		/// <summary>
		/// Xbsx deployment options.
		/// </summary>
		[Property("layout")]
		public Layout Layout
		{
			get
			{
				return _layout;
			}
		}
		private Layout _layout = new Layout();


		/// <summary>
		/// Xbsx deployment options.
		/// </summary>
		[Property("deployment")]
		public Deployment Deploy
		{
			get
			{
				return _deploy;
			}
		}
		private Deployment _deploy = new Deployment();

		protected override void ExecuteTask()
		{
			if (!String.IsNullOrEmpty(RemoteIPAddress.Value))
			{
				Module.SetModuleProperty("remotemachine", RemoteIPAddress.Value.TrimWhiteSpace());
			}

			SetLayout();

			SetDeployment();
		}

		private void SetLayout()
		{
			if (!String.IsNullOrEmpty(Layout.LayoutDir.Value))
			{
				Module.SetModuleProperty("layoutdir", Layout.LayoutDir.Value.TrimWhiteSpace());
			}

			if (Layout._isolateconfigurations != null)
			{
				Module.SetModuleProperty("isolate-configurations-on-deploy", Layout.IsolateConfigurations.ToString());
			}

			if (!String.IsNullOrEmpty(Layout.GameOS.Value))
			{
				Module.SetModuleProperty("gameos", Layout.GameOS.Value.TrimWhiteSpace());
			}

            // See if an inherited partial module also set this property, and use it as the default to append to. 
            // If that doesn't exist, use the eaconfig property as the default.
            string exclusionFilter = Module.GetModuleProperty("layout-extension-filter");
            if (String.IsNullOrEmpty(exclusionFilter))
            {
                exclusionFilter = Properties["eaconfig.Xbsx.layout-exclusion-filter"] ?? "";
            }

			if (!String.IsNullOrEmpty(Layout.ExclusionFilter.Value))
			{
				if (Layout.ExclusionFilter.Append)
				{
					exclusionFilter = String.Format("{0};{1}", exclusionFilter.TrimEnd(';'), Layout.ExclusionFilter.Value);
				}
				else
				{
					exclusionFilter = Layout.ExclusionFilter.Value;
				}
			}

			// Exclusion Filter is the name of this field in the VS properties dialog, but in the msbuild scripts/vcxproj its actually
			// called LayoutExtensionFilter, which is why layout-extension-filter is used as a property name
			if (!String.IsNullOrEmpty(exclusionFilter))
			{
				Module.SetModuleProperty("layout-extension-filter", exclusionFilter);
			}
		}

		private void SetDeployment()
		{
			if (Deploy._enable != null)
			{
				Properties["package.consoledeployment"] = Deploy.Enable.ToString();
			}

			if (Deploy._removeextrafiles != null)
			{
				Module.SetModuleProperty("remove-extra-deploy-files", Deploy.RemoveExtraFiles.ToString());
			}

			if (Deploy.DeployMode != Deployment.DeployModeType.Default)
			{
				Module.SetModuleProperty("deploy-mode", Deploy.DeployMode.ToString());
			}
			var pullmappingfile = Deploy.PullMappingFile.PullMappingFile.TrimWhiteSpace();
			if (!String.IsNullOrEmpty(pullmappingfile))
			{
				Module.SetModuleProperty("pullmappingfile", pullmappingfile);
			}
			if (Deploy.PullMappingFile.Options != null && Deploy.PullMappingFile.Options.Options.Count > 0)
			{
				if (Deploy.PullMappingFile.Append)
				{
					var existing = Project.NamedOptionSets[Module.Group + "." + Module.ModuleName + ".pullmapping"];
					if (existing != null)
					{
						foreach (var option in Deploy.PullMappingFile.Options.Options)
						{
							existing.Options.Add(option.Key + "_" + existing.Options.Count, option.Value);
						}
					}
					else
					{
						Project.NamedOptionSets[Module.Group + "." + Module.ModuleName + ".pullmapping"] = Deploy.PullMappingFile.Options;
					}
				}
				else
				{
					Project.NamedOptionSets[Module.Group + "." + Module.ModuleName + ".pullmapping"] = Deploy.PullMappingFile.Options;
				}
			}

			var pulltempfolder = Deploy.PullTempFolder.Value.TrimWhiteSpace();
			if (!String.IsNullOrEmpty(pulltempfolder))
			{
				Module.SetModuleProperty("pulltempfolder", pulltempfolder);
			}

			var networksharepath = Deploy.NetworkSharePath.Value.TrimWhiteSpace();
			if (!String.IsNullOrEmpty(networksharepath))
			{
				Module.SetModuleProperty("networksharepath", networksharepath);
			}
		}

	}

	[ElementName("XbsxLayout", StrictAttributeCheck = true, NestedElementsCheck = true)]
	public class Layout : Element
	{
		[TaskAttribute("isolateconfigurations")]
		public bool IsolateConfigurations
		{
			get { return (bool)_isolateconfigurations; }
			set { _isolateconfigurations = value; }
		}
		internal bool? _isolateconfigurations = null;

		[Property("layoutdir")]
		public ConditionalPropertyElement LayoutDir
		{
			get
			{
				return _layoutdir;
			}
		}
		private ConditionalPropertyElement _layoutdir = new ConditionalPropertyElement();

		[Property("exclusionfilter")]
		public ConditionalPropertyElement ExclusionFilter
		{
			get
			{
				return _exclusionfilter;
			}
		}
		private ConditionalPropertyElement _exclusionfilter = new ConditionalPropertyElement();

		[Property("gameos")]
		public ConditionalPropertyElement GameOS
		{
			get
			{
				return _gameos;
			}
		}
		private ConditionalPropertyElement _gameos = new ConditionalPropertyElement();

	}

	[ElementName("XbsxDeployment", StrictAttributeCheck = true, NestedElementsCheck = true)]
	public class Deployment : Element
	{
		public enum DeployModeType { Default, Push, Pull, External, RegisterNetworkShare };

		[TaskAttribute("enable")]
		public bool Enable
		{
			get { return (bool)_enable; }
			set { _enable = value; }
		}
		internal bool? _enable = null;

		[TaskAttribute("removeextrafiles")]
		public bool RemoveExtraFiles
		{
			get { return (bool)_removeextrafiles; }
			set { _removeextrafiles = value; }
		}
		internal bool? _removeextrafiles = null;

		[TaskAttribute("deploymode")]
		public DeployModeType DeployMode
		{
			get { return _deploymode; }
			set { _deploymode = value; }
		}
		private DeployModeType _deploymode = DeployModeType.Default;

		[Property("pullmapping")]
		public PullMapping PullMappingFile
		{
			get
			{
				return _pullmappingfile;
			}
		}
		private PullMapping _pullmappingfile = new PullMapping();

		[Property("pulltempfolder")]
		public ConditionalPropertyElement PullTempFolder
		{
			get
			{
				return _pulltempfolder;
			}
		}
		private ConditionalPropertyElement _pulltempfolder = new ConditionalPropertyElement();

		[Property("networksharepath")]
		public ConditionalPropertyElement NetworkSharePath
		{
			get
			{
				return _networksharepath;
			}
		}
		private ConditionalPropertyElement _networksharepath = new ConditionalPropertyElement();
	}

	public class PullMapping : ConditionalElementContainer
	{
		public PullMapping()
		{
			_mappingpath = new PullMappingPath(this);
			_mappingexcludepath = new PullMappingExcludePath(this);
		}
		/// <summary>Location of the pull mapping file.</summary>
		[TaskAttribute("pullmappingfile", Required = false)]
		public string PullMappingFile
		{
			get { return _pullmappingfile; }
			set { _pullmappingfile = value; }
		}
		private string _pullmappingfile;

		/// <summary>Append this mapping definition to existing optionset</summary>
		[TaskAttribute("append")]
		public virtual bool Append
		{
			get { return _append; }
			set { _append = value; }
		}
		private bool _append = true;


		[BuildElement("path", Required = false)]
		public PullMappingPath MappingPath
		{
			get { return _mappingpath; }

		}
		private PullMappingPath _mappingpath;

		[BuildElement("excludepath", Required = false)]
		public PullMappingExcludePath MappingExcludePath
		{
			get { return _mappingexcludepath; }

		}
		private PullMappingExcludePath _mappingexcludepath;

		internal OptionSet Options;

		public class PullMappingPath : ConditionalElement
		{
			private readonly PullMapping _pullmapping;

			internal PullMappingPath(PullMapping pullmapping)
			{
				_pullmapping = pullmapping;
			}

			/// <summary>source folder location.</summary>
			[TaskAttribute("source", Required = true)]
			public string Source
			{
				get { return _source; }
				set { _source = value; }
			}
			private string _source;

			/// <summary>target folder location.</summary>
			[TaskAttribute("target", Required = true)]
			public string Target
			{
				get { return _target; }
				set { _target = value; }
			}
			private string _target;

			/// <summary>include filter.</summary>
			[TaskAttribute("include", Required = false)]
			public string Include
			{
				get { return _include; }
				set { _include = value; }
			}
			private string _include;

			/// <summary>exclude filter.</summary>
			[TaskAttribute("exclude", Required = false)]
			public string Excllude
			{
				get { return _exclude; }
				set { _exclude = value; }
			}
			private string _exclude;


			public override void Initialize(XmlNode elementNode)
			{
				_source = null;
				_target = null;
				_include = null;
				_exclude = null;
				base.Initialize(elementNode);
			}

			protected override void InitializeElement(XmlNode elementNode)
			{
				if (this.IfDefined && !this.UnlessDefined)
				{
					if (_pullmapping.Options == null)
					{
						_pullmapping.Options = new OptionSet();
					}

					var value = new StringBuilder();

					value.AppendFormat("{0}:{1}", "source", _source);
					value.AppendLine();

					value.AppendFormat("{0}:{1}", "target", _target);


					if (!String.IsNullOrEmpty(_include))
					{
						value.AppendLine();
						value.AppendFormat("{0}:{1}", "include", _include);
					}
					if (!String.IsNullOrEmpty(_exclude))
					{
						value.AppendLine();
						value.AppendFormat("{0}:{1}", "exclude", _include);
					}
					if (value.Length > 0)
					{
						_pullmapping.Options.Options["p" + _pullmapping.Options.Options.Count] = value.ToString().TrimWhiteSpace();
					}
				}
			}
		}


		public class PullMappingExcludePath : ConditionalElement
		{
			private readonly PullMapping _pullmapping;

			internal PullMappingExcludePath(PullMapping pullmapping)
			{
				_pullmapping = pullmapping;
			}

			/// <summary>source folder location.</summary>
			[TaskAttribute("source", Required = true)]
			public string Source
			{
				get { return _source; }
				set { _source = value; }
			}
			private string _source;

			public override void Initialize(XmlNode elementNode)
			{
				_source = null;
				base.Initialize(elementNode);
			}

			protected override void InitializeElement(XmlNode elementNode)
			{
				if (this.IfDefined && !this.UnlessDefined)
				{
					if (_pullmapping.Options == null)
					{
						_pullmapping.Options = new OptionSet();
					}

					var value = new StringBuilder();

					value.AppendFormat("{0}:{1}", "source", _source);

					if (value.Length > 0)
					{
						_pullmapping.Options.Options["e" + _pullmapping.Options.Options.Count] = value.ToString().TrimWhiteSpace();
					}
				}
			}
		}
	}
}
