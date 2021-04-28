import sys
import CodeStripper
import os
import time
import shutil
    
def findPackages(srcMasterconfig):
    """This function finds the dependent packages in the masterconfig file"""
    EApackagesFound = False
    packages = {}
    f = open(srcMasterconfig)
    for line in f:
        if EApackagesFound:
            if "<!--" in line:
                break
            elif line.strip() == '':
                break
            else:
                package = line.split('=')[1][:-7].split()[0][1:-1]
                version = line.split('=')[2].split()[0][1:-1]
                packages[package] = version
        elif ("<!-- EA packages we build with -->" in line):
            EApackagesFound = True
    f.close()
    return (packages)

def RemovePlatformSpecific(src, argv):
    """This function takes a set and the defined platforms and performs a string search
    to remove any platform specific files or folders we do not want"""
    newsrc = set([])
    xbox = set([])
    ps3 = set([])
    srcSet = set([])
    removeSet = set([])
    xboxList = ['xenon', '360', 'Xenon', 'xbox']
    ps3List = ['ps3', 'PS3']
    argvList = []
    
    platformList = [xboxList, ps3List]
    
    for list in platformList:
        for element in list:
            if element in argv: #if true, we have found the list associated with the platform we want to keep after stripping
                platformList.remove(list) 
                break
                
    for list in platformList:
        for element in list:
            argvList.append(element) #argvList contains strings associated with the platform(s) we want to get rid of
    
    for x in src: #x is file or folder name
        add = 1
        for platform in argvList:           
            if platform in x: #if file or folder name is for a specific platform we want to get rid of
                add = 0
                break #as soon as a string matches, the file will be removed so we no longer need to search the other platforms
        if add == 1: #this is not a platform specific file for a platform we do not want
            srcSet.add(x)
    
    return srcSet   

deleteFiles = 0
custom = 0
    
try: #catches KeyboardInterrupt
    startTime = time.time() 
            
    #if --config is in the parameters, the script has been manually called
    for element in sys.argv:
        if '--config' in element:
            cfgFile = element[9:]
        else:
            cfgFile = 'BlazeSDK_strip.cfg'
            
    for element in sys.argv:
        if '--platforms' in element:
            custom = 0
            break
        else:
            custom = 1
            
    if '-h' in sys.argv or '--help' in sys.argv:
        print """
Usage: BlazeSDK_strip.py [options]

Options:
  -h, --help      show this help message and exit
  -v, --verbose   print out verbose messages
  -s, --silent    avoid printing out unnecessary messages
  -f,             automatically deletes target directory at
                  start of script if it exists. If this option
                  is not present, the user will be prompted
  --config=CONFIGPATH
                  specifies the config file from which to grab 
                  the codeStripper parameters. This will
                  automatically grab BlazeSDK_strip.cfg if
                  this option is not present"""
        exit(0)
    
    #if none of the above statements were true, it is assumed that the sys.argv parameters are to David Cope's requirements
    #if the parameters are incorrect or missing, CodeStripper.py will catch any errors
    if custom != 1: 
        forceFlag = 0
        for element in sys.argv:
            if element == '-f':
                sys.argv.remove(element) #this needs to be removed as there is no '-f' option in codeStripper
                forceFlag = 1 #this variable is set so that the option value can still be referenced after it is removed from sys.argv
                break
        for element in sys.argv:        
            if element == '-c': #'-c' comes from this script manually calling itself, it is used to identify that this was a manual call (ie not from nant)
                sys.argv.remove(element)
                custom = 1
                break
        codeStripper = CodeStripper.CreateCodeStripper(sys.argv) #creates the codeStripper object
        #go to BLAZESDK STRIPPING START
        
    else: #this is a manual call, the parameters are contained in the config file
        if os.path.exists(cfgFile):
            print "\n[echo] Using %s as config file" % cfgFile
            file = open(cfgFile, 'r')
            command = ''
            srcRoot = ''
            destRoot = ''
            platformList = ''
            
            #opens the config file and starts to parse the file for parameters and assigns them to respective variables
            for line in file:
                if '--source=' == line[:9]:
                    if "BlazeSDK.build" not in line[-15:]:
                        errorMessage = '%s:You must specify the BlazeSDK.build file:1' % os.path.basename(sys.argv[0])
                        exit(errorMessage)  
                    srcRoot = os.path.abspath(line[9:])
                    if srcRoot[-1:] == '\n':
                        srcRoot = srcRoot[:-1]
            
                elif '--target=' == line[:9]:
                    destRoot = os.path.abspath(line[9:])
                    if destRoot[-1:] == '\n':
                        destRoot = destRoot[:-1]
                    
                elif '--platforms=' == line[:12]:
                    platformList = line
                    if platformList[-1:] == '\n':
                        platformList = platformList[:-1]
                        platformList = platformList.replace('xbl2', 'xenon')
            file.close()
            
            #checks to see if the current OS is Windows or Linux by checking the paths
            BlazeDir = os.path.dirname(os.path.abspath(sys.argv[0]))
            BlazeDir = str(BlazeDir).rsplit('/',1)
            unifdefDir = "--unifdef=unifdef/installed/unifdef"
            if len(BlazeDir) == 1: #windows
                unifdefDir += ".exe"
            options = ''
            
            #populate options for the command to call BlazeSDK_strip.py with proper commands
            for element in sys.argv:
                if element in ['-v', '--verbose', '-s', '--silent', '-f', '--include-source']: #we dont want to pass --config or -h options to the BlazeSDSK_strip.py call
                    options += "%s " % element
                    
            #command calls this current script but with the parameters in the form of David Cope's requirements for the stripping scripts
            command = "python BlazeSDK_strip.py %s %s \"%s\" \"%s\" %s " % (unifdefDir, platformList, os.path.dirname(srcRoot), destRoot, options)
            print "\n[exec] %s \n" % command

