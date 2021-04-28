using System;
using System.Windows.Forms;

namespace EA.Common {

    public class WaitStatus : IDisposable {

        StatusBarPanel _panel = null;

        public WaitStatus() {
            Begin();
        }

        public WaitStatus(StatusBarPanel panel) {
            _panel = panel;
            Begin();
        }

        public string Status {
            get { 
                if (_panel != null) {
                    return _panel.Text; 
                } else {
                    return null;
                }
            }
            set { 
                if (_panel != null) {
                    _panel.Text = value;
                }
            }
        }

        #region IDisposable implementation

        // See C# Essentials 2nd p. 136 for more details on the rational of this code if you
        // have never implemented IDisposable.  Note I've made some slight changes for 
        // readability.  Gerry Shaw (2002-10-23)
        
        private bool _disposed = false;

        /// <summary>
        /// Helper method for implementing object clean up code.
        /// </summary>
        /// <param name="userInvoked">True if method called by <see cref="Dispose()"/> method, else false if called by finalizer (aka ~destructor).</param>
        protected virtual void Dispose(bool userInvoked) {
            // don't dispose more than once
            if (!_disposed) {
                if (userInvoked) {
                    // We are not in the finalize (aka Destructor) so we can reference other objects.
                    // close any open database connection
                    End();
                }
                _disposed = true;
            }
        }

        public void Dispose() {
            Dispose(true);

            // mark this object finalized (there is no need to call the finalizer on this instance anymore)
            GC.SuppressFinalize(this);
        }

        ~WaitStatus() {
            Dispose(false);
        }

        #endregion

        void Begin() {
            Cursor.Current = Cursors.WaitCursor;
        }

        public void End() {
            Status = "Ready";
            Cursor.Current = Cursors.Arrow;
        }
    }
}
