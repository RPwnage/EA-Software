using System;
using System.Windows.Forms;
using System.Threading;

using EA.Common.CommandHandling;

namespace EA.PackageSystem.PackageManager {

    public class App {

        // Singleton pattern 
        // See "Exploring the Singleton Design Pattern" on MSDN for implementation details of Singletons in .NET
        public static readonly App Instance = new App();

        [STAThread]
        public static void Main() 
        {
            var t = new Thread(new ThreadStart(thread_func)); t.SetApartmentState(ApartmentState.STA); 
            t.Start();
            t.Join();


            //App.Instance._mainForm = new MainForm();
            //System.Windows.Forms.Application.Run(App.Instance.MainForm);
        }

        private static void thread_func()
        {
            App.Instance._mainForm = new MainForm();
            System.Windows.Forms.Application.Run(App.Instance.MainForm);
        }

        MainForm _mainForm;

        public App() {
        }
        
        public MainForm MainForm {
            get { return _mainForm; }
        }

        
        public void Run() {
            
            System.Windows.Forms.Application.Run(MainForm);
        }

		public DialogResult ShowError(string message, MessageBoxButtons msgBoxButtons, MessageBoxIcon msgBoxIcon) {
			return MessageBox.Show(_mainForm, message, "Package Manager Error", msgBoxButtons, msgBoxIcon);
		}

		public DialogResult ShowError(string message, MessageBoxButtons msgBoxButtons) {
			return MessageBox.Show(_mainForm, message, "Package Manager Error", msgBoxButtons);
		}

		public DialogResult ShowError(string message) {
			return MessageBox.Show(_mainForm, message, "Package Manager Error");
		}

    }
}
