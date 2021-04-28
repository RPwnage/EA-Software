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
using NAnt.Core.Attributes;
using NAnt.Core.Structured;

namespace EA.Eaconfig.Structured
{
	/// <summary>PackagingElement</summary>
	[ElementName("packaging", StrictAttributeCheck = true, NestedElementsCheck = true)]
	public class PackagingElement : ConditionalElementContainer
	{
		public PackagingElement(ModuleBaseTask module) : base()
		{
		}

		/// <summary>Sets or gets the package name</summary>
		[TaskAttribute("packagename", Required = false)]
		public string PackageName { get; set; }

		/// <summary>If true assetfiles are deployed/packaged according to platform requirements. Default is true for programs.</summary>
		[TaskAttribute("deployassets", Required = false)]
		public bool DeployAssets
		{
			get { return _deployAssets??false; }
			set { _deployAssets = value; }
		} internal bool? _deployAssets;

		/// <summary>Specify assets files are copied only and do not perform asset directory synchronization. Default is false for programs.</summary>
		[TaskAttribute("copyassetswithoutsync", Required = false)]
		public bool CopyAssetsWithoutSync
		{
			get { return _copyAssetsWithoutSync ?? false; }
			set { _copyAssetsWithoutSync = value; }
		} internal bool? _copyAssetsWithoutSync;

		/// <summary>
		/// Asset files defined by the module. 
		/// The asset files directory will be synchronized to match this fileset, therefore only one assetfiles fileset can be defined per package.
		/// To attach asset files to multiple modules use the assetfiles 'fromfileset' attribute.
		/// </summary>
		[FileSet("assetfiles", Required = false)]
		public StructuredFileSetCollection Assets { get; } = new StructuredFileSetCollection();

		/// <summary>Sets or gets the asset dependencies</summary>
		[Property("assetdependencies", Required = false)]
		public ConditionalPropertyElement AssetDependencies { get; set; } = new ConditionalPropertyElement();

		/// <summary>Sets or gets the asset-configbuilddir</summary>
		[Property("asset-configbuilddir", Required = false)]
		public ConditionalPropertyElement AssetConfigBuildDir { get; set; } = new ConditionalPropertyElement();

		/// <summary>Gets the manifest file</summary>
		[FileSet("manifestfile", Required = false)]
		public StructuredFileSet ManifestFile { get; } = new StructuredFileSet();
	}

}
