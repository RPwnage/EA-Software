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
    [ElementName("VisualStudio", StrictAttributeCheck = true)]
    public class VisualStudioDataElement : Element
    {
        [NAnt.Core.Attributes.Description("")]
        [Property("pregenerate-target", Required = false)]
        public BuildTargetElement PregenerateTarget
        {
            get { return _pregenerateTarget; }
        }private BuildTargetElement _pregenerateTarget = new BuildTargetElement("vcproj.prebuildtarget");

        [NAnt.Core.Attributes.Description("")]
        [FileSet("excludedbuildfiles", Required = false)]
        public StructuredFileSet ExcludedBuildFiles
        {
            get { return _excludedbuildfiles; }
            set { _excludedbuildfiles = value; }
        }private StructuredFileSet _excludedbuildfiles = new StructuredFileSet();

        [NAnt.Core.Attributes.Description("")]
        [Property("pre-build-step", Required = false)]
        public ConditionalPropertyElement PreBuildStep
        {
            get { return _prebuildstep; }
            set { _prebuildstep = value; }
        }private ConditionalPropertyElement _prebuildstep = new ConditionalPropertyElement();

        [NAnt.Core.Attributes.Description("")]
        [Property("post-build-step", Required = false)]
        public ConditionalPropertyElement PostBuildStep
        {
            get { return _postbuildstep; }
            set { _postbuildstep = value; }
        }private ConditionalPropertyElement _postbuildstep = new ConditionalPropertyElement();

        [NAnt.Core.Attributes.Description("")]
        [Property("vcproj", Required = false)]
        public VcprojDataElement VcProj
        {
            get { return _vcproj; }
        }private VcprojDataElement _vcproj = new VcprojDataElement();

        [NAnt.Core.Attributes.Description("")]
        [Property("csproj", Required = false)]
        public CsprojDataElement CsProj
        {
            get { return _csproj; }
        }private CsprojDataElement _csproj = new CsprojDataElement();
    }

    [NAnt.Core.Attributes.Description("")]
    [NAnt.Core.Attributes.XmlSchema]
    [ElementName("Vcproj", StrictAttributeCheck = true)]
    public class VcprojDataElement : Element
    {
        [NAnt.Core.Attributes.Description("")]
        [Property("pre-link-step", Required = false)]
        public ConditionalPropertyElement PreLinkStep
        {
            get { return _prelinkstep; }
            set { _prelinkstep = value; }
        }private ConditionalPropertyElement _prelinkstep = new ConditionalPropertyElement();

        [NAnt.Core.Attributes.Description("")]
        [FileSet("input-resource-manifests", Required = false)]
        public StructuredFileSet InputResourceManifests
        {
            get { return _inputResourceManifests; }
            set { _inputResourceManifests = value; }
        }private StructuredFileSet _inputResourceManifests = new StructuredFileSet();

        [NAnt.Core.Attributes.Description("")]
        [FileSet("additional-manifest-files", Required = false)]
        public StructuredFileSet AdditionalManifestFiles
        {
            get { return _additionalManifestFiles; }
            set { _additionalManifestFiles = value; }
        }private StructuredFileSet _additionalManifestFiles = new StructuredFileSet();
    }

    [NAnt.Core.Attributes.Description("")]
    [NAnt.Core.Attributes.XmlSchema]
    [ElementName("Csproj", StrictAttributeCheck = true)]
    public class CsprojDataElement : Element
    {
        [NAnt.Core.Attributes.Description("")]
        [Property("link.resourcefiles.nonembed", Required = false)]
        public ConditionalPropertyElement LinkNonembeddedResources
        {
            get { return _linknonembeddedresources; }
            set { _linknonembeddedresources = value; }
        }private ConditionalPropertyElement _linknonembeddedresources = new ConditionalPropertyElement();

        //additional-manifest-files
    }
}
/*
E:\packages\eaconfig\dev\release_notes.txt(381):                 ${groupname}.vcproj.additional-manifest-files
  E:\packages\eaconfig\dev\release_notes.txt(382):                 ${groupname}.vcproj.additional-manifest-files.${config-system}
  E:\packages\eaconfig\dev\release_notes.txt(384):                 ${groupname}.vcproj.input-resource-manifests
  E:\packages\eaconfig\dev\release_notes.txt(385):                 ${groupname}.vcproj.input-resource-manifests.${config-system}
  E:\packages\eaconfig\dev\release_notes.txt(738):                 group.<module name>.vcproj.excludedbuildfiles
  E:\packages\eaconfig\dev\release_notes.txt(739):                 group.<module>.vcproj.excludedbuildfiles.${config-system}
 * 
 * vcproj.input-resource-manifests
 * 
 * E:\packages\eaconfig\dev\source\targets\EaconfigVcproj.cs(397):            FileSetUtil.CreateFileSetInProjectIfMissing(Project, PropGroupName("vcproj.custom-build-dependencies"));
  E:\packages\eaconfig\dev\source\targets\EaconfigVcproj.cs(398):            FileSetUtil.CreateFileSetInProjectIfMissing(Project, PropGroupName("vcproj.custom-build-rules"));
 * 
          FileSetUtil.CreateFileSetInProjectIfMissing(Project, PropGroupName("vcproj.custom-build-dependencies"));
  E:\packages\eaconfig\dev\source\targets\EaconfigVcproj.cs(398):            FileSetUtil.CreateFileSetInProjectIfMissing(Project, PropGroupName("vcproj.custom-build-rules"));
  E:\packages\eaconfig\dev\source\targets\EaconfigVcproj.cs(447):            BuildData.BuildProperties["custom-build-tool"] = PropertyUtil.GetPropertyOrDefault(Project, PropGroupName("vcproj.custom-build-tool"), String.Empty);
  E:\packages\eaconfig\dev\source\targets\EaconfigVcproj.cs(448):            BuildData.BuildProperties["custom-build-outputs"] = StringUtil.EnsureSeparator(PropertyUtil.GetPropertyOrDefault(Project, PropGroupName("vcproj.custom-build-outputs"), String.Empty), ';');
  E:\packages\eaconfig\dev\source\targets\EaconfigVcproj.cs(455):            BuildData.BuildProperties["pre-build-step"] = Properties[PropGroupName("vcproj.pre-build-step")];
  E:\packages\eaconfig\dev\source\targets\EaconfigVcproj.cs(456):            BuildData.BuildProperties["pre-link-step"] = Properties[PropGroupName("vcproj.pre-link-step")];
 * 
 *  To control whether to link or not non embedded resources use property:
          
                    csproj.link.resourcefiles.nonembed 
                    
          Default value is true.
*/
