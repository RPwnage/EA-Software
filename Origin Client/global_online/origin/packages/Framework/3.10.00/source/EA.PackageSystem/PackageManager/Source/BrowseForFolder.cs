using System;
using System.Text;
using System.Runtime.InteropServices;
using System.Windows.Forms;

namespace EA.PackageSystem.PackageManager
{
	public delegate Int32 BrowseCallbackProc(IntPtr hwnd, UInt32 uMsg, Int32 lParam, Int32 lpData);
	
	[StructLayout(LayoutKind.Sequential)]
	public class BrowseInfo
	{
		//
		// Flags specifying the options for the dialog box.
		//
		public static readonly UInt32 BIF_RETURNONLYFSDIRS   = 0x0001;
		public static readonly UInt32 BIF_DONTGOBELOWDOMAIN  = 0x0002;
		public static readonly UInt32 BIF_STATUSTEXT         = 0x0004;
		public static readonly UInt32 BIF_RETURNFSANCESTORS  = 0x0008;
		public static readonly UInt32 BIF_EDITBOX            = 0x0010;
		public static readonly UInt32 BIF_VALIDATE           = 0x0020;
		public static readonly UInt32 BIF_NEWDIALOGSTYLE     = 0x0040;
		public static readonly UInt32 BIF_USENEWUI           = (BIF_NEWDIALOGSTYLE | BIF_EDITBOX);
		public static readonly UInt32 BIF_BROWSEINCLUDEURLS  = 0x0080;
		public static readonly UInt32 BIF_UAHINT             = 0x0100;
		public static readonly UInt32 BIF_NONEWFOLDERBUTTON  = 0x0200;
		public static readonly UInt32 BIF_NOTRANSLATETARGETS = 0x0400;
		public static readonly UInt32 BIF_BROWSEFORCOMPUTER  = 0x1000;
		public static readonly UInt32 BIF_BROWSEFORPRINTER   = 0x2000;
		public static readonly UInt32 BIF_BROWSEINCLUDEFILES = 0x4000;
		public static readonly UInt32 BIF_SHAREABLE          = 0x8000;

		public IntPtr hwndOwner;
		public IntPtr pidlRoot;
		[MarshalAs(UnmanagedType.LPStr)]
		public String pszDisplayName;
		[MarshalAs(UnmanagedType.LPStr)]
		public String lpszTitle;
		public UInt32 ulFlags;
		[MarshalAs(UnmanagedType.FunctionPtr)]
		public BrowseCallbackProc lpfn;       
		public Int32 lParam;                  
		public Int32 iImage;                  
	}

	public class OpenFolderDialog
	{
		public static readonly int MAX_PATH = 260;
		
		[DllImport("shell32.dll")]
		public static extern int SHGetPathFromIDList(IntPtr pidl, StringBuilder pszPath);

		[DllImport("shell32.dll")]
		public static extern IntPtr SHBrowseForFolder(BrowseInfo lbpi);

		
		BrowseInfo _browseInfo;
		IntPtr _idList;

		public OpenFolderDialog(BrowseInfo browseInfo)
		{
			_browseInfo = browseInfo;
			_idList = IntPtr.Zero;
		}

		public DialogResult ShowDialog()
		{
			_idList = SHBrowseForFolder(_browseInfo);

			if (_idList.ToInt32() <= 0)
				return DialogResult.Cancel;
			return DialogResult.OK;
		}

		public string FolderName
		{
			get 
			{
				if (_idList.ToInt32() <= 0)
                    return String.Empty;

				StringBuilder pszPath = new StringBuilder(MAX_PATH + 1);
				int isPathFound = SHGetPathFromIDList(_idList, pszPath);
				return (isPathFound == 0) ? String.Empty : pszPath.ToString();
			}
		}
	}
}
