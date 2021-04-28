using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Tasks;
using NAnt.Core.Reflection;
using NAnt.Core.Threading;
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
    [ElementName("StructuredOptionSet", StrictAttributeCheck = true)]
    public class StructuredOptionSet : OptionSet
    {
        public StructuredOptionSet() : base() { }

        public StructuredOptionSet(StructuredOptionSet source)
        {
            _append = source.AppendBase;
        }

        public StructuredOptionSet(OptionSet source)
        {
            base.Append(source);
        }

        public StructuredOptionSet(OptionSet source, bool append)
            :   this(source)
        {
            _append = append;
        }

        [TaskAttribute("append", Required = false)]
        public bool AppendBase
        {
            get { return _append; }
            set { _append = value; }
        } private bool _append = true;

        /// <summary>Optimization. Directly intialize</summary>
        public override void Initialize(XmlNode elementNode)
        {
            if (Project == null)
            {
                throw new InvalidOperationException("OptionSet Element has invalid (null) Project property.");
            }

            // Save position in buildfile for reporting useful error messages.
            if (!String.IsNullOrEmpty(elementNode.BaseURI))
            {
                try
                {
                    Location = Location.GetLocationFromNode(elementNode);
                }
                catch (ArgumentException ae)
                {
                    Log.Warning.WriteLine("Can't find node '{0}' location, file: '{1}'{2}{3}", elementNode.Name, elementNode.BaseURI, Environment.NewLine, ae.ToString());
                }

            }
            try
            {
                foreach (XmlAttribute attr in elementNode.Attributes)
                {
                    switch (attr.Name)
                    {
                        case "fromoptionset":
                            FromOptionSetName = attr.Value;
                            break;
                        case "append":
                            _append = ElementInitHelper.InitBoolAttribute(this, attr.Name, attr.Value);
                            break;

                        default:
                            string msg = String.Format("Unknown attribute '{0}'='{1}' in OptionSet element", attr.Name, attr.Value);
                            throw new BuildException(msg, Location);
                    }
                }

                InitializeElement(elementNode);

            }
            catch (BuildException e)
            {
                if (e.Location == Location.UnknownLocation)
                {
                    e = new BuildException(e.Message, this.Location, e.InnerException);
                }
                throw new ContextCarryingException(e, Name);
            }
            catch (System.Exception e)
            {
                Exception ex = ThreadUtil.ProcessAggregateException(e);

                if (ex is BuildException)
                {
                    BuildException be = ex as BuildException;
                    if (be.Location == Location.UnknownLocation)
                    {
                        be = new BuildException(be.Message, Location, be.InnerException, be.StackTraceFlags);
                    }
                    throw new ContextCarryingException(be, Name);
                }

                throw new ContextCarryingException(ex, Name, Location);
            }
        }

    }

}
