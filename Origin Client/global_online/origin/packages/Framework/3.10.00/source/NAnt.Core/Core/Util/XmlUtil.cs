using System;
using System.Xml;

using NAnt.Core;
using NAnt.Core.Logging;

namespace NAnt.Core.Util
{
    public static class XmlUtil
    {

        static public string GetRequiredAttribute(XmlNode el, string name)
        {
            XmlAttribute attr = el.Attributes[name];

            if (attr == null)
            {
                Error(el, String.Format("Xml element '{0}' is missing required attribute '{1}'.", el.Name, name));
            }
            if (String.IsNullOrEmpty(attr.Value))
            {
                Error(el, String.Format("required attribute '{0}' in Xml element '{1}' is defined but has no value.", name, el.Name));
            }
            return attr.Value.TrimWhiteSpace();
        }

        static public string GetOptionalAttribute(XmlNode el, string name, string defaultVal = null)
        {
            XmlAttribute attr = el.Attributes[name];

            if (attr != null)
            {

                if (!String.IsNullOrEmpty(attr.Value))
                {
                    return attr.Value.TrimWhiteSpace();
                }
                else
                {
                    return String.Empty;
                    //Warning(el, String.Format("optinal attribute '{0}' in Xml element '{1}' is defined but has no value. Ignoring this attribute", name, el.Name));
                }
            }
            return defaultVal;
        }


        static public void Error(XmlNode el, string msg)
        {
            throw new BuildException(msg, Location.GetLocationFromNode(el));
        }

        static public void Warning(XmlNode el, string msg, Log log)
        {            
            Location loc = Location.GetLocationFromNode(el);
            // only include location string if not empty
            string locationString = loc.ToString();
            if (locationString != String.Empty)
            {
                msg = locationString + " " + msg;
            }
            if (log != null)
            {
                log.Warning.WriteLine(msg);
            }
            else
            {
                Console.WriteLine(msg);
            }
        }

    }
}
