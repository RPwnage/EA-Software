// (c) Electronic Arts. All rights reserved.

using System;
using System.Linq;

using NAnt.Core;
using NAnt.Core.Attributes;

using EA.Eaconfig.Structured;

namespace EA.Eaconfig
{
	[TaskName("structured-extension-iphone")]
	class StructuredExtensionIPhone : PlatformExtensionBase
	{
		/// <summary>
		/// Option to rename final app bundle name.
		/// </summary>
		[Property("app-bundle-name")]
		public ConditionalPropertyElement AppBundleName
		{
			get { return _appbundlename; }
		}
		private ConditionalPropertyElement _appbundlename = new ConditionalPropertyElement();

		/// <summary>
		/// Boolean option to indicate whether this module is an IOS Framework bundle.  At the moment, this
		/// property is only supported for Dynamic Library modules.  For other modules, this property will be ignored.
		/// </summary>
		[Property("is-framework-bundle")]
		public ConditionalPropertyElement IsFrameworkBundle
		{
			get { return _isframeworkbundle; }
		}
		private ConditionalPropertyElement _isframeworkbundle = new ConditionalPropertyElement();

		/// <summary>
		/// Boolean option to indicate whether module is an app extension or not
		/// </summary>
		[Property("is-app-extension")]
		public ConditionalPropertyElement IsAppExtension
		{
			get { return _isappextension; }
		}
		private ConditionalPropertyElement _isappextension = new ConditionalPropertyElement();

		/// <summary>
		/// Option to rename final app extension name.
		/// </summary>
		[Property("app-extension-name")]
		public ConditionalPropertyElement AppExtensionName
		{
			get { return _appextensionname; }
		}
		private ConditionalPropertyElement _appextensionname = new ConditionalPropertyElement();

		/// <summary>
		/// Specify list of App Extension modules (whitespace separated) to embed to current App Bundle module
		/// (This is now deprecated in XcodeProjectizer 3.07.00.  If you app-bundle module has "copylocal" to
		/// true, all dependent app-extension modules and framework modules will be setup with embed to app bundle)
		/// </summary>
		[Property("embed-app-extension-modules")]
		public ConditionalPropertyElement EmbedAppExtensionModules
		{
			get { return _embedappextensionmodule; }
		}
		private ConditionalPropertyElement _embedappextensionmodule = new ConditionalPropertyElement();

		/// <summary>
		/// Option to set custom bundle id.
		/// </summary>
		[Property("bundle-id")]
		public ConditionalPropertyElement BundleId
		{
			get { return _bundleid; }
		}
		private ConditionalPropertyElement _bundleid = new ConditionalPropertyElement();

		/// <summary>
		/// Optionset providing extra info to insert/update the info.plist file.
		/// </summary>
		[Property("info-plist-data")]
		public StructuredOptionSet InfoPlistData
		{
			get { return _infoplistdata; }
		}
		private StructuredOptionSet _infoplistdata = new StructuredOptionSet();

		/// <summary>
		/// Embed all Non-SDK frameworks (the ones with explicit path in -framework in the &lt;frameworks&gt; block.)
		/// </summary>
		[Property("embed-all-nonsdk-frameworks")]
		public ConditionalPropertyElement EmbedAllNonSDKFrameworks
		{
			get { return mEmbedAllNonSDKFrameworks; }
		}
		private ConditionalPropertyElement mEmbedAllNonSDKFrameworks = new ConditionalPropertyElement();

		/// <summary>
		/// For the list of frameworks that is specified in the &lt;frameworks&gt; block, if any of them
		/// need to be embedded to the app bundle, they can be listed here (use Framework names only without
		/// the .framework extension and separated by space or new line).
		/// </summary>
		[Property("embed-frameworks")]
		public ConditionalPropertyElement EmbedFrameworks
		{
			get { return mEmbedFrameworks; }
		}
		private ConditionalPropertyElement mEmbedFrameworks = new ConditionalPropertyElement();

