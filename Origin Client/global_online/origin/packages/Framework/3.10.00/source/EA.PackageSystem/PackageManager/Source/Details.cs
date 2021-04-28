using System;
using System.Reflection;
using System.IO;
using System.Text;
using System.Collections.Generic;

using EA.PackageSystem.PackageCore;
using EA.PackageSystem.PackageCore.Services;

namespace EA.PackageSystem.PackageManager
{

    abstract class Details
    {
        protected static readonly string TemplateBodyPlaceholder = "<!-- body -->";
        protected static readonly string Template = ResourceLoader.Instance.LoadTextFile("template.html");

        public abstract string ToHtml();
    }

    class BlankDetails : Details
    {

        public static readonly BlankDetails Instance = new BlankDetails();

        private BlankDetails()
        {
        }

        public override string ToHtml()
        {
            return Template.Replace(TemplateBodyPlaceholder, "<p><p><center>No package selected</center>");
        }
    }

    class ReleaseDetails : Details
    {
        NAnt.Core.PackageCore.Release _release;
        long _size = -1;

        public ReleaseDetails(NAnt.Core.PackageCore.Release release)
        {
            _release = release;
        }

        public NAnt.Core.PackageCore.Release Release
        {
            get { return _release; }
        }

        public long Size
        {
            get { return _size; }
            set { _size = value; }
        }

        public override string ToHtml()
        {
            if (_release.HasManifest)
            {
                return ToDetailedHtml();
            }
            else
            {
                return ToBasicHtml();
            }
        }

