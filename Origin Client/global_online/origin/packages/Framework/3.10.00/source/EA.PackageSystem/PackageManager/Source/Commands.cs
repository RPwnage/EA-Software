using System;
using System.Linq;
using System.Diagnostics;
using System.IO;
using System.Text;
using System.Windows.Forms;

using EA.Common;
using EA.Common.CommandHandling;
using EA.PackageSystem.PackageManager;
using NAnt.Core;
using NAnt.Core.PackageCore;
using NAnt.Core.Util;

namespace EA.PackageSystem.PackageManager.Commands
{

    [Command("File.Exit")]
    public class ExitCommand : Command
    {
        public override void Do()
        {
            Application.Exit();
        }
    }

    [Command("Package.Delete")]
    public class DeleteCommand : Command
    {
        public override void Do()
        {
            StringBuilder msg = new StringBuilder("Are you sure you want to delete:\n");
            Release r = PackageMap.Instance.GetFrameworkRelease();
            foreach (Release release in App.Instance.MainForm.GetSelectedReleases())
            {
                msg.AppendFormat("* {0}\n", release.FullName);
            }
            msg.Append("\nTHIS OPERATION CANNOT BE UNDONE.");

            if (MessageBox.Show(App.Instance.MainForm, msg.ToString(), "Delete Package", MessageBoxButtons.OKCancel) == DialogResult.OK)
            {
                using (WaitStatus status = new WaitStatus(App.Instance.MainForm.BackgroundActionPanel))
                {
                    foreach (Release release in App.Instance.MainForm.GetSelectedReleases())
                    {
                        if (r != null)
                        {
                            if (release.Name != r.Name || release.Version != r.Version)
                            {
                                status.Status = String.Format("Deleting {0}", release.FullName);
                                release.Delete();
                            }
                            else
                            {
                                MessageBox.Show("PackageManager is a part of " + r.FullName + " and hence can not uninstall " + r.FullName + ". If you would like to uninstall " + r.FullName + ", please exit PackageManager and use the 'Uninstall' option in the Start Menu under " + r.FullName + "!");
                            }
                        }
                        else
                        {
                            status.Status = String.Format("Deleting {0}", release.FullName);
                            release.Delete();
                        }
                    }
                }
                App.Instance.MainForm.RefreshAll();
            }
        }
    }

    [Command("Edit.SelectAll")]
    public class EditSelectAll : Command
    {
        public override void Do()
        {
            App.Instance.MainForm.SelectAllReleases();
        }
    }

    [Command("View.Refresh")]
    public class ViewRefreshCommand : Command
    {
        public override void Do()
        {
            Cursor.Current = Cursors.WaitCursor;
            App.Instance.MainForm.RefreshAll();
            Cursor.Current = Cursors.Arrow;
        }
    }

    [Command("Package.BrowsePackageRoot")]
    public class BrowsePackageRootCommand : Command
    {
        public override void Do()
        {
            string path = PackageMap.Instance.PackageRoots.ToString();
            try
            {
                Process.Start(path);
            }
            catch (Exception e)
            {
                string msg = String.Format("Could not browse '{0}' because:\n{1}", path, e.Message);
                throw new BuildException(msg);
            }
        }
    }

    [Command("Package.Browse")]
    public class BrowseCommand : Command
    {
        public override void Do()
        {
            Release release = App.Instance.MainForm.GetFocusedRelease();
            if (release != null)
            {
                string path = release.Path;
                try
                {
                    Process.Start(path);
                }
                catch (Exception e)
                {
                    string msg = String.Format("Could not browse '{0}' because:\n{1}", path, e.Message);
                    throw new BuildException(msg);
                }
            }
        }
    }

    [Command("Package.Update")]
    public class UpdateCommand : Command
    {
        public override void Do()
        {
            App.Instance.ShowError("TODO: Update Command");
        }
    }

    [Command("Package.SetPackageRoot")]
    public class SetPackageRoot : Command
    {
        public override void Do()
        {
            BrowseInfo bi = new BrowseInfo();
            bi.hwndOwner = IntPtr.Zero;
            bi.lpszTitle = "Change Packages Directory";
            bi.ulFlags = BrowseInfo.BIF_USENEWUI | BrowseInfo.BIF_VALIDATE | BrowseInfo.BIF_NONEWFOLDERBUTTON;

            OpenFolderDialog folder = new OpenFolderDialog(bi);
            if (folder.ShowDialog() == DialogResult.OK)
            {
                DirectoryInfo dirInfo = null;
                try
                {
                    dirInfo = new DirectoryInfo(folder.FolderName);
                }
                catch { }

                if (dirInfo == null) // the isPackageRoot requirement is removed because we no longer need to check for PackageRoot.xml
                {
                    App.Instance.ShowError("Invalid Packages Directory.", MessageBoxButtons.OK, MessageBoxIcon.Error);
                }
                else
                {
                    // update ui
                    Cursor.Current = Cursors.WaitCursor;

                    // reset package map
                    PackageMap.Instance.PackageRoots.Clear();
                    PackageMap.Instance.PackageRoots.Add(PathString.MakeNormalized(folder.FolderName), PackageRootList.DefaultMinLevel, PackageRootList.DefaultMaxLevel);

                    // update ui
                    App.Instance.MainForm.RefreshAll();
                    App.Instance.MainForm.PackageRootPanelText = folder.FolderName;
                    Cursor.Current = Cursors.Arrow;
                }
            }
        }
    }

    [Command("Package.CalculateSize")]
    public class CalculateSizeCommand : Command
    {
        public override void Do()
        {
            using (WaitStatus status = new WaitStatus(App.Instance.MainForm.BackgroundActionPanel))
            {
                foreach (Release release in App.Instance.MainForm.GetSelectedReleases())
                {
                    status.Status = String.Format("Calculating {0}", release.FullName);

                    ReleaseDetails details = (ReleaseDetails)release.Tag;
                    details.Size = release.GetSize();
                }
            }
            App.Instance.MainForm.RefreshAll();
        }
    }

    [Command("Help.Contents")]
    public class HelpContentsCommand : Command
    {
        public override void Do()
        {
            Release r = PackageMap.Instance.GetFrameworkRelease();
            if (r != null)
            {
                string helpPath = Path.Combine(r.Path, String.Format("doc\\{0}.chm", r.FullName));
                if (File.Exists(helpPath))
                {
                    Help.ShowHelp(App.Instance.MainForm, helpPath, "User+Guide/Package+Manager/Managing+Packages.html");
                }
                else
                {
                    string msg = String.Format("Could not find help file '{0}'.", helpPath);
                    App.Instance.ShowError(msg);
                }
            }
        }
    }
    [Command("Help.HomePage")]
    public class HomePageCommand : Command
    {
        public override void Do()
        {
            Process.Start("http://eatech/C15/P62/default.aspx");
        }
    }
    [Command("Help.PackageServer")]
    public class PackageServerHomePageCommand : Command
    {
        public override void Do()
        {
            string webUrl = "http://packages.ea.com/";
            string webServicesURL = EA.PackageSystem.PackageCore.Services.WebServicesURL.GetWebServicesURL();
            if (webServicesURL != null)
            {
                Uri uri = new Uri(webServicesURL);
                webUrl = "http://" + uri.Host + "/";
            }
            Process.Start(webUrl);
        }
    }

    [Command("Help.About")]
    public class AboutCommand : Command
    {
        public override void Do()
        {
            AboutForm about = new AboutForm();
            about.ShowDialog(App.Instance.MainForm);
            about.Dispose();
        }
    }

}