		/// <summary>
		/// Option to specify frameworks list.  You can either use full path or just the Framework name:
		///     -framework MyFramework
		///     -weak_framework ${package.dir}/Frameworks/MyWeakFramework
		/// If you specify just the Framework name and it is not a system Framework, you will need to use
		/// the following 'extra-framework-search-paths' property to provide the path.
		/// </summary>
		[Property("frameworks")]
		public ConditionalPropertyElement Frameworks
		{
			get { return _frameworks; }
		}
		private ConditionalPropertyElement _frameworks = new ConditionalPropertyElement();

		/// <summary>
		/// Option to specify extra Framework Search Paths.  If you use non-system Framework and you provided
		/// just the Framework name in the above "frameworks" field, you need to provide the path for these
		/// in this property.  Different paths should be separated by new line.
		/// </summary>
		[Property("extra-framework-search-paths")]
		public ConditionalPropertyElement ExtraFrameworkSearchPaths
		{
			get { return mExtraFrameworkSearchPaths; }
		}
		private ConditionalPropertyElement mExtraFrameworkSearchPaths = new ConditionalPropertyElement();

		/// <summary>
		/// Extra link options to be added
		/// </summary>
		[Property("extra-link-options")]
		public ConditionalPropertyElement ExtraLinkOptions
		{
			get { return _extralinkoptions; }
		}
		private ConditionalPropertyElement _extralinkoptions = new ConditionalPropertyElement();

		/// <summary>
		/// User defined info.plist file source location.
		/// </summary>
		[Property("info.plist")]
		public ConditionalPropertyElement InfoPList
		{
			get { return _infoplist; }
		}
		private ConditionalPropertyElement _infoplist = new ConditionalPropertyElement();

		/// <summary>
		/// Explicitly set the SKIP_INSTALL flag in Xcode project.
		/// (Default is true in XcodeProjectizer unless the module is a top level module)
		/// </summary>
		[Property("skip-install")]
		public ConditionalPropertyElement SkipInstall
		{
			get { return _skipInstall; }
		}
		private ConditionalPropertyElement _skipInstall = new ConditionalPropertyElement();

		/// <summary>
		/// Explicitly set the INSTALL_OWNER flag in Xcode project. XcodeProjectizer will create an empty field as default.
		/// </summary>
		[Property("install-owner")]
		public ConditionalPropertyElement InstallOwner
		{
			get { return _installOwner; }
		}
		private ConditionalPropertyElement _installOwner = new ConditionalPropertyElement();

		/// <summary>
		/// Explicitly set the INSTALL_GROUP flag in Xcode project. XcodeProjectizer will create an empty field as default.
		/// </summary>
		[Property("install-group")]
		public ConditionalPropertyElement InstallGroup
		{
			get { return _installGroup; }
		}
		private ConditionalPropertyElement _installGroup = new ConditionalPropertyElement();

		/// <summary>
		/// Optionset providing extra info to insert/update the export option plist file for archiveExport task
		/// Default will only setup "method" (as "development") and the "provisioningProfiles" 
		/// (with current module's bundle id and dependent plugin's bundle id).
		/// </summary>
		[Property("archive-export-plist-data")]
		public StructuredOptionSet ArchiveExportPlistData
		{
			get { return _archiveexportplistdata; }
		}
		private StructuredOptionSet _archiveexportplistdata = new StructuredOptionSet();

		/// <summary>
		/// Provide user defined ExportOptions.plist for for use by archiveExport task.
		/// </summary>
		[Property("archive-export-plist-file")]
		public ConditionalPropertyElement ArchiveExportPlistFile
		{
			get { return _archiveexportplistfile; }
		}
		private ConditionalPropertyElement _archiveexportplistfile = new ConditionalPropertyElement();

