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
using System.Text;


namespace EA.Eaconfig.Structured
{
    /// <summary></summary>
    [ElementName("dependencies", StrictAttributeCheck = true)]
    public class DependenciesPropertyElement : Element
    {
        /// <summary>Sets auto build/use dependencies</summary>
        [Property("auto", Required = false)]
        public DependencyDefinitionPropertyElement Dependencies
        {
            get { return _dependencies; }
            set { _dependencies = value; }
        }private DependencyDefinitionPropertyElement _dependencies = new DependencyDefinitionPropertyElement(isinterface: true, islink: true);

        /// <summary>Sets the dependencies to be used by the package</summary>
        [Property("use", Required = false)]
        public DependencyDefinitionPropertyElement UseDependencies
        {
            get { return _useDependencies; }
            set { _useDependencies = value; }
        }private DependencyDefinitionPropertyElement _useDependencies = new DependencyDefinitionPropertyElement(isinterface:true, islink:true);

        /// <summary>Sets the dependencies to be built by the package</summary>
        [Property("build", Required = false)]
        public DependencyDefinitionPropertyElement BuildDependencies
        {
            get { return _buildDependencies; }
            set { _buildDependencies = value; }
        }private DependencyDefinitionPropertyElement _buildDependencies = new DependencyDefinitionPropertyElement(isinterface:true, islink:true);

        /// <summary></summary>
        [Property("interface", Required = false)]
        public DependencyDefinitionPropertyElement InterfaceDependencies
        {
            get { return _interfaceDependencies; }
            set { _interfaceDependencies = value; }
        }private DependencyDefinitionPropertyElement _interfaceDependencies = new DependencyDefinitionPropertyElement(isinterface: true, islink: false);

        /// <summary>Sets or gets the dependencies that needs to be used during the linking task for this package</summary>
        [Property("link", Required = false)]
        public DependencyDefinitionPropertyElement LinkDependencies
        {
            get { return _linkDependencies; }
            set { _linkDependencies = value; }
        }private DependencyDefinitionPropertyElement _linkDependencies = new DependencyDefinitionPropertyElement(isinterface: false, islink: true);

    }

    /// <summary></summary>
    [ElementName("conditionalproperty", StrictAttributeCheck = true, Mixed = true)]
    public class DependencyDefinitionPropertyElement : ConditionalPropertyElement
    {
        internal DependencyDefinitionPropertyElement(bool isinterface=true, bool islink=true)
        {
            _isInterface = _isInterfaceInit = isinterface;
            _isLink = _isLinkInit = islink;
            _copyLocalValue = new StringBuilder();
        }

        /// <summary>Public include directories from dependent packages are added.</summary>
        [TaskAttribute("interface", Required = false)]
        public bool IsInterface
        {
            get { return _isInterface; }
            set { _isInterface = value; }
        } private bool _isInterface;

        /// <summary>Public librariess from dependent packages are addedif this attrubute is true.</summary>
        [TaskAttribute("link", Required = false)]
        public bool IsLink
        {
            get { return _isLink; }
            set { _isLink = value; }
        } private bool _isLink;

        /// <summary>Set copy local flag for this dependency output.</summary>
        [TaskAttribute("copylocal", Required = false)]
        public bool IsCopyLocal
        {
            get { return _isCopyLocal; }
            set { _isCopyLocal = value; }
        } private bool _isCopyLocal;


        public override string Value
        {
            set { SelectedValue = value; }
            get { return SelectedValue; }
        }

        public string CopyLocalValue
        {
            get { return _copyLocalValue.ToString();  }
        }

        public override void Initialize(XmlNode elementNode)
        {
            _isCopyLocal = false;
            _isInterface = _isInterfaceInit;
            _isLink = _isLinkInit;

            base.Initialize(elementNode);
        }


        protected override void InitializeElement(XmlNode node)
        {

            if (IfDefined && !UnlessDefined)
            {
                _oldValue = Value;
                
                base.InitializeElement(node);
            }
        }

        private string SelectedValue
        {
            get
            {
                if (_isLink && _isInterface)
                {
                    return _interface_link_Value;
                }
                else if (_isLink && !_isInterface)
                {
                    return _linkValue;
                }
                else if (!_isLink && _isInterface)
                {
                    return _interfaceValue;
                }
                return _Value;
            }

            set
            {
                if (_isLink && _isInterface)
                {
                    _interface_link_Value = value;
                }
                else if (_isLink && !_isInterface)
                {
                    _linkValue = value;
                }
                else if (!_isLink && _isInterface)
                {
                    _interfaceValue = value;
                }
                _Value = value;

                if (_isCopyLocal)
                {

                    var val = value.TrimWhiteSpace();

                    if (!String.IsNullOrEmpty(val))
                    {

                        if (!String.IsNullOrEmpty(_oldValue))
                        {
                            if (_oldValue.Length < val.Length)
                            {
                                val = val.Substring(_oldValue.Length);
                            }
                        }
                        _copyLocalValue.AppendLine(val);
                    }
                }
            }
        }

        internal string Interface_0_Link_0 { get { return _Value; } }
        internal string Interface_1_Link_1 { get { return _interface_link_Value; } }
        internal string Interface_1_Link_0 { get { return _interfaceValue; } }
        internal string Interface_0_Link_1 { get { return _linkValue; } }


        //private string _Value                     // - interface = 0, link = 0;
        private string _interface_link_Value;       // - interface = 1, link = 1;
        private string _interfaceValue;             // - interface = 1, link = 0;
        private string _linkValue;                  // - interface = 0, link = 1;
        private StringBuilder _copyLocalValue;
        private string _oldValue;

        private readonly bool _isInterfaceInit;
        private readonly bool _isLinkInit;
    }


}
