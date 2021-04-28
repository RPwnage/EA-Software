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
using System.Text.RegularExpressions;
using System.Xml;

using NAnt.Core.Util;

namespace NAnt.Core.PackageCore
{
	public class Release : ReleaseBase, IComparable<Release> 
	{

		public string FullName => $"{Name}-{Version}";

		public readonly MasterConfig.IPackage Package;

		public readonly string Path;
		public readonly bool IsFlattened;

		public readonly Manifest Manifest;

		public const string Flattened = "flattened";

		[Obsolete("Deprecated. Compatibility should be accessed through .Manifest.Compatibility.")] public Compatibility Compatibility => Manifest.Compatibility;
		[Obsolete("Deprecated. ManifestVersion should be accessed through .Manifest.DriftVersion.")] public int ManifestVersion => (int)Manifest.ManifestFileVersion;
		[Obsolete("Deprecated. Status should be accessed through .Manifest.Status.")] public Manifest.ReleaseStatus Status => Manifest.Status;
		[Obsolete("Deprecated. About should be accessed through .Manifest.About.")] public string About => Manifest.About;
		[Obsolete("Deprecated. Changes should be accessed through .Manifest.Changes.")] public string Changes => Manifest.Changes;
		[Obsolete("Deprecated. ContactEmail should be accessed through .Manifest.ContactEmail.")] public string ContactEmail => Manifest.ContactEmail;
		[Obsolete("Deprecated. ContactName should be accessed through .Manifest.ContactName.")] public string ContactName => Manifest.ContactName;
		[Obsolete("Deprecated. DocumentationUrl should be accessed through .Manifest.DocumentationUrl.")] public string DocumentationUrl => Manifest.DocumentationUrl;
		[Obsolete("Deprecated. DriftVersion should be accessed through .Manifest.DriftVersion.")] public string DriftVersion => Manifest.DriftVersion;
		[Obsolete("Deprecated. HomePageUrl should be accessed through .Manifest.HomePageUrl.")] public string HomePageUrl => Manifest.HomePageUrl;
		[Obsolete("Deprecated. License should be accessed through .Manifest.License.")] public string License => Manifest.License;
		[Obsolete("Deprecated. LicenseComment should be accessed through .Manifest.LicenseComment.")] public string LicenseComment => Manifest.LicenseComment;
		[Obsolete("Deprecated. OwningTeam should be accessed through .Manifest.OwningTeam.")] public string OwningTeam => Manifest.OwningTeam;
		[Obsolete("Deprecated. SourceLocation should be accessed through .Manifest.SourceLocation.")] public string SourceLocation => Manifest.SourceLocation;
		[Obsolete("Deprecated. StatusComment should be accessed through .Manifest.StatusComment.")] public string StatusComment => Manifest.StatusComment;
		[Obsolete("Deprecated. Summary should be accessed through .Manifest.Summary.")] public string Summary => Manifest.Summary;

		internal bool UpToDate;

		public Release(MasterConfig.IPackage package, string path, bool isFlattened = false, Manifest manifest = null)
		{
			Package = package;

			Path = PathNormalizer.Normalize(path);
			IsFlattened = isFlattened;
			Name = Package.Name;
			Version = Package.Version;
			Manifest = manifest ?? Manifest.DefaultManifest;
		}

		public int CompareTo(Release value) // TODO: this should maybe be moved to the one place it's used
		{
			// sort package names in descending order
			int difference = String.Compare(value.Name, Name);
			if (difference == 0)
			{
				// special case version sorting
				difference = NumericalStringComparator.Compare(value.Version, Version);

				if (difference == 0)
				{
					// If one is flattened and the other one isn't, we consider that as separate version so that
					// we can track them later and we always list the flattened version first.
					if (IsFlattened != value.IsFlattened)
					{
						if (IsFlattened)
						{
							difference = -1;   // return -1 to have flattened version listed first in sort order.
						}
						else
						{
							difference = 1;    // return 1 to have non flattened version listed last in sort order.
						}
					}
				}
			}
			return difference;
		}

		public override string ToString()
		{
			return $"{FullName} ({Path})";
		}

		public static bool IsValidReleaseDirectoryName(string name)
		{
			for (int i = 0; i < name.Length; i++)
			{
				var ch = name[i];
				if (!(Char.IsLetterOrDigit(ch) || ch == '_' || ch == '.' || ch == '-'))
				{
					return false;
				}
			}
			return true;
		}

		public static void ParseFullPackageName(string fullPackageName, out string name, out string version)
		{
			string pattern = @"^(?<name>[\.\w]+)-(?<version>[\.\-\w]+)$";
			Match m = Regex.Match(fullPackageName, pattern);
			if (!m.Success)
			{
				string msg = String.Format("Cannot parse name and version from '{0}'.", fullPackageName);
				throw new Exception(msg);
			}

			name = m.Groups["name"].Value;
			version = m.Groups["version"].Value;
		}

