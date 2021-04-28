using System;
using System.IO;
using System.Xml;
using System.Collections.Generic;

using NAnt.Core;
using NAnt.Core.Util;
using NAnt.Core.Logging;

namespace NAnt.Core.PackageCore
{
    internal class PackageRootFile
    {
        private const string MagicNumber = "72c39985-ed5b-4f46-869f-0ed6424c726f";
        internal const string PackageRootFileName = "PackageRoot.xml";

        internal readonly PathString MainPackageRoot;
        internal readonly List<PathString> AdditionalPackageRoots = new List<PathString>();

        private PackageRootFile(string mainRoot)
        {
            MainPackageRoot = PathString.MakeNormalized(mainRoot, PathString.PathParam.NormalizeOnly);
            AdditionalPackageRoots = new List<PathString>();
        }

        internal static PackageRootFile Load(string dir, Log log)
        {
            PackageRootFile packagerootFile = null;

            string file = Path.Combine(dir, PackageRootFileName);

            try
            {
                if (File.Exists(file))
                {
                    XmlDocument doc = new XmlDocument();
                    doc.Load(file);

                    if (doc.DocumentElement.Name != "packageRoot")
                    {
                        log.Warning.WriteLine("Expected root element is 'packageRoot', found root '{0}' in file '{1}'.", doc.DocumentElement.Name, file);
                    }

                    XmlNode packageDirsNode = null;
                    string magicNumber = String.Empty;

                    foreach (XmlNode childNode in doc.DocumentElement.ChildNodes)
                    {
                        if (childNode.NodeType == XmlNodeType.Element && childNode.NamespaceURI.Equals(doc.DocumentElement.NamespaceURI))
                        {
                            switch (childNode.Name)
                            {
                                case "magicNumber":
                                    magicNumber = StringUtil.Trim(childNode.InnerText);
                                    break;
                                case "packageDirs":
                                    packageDirsNode = childNode;                                    
                                    break;
                                default:
                                    log.Warning.WriteLine("Unknown Xml element '{0}' in file: '{1}'.", childNode.Name, file);
                                    break;
                            }
                        }
                    }
                    if (MagicNumber == magicNumber )
                    {
                        packagerootFile = new PackageRootFile(dir);                        
                        ParsePackageDirs(packageDirsNode, packagerootFile, file);
                    }
                }
            }
            catch (Exception ex)
            {
                throw new BuildException(String.Format("Failed to load PackageRoots.xml file:'{0}'", file), ex);
                
            }

            return packagerootFile;
        }

        private static void ParsePackageDirs(XmlNode node, PackageRootFile packagerootFile, string file)
        {
            if (node != null)
            {
                foreach (XmlNode childNode in node.ChildNodes)
                {
                    if (childNode.NodeType == XmlNodeType.Element && childNode.NamespaceURI.Equals(node.NamespaceURI))
                    {
                        switch (childNode.Name)
                        {
                            case "packageDir":
                                packagerootFile.AdditionalPackageRoots.Add(PathString.MakeCombinedAndNormalized(packagerootFile.MainPackageRoot.Path, StringUtil.Trim(childNode.InnerText)));
                                break;
                            default:
                                //IMTODO
                                //Log.Warning.WriteLine("Unknown element '{0}' in file: '{1}'.", childNode.Name, file);
                                break;
                        }
                    }
                }
            }
        }

    }
}
