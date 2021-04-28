using System;
using System.Windows.Forms;

namespace EA.Common {

    // http://www.syncfusion.com/FAQ/WinForms/FAQ_c94c.asp#q808q

    public class MyTextBox : TextBox { 
        const int WM_KEYDOWN = 0x100; 
        const int WM_KEYUP = 0x101; 
 
        public override bool PreProcessMessage(ref Message msg) { 
            Keys keyCode = (Keys) (int) msg.WParam & Keys.KeyCode; 
            if ((msg.Msg == WM_KEYDOWN || msg.Msg == WM_KEYUP) && 
                (keyCode == Keys.Delete)) { 
                return false; 
            }      
            return base.PreProcessMessage(ref msg); 
        } 
    }  
}
