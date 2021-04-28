using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Tasks;
using EA.FrameworkTasks;

using System;
using System.Reflection;
using System.Collections;
using System.Collections.Specialized;
using System.Collections.Generic;
using System.Xml;


namespace EA.Eaconfig.Structured
{
    /// <summary>PackagingElement</summary>
    [ElementName("packaging", StrictAttributeCheck = true)]
    public class PackagingElement : Element
    {
        public PackagingElement(ModuleBaseTask module) : base()
        {
        }

        /// <summary>Sets or gets the package name</summary>
        [TaskAttribute("packagename", Required = false)]
        public string PackageName
        {
            get { return _packageName; }
            set { _packageName = value; }
        } private string _packageName;

        /// <summary>Sets of gets the asset files</summary>
        [Property("assetfiles", Required = false)]
        public StructuredFileSetCollection Assets
        {
            get { return _assets; }
        }private StructuredFileSetCollection _assets = new StructuredFileSetCollection();

        /// <summary>Sets or gets the asset dependencies</summary>
        [Property("assetdependencies", Required = false)]
        public ConditionalPropertyElement AssetDependencies
        {
            get { return _assetDependencies; }
            set
            {
                _assetDependencies = value;
            }
        }private ConditionalPropertyElement _assetDependencies = new ConditionalPropertyElement();

        /// <summary>Gets the manifest file</summary>
        [FileSet("manifestfile", Required = false)]
        public StructuredFileSet ManifestFile
        {
            get { return _manifestfile; }

        }private StructuredFileSet _manifestfile = new StructuredFileSet();


    }

}