#-----------SYSTEM COMMAND - BLAZESDK_STRIP.PY          
            response = os.system(command + "-c") #adds the '-c' option to signify it was a custom call
            
            if response == 1: #if there is an error, exit out of the script
                exit(0)
            deleteFiles = 2 #at this point, the target directory can be deleted as the user has been prompted
            
            #copies the masterconfig so that it can be used locally and adds ActivePython dependency
            #the masterconfig is used by nant to determine the versions for the dependent packages
            #the file is copied because the file must be in the current working directory for nant to open it
            masterconfig = os.path.dirname(srcRoot) + "/masterconfig.xml"
            f_old = open(masterconfig, 'r')
            f_new = open('masterconfig.xml', 'a')
            for line in f_old:
                if "<masterversions>" in line:                  
                    f_new.write(line)
                    f_new.write("\n <package name=\"ActivePython\"      version=\"2.6.5.1\" />\n")
                    continue
                    
                #dont copy over ActivePython package line if it exists in masterconfig, use the specified one from EATech
                if "ActivePython" not in line:
                    f_new.write(line)
            f_old.close()
            f_new.close()           
            
            #populates the package dependency list for the nant command 
            packageCommaList = ""       
            for package in findPackages(masterconfig):
                if 'ps3' not in platformList and 'PS3' in package: #if ps3 is supposed to be stripped out, do not copy over ps3commerce2arbitrator
                    continue
                packageCommaList = packageCommaList + package + ","
            
            #sets the destination directory for the packages
            outputDir = destRoot + "/packages/common"
            
            #sets the nant command
            Nantcommand = "nant -D:codestripper_packages=" + packageCommaList + " -D:codestripper_platforms=" + platformList[12:] + " -D:codestripper_fallbackscriptdir=\"" + os.getcwd() + "\" -D:codestripper_outputdir=\"" + outputDir + "\" -masterconfigfile:masterconfig.xml"
            if '-s' in sys.argv or '--silent' in sys.argv:
                Nantcommand += " -D:codestripper_silent=true"
            if '-v' in sys.argv or '--verbose' in sys.argv:
                Nantcommand += " -D:codestripper_verbose=true"
            if '--include-source' in sys.argv:
                Nantcommand += " -D:codestripper_includesource=true"                    
            print "\n[exec] %s\n" % Nantcommand
            
#-----------SYSTEM COMMAND - NANT           
            result = os.system(Nantcommand)
                
            #removes the copied masterconfig.xml as it is no longer needed  
            os.remove('masterconfig.xml')
            if result == 1:
                errorMessage = '\n%s:Nant has failed. Please look at the above error message:1' % (os.path.basename(sys.argv[0]))
                exit(errorMessage)  
            else:
                print "\n[echo] Command completed: %s\n" % command
                print "\n%s:Operation took %.2f seconds to complete. Stripped files can be found at your specified target here: %s" % (os.path.basename(sys.argv[0]), (time.time()-startTime), str(os.path.abspath(destRoot)))
    
#-----------MAIN SCRIPT FINISHED            
            exit(0) 
        
        #checks that the non-existent custom config file passed isn't the deafult one to prevent a confusing error message          
        elif cfgFile != 'BlazeSDK_strip.cfg': 
            errorMessage = '\n%s:The specified config file %s could not be found. To use the default config file, do not use the --config option:1' % (os.path.basename(sys.argv[0]), cfgFile)
            exit(errorMessage)      
        
        else: #cannot find default config file
            errorMessage = "\n%s:The default config file 'BlazeSDK_strip.cfg' could not be found. To specify a config file, use the --config option:1" % (os.path.basename(sys.argv[0]))
            exit(errorMessage)      
    
