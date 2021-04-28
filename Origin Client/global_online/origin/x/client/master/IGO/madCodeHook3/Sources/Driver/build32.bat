@cls
@echo -------------------------------------------------------------------------------
set WinDDK=C:\WinDDK\7600.16385.1
@set OLDDIR=%CD%
@call %WinDDK%\bin\setenv.bat %WINDDK% fre WNET
@set Include=%WinDDK%\inc\api;%WinDDK%\inc\crt;%WinDDK%\inc\ddk;%WinDDK%\inc\ddk\wnet;%WinDDK%\inc\crt;%WinDDK%\inc\wnet
@set Lib=%WinDDK%\lib\crt\i386;%WinDDK%\lib\wnet\i386;%WinDDK%\lib
@chdir /d %OLDDIR%
@cl RipeMD.c -FoRipeMD.obj /D_X86_ /D__BUILDMACHINE__=WinDDK /c -D_X86_=1 -Di386=1 -DSTD_CALL -DCONDITION_HANDLING=1 -DNT_UP=1 -DNT_INST=0 -D_NT1X_=100 -DWINNT=1 -D_WIN32_WINNT=0x502 -DWIN32_LEAN_AND_MEAN=1 -DDBG=1 -DDEVL=1 -DFPO=0 -DNDEBUG -D_DLL=1 -D_IDWBUILD -DRDRDBG -DSRVDBG /c /Zel /Zp8 /Gy -cbstring /W3 /Gz /QIfdiv- /QIf /Gi- /Gm- /GX- /GR- /GS- /GF -Zi /Od /Oi /Oy-
@cl StrMatch.c -FoStrMatch.obj /D_X86_ /D__BUILDMACHINE__=WinDDK /c -D_X86_=1 -Di386=1 -DSTD_CALL -DCONDITION_HANDLING=1 -DNT_UP=1 -DNT_INST=0 -D_NT1X_=100 -DWINNT=1 -D_WIN32_WINNT=0x502 -DWIN32_LEAN_AND_MEAN=1 -DDBG=1 -DDEVL=1 -DFPO=0 -DNDEBUG -D_DLL=1 -D_IDWBUILD -DRDRDBG -DSRVDBG /c /Zel /Zp8 /Gy -cbstring /W3 /Gz /QIfdiv- /QIf /Gi- /Gm- /GX- /GR- /GS- /GF -Zi /Od /Oi /Oy-
@cl ToolFuncs.c -FoToolFuncs.obj /D_X86_ /D__BUILDMACHINE__=WinDDK /c -D_X86_=1 -Di386=1 -DSTD_CALL -DCONDITION_HANDLING=1 -DNT_UP=1 -DNT_INST=0 -D_NT1X_=100 -DWINNT=1 -D_WIN32_WINNT=0x502 -DWIN32_LEAN_AND_MEAN=1 -DDBG=1 -DDEVL=1 -DFPO=0 -DNDEBUG -D_DLL=1 -D_IDWBUILD -DRDRDBG -DSRVDBG /c /Zel /Zp8 /Gy -cbstring /W3 /Gz /QIfdiv- /QIf /Gi- /Gm- /GX- /GR- /GS- /GF -Zi /Od /Oi /Oy-
@cl InjectLibrary.c -FoInjectLibrary.obj /D_X86_ /D__BUILDMACHINE__=WinDDK /c -D_X86_=1 -Di386=1 -DSTD_CALL -DCONDITION_HANDLING=1 -DNT_UP=1 -DNT_INST=0 -D_NT1X_=100 -DWINNT=1 -D_WIN32_WINNT=0x502 -DWIN32_LEAN_AND_MEAN=1 -DDBG=1 -DDEVL=1 -DFPO=0 -DNDEBUG -D_DLL=1 -D_IDWBUILD -DRDRDBG -DSRVDBG /c /Zel /Zp8 /Gy -cbstring /W3 /Gz /QIfdiv- /QIf /Gi- /Gm- /GX- /GR- /GS- /GF -Zi /Od /Oi /Oy-
@cl ProcessList.c -FoProcessList.obj /D_X86_ /D__BUILDMACHINE__=WinDDK /c -D_X86_=1 -Di386=1 -DSTD_CALL -DCONDITION_HANDLING=1 -DNT_UP=1 -DNT_INST=0 -D_NT1X_=100 -DWINNT=1 -D_WIN32_WINNT=0x502 -DWIN32_LEAN_AND_MEAN=1 -DDBG=1 -DDEVL=1 -DFPO=0 -DNDEBUG -D_DLL=1 -D_IDWBUILD -DRDRDBG -DSRVDBG /c /Zel /Zp8 /Gy -cbstring /W3 /Gz /QIfdiv- /QIf /Gi- /Gm- /GX- /GR- /GS- /GF -Zi /Od /Oi /Oy-
@cl Config.c -FoConfig.obj /D_X86_ /D__BUILDMACHINE__=WinDDK /c -D_X86_=1 -Di386=1 -DSTD_CALL -DCONDITION_HANDLING=1 -DNT_UP=1 -DNT_INST=0 -D_NT1X_=100 -DWINNT=1 -D_WIN32_WINNT=0x502 -DWIN32_LEAN_AND_MEAN=1 -DDBG=1 -DDEVL=1 -DFPO=0 -DNDEBUG -D_DLL=1 -D_IDWBUILD -DRDRDBG -DSRVDBG /c /Zel /Zp8 /Gy -cbstring /W3 /Gz /QIfdiv- /QIf /Gi- /Gm- /GX- /GR- /GS- /GF -Zi /Od /Oi /Oy-
@cl DllList.c -FoDllList.obj /D_X86_ /D__BUILDMACHINE__=WinDDK /c -D_X86_=1 -Di386=1 -DSTD_CALL -DCONDITION_HANDLING=1 -DNT_UP=1 -DNT_INST=0 -D_NT1X_=100 -DWINNT=1 -D_WIN32_WINNT=0x502 -DWIN32_LEAN_AND_MEAN=1 -DDBG=1 -DDEVL=1 -DFPO=0 -DNDEBUG -D_DLL=1 -D_IDWBUILD -DRDRDBG -DSRVDBG /c /Zel /Zp8 /Gy -cbstring /W3 /Gz /QIfdiv- /QIf /Gi- /Gm- /GX- /GR- /GS- /GF -Zi /Od /Oi /Oy-
@cl SessionList.c -FoSessionList.obj /D_X86_ /D__BUILDMACHINE__=WinDDK /c -D_X86_=1 -Di386=1 -DSTD_CALL -DCONDITION_HANDLING=1 -DNT_UP=1 -DNT_INST=0 -D_NT1X_=100 -DWINNT=1 -D_WIN32_WINNT=0x502 -DWIN32_LEAN_AND_MEAN=1 -DDBG=1 -DDEVL=1 -DFPO=0 -DNDEBUG -D_DLL=1 -D_IDWBUILD -DRDRDBG -DSRVDBG /c /Zel /Zp8 /Gy -cbstring /W3 /Gz /QIfdiv- /QIf /Gi- /Gm- /GX- /GR- /GS- /GF -Zi /Od /Oi /Oy-
@cl DriverEvents.c -FoDriverEvents.obj /D_X86_ /D__BUILDMACHINE__=WinDDK /c -D_X86_=1 -Di386=1 -DSTD_CALL -DCONDITION_HANDLING=1 -DNT_UP=1 -DNT_INST=0 -D_NT1X_=100 -DWINNT=1 -D_WIN32_WINNT=0x502 -DWIN32_LEAN_AND_MEAN=1 -DDBG=1 -DDEVL=1 -DFPO=0 -DNDEBUG -D_DLL=1 -D_IDWBUILD -DRDRDBG -DSRVDBG /c /Zel /Zp8 /Gy -cbstring /W3 /Gz /QIfdiv- /QIf /Gi- /Gm- /GX- /GR- /GS- /GF -Zi /Od /Oi /Oy-
@cl Ioctl.c -FoIoctl.obj /D_X86_ /D__BUILDMACHINE__=WinDDK /c -D_X86_=1 -Di386=1 -DSTD_CALL -DCONDITION_HANDLING=1 -DNT_UP=1 -DNT_INST=0 -D_NT1X_=100 -DWINNT=1 -D_WIN32_WINNT=0x502 -DWIN32_LEAN_AND_MEAN=1 -DDBG=1 -DDEVL=1 -DFPO=0 -DNDEBUG -D_DLL=1 -D_IDWBUILD -DRDRDBG -DSRVDBG /c /Zel /Zp8 /Gy -cbstring /W3 /Gz /QIfdiv- /QIf /Gi- /Gm- /GX- /GR- /GS- /GF -Zi /Od /Oi /Oy-
@cl mchInjDrv.c -Forenameme32.obj /D_X86_ /D__BUILDMACHINE__=WinDDK /c -D_X86_=1 -Di386=1 -DSTD_CALL -DCONDITION_HANDLING=1 -DNT_UP=1 -DNT_INST=0 -D_NT1X_=100 -DWINNT=1 -D_WIN32_WINNT=0x502 -DWIN32_LEAN_AND_MEAN=1 -DDBG=1 -DDEVL=1 -DFPO=0 -DNDEBUG -D_DLL=1 -D_IDWBUILD -DRDRDBG -DSRVDBG /c /Zel /Zp8 /Gy -cbstring /W3 /Gz /QIfdiv- /QIf /Gi- /Gm- /GX- /GR- /GS- /GF -Zi /Od /Oi /Oy-
@echo -------------------------------------------------------------------------------
@IF NOT EXIST renameme32.pdb GOTO DontDelete1
@del renameme32.pdb >NUL
:DontDelete1
@IF NOT EXIST renameme32.sys GOTO DontDelete2
@del renameme32.sys >NUL
:DontDelete2
@IF NOT EXIST "renameme32 (with version resource).sys" GOTO DontDelete3
@del "renameme32 (with version resource).sys" >NUL
:DontDelete3
@link renameme32 driver.res RipeMD StrMatch ToolFuncs InjectLibrary ProcessList Config DllList SessionList DriverEvents Ioctl -merge:_PAGE=PAGE -merge:_TEXT=.text -opt:REF -incremental:NO -force:MULTIPLE -release -fullbuild -nodefaultlib -merge:.rdata=.text -driver -align:0x20 -subsystem:native -base:0x10000 -entry:DriverEntry@8 %SDK_LIB_DEST%\i386\exsup.lib ntoskrnl.lib
@ren vc90.pdb renameme32.pdb >NUL
@del RipeMD.obj >NUL
@del StrMatch.obj >NUL
@del ToolFuncs.obj >NUL
@del InjectLibrary.obj >NUL
@del ProcessList.obj >NUL
@del Config.obj >NUL
@del DllList.obj >NUL
@del SessionList.obj >NUL
@del DriverEvents.obj >NUL
@del Ioctl.obj >NUL
@del renameme32.obj >NUL
