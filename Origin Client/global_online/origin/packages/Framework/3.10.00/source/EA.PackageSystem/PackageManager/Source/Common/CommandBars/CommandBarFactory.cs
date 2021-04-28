using System;
using System.IO;
using System.Reflection;
using System.Windows.Forms;
using System.Xml;

namespace EA.Common.CommandBars {

    public class CommandBarFactory {

        // Singleton pattern 
        // See "Exploring the Singleton Design Pattern" on MSDN for implementation details of Singletons in .NET
        public static readonly CommandBarFactory Instance = new CommandBarFactory();

        public MainMenu CreateMainMenuFromResource(string resourceName, string mainMenuName, EventHandler clickHandler) {
            // load command bar configuration file from the named resource
            XmlDocument doc = new XmlDocument();
            using (Stream f = Assembly.GetCallingAssembly().GetManifestResourceStream(resourceName)) {
                doc.Load(f);
                f.Close();
            }

            // find the node specified by the user
            string xpath = String.Format("/commandBars/mainMenu[@name='{0}']", mainMenuName);
            return CreateMainMenu(doc.DocumentElement.SelectSingleNode(xpath), clickHandler);
        }

        MainMenu CreateMainMenu(XmlNode mainMenuNode, EventHandler clickHandler) {
            MainMenu mainMenu = new MainMenu();

            foreach (XmlNode childNode in mainMenuNode.SelectNodes("commandItem")) {
                CommandMenuItem item = new CommandMenuItem(childNode, clickHandler);
                mainMenu.MenuItems.Add(item);
            }
            return mainMenu;
        }
    }
}