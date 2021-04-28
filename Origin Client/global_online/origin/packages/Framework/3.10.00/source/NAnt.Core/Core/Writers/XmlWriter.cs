using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Xml;

namespace NAnt.Core.Writers
{
    public class XmlWriter : CachedWriter, IXmlWriter
    {
        #region Internal Constructor

        public XmlWriter(IXmlWriterFormat format)
            : base()
        {
#if FRAMEWORK_PARALLEL_TRANSITION
            XmlTextWriter writer = new System.Xml.XmlTextWriter(Internal, Encoding.UTF8);
            writer.Formatting = System.Xml.Formatting.Indented;
            writer.IndentChar = format.IndentChar;
            writer.Indentation = format.Indentation;
            _XmlWriterImpl = writer;
#else
            XmlWriterSettings writerSettings = new XmlWriterSettings();
            writerSettings.Encoding = format.Encoding;
            writerSettings.Indent = true;
            writerSettings.IndentChars = new string(format.IndentChar, format.Indentation);
            writerSettings.NewLineOnAttributes = format.NewLineOnAttributes;
            writerSettings.CloseOutput = true;

            _XmlWriterImpl = XmlTextWriter.Create(Internal, writerSettings);
#endif
        }
        #endregion Internal Constructor

        #region Implementation of IXmlWriter

        virtual public void WriteStartElement(string localName)
        {
            _XmlWriterImpl.WriteStartElement(localName);
        }

        virtual public void WriteStartElement(string localName, string ns)
        {
            _XmlWriterImpl.WriteStartElement(localName, ns);
        }

        virtual public void WriteAttributeString(string localName, string value)
        {
            _XmlWriterImpl.WriteAttributeString(localName, value);
        }

        virtual public void WriteAttributeString(string prefix, string localName, string ns, string value)
        {
            _XmlWriterImpl.WriteAttributeString(prefix, localName, ns, value);
        }

        virtual public void WriteAttributeString(string localName, bool value)
        {
            _XmlWriterImpl.WriteAttributeString(localName, value.ToString().ToLowerInvariant());
        }


        virtual public void WriteEndElement()
        {
            _XmlWriterImpl.WriteEndElement();
        }

        virtual public void WriteFullEndElement()
        {
            _XmlWriterImpl.WriteFullEndElement();
        }

        virtual public void WriteElementString(string name, string value)
        {
            _XmlWriterImpl.WriteElementString(name, value);
        }

        virtual public void WriteElementString(string prefix, string localName, string ns, string value)
        {
            _XmlWriterImpl.WriteElementString(prefix, localName, ns, value);
        }

        virtual public void WriteElementString(string name, bool value)
        {
            _XmlWriterImpl.WriteElementString(name, value.ToString().ToLowerInvariant());
        }

        virtual public void WriteString(string text)
        {
            _XmlWriterImpl.WriteString(text);
        }

        virtual public void WriteStartDocument()
        {
            _XmlWriterImpl.WriteStartDocument();
        }

        public void Close()
        {
            _XmlWriterImpl.Close();
        }

        #endregion Implementation of IXmlWriter

        #region Protected Methods
        protected override void DisposeResources()
        {
            _XmlWriterImpl.Close();
        }
        #endregion Protected Methods

        #region Private Fields
        private System.Xml.XmlWriter _XmlWriterImpl;
        #endregion Private Fields
    }
}
