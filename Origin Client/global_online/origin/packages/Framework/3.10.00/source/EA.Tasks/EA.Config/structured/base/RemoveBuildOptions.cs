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
    /// <summary>Sets options to be removed from final configuration</summary>
    [ElementName("remove", StrictAttributeCheck = true)]
    public class RemoveBuildOptions : Element
    {
        /// <summary>Defines to be removed</summary>
        [Property("defines", Required = false)]
        public ConditionalPropertyElement Defines
        {
            get { return _defines; }
        }private ConditionalPropertyElement _defines = new ConditionalPropertyElement();

        /// <summary>C/CPP compiler options to be removed</summary>
        [Property("cc.options", Required = false)]
        public ConditionalPropertyElement CcOptions
        {
            get { return _ccoptions; }
        }private ConditionalPropertyElement _ccoptions = new ConditionalPropertyElement();

        /// <summary>Assembly compiler options to be removed</summary>
        [Property("as.options", Required = false)]
        public ConditionalPropertyElement AsmOptions
        {
            get { return _asmoptions; }
        }private ConditionalPropertyElement _asmoptions = new ConditionalPropertyElement();

        /// <summary>Linker options to be removed</summary>
        [Property("link.options", Required = false)]
        public ConditionalPropertyElement LinkOptions
        {
            get { return _linkoptions; }
        }private ConditionalPropertyElement _linkoptions = new ConditionalPropertyElement();

        /// <summary>librarian options to be removed</summary>
        [Property("lib.options", Required = false)]
        public ConditionalPropertyElement LibOptions
        {
            get { return _liboptions; }
        }private ConditionalPropertyElement _liboptions = new ConditionalPropertyElement();
    }

}
