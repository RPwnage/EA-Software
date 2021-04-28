autoexp.dat
    * The documentation here is tested against VS 2015 Update 3. 
    * Although current standard way to support visualizers in VS debugger is to use natvis files, autoexp.dat still works. 
    * The following steps come from empirical experience. YMMV. 
        * Make sure that Visual Studio is closed. 
        * Copy the autoexp.dat to 
            * C:\Program Files (x86)\Microsoft Visual Studio 14.0\Common7\Packages\Debugger
            * C:\Program Files (x86)\Microsoft Visual Studio 14.0\Common7\Packages\Debugger\Visualizers
            * C:\Users\<user name>\Documents\Visual Studio 2015\Visualizers 
        * On internet, some users have reported that following options need to be unchecked and some users have reported that they should be checked. For me (Arpit), they had to be checked.
            * Tools/Options/Debugging/General/Use Managed Compatibility Mode
            * Tools/Options/Debugging/General/Use Native Compatibility Mode
    * To verify, just put a breakpoint next to an eastl container in your code. The watch window should present you with a nice visualization of the container.  