using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Tasks;
using NAnt.Core.Logging;
using EA.FrameworkTasks;

using System;
using System.Collections;
using System.Collections.Generic;
using System.Text;
using System.Xml;

namespace EA.Eaconfig.Structured
{
	internal class InMemoryLineInfoElement : XmlElement, IXmlLineInfo {
		int _lineNumber = 0 ;
		int _linePosition = 0 ;

		internal InMemoryLineInfoElement( string prefix, string localname, string nsURI, XmlDocument doc ) : base( prefix, localname, nsURI, doc ) {
		}

		public void SetLineInfo( int linenum, int linepos ) {
			_lineNumber = linenum;
			_linePosition = linepos;
		}
		public int LineNumber {
			get {
				return _lineNumber;
			}
		}
		public int LinePosition {
			get {
				return _linePosition;
			}
		}
		public bool HasLineInfo() { return _lineNumber != 0 && _linePosition != 0 ; }

        public override string BaseURI
        {
            get
            {
                string baseUri = base.BaseURI;
                if (String.IsNullOrEmpty(baseUri) && OwnerDocument != null)
                {
                    baseUri = OwnerDocument.BaseURI;
                }
                return baseUri;
            }
        }

		public override XmlNode CloneNode (bool deep)
        {
            XmlElement cloned = OwnerDocument.CreateElement(Prefix, LocalName, NamespaceURI);

            InMemoryLineInfoElement clonedLine = cloned as InMemoryLineInfoElement;			
            if (clonedLine != null)
            {                
                clonedLine.SetLineInfo(_lineNumber,_linePosition);
            }
			
 
            for (int i = 0; i < Attributes.Count; i++)
			{
                cloned.SetAttributeNode ((XmlAttribute)Attributes [i].CloneNode (true));
			}
 
            if (deep) 
			{
               	for (int i = 0; i < ChildNodes.Count; i++)
				{
                   	cloned.AppendChild (ChildNodes [i].CloneNode (true));
               	}
			}
 
            return cloned;
        }		
		
	}

	internal class InMemoryLineInfoXmlDoc : XmlDocument {

        private Location _location;

        internal InMemoryLineInfoXmlDoc(Location location) : base()
        {
            _location = location;
        }

		public override XmlElement CreateElement( string prefix, string localname, string namespaceuri ) {
            InMemoryLineInfoElement elt = new InMemoryLineInfoElement(prefix, localname, namespaceuri, this);

            if (_location != null && _location != Location.UnknownLocation)
                elt.SetLineInfo(_location.LineNumber, _location.ColumnNumber);

			return elt;
		}
		
		public override XmlNode CreateNode( string prefix, string localname, string namespaceuri ) {
			return base.CreateNode(prefix,localname, namespaceuri);
		}
		

	}


}
