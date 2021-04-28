This is a small collection of batch files I've been using for awhile and found to be helpful.

Each file calls loadFrameworkEnv.bat (which loads up the current framework2 cmd environment & backs up to the sdk root dir), then performs sone nant task and pauses (so you can check the outcome).

NOTE: scripts expect to be run in this directory (for ease of shortcut creation)

makeBlazeSDK_Sln.bat
    Build the visual studio solution files for the BlazeSDK.

makeBlazeSDK_Sln_nonBulkBuild.bat
    Build the visual studio solution files with bulk build disabled

makeDocs.bat
    Run doxygen over the BlazeSDK, then build a chm help file from the doxygen output

smokeBuild.bat
    Build the BlazeSDK (and internal samples) in debug for each of the 3 platforms.  
    Note: this is quicker than build all, since we only build debug (for 3 platforms).

makePackage.bat
    simply runs nant package, to build the blazeSDK packages.

