This .gameconfig file was generated from the XBox Live Game Configuration tool, 
which is downloadable from Microsoft. This tool supercides the old XLAST tool,
and it generates .gameconfig files as opposed to the older .xlast files.
https://developer.xboxlive.com/en-us/xbox/resources/tools/Pages/download.aspx

The .spa file is generated from the .gameconfig file with the spac.exe tool that
Microsoft distributes with the 360 SDK. Also the XBox Live Game Configuration tool
can generate the .spa file directly itself.

You need to link the .spa file and the xex.xml file into your 360 .xex binary via 
linker commands like so:

   <property name="link.template.commandline">
        ${property.value}
        /XEXCONFIG:${package.DirtySDK.dir}\xenon\xexconfig.xml
        /XEXSECTION:454107D5=${package.DirtySDK.dir}\xenon\file.spa
   </property>