		protected override void ExecuteTask()
		{
			if (!String.IsNullOrEmpty(AppBundleName.Value))
			{
				Module.SetModuleProperty("app-bundle-name", AppBundleName.Value.TrimWhiteSpace());
			}
			
			if (!String.IsNullOrEmpty(IsFrameworkBundle.Value))
			{
				Module.SetModuleProperty("is-framework-bundle", IsFrameworkBundle.Value.TrimWhiteSpace());
			}

			if (!String.IsNullOrEmpty(IsAppExtension.Value))
			{
				Module.SetModuleProperty("is-app-extension", IsAppExtension.Value.TrimWhiteSpace());
			}

			if (!String.IsNullOrEmpty(AppExtensionName.Value))
			{
				Module.SetModuleProperty("app-extension-name", AppExtensionName.Value.TrimWhiteSpace());
			}

			if (!String.IsNullOrEmpty(EmbedAppExtensionModules.Value))
			{
				Module.SetModuleProperty("embed-app-extension-modules", EmbedAppExtensionModules.Value.TrimWhiteSpace());
			}

			if (!String.IsNullOrEmpty(BundleId.Value))
			{
				Module.SetModuleProperty("iphone-bundle-id", BundleId.Value.TrimWhiteSpace());
			}

			if (InfoPlistData.Options.Any())
			{
				Module.SetModuleOptionSet("iphone-info-plist-data", InfoPlistData);
			}

			if (!String.IsNullOrEmpty(EmbedAllNonSDKFrameworks.Value))
			{
				Module.SetModuleProperty("iphone-embed-all-nonsdk-frameworks", EmbedAllNonSDKFrameworks.Value.TrimWhiteSpace());
			}

			if (!String.IsNullOrEmpty(EmbedFrameworks.Value))
			{
				Module.SetModuleProperty("iphone-embed-frameworks", EmbedFrameworks.Value.TrimWhiteSpace());
			}

			if (!String.IsNullOrEmpty(Frameworks.Value))
			{
				Module.SetModuleProperty("iphone-frameworks", Frameworks.Value.TrimWhiteSpace());
			}

			if (!String.IsNullOrEmpty(ExtraFrameworkSearchPaths.Value))
			{
				Module.SetModuleProperty("iphone-extra-framework-search-paths", ExtraFrameworkSearchPaths.Value.TrimWhiteSpace());
			}

			if (!String.IsNullOrEmpty(ExtraLinkOptions.Value))
			{
				Module.SetModuleProperty("iphone-extra-link-options", ExtraLinkOptions.Value.TrimWhiteSpace());
			}

			if (!String.IsNullOrEmpty(InfoPList.Value))
			{
				Module.SetModuleProperty("iphone-info.plist", InfoPList.Value.TrimWhiteSpace());
			}

			if (!String.IsNullOrEmpty(SkipInstall.Value))
			{
				string skipInstall = SkipInstall.Value.TrimWhiteSpace().ToLower();
				if (skipInstall != "true" && skipInstall != "false")
					throw new BuildException(String.Format("Module {0} has setup 'skip-install' element in build file.  It's value can only be 'true' or 'false'.  You have '{1}'", Module.Name, SkipInstall.Value));
				Module.SetModuleProperty("iphone-xcode-skip-install", skipInstall);
			}

			if (!String.IsNullOrEmpty(InstallOwner.Value))
			{
				Module.SetModuleProperty("iphone-xcode-install-owner", InstallOwner.Value.TrimWhiteSpace());
			}

			if (!String.IsNullOrEmpty(InstallGroup.Value))
			{
				Module.SetModuleProperty("iphone-xcode-install-group", InstallGroup.Value.TrimWhiteSpace());
			}

			if (ArchiveExportPlistData.Options.Any())
			{
				Module.SetModuleOptionSet("iphone-archive-export-plist-data", ArchiveExportPlistData);
			}

			if (!String.IsNullOrEmpty(ArchiveExportPlistFile.Value))
			{
				Module.SetModuleProperty("iphone-archive-export-plist-file", ArchiveExportPlistFile.Value.TrimWhiteSpace());
			}
		}
	}

}
