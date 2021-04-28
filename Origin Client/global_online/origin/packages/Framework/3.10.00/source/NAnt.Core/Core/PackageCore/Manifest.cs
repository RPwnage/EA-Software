using System;
using System.IO;
using System.Xml;
using System.Text;
using System.Linq;
using System.Collections.Generic;

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Logging;


namespace NAnt.Core.PackageCore
{
    public class Manifest
    {
        // Required for manifest parsing
        public class Platform
        {
            public enum SupportType
            {
                NotSupported = 0,
                Supported,
                FutureSupport,
                Deprecated,
                KnownFailure
            }

            public string Name { get; set; }
            public SupportType Build { get; set; }
            public SupportType Test { get; set; }

            //Really optional
            public bool ContainsPlatformCode { get; set; }
            public string Defines { get; set; }
            public string Configs { get; set; }

            public bool SupportsConfig(string inConfig)
            {
                if (!inConfig.Contains(Name))
                    return false;

                if (!String.IsNullOrEmpty(Configs))
                {
                    string[] configs = Configs.Split(new char[] { ' ' }, StringSplitOptions.None);
                    foreach (string config in configs)
                    {
                        if (inConfig.StartsWith(config))
                            return true;
                    }
                    return false;
                }

                return true;
            }

            public bool SupportsBuild
            {
                get
                {
                    return Build == SupportType.Supported;
                }
            }

            public bool SupportsTest
            {
                get
                {
                    return Test == SupportType.Supported;
                }
            }

            internal Platform(string name, SupportType build, SupportType test, bool containsPlatformCode, string defines, string configs)
            {
                Name = name;
                Build = build;
                Test = test;
                ContainsPlatformCode = containsPlatformCode;
                Defines = defines;
                Configs = configs;
            }
            public Platform() {}
        }

        public class Build
        {
            public string Name { get; set; }
            public bool Supported { get; set; }
            public string Targets { get; set; }
            public string Defines { get; set; }

            internal Build(string name, bool supported, string targets, string defines)
            {
                Name = name;
                Supported = supported;
                Targets = targets;
                Defines = defines;
            }
            public Build() { }
        }

        public enum ManifestFileVersionType { v1, v2 };
        public enum ReleaseStatus { Official, Accepted, Prerelease, Deprecated, Broken, Unknown };
        public static readonly Manifest DefaultManifest = new Manifest();

        public readonly ManifestFileVersionType ManifestFileVersion;
        public readonly FrameworkVersions FrameworkVersion;
        public readonly bool Buildable;
        public readonly string ContactName;
        public readonly string ContactEmail;
        public readonly string Summary;
        public readonly string Description;
        public readonly string About;
        public readonly string Changes;
        public readonly string HomePageUrl;
        public readonly ReleaseStatus Status;
        public readonly string StatusComment;
        public readonly string License;
        public readonly string LicenseComment;
        public readonly string DocumentationUrl;
        public readonly string Version;
        public readonly Compatibility Compatibility;
        // Version 2
        public readonly string OwningTeam;
        public readonly string SourceLocation;
        public readonly string DriftVersion;
        public readonly string Tags;
        public readonly string Community;
        public readonly bool IsSupported;
        public readonly string PlatformsInCode;
        public readonly string ContainsOpenSource;
        public readonly string LicenseType;
        public readonly bool InternalOnly;
        public readonly IEnumerable<Platform> Platforms;
        public readonly IEnumerable<Build> Builds;

        public bool SolutionSupported
        {
            get
            {
                foreach (Build b in Builds)
                {
                    if (b.Name.ToLower() == "solution")
                    {
                        return b.Supported;
                    }
                }
                return false;
            }
        }

        public bool NantSupported
        {
            get
            {
                foreach (Build b in Builds)
                {
                    if (b.Name.ToLower() == "nant")
                    {
                        return b.Supported;
                    }
                }
                return false;
            }
        }

        public string NantTargets
        {
            get
            {
                foreach (Build b in Builds)
                {
                    if (b.Name.ToLower() == "nant")
                    {
                        return b.Targets;
                    }
                }
                return "";
            }
        }

        public bool TestSupported
        {
            get
            {
                foreach (Build b in Builds)
                {
                    if (b.Name.ToLower() == "test")
                    {
                        return b.Supported;
                    }
                }
                return false;
            }
        }

        public string TestTargets
        {
            get
            {
                foreach (Build b in Builds)
                {
                    if (b.Name.ToLower() == "test")
                    {
                        return b.Targets;
                    }
                }
                return "";
            }
        }