        string ToDetailedHtml()
        {
            StringBuilder sb = new StringBuilder(@"

${SideBar}

<!-- main content -->
<p>
    <span class='PackageTitle'>${FullName}</span><br>
    by ${ContactName} &lt;<a title='Send email to ${ContactEmail}' href='mailto:${ContactEmail}'>${ContactEmail}</a>&gt;
</p>

<p><b>About:</b> ${About}</p>

<p><b>Changes:</b> ${Changes}</p>

<p><b>Status:</b> <a title='Go to status descriptions' href='start://http://packages.ea.com/Status.aspx'>${Status}</a><br>${StatusComment}</p>

<p><b>License:</b> <a title='Go to license descriptions' href='start://http://packages.ea.com/LicenseAndRestrictions.aspx'>${License}</a><br>${LicenseComment}</p>

<p><b>Home Page:</b> &lt;<a style='word-wrap: break-word' title='Go to package home page' href='start://${HomePageUrl}'>${HomePageUrl}</a>&gt;</p>

<p><b>Documentation:</b> &lt;<a style='word-wrap: break-word' title='Go to package documentation' href='start://${DocumentationUrl}'>${DocumentationUrl}</a>&gt;</p>

");
            string webServicesURL = EA.PackageSystem.PackageCore.Services.WebServicesURL.GetWebServicesURL();
            if (webServicesURL != null)
            {
                Uri uri = new Uri(webServicesURL);
                sb.Replace("packages.ea.com", uri.Host);
            }
            ReplaceTokens(sb);
            return Template.Replace(TemplateBodyPlaceholder, sb.ToString());
        }

        string ToBasicHtml()
        {
            StringBuilder sb = new StringBuilder(@"

${SideBar}

<p>
    <span class='PackageTitle'>${FullName}</span>
</p>

<p>Cannot display any details because package does not have a Manifest.xml file.

");
            ReplaceTokens(sb);
            return Template.Replace(TemplateBodyPlaceholder, sb.ToString());
        }

        public string GetSizeString()
        {
            if (Size < 0)
            {
                return "";
            }

            long bytes = Size;
            long kb = bytes / 1024;
            long mb = kb / 1024;

            if (mb >= 1)
            {
                return String.Format("{0} MB", mb);
            }
            else if (kb >= 1)
            {
                return String.Format("{0} KB", kb);
            }
            else
            {
                return String.Format("{0} bytes", bytes);
            }
        }

        void ReplaceTokens(StringBuilder sb)
        {
            sb.Replace("${SideBar}", SideBarTemplate);
            sb.Replace("${FullName}", _release.FullName);
            sb.Replace("${Path}", _release.Path);

            string webServicesURL = EA.PackageSystem.PackageCore.Services.WebServicesURL.GetWebServicesURL();
            if (webServicesURL != null)
            {
                Uri uri = new Uri(webServicesURL);
                string correctWebHost = uri.Host;
                string templateWebHost = "packages.ea.com";
                sb.Replace(templateWebHost, correctWebHost);
            }

            if (Size < 0)
            {
                sb.Replace("${Size}", "<a href='command://Package.CalculateSize'>Calculate</a>");
            }
            else
            {
                sb.Replace("${Size}", GetSizeString());
            }

            sb.Replace("${Contents}", "");


            if (_release.HasManifest)
            {
                sb.Replace("${ContactName}", _release.ContactName);
                sb.Replace("${ContactEmail}", _release.ContactEmail);
                sb.Replace("${About}", _release.About);
                sb.Replace("${Changes}", _release.Changes);
                sb.Replace("${Status}", _release.Status.ToString());

                sb.Replace("${StatusComment}", _release.StatusComment);
                if (_release.License.Length > 0)
                {
                    sb.Replace("${License}", _release.License);
                }
                else
                {
                    sb.Replace("${License}", "Unknown");
                }
                sb.Replace("${LicenseComment}", _release.LicenseComment);
                sb.Replace("${HomePageUrl}", _release.HomePageUrl);
                sb.Replace("${DocumentationUrl}", _release.DocumentationUrl);
            }
        }

        string GetReleaseListHtml(ICollection<Release> releases)
        {
            StringBuilder sb = new StringBuilder();
            sb.Append("<div style='margin-top: 8px; margin-left: 16px; margin-right:16px;'>");
            if (releases.Count > 0)
            {
                foreach (EA.PackageSystem.PackageCore.Services.Release r in releases)
                {
                    sb.AppendFormat("<a href='package://{0}'>{0}</a><br />", r.Name);
                }
            }
            else
            {
                string webHost = "packages.ea.com";
                string webServices2URL = EA.PackageSystem.PackageCore.Services.WebServicesURL.GetWebServicesURL();
                if (webServices2URL != null)
                {
                    Uri uri = new Uri(webServices2URL);
                    webHost = uri.Host;
                }
                sb.Append("No packages (<a href='start://http://" + webHost + "/article.aspx?id=255'>Why?</a>)");
            }
            sb.Append("</div>");
            return sb.ToString();
        }

        const string SideBarTemplate = @"
<!-- side bar -->
<table valign='top' align='right' width='200' cellspacing='1' cellpadding='8' border='0' bgcolor='#919B9C'>
<tr>
<td class='SideBar'>
    <div style='margin-bottom: 8px; border-bottom: 1px solid #D0D0BF'><b>Actions</b></div>

    <div><img align='absbottom' src='http://packages.worldwide.ea.com/images/folder-open.gif'> 
        <a title='${Path}' href='command://Package.Browse'>Open</a></div>
<!--
    <div><img align='absbottom' src='http://packages.worldwide.ea.com/images/globe.gif'> 
        <a title='Check for newer version' href='command://Package.Update'>Update</a></div>
-->
    <div><img align='absbottom' src='http://packages.worldwide.ea.com/images/delete.gif'> 
        <a title='Delete installed package' href='command://Package.Delete'>Delete</a></div>


    <p>
    
    <div style='margin-bottom: 8px; border-bottom: 1px solid #D0D0BF'><b>Properties</b></div>

    <table border='0'>
        <tr>
            <td title='Amount of disk space package uses'>Size:</td>
            <td>${Size}</td>
        </tr>        
    </table>

</td>
</tr>
</table>
";

    }
}
