using System;
using System.Reflection;
using System.IO;
using System.Windows.Forms;

using Microsoft.Win32;
using EA.SharpZipLib;
using NAnt.Core.PackageCore;

namespace EA.PackageSystem.PackageInstaller
{

    public class App
    {

        // Singleton pattern 
        // See "Exploring the Singleton Design Pattern" on MSDN for implementation details of Singletons in .NET
        public static readonly App Instance = new App();

        [STAThread()]
        public static void Main(string[] args)
        {
            try
            {
                if (args.Length != 1)
                {
                    throw new Exception("Usage: PackageInstaller <package.eap>");
                }

                if (args[0] == "/install")
                {
                    AddRegistryEntires();
                    return;
                }

                if (args[0] == "/uninstall")
                {
                    RemoveRegistryEntires();
                    return;
                }

                PackageMap.Init(null);

                App.Instance.Run(args[0]);

            }
            catch (Exception e)
            {
                App.Instance.ShowError(e.Message, MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
            }
        }

        MainForm _mainForm = new MainForm();

        public App()
        {
        }

        public MainForm MainForm
        {
            get { return _mainForm; }
        }

        public void Run(string path)
        {
            string name;
            string version;

            ParseEapFile(path, out name, out version);

            MainForm.PackageName = name;
            MainForm.PackageVersion = version;

            System.Windows.Forms.Application.Run(MainForm);
        }


        public DialogResult ShowError(string message, MessageBoxButtons msgBoxButtons, MessageBoxIcon msgBoxIcon)
        {
            return MessageBox.Show(MainForm, message, "Package Installer Error", msgBoxButtons, msgBoxIcon);
        }

        public DialogResult ShowError(string message, MessageBoxButtons msgBoxButtons)
        {
            return MessageBox.Show(MainForm, message, "Package Installer Error", msgBoxButtons);
        }

        public DialogResult ShowError(string message)
        {
            return MessageBox.Show(MainForm, message, "Package Installer Error");
        }

        void ParseEapFile(string path, out string name, out string version)
        {
            if (!File.Exists(path))
            {
                string msg = String.Format("File '{0}' does not exist.  Cannot install package.", path);
                throw new Exception(msg);
            }

            string tempDir = Path.Combine(Path.GetTempPath(), "FrameworkTemp");
            tempDir = Path.Combine(tempDir, String.Format("{0}.eap", Path.GetFileNameWithoutExtension(path)));
            try
            {
                Directory.CreateDirectory(tempDir);

                try
                {
                    ZipLib zipLib = new ZipLib();
                    zipLib.UnzipFile(path, tempDir);
                }
                catch (System.Exception e)
                {
                    throw new Exception("Package install could not unzip eap file.", e);
                }

                string[] files = Directory.GetFiles(tempDir);
                if (files.Length != 1)
                {
                    throw new Exception("Package install file should have a single file inside it.");
                }

                string fullPackageName = Path.GetFileNameWithoutExtension(files[0]);
                Release.ParseFullPackageName(fullPackageName, out name, out version);
            }
            finally
            {
                try
                {
                    Directory.Delete(tempDir, true);
                }
                catch
                {
                }
            }
        }

        const string FileExtension   = ".eap";
        const string RegsitryAppKey  = "EA.Framework.PackageInstaller.1";

        static void AddRegistryEntires()
        {
            try
            {
                // setup the extension stuff
                RegistryKey fileExtensionKey = Registry.ClassesRoot.CreateSubKey(FileExtension);
                fileExtensionKey.SetValue("", RegsitryAppKey);
                fileExtensionKey.SetValue("PerceivedType", "compressed");
                fileExtensionKey.SetValue("Content Type", "application/vnd.ea-package");

                // persistent handler
                RegistryKey persistKey = fileExtensionKey.CreateSubKey("PersistentHandler");
                persistKey.SetValue("", "{098f2470-bae0-11cd-b579-08002b30bfeb}");

                // eap file
                RegistryKey appKey = Registry.ClassesRoot.CreateSubKey(RegsitryAppKey);
                appKey.SetValue("", "EA Package Installation file");
                appKey.SetValue("EditFlags", new byte[] { 0x00, 0x00, 0x01, 0x00 });

                // default icon
                RegistryKey iconKey = appKey.CreateSubKey("DefaultIcon");
                iconKey.SetValue("", Assembly.GetExecutingAssembly().Location + ",1");

                // shell commands
                RegistryKey shellKey = appKey.CreateSubKey("shell");
                shellKey.SetValue("", "Install");

                // shell/install
                RegistryKey installKey = shellKey.CreateSubKey("install");

                // shell/install/command (not sure what this is for)
                RegistryKey commandKey = installKey.CreateSubKey("command");
                commandKey.SetValue("", "\"" + Assembly.GetExecutingAssembly().Location + "\"" + " \"%1\"");
            }
            catch (Exception e)
            {
                MessageBox.Show(e.Message);
            }
        }

        static void RemoveRegistryEntires()
        {
            try
            {
                // extension key
                if (Registry.ClassesRoot.OpenSubKey(FileExtension) != null)
                {
                    Registry.ClassesRoot.DeleteSubKeyTree(FileExtension);
                }

                // application key
                if (Registry.ClassesRoot.OpenSubKey(RegsitryAppKey) != null)
                {
                    Registry.ClassesRoot.DeleteSubKeyTree(RegsitryAppKey);
                }
            }
            catch (Exception e)
            {
                MessageBox.Show(e.Message);
            }
        }
    }
}