        public Manifest(FrameworkVersions frameworkVersion, bool buildable, string contactName, string contactEmail,
                         string summary, string description, string about, string changes, string homePageUrl, ReleaseStatus status, string statusComment,
                         string license, string licenseComment, string documentationUrl, string version, Compatibility compatibility,
                         string owningTeam, string sourceLocation, string driftVersion, string tags, string community, bool isSupported, string platformsInCode,
                         string containsOpenSource, string licenseType, bool internalOnly, IEnumerable<Platform> platforms, IEnumerable<Build> builds)
        {
            FrameworkVersion = frameworkVersion;
            Buildable = buildable;
            ContactName = contactName; ;
            ContactEmail = contactEmail;
            Summary = summary;
            Description = description;
            About = about;
            Changes = changes;
            HomePageUrl = homePageUrl;
            Status = status;
            StatusComment = statusComment;
            License = license;
            LicenseComment = licenseComment;
            DocumentationUrl = documentationUrl;
            Version = version;
            Compatibility = compatibility;

            OwningTeam = owningTeam;
            SourceLocation = sourceLocation;
            DriftVersion = driftVersion;
            Tags = tags;
            Community = community;
            IsSupported = isSupported;
            PlatformsInCode = platformsInCode;
            ContainsOpenSource = containsOpenSource;
            LicenseType = licenseType;
            InternalOnly = internalOnly;

            Platforms = platforms == null ? new List<Platform>() : new List<Platform>(platforms);
            Builds = builds == null ? new List<Build>() : new List<Build>(builds);
        }

        private Manifest()
        {
            FrameworkVersion = FrameworkVersions.Framework1;
            Buildable = false;
            ContactName = String.Empty;
            ContactEmail = String.Empty;
            Summary = String.Empty;
            Description = String.Empty;
            About = String.Empty;
            Changes = String.Empty;
            HomePageUrl = String.Empty;
            Status = ReleaseStatus.Unknown;
            StatusComment = String.Empty;
            License = String.Empty;
            LicenseComment = String.Empty;
            DocumentationUrl = String.Empty;
            Version = String.Empty;
            Compatibility = null;
            OwningTeam = String.Empty;
            SourceLocation = String.Empty;
            Tags = String.Empty;
            Community = String.Empty;
            IsSupported = true;
            PlatformsInCode = String.Empty;
            ContainsOpenSource = String.Empty;
            LicenseType = String.Empty;
            InternalOnly = false;

            Platforms = new List<Platform>();
            Builds = new List<Build>();

        }

