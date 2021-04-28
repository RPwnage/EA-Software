using System;
using System.IO;
using System.Windows.Forms;
using System.Xml;

namespace EA.Common.CommandBars
{

    public class CommandMenuItem : MenuItem
    {

        // helper function
        static string GetAttributeValue(XmlNode node, string attributeName)
        {
            XmlNode attribute = node.Attributes[attributeName];
            if (attribute != null)
            {
                return attribute.Value;
            }
            return "";
        }

        string _name;

        public CommandMenuItem(XmlNode node, EventHandler clickHandler)
        {

            Name = GetAttributeValue(node, "name");
            Text = GetAttributeValue(node, "text").Replace('_', '&');

            string shortcut = GetAttributeValue(node, "shortcut");
            if (shortcut != "")
            {
                Shortcut = (Shortcut)Enum.Parse(Shortcut.GetType(), shortcut);
            }

            if (clickHandler != null)
            {
                this.Click += clickHandler;
            }

            foreach (XmlNode childNode in node.SelectNodes("commandItem"))
            {
                CommandMenuItem item = new CommandMenuItem(childNode, clickHandler);
                MenuItems.Add(item);
            }
        }

        new public string Name
        {
            get { return _name; }
            set { _name = value; }
        }
    }
}