#------------BLAZESDK STRIPPING START
    if os.path.exists(codeStripper.destRoot) and custom != 0:   
        targetFolders = codeStripper.GenerateRecursiveFolderList(([codeStripper.destRoot]))
        targetFiles = codeStripper.GenerateFileList(targetFolders)
        
        #check if path is empty. If true, there are folders or files in this directory
        if len(targetFolders) != 1 or len(targetFiles) != 0: 
            print("[WARNING] " + os.path.abspath(codeStripper.destRoot) + " must be empty! Continuing will DELETE ALL existing files in the target directory.  Continue('Y') or quit('q')?\n")
            if forceFlag == 1: #if the user has specified -f, dont prompt user for delete permission
                print "[echo] '-f' option used. Bypassing user prompt and continuing script execution"
                response = 'Y'
            else:
                response = raw_input()
            if response != 'Y':
                print ("User did not specify 'Y'. Now exiting script")
                errorMessage = '%s:Process manually terminated by user:1' % os.path.basename(sys.argv[0])
                exit(errorMessage)
            else: #user has specified that the target directory can be deleted              
                print ("\nDeleting files...")
                try:
                    shutil.rmtree(os.path.abspath(codeStripper.destRoot))
                    print ("Deleting completed!\n")
                except WindowsError, e:
                    errorMessage = '\n%s:WindowsError%s:1' % (os.path.basename(sys.argv[0]), e)
                    exit(errorMessage)          
    
    destRoot = os.path.abspath(codeStripper.destRoot)
    if custom != 1:
        codeStripper.destRoot = os.path.dirname(os.path.dirname(destRoot)) #solves folder hierarchial issue
    
    #at this point destRoot does not exist and anything the script creates in this new directory can be deleted (used for KeyboardInterruption)
    deleteFiles = 1
    
    #start generating the set of files to copy and strip
    includedSubDirectories = [os.path.abspath(codeStripper.srcRoot)]    
    srcFolders = codeStripper.GenerateRecursiveFolderList(includedSubDirectories)           
    for element in sys.argv:
        if '--platforms' in element:
            platformList = element[12:]
            break
            
    args = platformList 
    srcFolders = RemovePlatformSpecific(srcFolders, args) #removes unwanted platform specific folders

    srcFoldersToFilter = srcFolders 
    
    
    # Generate a list of files that will be stripped using unifdef
    # All original BlazeSDK files are copied but only some are stripped and overwrite the original files
    
    #remove 'source' and 'gen' folders from being copied over
    if '--include-source' in sys.argv:
        srcFilesToFilter = codeStripper.GenerateFilteredFileList(srcFoldersToFilter, codeStripper.GetDefaultSourceFileExtensions()) 
        srcFilesToCopy = codeStripper.GenerateFileList(srcFolders)  
    
        srcFilesToFilter = RemovePlatformSpecific(srcFilesToFilter, args)
        srcFilesToCopy = RemovePlatformSpecific(srcFilesToCopy, args)   
        
        srcFilesCopy = srcFilesToFilter
        for file in srcFilesCopy:
            if "yacc.c" in file: #this file causes an error for unifdef as its '#ifdef' is not at the start of a new line - EA does not have permission to change this file
                srcFilesToFilter = srcFilesToFilter - set([file])
    
    else:   
        srcFoldersCopy = srcFolders
        for folder in srcFoldersCopy:
            if "\\source" in folder or ("\\gen" in folder and "\\build" not in folder) : #dont include 'source' if they exist
                srcFolders = srcFolders - set([folder])
        
        srcFoldersToFilter = srcFolders     
        
        srcFilesToFilter = codeStripper.GenerateFilteredFileList(srcFoldersToFilter, codeStripper.GetDefaultSourceFileExtensions()) 
        srcFilesToCopy = codeStripper.GenerateFileList(srcFolders)
        
        srcFilesCopy = srcFilesToCopy
        for file in srcFilesCopy:
            if ".java" in file or ".cpp" in file:
                srcFilesToCopy = srcFilesToCopy - set([file])
        
        srcFilesToFilter = RemovePlatformSpecific(srcFilesToFilter, args)
        srcFilesToCopy = RemovePlatformSpecific(srcFilesToCopy, args)
    
    # Get the default set of undefines.  These undefines are determined based on the currently
    # active platforms which are specified by the user.
    undefines = codeStripper.GetDefaultUndefines()

    # Get the default set of defines.  These defines are determined based on the currently
    # active platforms which are specified by the user.
    defines = codeStripper.GetDefaultDefines()

    filename = str(codeStripper.srcRoot).rsplit('/',1)
    if len(filename) == 1:
        filename = str(codeStripper.srcRoot).rsplit('\\',1)
    codeStripper.destRoot += "/BlazeSDK/%s" % filename[1]
    
    codeStripper.CopyFiles(srcFilesToCopy) #copies BlazeSDK folders to new location
    codeStripper.UnifdefFiles(srcFilesToFilter, undefines, defines) #strips the BlazeSDK files
    exit(0)
    
    
except KeyboardInterrupt:
    if deleteFiles != 0 and custom == 1:        
        target = destRoot
        if os.path.exists(target):
            print ("Stopping script\nCleaning up files...")
            shutil.rmtree(target)
            print ("Clean up completed!")
        if os.path.exists('masterconfig.xml'):
            os.remove('masterconfig.xml')
        if deleteFiles == 2:
            print ("Exiting script")
    else:
        print ("Exiting script")    
    exit(0)

        