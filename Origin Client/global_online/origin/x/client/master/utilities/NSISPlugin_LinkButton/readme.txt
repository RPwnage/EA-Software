NSIS 'Link Button' Plugin
Ryan Binns - 07/23/2015

This plugin is necessary for the Origin build process to complete.  This plugin simply
subclasses an existing HWND and captures 'click' events (mousedown and mouseup with capture)
and then calls back to an NSIS function.

Build in 'ReleaseUnicode' configuration.

To use in NSIS, you need the .NSH file and the .DLL file installed in your NSIS directory.