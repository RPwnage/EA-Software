using System;
using System.ComponentModel;
using System.IO;
using System.Runtime.InteropServices;
using System.Windows.Forms;

using Microsoft.Win32;

namespace EA.Common {

    public class WebBrowser : AxHost, IWebBrowserEvents {

        public event BeforeNavigateEventHandler BeforeNavigate;
        public event NavigateCompleteEventHandler NavigateComplete;

        private IWebBrowser control;
        private ConnectionPointCookie cookie;

        string url;
        string html;
        string tempfile = Path.GetTempFileName();

        public WebBrowser() : base("8856f961-340a-11d0-a96b-00c04fd705a2") {
        }

        public string Url {
            get { return url; }
            set {
                url = value;
                object o = null;
                control.Navigate(url, ref o, ref o, ref o, ref o);
            }
        }

        public string Html {
            get { return html; }
            set {
                html = value;
                url = null;
                StreamWriter writer = new StreamWriter(tempfile);
                writer.Write(html);
                writer.Close();

                object o = null;
                control.Navigate(tempfile, ref o, ref o, ref o, ref o);
            }
        }

        protected override void CreateSink() {
            try { 
                cookie = new ConnectionPointCookie(control, this, typeof(IWebBrowserEvents)); 
            } 
            catch { 
            }
        }

        protected override void DetachSink() {
            try { 
                cookie.Disconnect(); 
            } catch {
            }
            if (tempfile != null) {
                try { 
                    File.Delete(tempfile); 
                } catch {
                }
                tempfile = null;
            }
        }

        protected override void AttachInterfaces() {
            try { 
                control = (IWebBrowser) GetOcx(); 
            } catch { 
            }
        }

        public void RaiseBeforeNavigate(ref string url, int flags, string targetFrameName, ref object postData, string headers, ref bool cancel) {
            if (url.IndexOf("%20") > -1) {
                url = url.Replace("%20", " ");
            }

            BeforeNavigateEventArgs e = new BeforeNavigateEventArgs(url, false);            
            if (BeforeNavigate != null) {
                BeforeNavigate(this, e);
            }
            cancel = e.Cancel;
        }

        public void RaiseNavigateComplete(string url) {
            if (NavigateComplete != null) {
                NavigateComplete(this, new NavigateCompleteEventArgs(url));
            }
        }
    }

    [Guid("eab22ac1-30c1-11cf-a7eb-0000c05bae0b")]
    interface IWebBrowser {
        void GoBack();
        void GoForward();
        void GoHome();
        void GoSearch();
        void Navigate(string url, ref object flags, ref object targetFrame, ref object postData, ref object headers);
        void Refresh();
        void Refresh2();
        void Stop();
        void GetApplication();
        void GetParent();
        void GetContainer();
    }

    [Guid("eab22ac2-30c1-11cf-a7eb-0000c05bae0b"),
    InterfaceType(ComInterfaceType.InterfaceIsIDispatch)]
    public interface IWebBrowserEvents {
        [DispId(100)]
        void RaiseBeforeNavigate(ref string url, int flags, string targetFrameName, ref object postData, string headers, ref bool cancel);
        [DispId(101)]
        void RaiseNavigateComplete(string url);
    }

    public delegate void BeforeNavigateEventHandler(object sender, BeforeNavigateEventArgs e);

    public class BeforeNavigateEventArgs {
        public BeforeNavigateEventArgs(string url, bool cancel) {
            this.url = url;
            this.cancel = cancel;
        }

        protected string url;
        public string Url {
            get{ return url; }
        }

        protected bool cancel;
        public bool Cancel {
            get{ return cancel; }
            set{ cancel = value; }
        }
    }

    public delegate void NavigateCompleteEventHandler(object sender, NavigateCompleteEventArgs e);

    public class NavigateCompleteEventArgs {
        public NavigateCompleteEventArgs(string url) {
            this.url = url;
        }

        protected string url;
        public string Url {
            get{ return url; }
        }
    }
}
