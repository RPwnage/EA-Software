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
    /// <summary></summary>
    [ElementName("VisualStudio", StrictAttributeCheck = true)]
    public class VisualStudioDataElement : Element
    {
        /// <summary></summary>
        [Property("pregenerate-target", Required = false)]
        public BuildTargetElement PregenerateTarget
        {
            get { return _pregenerateTarget; }
        }private BuildTargetElement _pregenerateTarget = new BuildTargetElement("vcproj.prebuildtarget");

        /// <summary>A list of files that are excluded from the build but are added to the visual studio
        /// project as non-buildable files</summary>
        [FileSet("excludedbuildfiles", Required = false)]
        public StructuredFileSet ExcludedBuildFiles
        {
            get { return _excludedbuildfiles; }
            set { _excludedbuildfiles = value; }
        }private StructuredFileSet _excludedbuildfiles = new StructuredFileSet();

        /// <summary></summary>
        [Property("pre-build-step", Required = false)]
        public ConditionalPropertyElement PreBuildStep
        {
            get { return _prebuildstep; }
            set { _prebuildstep = value; }
        }private ConditionalPropertyElement _prebuildstep = new ConditionalPropertyElement();

        /// <summary></summary>
        [Property("post-build-step", Required = false)]
        public ConditionalPropertyElement PostBuildStep
        {
            get { return _postbuildstep; }
            set { _postbuildstep = value; }
        }private ConditionalPropertyElement _postbuildstep = new ConditionalPropertyElement();

        /// <summary></summary>
        [Property("vcproj", Required = false)]
        public VcprojDataElement VcProj
        {
            get { return _vcproj; }
        }private VcprojDataElement _vcproj = new VcprojDataElement();

        /// <summary></summary>
        [Property("csproj", Required = false)]
        public CsprojDataElement CsProj
        {
            get { return _csproj; }
        }private CsprojDataElement _csproj = new CsprojDataElement();
    }

    /// <summary></summary>
    [ElementName("Vcproj", StrictAttributeCheck = true)]
    public class VcprojDataElement : Element
    {
        /// <summary></summary>
        [Property("pre-link-step", Required = false)]
        public ConditionalPropertyElement PreLinkStep
        {
            get { return _prelinkstep; }
            set { _prelinkstep = value; }
        }private ConditionalPropertyElement _prelinkstep = new ConditionalPropertyElement();

        /// <summary></summary>
        [FileSet("input-resource-manifests", Required = false)]
        public StructuredFileSet InputResourceManifests
        {
            get { return _inputResourceManifests; }
            set { _inputResourceManifests = value; }
        }private StructuredFileSet _inputResourceManifests = new StructuredFileSet();

        /// <summary></summary>
        [FileSet("additional-manifest-files", Required = false)]
        public StructuredFileSet AdditionalManifestFiles
        {
            get { return _additionalManifestFiles; }
            set { _additionalManifestFiles = value; }
        }private StructuredFileSet _additionalManifestFiles = new StructuredFileSet();
    }

    /// <summary></summary>
    [ElementName("Csproj", StrictAttributeCheck = true)]
    public class CsprojDataElement : Element
    {
        /// <summary></summary>
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