		private class Utility
		{
			/// <summary>Remove all files and directories in the given path regardless of readonly attributes.</summary>
			/// <param name="path">The path </param>
			public static void ObliterateDirectory(string path)
			{
				PathUtil.DeleteDirectory(path, verify: false);
			}
		}
	}

	/// <summary>
	/// Class to store package compatibility.
	/// </summary>
	/// <remarks>
	///  <para>One declares package compatibility in the Manifest.xml file of a package. An example may look like:
	///   &lt;compatibility package="eacore"&gt;
	///   &lt;api-supported&gt;
	///   1.00.00
	///   1.00.01
	///   &lt;/api-supported&gt;
	///   &lt;binary-compatible&gt;
	///   1.00.01
	///   &lt;/binary-compatible&gt;
	///   &lt;dependent package="EABase"&gt;
	///   &lt;compatible&gt;
	///   2.0.4
	///   2.0.5
	///   &lt;/compatible&gt;
	///   &lt;incompatible version="2.0.3" message="Known bug in blah blah blah" /&gt;
	///   &lt;/dependent&gt;
	///   &lt;/compatibility&gt;
	///  </para>
	/// </remarks>
	public class Compatibility
	{
		#region Helper Classes

		/// <summary>
		/// Class to store &lt;incompatible&gt;
		/// </summary>
		public class Incompatible
		{
			public string Version, Message;

			public Incompatible(string version)
			{
				Version = version;
			}
		}

		/// <summary>
		/// Class to store &lt;depenent&gt;
		/// </summary>
		public class Dependent
		{
			internal bool fw3Verified = false;

			//FW3:

			public readonly string MinRevision;
			public readonly string MaxRevision;
			public readonly string MinVersion;
			public readonly string MaxVersion;

			public readonly bool Fail;
			public readonly string Message;

			internal Dependent(XmlNode node)
			{
				// FW3
				PackageName = XmlUtil.GetRequiredAttribute(node, "package");
				MinRevision = XmlUtil.GetOptionalAttribute(node, "minrevision");
				MaxRevision = XmlUtil.GetOptionalAttribute(node, "maxrevision");
				MinVersion = XmlUtil.GetOptionalAttribute(node, "minversion");
				MaxVersion = XmlUtil.GetOptionalAttribute(node, "maxversion");

				Fail = XmlUtil.ToBooleanOrDefault(XmlUtil.GetOptionalAttribute(node, "fail"), false, node);
				Message = XmlUtil.GetOptionalAttribute(node, "message");
				// End FW3

				XmlNode child = node.GetChildElementByName("compatible");
				if (child != null)
				{
					string val = child.InnerText.Replace("\t", "").Replace("\r", "").Trim('\n');
					Compatibles = val.Split('\n');
				}

				XmlNodeList nodes = node.SelectNodes("incompatible");
				if (nodes != null)
				{
					Incompatibles = new Incompatible[nodes.Count];
					for (int i = 0; i < nodes.Count; i++)
					{
						Incompatibles[i] = new Incompatible(nodes[i].Attributes["version"].Value);
						if (nodes[i].Attributes["message"] != null)
							Incompatibles[i].Message = nodes[i].Attributes["message"].Value;
					}
				}
			}

			#region Properties

			public string PackageName { get; } = null;

			public string[] Compatibles { get; } = null;
			public Incompatible[] Incompatibles { get; } = null;

			#endregion Properties
		}

		#endregion Helper Classes

		#region Attributes

		//FW3
		public readonly string Revision = null;
		private Dependent[] _dependents = null;

		internal void SetPackageName(string name)
		{
			if (PackageName == null)
			{
				PackageName = name;
			}
		}

		#endregion Attributes

		#region Constructor and Method

		internal Compatibility(XmlNode node)
		{
			PackageName = XmlUtil.GetOptionalAttribute(node, "package");
			Revision = XmlUtil.GetOptionalAttribute(node, "revision");

			string val;

			XmlNode child = node.GetChildElementByName("api-supported");
			if (child != null)
			{
				val = child.InnerText.Replace("\t", "").Replace("\r", "").Trim('\n');
				APISupported = val.Split('\n');
			}

			child = node.GetChildElementByName("binary-compatible");
			if (child != null)
			{
				val = child.InnerText.Replace("\t", "").Replace("\r", "").Trim('\n');
				BinaryCompatible = val.Split('\n');
			}

			XmlNodeList nodes = node.SelectNodes("dependent");
			if (nodes != null)
			{
				_dependents = new Dependent[nodes.Count];
				for (int i = 0; i < nodes.Count; i++)
				{
					_dependents[i] = new Dependent(nodes[i]);
				}
			}
		}

		#endregion Constructor and Method

		#region Properties

		public string PackageName { get; private set; } = null;

		public string[] APISupported { get; } = null;
		public string[] BinaryCompatible { get; } = null;

		public Dependent[] Dependents
		{
			get
			{
				return _dependents;
			}
		}

		#endregion Properties
	}
}