        public static Manifest Load(string directory, Log log = null)
        {
            Manifest manifest = DefaultManifest;

            string file = Path.Combine(directory, "Manifest.xml");

            if(File.Exists(file))
            {
                LineInfoDocument doc = LineInfoDocument.Load(file);

                if (doc.DocumentElement.Name != "package")
                {
                    Warning(doc.DocumentElement, String.Format("Expected root element is 'package', found root '{0}'.", doc.DocumentElement.Name), log);
                }

                FrameworkVersions frameworkVersion = FrameworkVersions.Framework1;
                bool? buildable = null;
                string contactName = String.Empty;
                string contactEmail = String.Empty;
                string summary = String.Empty;
                string description = String.Empty;
                string about = String.Empty;
                string changes = String.Empty;
                string homePageUrl = String.Empty;
                ReleaseStatus status = ReleaseStatus.Unknown;
                string statusComment = String.Empty;
                string license = String.Empty;
                string licenseComment = String.Empty;
                string documentationUrl = String.Empty;
                string version = String.Empty;
                Compatibility compatibility = null;

                // Version 2
                string owningTeam = String.Empty;
                string sourceLocation = String.Empty;
                string driftVersion = String.Empty;
                string tags = String.Empty;
                bool? isSupported = true;
                string community = String.Empty;
                string platformsInCode = String.Empty;
                string containsOpenSource = String.Empty;
                string licenseType = String.Empty;
                bool? internalOnly = false;
                List<Platform> platforms = new List<Platform>();
                List<Build> builds = new List<Build>();

                bool foundUnknownElements = false;

                foreach (XmlNode childNode in doc.DocumentElement.ChildNodes)
                {
                    if (childNode.NodeType == XmlNodeType.Element && childNode.NamespaceURI.Equals(doc.DocumentElement.NamespaceURI))
                    {
                        switch (childNode.Name)
                        {
                            case "frameworkVersion":

                                int? fwv = ConvertUtil.ToNullableInt(StringUtil.Trim(childNode.InnerText));
                                if(fwv != null && fwv > 0 && fwv < 3)
                                {
                                    frameworkVersion = (fwv == 2) ? FrameworkVersions.Framework2 :  FrameworkVersions.Framework1;
                                }
                                else
                                {
                                    Error(childNode, "Value of 'frameworkVersion' element can be '1' or '2', found: '"+StringUtil.Trim(childNode.InnerText)+"'.");
                                }
                                break;
                            case "buildable":
                                buildable = ConvertUtil.ToNullableBoolean(StringUtil.Trim(childNode.InnerText));
                                break;
                            case "manifestVersion":
                                version = StringUtil.NormalizeText(childNode.InnerText);
                                break;
                            case "contactName":
                                contactName = StringUtil.NormalizeText(childNode.InnerText);
                                break;
                            case "contactEmail":
                                contactEmail = StringUtil.NormalizeText(childNode.InnerText);
                                break;
                            case "summary":
                                summary = StringUtil.NormalizeText(childNode.InnerText);
                                break;
                            case "description":
                                description = StringUtil.NormalizeText(childNode.InnerText);
                                break;
                            case "changes":
                                changes = StringUtil.NormalizeText(childNode.InnerText);
                                break;
                            case "homePageUrl":
                                homePageUrl = StringUtil.NormalizeText(childNode.InnerText);
                                break;
                            case "status":
                                if (!Enum.TryParse<ReleaseStatus>(StringUtil.NormalizeText(childNode.InnerText), true, out status))
                                {
                                    status = ReleaseStatus.Unknown;
                                }
                                break;
                            case "statusComment":
                                statusComment = StringUtil.NormalizeText(childNode.InnerText);
                                break;
                            case "license":
                                license = StringUtil.NormalizeText(childNode.InnerText);
                                break;
                            case "licenseComment":
                                licenseComment = StringUtil.NormalizeText(childNode.InnerText);
                                break;
                            case "documentationUrl":
                                documentationUrl = StringUtil.NormalizeText(childNode.InnerText);
                                break;
                            case "compatibility":
                                compatibility = new Compatibility(childNode);
                                break;
                            case "owningTeam":
                                owningTeam = StringUtil.NormalizeText(childNode.InnerText);
                                break;
                            case "sourceLocation":
                                sourceLocation = StringUtil.NormalizeText(childNode.InnerText);
                                break;
                            case "driftVersion":
                                driftVersion = StringUtil.NormalizeText(childNode.InnerText);
                                break;
                            case "tags":
                                tags = StringUtil.NormalizeText(childNode.InnerText);
                                break;
                            case "isSupported":
                                isSupported = ConvertUtil.ToNullableBoolean(childNode.InnerText.TrimWhiteSpace(), childNode);
                                break;
                            case "community":
                                community = StringUtil.NormalizeText(childNode.InnerText);
                                break;
                            case "platformsInCode":
                                platformsInCode = StringUtil.NormalizeText(childNode.InnerText);
                                break;
                            case "licenseDetails":

                                foreach (XmlNode licenseNode in childNode.ChildNodes)
                                {
                                    if (licenseNode.NodeType == XmlNodeType.Element && childNode.NamespaceURI.Equals(doc.DocumentElement.NamespaceURI))
                                    {
                                        switch (licenseNode.Name)
                                        {
                                            case "containsOpenSource":
                                                containsOpenSource = StringUtil.NormalizeText(licenseNode.InnerText);
                                                break;
                                            case "licenseType":
                                                licenseType = StringUtil.NormalizeText(licenseNode.InnerText);
                                                break;
                                            case "internalOnly":
                                                internalOnly = ConvertUtil.ToNullableBoolean(licenseNode.InnerText.TrimWhiteSpace(), childNode);
                                                break;
                                            case "license":
                                                license = StringUtil.NormalizeText(childNode.InnerText);
                                                break;
                                            case "licenseComment":
                                                licenseComment = StringUtil.NormalizeText(childNode.InnerText);
                                                break;
                                            default:
                                                foundUnknownElements = true;
                                                if (log != null)
                                                {
                                                    log.Debug.WriteLine("Unknown element '{0}' in manifest file: {1}", licenseNode.Name, Location.GetLocationFromNode(licenseNode).ToString());
                                                }
                                                break;
                                        }
                                    }
                                }
                                break;
                            case "relationship":
                                foreach (XmlNode relNode in childNode.ChildNodes)
                                {
                                    if (relNode.NodeType == XmlNodeType.Element && childNode.NamespaceURI.Equals(doc.DocumentElement.NamespaceURI))
                                    {
                                        switch (relNode.Name)
                                        {
                                            case "platforms":
                                                ParsePlatforms(relNode, platforms);
                                                break;
                                            case "builds":
                                                ParseBuilds(relNode, builds);
                                                break;
                                            default:
                                                foundUnknownElements = true;
                                                if (log != null)
                                                {
                                                    log.Debug.WriteLine("Unknown element '{0}' in manifest file: {1}", relNode.Name, Location.GetLocationFromNode(relNode).ToString());
                                                }
                                                break;
                                        }
                                    }
                                }

                                break;
                            default:
                                foundUnknownElements = true;
                                if (log != null)
                                {
                                    log.Debug.WriteLine("Unknown element in manifest file: '{0}'\n{1}", childNode.Name, Location.GetLocationFromNode(childNode).ToString());
                                }
                                break;
                        }
                    }
                }

                if(foundUnknownElements)
                {
                    //IMTODO
                    //PrintValidManifestElements();
                }

                if (buildable == null)
                {
                    if (frameworkVersion == FrameworkVersions.Framework2)
                    {
                        throw new BuildException(String.Format("Framework 2 package Manifest file '{0}' must contain <buildable> element", file));
                    }
                    else
                    {
                        // For Framework 1 buildable is not used. Initialize to any value;
                        buildable = true;
                    }
                }

                manifest = new Manifest(frameworkVersion, (bool)buildable, contactName, contactEmail, summary, description, about, changes,
                                        homePageUrl, status, statusComment, license, licenseComment, documentationUrl, version, compatibility,
                                        owningTeam, sourceLocation, driftVersion, tags, community, isSupported ?? true, platformsInCode,
                                        containsOpenSource, licenseType, internalOnly??false, platforms, builds);

            }
               return manifest;
        }

