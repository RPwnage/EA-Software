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
    [NAnt.Core.Attributes.Description("")]
    [NAnt.Core.Attributes.XmlSchema]
    [ElementName("packaging", StrictAttributeCheck = true)]
    public class PackagingElement : Element
    {
        private ModuleBaseTask _module;

        public PackagingElement(ModuleBaseTask module) : base()
        {
            _module = module;
        }

        [NAnt.Core.Attributes.Description("Sets or gets the package name")]
        [TaskAttribute("packagename", Required = false)]
        public string PackageName
        {
            get { return _packageName; }
            set { _packageName = value; }
        } private string _packageName;


        [NAnt.Core.Attributes.Description("Sets of gets the asset files")]
        [Property("assetfiles", Required = false)]
        public StructuredFileSetCollection Assets
        {
            get { return _assets; }
        }private StructuredFileSetCollection _assets = new StructuredFileSetCollection();

        [NAnt.Core.Attributes.Description("Sets or gets the asset dependencies")]
        [Property("assetdependencies", Required = false)]
        public ConditionalPropertyElement AssetDependencies
        {
            get { return _assetDependencies; }
            set
            {
                _assetDependencies = value;
            }
        }private ConditionalPropertyElement _assetDependencies = new ConditionalPropertyElement();

        [NAnt.Core.Attributes.Description("Gets the manifest file")]
        [FileSet("manifestfile", Required = false)]
        public StructuredFileSet ManifestFile
        {
            get { return _manifestfile; }

        }private StructuredFileSet _manifestfile = new StructuredFileSet();


    }

}