        private static void ParsePlatforms(XmlNode platformsNode, List<Platform> platforms)
        {
            foreach (XmlNode childNode in platformsNode.ChildNodes)
            {
                if (childNode.NodeType == XmlNodeType.Element && childNode.NamespaceURI.Equals(platformsNode.NamespaceURI))
                {
                    platforms.Add(new Platform(
                           XmlUtil.GetRequiredAttribute(childNode, "name"),
                           ConvertUtil.ToEnum<Platform.SupportType>(XmlUtil.GetOptionalAttribute(childNode, "build", Enum.GetName(typeof(Platform.SupportType), default(Platform.SupportType)))),
                           ConvertUtil.ToEnum<Platform.SupportType>(XmlUtil.GetOptionalAttribute(childNode, "test", Enum.GetName(typeof(Platform.SupportType), default(Platform.SupportType)))),
                           ConvertUtil.ToBooleanOrDefault(XmlUtil.GetOptionalAttribute(childNode, "containsplatformcode"), false),
                           XmlUtil.GetOptionalAttribute(childNode, "defines"),
                           XmlUtil.GetOptionalAttribute(childNode, "configs")
                     ));
                }
            }
        }

        private static void ParseBuilds(XmlNode buildsNode, List<Build> builds)
        {
            foreach (XmlNode childNode in buildsNode.ChildNodes)
            {
                if (childNode.NodeType == XmlNodeType.Element && childNode.NamespaceURI.Equals(buildsNode.NamespaceURI))
                {
                    builds.Add(new Build(
                           XmlUtil.GetRequiredAttribute(childNode, "name"),
                           ConvertUtil.ToBooleanOrDefault(XmlUtil.GetOptionalAttribute(childNode, "supported"), false),
                           XmlUtil.GetOptionalAttribute(childNode, "targets"),
                           XmlUtil.GetOptionalAttribute(childNode, "defines")
                     ));
                }
            }
        }

        private static void Error(XmlNode el, string msg)
        {
            throw new BuildException("Error in manifest file: " + msg, Location.GetLocationFromNode(el));
        }

        private static void Warning(XmlNode el, string msg, Log log)
        {
            msg = "Error in manifest file: " + msg;
            Location loc = Location.GetLocationFromNode(el);
            // only include location string if not empty
            string locationString = loc.ToString();
            if (locationString != String.Empty)
            {
                msg = locationString + " " + msg;
            }
            if (log != null)
            {
                log.Warning.WriteLine(msg);
            }
            else
            {
                Console.WriteLine("[warning] {0}", msg);
            }
        }
    }
}
