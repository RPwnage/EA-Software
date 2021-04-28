import sys
import os
myfolder = os.path.dirname(__file__)
sys.path.insert(0,os.path.join(myfolder,'packages'))#we have to override the default email module because the EAPackage is messed up
import Tkinter
import zipfile
import tkMessageBox
import xml.etree.ElementTree as ET
import ttk
import boto
from boto.s3.key import Key
from boto.s3.prefix import Prefix
from boto.s3.bucket import Bucket
from P4 import P4,P4Exception,OutputHandler as P4OutputHandler,Progress as P4Progress,ReportHandler
import time
import tkFont
import json
from threading import Thread
import filecmp
import datetime
import subprocess
import base64
import shutil
import time
download_path = os.path.join(myfolder,'downloads')
if not os.path.exists(download_path):
    os.mkdir(download_path)
log_path = os.path.join(myfolder,'logs')
if not os.path.exists(log_path):
    os.mkdir(log_path)
class OutputThread(Thread):
    def __init__(self):
        Thread.__init__(self)
        self.stop_me = False
        
    ## Do the download action intended to do
    def run(self):
        while not self.stop_me:
            time.sleep(1)
            sys.stdout.flush()
            sys.stderr.flush()

debug = False
if debug:
    output_thread = OutputThread()
    output_thread.start() #This thread spits ouput out to console immediately, use for debugging
else:
    sys.stdout = open(os.path.join(log_path,'client_downloader.out.txt'),'a')
    sys.stderr = open(os.path.join(log_path,'client_downloader.err.txt'),'a')

s3_info = {}
ACCESS_KEY = 'TODO - retrieve from credentials vault solution if this is still needed, otherwise leave it as it is'
SECRET_KEY = 'TODO - retrieve from credentials vault solution if this is still needed, otherwise leave it as it is'
BUCKET = 'client-automation'
SUBPANE = "#DDDDDD"
P4_PORT_EARS = "go-perforce.rws.ad.ea.com:4487"
P4_PORT_EAC = "eac-p4proxy:4487"
P4_CHARSET = "utf8"
P4_DEFAULT_USERNAME = "origin_debug_helper"
P4_DEFAULT_PASSWORD = "TODO - retrieve from credentials vault solution if this is still needed, otherwise leave it as it is"
MAX_CALLBACKS = 200
SETTING_CHECK_DEBUG = '1'
SETTING_CHECK_INSTALL = '2'
SETTING_CHECK_P4 = '3'
SETTING_BRANCH = '4'
SETTING_VERSION = '5'
SETTING_P4_USERNAME = '6'
SETTING_P4_PASSWORD = '7'
SETTING_P4_WORKSPACE = '8'
SETTING_P4_PORT = '9'
ROLLOVER_NUMBER = 65536 #2^16
setting_path = os.path.join(myfolder,'settings.json')
ORIGIN_INSTALL = os.path.join(os.environ['ProgramFiles'],'Origin')

##
# Returns the bucket object from the connection
#
def get_bucket():
    if 'bucket' not in s3_info:
        s3_info['bucket'] = boto.connect_s3(ACCESS_KEY, SECRET_KEY).get_bucket(BUCKET,validate=False)
    mybucket = s3_info['bucket']
    assert isinstance(mybucket,Bucket)
    return mybucket
    
### Do robocopy - so much faster, also required to copy to the origin folder now that it's readonly
def robocopy(new_dir,filename):
    new_dest = os.path.join(new_dir,os.path.basename(filename))
    if not os.path.exists(new_dest) or not filecmp.cmp(new_dest,filename):
        command = "robocopy \"{0}\" \"{1}\" \"{2}\"".format(os.path.dirname(filename),new_dir,os.path.basename(filename))
        os.system(command) #fuck shutils.copy


#This is just an exception I throw to tell the thread to stop running. 
#Threads are in loops when idle and the loop constantly checks flags to see if it should exit
#If it should it throws this exception
class StopThread(Exception):
    pass

#abstract
class DebugTaskThread(Thread):
    DOWNLOAD_WEIGHT = 0.9 # what weight the download task is of the total tasks
    def __init__(self, branch,version,progress,downloader):
        Thread.__init__(self)
        self.stop_me = False
        self.branch = branch
        self.version = version
        self.progress = progress
        self.downloader = downloader
        self.progress.update("Action started")
        self.error_message = ''
        self.stopped = False

    ## This is called frequently from the thread to see if the thread needs to stop running
    # If it does the
    def check_exit(self):
        if self.downloader.exit_me or self.stop_me:
            self.error_message = "User stopped thread manually"
            raise StopThread()
        
    def cleanup(self):
        self.downloader.clean_thread(self)
    
    ## Update the progress bar and progress label. Callback from boto get_contents_to_filename
    # Usually just for downloads
    # @param part 
    def show_progress(self,part,total):
        self.check_exit()
        percentage = int((float(part) / float(total)) * 100)
        #the progress bar shouldn't fill up for these cause there's more actions after the download
        self.progress.update('Downloading {0}, {1}%'.format(self.filename,percentage),int(float(percentage) * self.DOWNLOAD_WEIGHT))
    
    ## Download the file
    # @path the key prefix to download the file form
    def download_file(self,path):
        print self.filename
        mykey = get_bucket().get_key(path.format(self.branch,self.filename))
        #check to make sure file doesn't already exist
        local_file = os.path.join(download_path,self.filename)
        if os.path.exists(local_file) and mykey and mykey.etag:
            md5, md5hashed = mykey.compute_md5(open(local_file,'rb'),)
            if md5 == mykey.etag.strip('"'):
                #file already downloaded
                self.show_progress(1, 1)
                return
        if not mykey:
            self.error_message = '{1} not found for branch {0}'.format(self.branch,self.filename)
            raise StopThread()
        else:
            assert isinstance(mykey,Key)
            #download the file using s3
            mykey.get_contents_to_filename(local_file, cb=self.show_progress, num_cb=MAX_CALLBACKS)
            #complete download
            self.progress.update('Download complete',100*self.DOWNLOAD_WEIGHT)

class DownloadClientThread(DebugTaskThread):
    def __init__(self,branch,version,progress,downloader):
        DebugTaskThread.__init__(self,branch,version,progress,downloader)    

    ## Launch the origin setup installer       
    def launch_installer(self):
        try:
            #launch updater
            args = ['/forceInstall','/silent']
            installer = subprocess.Popen([os.path.join(download_path,self.filename)]+args)
            #update progress
            self.progress.update('Installer launching',90)
            #wait for installer to launch
            while True:
                mycode = installer.poll()
                self.check_exit()
                if mycode is not None:
                    self.progress.update('Installer complete',100)
                    break
        except StopThread:
            installer.terminate()
                    
    ## Do the download action intended to do
    def run(self):
        try:
            self.filename = 'OriginSetup-{0}.exe'.format(self.version)
            self.download_file('builds/{0}/installer/{1}')
            self.launch_installer()
        except StopThread:
            print 'thread stopped DownloadClientThread {0} {1}'.format(self.branch,self.version)
            self.stopped = True
        except Exception as exp:
            print 'thread error {0}'.format(exp)
            self.stopped = True
            self.error_message = "Error:{0}".format(str(exp))
        finally:
            self.cleanup()

class DownloadDebugThread(DebugTaskThread):
    def __init__(self,branch,version,progress,downloader):
        DebugTaskThread.__init__(self,branch,version,progress,downloader)

    def copy_pdbs_in_place(self):
        w = zipfile.ZipFile(os.path.join(download_path,self.filename))
        extract_folder = os.path.join(download_path,self.filename[:-4])
        to_extract = []
        for name in w.namelist():
            if '.pdb' in name:
                to_extract.append(name)
        counter = 0
        if not os.path.exists(extract_folder):
            os.mkdir(extract_folder)
        for subfile in to_extract:
            counter += 1
            self.progress.update("Extracting {0}/{1}".format(counter,len(to_extract)),90+(10*(float(counter)/float(len(to_extract)))))
            try:
                tempfile = os.path.join(extract_folder,os.path.basename(subfile))
                shutil.copyfileobj(w.open(subfile),file(tempfile,"wb"))
                robocopy(ORIGIN_INSTALL,tempfile)
            except Exception as exp:
                print exp
                pass #throws an exception on identical file
            self.check_exit()

            self.check_exit()
        self.progress.update("Extraction Complete",100)
                
    def run(self):
        try:
            self.filename = 'OriginDebug_{0}.zip'.format(self.version)
            self.download_file('builds/{0}/debug/{1}')
            self.copy_pdbs_in_place()
        except StopThread:
            print 'thread stopped DownloadDebugThread {0} {1}'.format(self.branch,self.version)
            self.stopped = True
        except Exception as exp:
            print 'thread error {0}'.format(exp)
            self.error_message = "Error:{0}".format(str(exp))
        finally:
            self.cleanup()

            
class OriginP4Progress(ReportHandler):
    def __init__(self,parent):
        P4OutputHandler.__init__(self)
        self.parent = parent
        self.count = 0
        self.total_packages = len(parent.folders_to_sync)
        
    def size_in_human_readable(self,num):
        #http://stackoverflow.com/questions/1094841/reusable-library-to-get-human-readable-version-of-file-size
        for x in ['bytes','KB','MB','GB','TB']:
            if num < 1024.0:
                return "%3.1f %s" % (num,x)
            num /= 1024.0
            
    def outputStat(self, stat):
        try:
            self.parent.check_exit()
        except StopThread:
            print "Cancelling Perforce Operation"
            return P4OutputHandler.CANCEL | P4OutputHandler.HANDLED
        if 'totalFileCount' in stat:
            self.count +=1
        #percent = int((1.0 - (float(self.fileSizeRemaining)/float(self.totalFileSize)))*100)
        if stat['action'] in ('added','updated'):
            percent = (float(self.count) / float(self.total_packages)) * 100
            pieces = stat['depotFile'].split('/')
            #myfile = "{0}/{1}".format(pieces[5],pieces[-1]) 
            
            self.parent.progress.update("Syncing Files {0}/{1} ({2})".format(self.count,self.total_packages,pieces[5] if pieces[4] == 'packages' else pieces[4]),(percent*0.89)+1,\
                "Syncing {0}{1}, Size: {2}".format(pieces[-1][:20],"..." if len(pieces[-1]) > 23 else "",self.size_in_human_readable(float(stat['fileSize']))))
        return P4OutputHandler.HANDLED
                
        
    def outputInfo(self, info):
        try:
            self.parent.check_exit()
        except StopThread:
            print "Cancelling Perforce Operation"
            return P4OutputHandler.CANCEL | P4OutputHandler.HANDLED
        if "file(s) up-to-date" in str(info):
            self.count +=1
        return P4OutputHandler.HANDLED

    def outputMessage(self, msg):
        return P4OutputHandler.HANDLED

        
class P4SyncThread(DebugTaskThread):
    DOWNLOAD_WEIGHT=0.01#just downloading commit information on this thread
    def __init__(self,branch,version,progress,downloader,username,password,workspace,port):
        DebugTaskThread.__init__(self,branch,version,progress,downloader)
        self.p4 = P4()
        self.p4.port = port
        self.p4.charset = P4_CHARSET
        self.p4.user=username
        self.p4.password=password
        self.p4.client =workspace
        #my_progress = OriginP4Progress(self.progress)
        #self.p4.progress = my_progress
        self.folders_to_sync = []
        self.CL = 0
        self.package_versions = {}
        
    def find_latest_changelist(self):
        buildinfo = json.load(open(os.path.join(download_path,self.filename),'r'))
        for change in buildinfo['changeSet']['items']:
            if int(change['changeNumber']) > self.CL:
                self.CL = int(change['changeNumber'])
        if self.CL == 0:
            #since version numbers are based on CLs now we just have to take the version and derive the CL from it
            #assuming it happened during the last cycle (rollovers happen once every couple of years)
            try:
                latest = 0
                changes = self.p4.run_changes("-ssubmitted","-m 1")[0]
                latest = int(changes['change']) #latest known submitted changelist
                cycle = latest / ROLLOVER_NUMBER #we cycle. This is the number of cycles that have happened
                floor = cycle * ROLLOVER_NUMBER #CL should be above this number, this was the latest cycle
                self.CL = int(self.version.split('.')[-1]) + floor #this is what we think the CL should be
                if self.CL > latest:
                    self.CL -= ROLLOVER_NUMBER #rollover must've happened recently, this is just before the rollover
                print self.CL
            except Exception as exp:
                print exp
                raise StopThread()
        
    def find_package_versions(self):
        try:
            results = self.p4.run_print("//global_online/origin/{0}/OriginBootstrap/dev/masterconfig_versions.xml@{1}".format(self.branch.path,self.CL))[1]
        except:
            #Try the old file, if this also throws an exception, the sync will have to stop anyways
            results = self.p4.run_print("//global_online/origin/{0}/OriginBootstrap/dev/masterconfig.xml@{1}".format(self.branch.path,self.CL))[1]
        root = ET.fromstring(results)
        versions = root.findall('.//masterversions//package')
        self.package_versions = {}
        for version in versions:
            self.package_versions[version.attrib['name']] = version.attrib['version']
        dirs = self.p4.run_dirs("//global_online/origin/packages/*/*")
        for dir in dirs:
            splits = dir['dir'].split('/')
            if splits[-2] in self.package_versions:
                if self.package_versions[splits[-2]] == splits[-1]:
                    self.folders_to_sync.append("{0}/...@{1}".format(dir['dir'],self.CL))
            

    def sync(self):
        if len(self.folders_to_sync) == 0:
            raise Exception("No folders to sync") #This will sync everything to head
        callback = OriginP4Progress(self)
        print "Syncing ... {0}".format(self.folders_to_sync)
        self.p4.run_sync(*self.folders_to_sync,handler=callback)
        self.check_exit()
            
    def copy_pdbs(self):
        pdbs = []
        counter = 0
        self.progress.update("Checking for Qt pdbs",90,"")
        where = self.p4.run_where("//global_online/origin/packages/Qt/{0}/Qt.build".format(self.package_versions['Qt']))[0]['path']
        for dir in ('dist/qtbase/lib/','dist/qtbase/plugins'):
            try:
                for maindir,subdirs,subfiles in os.walk(os.path.join(os.path.dirname(where),dir)):
                    for filename in subfiles:
                        if '.pdb' in filename:
                            pdbs.append(os.path.join(maindir,filename))
            except Exception as exp:
                print "Could not load dir {0} {1}".format(dir,exp) 

        for subfile in pdbs:
            counter += 1
            self.progress.update("Extracting Qt pdbs {0}/{1}".format(counter,len(pdbs)),90+(10*(float(counter)/float(len(pdbs)))))
            try:
                robocopy(ORIGIN_INSTALL,subfile)
            except Exception as exp:
                print exp
                pass #throws an exception on identical file
            self.check_exit()
            
        
    def run(self):
        try:
            self.p4.connect()
            self.p4.run_login()
            self.filename = 'buildinfo-{0}.json'.format(self.version)
            self.download_file('builds/{0}/buildinfo/{1}')
            self.find_latest_changelist()
            self.progress.update("Parsing masterconfig for package versions")
            self.folders_to_sync.append("//global_online/origin/{0}/...@{1}".format(self.branch.path,self.CL))#Add the branch and CL to sync
            self.find_package_versions()#add remaining packages to sync, including Qt and others
            self.sync()
            self.copy_pdbs()
        except StopThread:
            print 'thread stopped P4SyncThread {0} {1}'.format(self.branch,self.version)
            self.stopped = True
        except Exception as exp:
            print 'thread error {0}'.format(exp)
            self.stopped = True
        finally:
            self.p4.disconnect()
            self.cleanup()

## Holds the progress bar and status label and provides an interface to update it
class Progress:
    padding = {'padx':'0.5m','pady':'0.5m'}
    def __init__(self,window,text):
        self.window = window
        self.default = text
        self.progress_bar = ttk.Progressbar(window)
        self.progress_info = Tkinter.StringVar(window,text)
        self.progress_label = Tkinter.Label(window,textvariable=self.progress_info,bg=SUBPANE)
        
    def update(self,status,percentage=-1):
        self.progress_info.set(status)
        if percentage != -1:
            self.progress_bar.config(value = percentage)
        
    def reset(self,stopped=False,error_message=''):
        if stopped:
            if error_message == '':
                self.progress_info.set("Error Occurred. {0}".format(self.default))
            else:
                self.progress_info.set("Error: {0}. {1}".format(error_message,self.default))
            self.progress_bar.config(value=0)
        else:
            self.progress_info.set("Completed Successfully. {0}".format(self.default))
            self.progress_bar.config(value=100)
        
    def placeatrow(self,row,colspan=1):
        self.progress_bar.grid(row=row,column=1,columnspan=colspan,sticky=Tkinter.EW,**self.padding)
        #self.window.rowconfigure(row,weight=1)
        self.progress_label.grid(row=(row+1),column=1,columnspan=colspan,sticky=Tkinter.W,**self.padding)
        #self.window.rowconfigure(row+1,weight=1)
        for i in range(1,self.window.grid_size()[0]):
            self.window.columnconfigure(i,weight=1)        
        for i in range(1,self.window.grid_size()[1]):
            self.window.rowconfigure(i,weight=1)

##Multi-line progress
class DoubleProgress(Progress):
    def __init__(self,window,text):
        Progress.__init__(self,window,text)
        self.progress_info_2 = Tkinter.StringVar(window,'')
        self.progress_label_2 = Tkinter.Label(window,textvariable=self.progress_info_2,bg=SUBPANE)
        
    def update(self,status,percentage=-1,status2=''):
        Progress.update(self,status,percentage)
        if self.progress_info_2 != '':
            self.progress_info_2.set(status2)
            
    def reset(self,stopped=False,error_message=''):
        Progress.reset(self,stopped,error_message)
        self.progress_info_2.set('')
        
    def placeatrow(self,row,colspan=2):
        self.progress_label_2.grid(row=(row+2),column=1,columnspan=colspan,sticky=Tkinter.W,**self.padding)
        Progress.placeatrow(self,row,colspan=2)
    
## Encapsulate logic of branch pathing
## (X branches have a different format)
class Branch(object):
    def __init__(self,branch):
        self.branch = self.path = branch
        if self.branch == 'X master':
            self.path = 'products/client/master'
            self.branch = 'master'
        elif self.branch.startswith('X '):
            branchname = self.branch.split(' ')[-1]
            self.path = 'products/client/releases/{0}'.format(branchname)
            self.branch = branchname
            #TODO add X releases using this format
    
    def __repr__(self):
        return self.branch
        
    def __str__(self):
        return self.branch
        
class ClientDownloader(object):
    p4_workspaces = []
    def __init__(self,root):
        self.root = root
        self.exit_me = False

        self.download_install_thread = None
        self.debug_thread = None
        self.p4_sync_thread = None
        
        #fonts for the labels
        heading_font = tkFont.Font(weight='bold')
        label_font = tkFont.Font(weight='bold',size=10)
        sublabel_font = tkFont.Font(weight='bold',size=8)

        #Presetup Querying S3
        self.branches = ['X master','X 10.0','DL','RL','HL']

        #Possible 
        self.p4_ports = [P4_PORT_EARS,P4_PORT_EAC]

        #Title and window
        self.window = Tkinter.Frame(root,width=700,height=1000)
        self.label = Tkinter.Label(self.window, text="Origin Client Debug Helper",font=heading_font)
        
        #Version and label
        self.version_label = Tkinter.Label(self.window,text="Version",font=label_font)
        self.version = Tkinter.Entry(self.window)
        self.version.bind("<KeyRelease>",func=self.validate_inputs)

        
        #buttons - both shouldn't be enabled at the same time
        self.download_button = Tkinter.Button(self.window, text='Download',command = self.download_button_click)
        self.stop_button = Tkinter.Button(self.window,text='Stop', command = self.stop_button_click,state=Tkinter.DISABLED)
        
        pane_style = {"bg":SUBPANE,"borderwidth":1,"relief":"solid"}
        #panes
        self.debug_pane = Tkinter.Frame(self.window,**pane_style)
        self.download_install_pane = Tkinter.Frame(self.window,**pane_style)
        self.perforce_pane = Tkinter.Frame(self.window,**pane_style)
        
        #progress indicator
        #info is used everywhere and therefore needs to be easily settable
        self.progress = {}
        self.debug_progress = Progress(self.debug_pane,"Ready to download.")
        self.download_install_progress = Progress(self.download_install_pane,"Ready to download.")
        self.perforce_progress = DoubleProgress(self.perforce_pane,"Ready to sync.")

        #checkboxes
        self.debug_enabled = Tkinter.IntVar()
        self.debug_checkbox = Tkinter.Checkbutton(self.debug_pane,text="Download pdbs and put in place",bg=SUBPANE,command=self.validate_inputs,variable=self.debug_enabled)
        self.debug_checkbox.select()
        self.download_install_enabled = Tkinter.IntVar()
        self.download_install_checkbox = Tkinter.Checkbutton(self.download_install_pane,text="Download and install client",bg=SUBPANE,command=self.validate_inputs,variable=self.download_install_enabled)
        self.download_install_checkbox.select()
        self.perforce_enabled = Tkinter.IntVar()
        self.perforce_checkbox = Tkinter.Checkbutton(self.perforce_pane,text="Sync Perforce to changelist",bg=SUBPANE,command=self.validate_inputs,variable = self.perforce_enabled)
        self.perforce_checkbox.select()
        
        ##Perforce stuff
        #Perforce controls
        p4_username = Tkinter.StringVar(self.perforce_pane,"{0}{1}{2}".format(os.environ['USERDOMAIN'],"\\",os.environ['USERNAME']))
        self.computername = os.environ["COMPUTERNAME"]
        self.perforce_server_label=Tkinter.Label(self.perforce_pane,text="Server",font=sublabel_font,bg=SUBPANE)
        self.perforce_server_value=ttk.Combobox(self.perforce_pane,values=self.p4_ports)
        self.perforce_server_value.current(0)
        self.perforce_username_label=Tkinter.Label(self.perforce_pane,text="Username",font=sublabel_font,bg=SUBPANE)
        self.perforce_username_value=Tkinter.Entry(self.perforce_pane,textvariable=p4_username)
        self.perforce_password_label=Tkinter.Label(self.perforce_pane,text="Password",font=sublabel_font,bg=SUBPANE)
        self.perforce_password_value=Tkinter.Entry(self.perforce_pane,show="*")
        self.perforce_workspace_label=Tkinter.Label(self.perforce_pane,text="Workspace",font=sublabel_font,bg=SUBPANE)
        self.perforce_workspace_value=ttk.Combobox(self.perforce_pane,values=self.p4_workspaces)
        self.perforce_workspace_value.bind('<<ComboboxSelected>>',func=self.validate_inputs)

        #Perforce APIs
        self.p4 = P4()
        self.p4.port = P4_PORT_EARS #Works for EAC/EARS, just slower
        self.p4.charset = P4_CHARSET
        self.p4.user = P4_DEFAULT_USERNAME
        self.p4.password = P4_DEFAULT_PASSWORD
        self.update_p4_workspaces()
        
        #branch dropdown
        self.branch_label = Tkinter.Label(self.window,text="Branch",font=label_font)
        self.branch_list = ttk.Combobox(self.window,values = self.branches)
        self.branch_list.current(0)
        
        #default padding
        padding = {'padx':'0.5m','pady':'0.5m'}
        #adding the controls to the window
        self.label.grid(row=0,column=1,columnspan=2,sticky=Tkinter.NSEW,**padding)
        self.branch_label.grid(row=1,column=1,sticky=Tkinter.W,**padding)
        self.branch_list.grid(row=1,column=2,sticky=Tkinter.W,**padding)
        self.version_label.grid(row=2,column=1,sticky=Tkinter.W,**padding)
        self.version.grid(row=2,column=2,sticky=Tkinter.W,**padding)

        #panes
        #download install pane
        self.download_install_pane.grid(row=3,column=1,columnspan=2,sticky=Tkinter.NSEW,**padding)
        self.download_install_checkbox.grid(row=0,column=1,sticky=Tkinter.NS+Tkinter.W,**padding)        
        self.download_install_progress.placeatrow(1)
        #debug pane
        self.debug_pane.grid(row=4,column=1,columnspan=2,sticky=Tkinter.NSEW,**padding)
        self.debug_checkbox.grid(row=0,column=1,sticky=Tkinter.NS+Tkinter.W,**padding)        
        self.debug_progress.placeatrow(1)
        #perforce pane
        self.perforce_pane.grid(row=5,column=1,columnspan=2,sticky=Tkinter.NSEW,**padding)
        self.perforce_checkbox.grid(row=0,column=1,columnspan=2,sticky=Tkinter.NS+Tkinter.W,**padding)
        self.perforce_server_label.grid(row=1,column=1,sticky=Tkinter.W,**padding)
        self.perforce_server_value.grid(row=1,column=2,sticky=Tkinter.W,**padding)
        self.perforce_username_label.grid(row=2,column=1,sticky=Tkinter.W,**padding)
        self.perforce_username_value.grid(row=2,column=2,sticky=Tkinter.W,**padding)
        self.perforce_username_value.bind("<KeyRelease>",func=self.validate_inputs)
        self.perforce_password_label.grid(row=3,column=1,sticky=Tkinter.W,**padding)
        self.perforce_password_value.grid(row=3,column=2,sticky=Tkinter.W,**padding)
        self.perforce_password_value.bind("<KeyRelease>",func=self.validate_inputs)
        self.perforce_workspace_label.grid(row=4,column=1,sticky=Tkinter.W,**padding)
        self.perforce_workspace_value.grid(row=4,column=2,sticky=Tkinter.W,**padding)
        self.perforce_workspace_value.bind("<KeyRelease>",func=self.validate_inputs)
        self.perforce_progress.placeatrow(5,colspan=2)
        #buttons
        self.download_button.grid(row=6,column=1,sticky=Tkinter.W,padx='2m',pady='1m')
        self.stop_button.grid(row=6,column=2,sticky=Tkinter.W,padx='2m',pady='1m')
        #creating the window
        self.load_settings()
        self.validate_inputs()
        for i in range(1,self.window.grid_size()[0]):
            self.window.columnconfigure(i,weight=1)        
        for i in range(1,self.window.grid_size()[1]):
            self.window.rowconfigure(i,weight=1)
        self.window.grid(sticky=Tkinter.NSEW)
        

    def get_active_threads(self):
        active_threads = []
        if self.download_install_thread != None:
            active_threads.append(self.download_install_thread)
        if self.debug_thread != None:
            active_threads.append(self.debug_thread)
        if self.p4_sync_thread != None:
            active_threads.append(self.p4_sync_thread)
        return active_threads
        
    def update_p4_workspaces(self):
        try:
            self.p4.connect()
            self.p4.run_login()
            clients = self.p4.run_clients("-u",self.perforce_username_value.get())
            #current = self.perforce_workspace_value.current()
            self.p4_workspaces = []
            for client in clients:
                if self.computername == client["Host"]:
                    self.p4_workspaces.append(client['client'])
            self.perforce_workspace_value['values'] = self.p4_workspaces
            #self.perforce_workspace_value.current(0)
        finally:
            self.p4.disconnect()
            
            
    #Validates the inputs, enables/disables download stop buttons
    def validate_inputs(self,evt=None):
        if self._download_enabled():
            self.download_button.config(state=Tkinter.NORMAL)
        else:
            self.download_button.config(state=Tkinter.DISABLED)
            
        if len(self.get_active_threads()) > 0:
            self.stop_button.config(state=Tkinter.NORMAL)
        else:
            self.stop_button.config(state=Tkinter.DISABLED)
                    
    def _download_enabled(self):
        #make sure main inputs are valid first
        if self.version.get().count('.') != 3:
            return False
        
        #Check that at least one checkbox is checked that is not active:
        active_checkboxes = 0
        if self.debug_enabled.get() == 1 and not self.debug_thread:
            active_checkboxes += 1
        if self.download_install_enabled.get() == 1 and not self.download_install_thread:
            active_checkboxes += 1
        if self.perforce_enabled.get() == 1 and not self.p4_sync_thread:
            active_checkboxes += 1
        if active_checkboxes == 0:
            return False
            
        #If the perforce checkbox is checked verify the Perforce textviews
        if self.perforce_enabled.get() == 1:
            if len(self.perforce_username_value.get()) == 0:
                return False
            if len(self.perforce_password_value.get()) == 0:
                return False
            if len(self.perforce_workspace_value.get()) == 0:
                return False

        return True

    selected_branch = property(lambda self: self.branch_list.get())
    

    ## Gets called when the download button is clicked
    def download_button_click(self):
        active_threads = self.get_active_threads()
        if len(active_threads) == 3:
            raise Exception("All Downloads already running")

        mybranch = Branch(self.selected_branch)
        myversion = self.version.get()
            
        if not self.debug_thread and self.debug_enabled.get() == 1:
            self.debug_thread = DownloadDebugThread(mybranch,myversion,self.debug_progress,self)            
            self.debug_thread.start()
        
        if not self.download_install_thread and self.download_install_enabled.get() == 1:
            self.download_install_thread = DownloadClientThread(mybranch,myversion,self.download_install_progress,self)
            self.download_install_thread.start()
            
        if not self.p4_sync_thread and self.perforce_enabled.get() == 1:
            self.p4_sync_thread = P4SyncThread(mybranch,myversion,self.perforce_progress,self,self.perforce_username_value.get(),self.perforce_password_value.get(),self.perforce_workspace_value.get(), self.perforce_server_value.get())
            self.p4_sync_thread.start()

        self.save_settings()
        self.validate_inputs()

    ## Gets called when the stop button is clicked    
    def stop_button_click(self):
        if self.debug_thread:
            self.debug_thread.stop_me = True
        if self.download_install_thread:
            self.download_install_thread.stop_me = True
        if self.p4_sync_thread:
            self.p4_sync_thread.stop_me = True
        
        self.save_settings()
        self.validate_inputs()

    def load_settings(self):
        try:
            if os.path.exists(setting_path):
                saved_settings = json.load(open(setting_path,'r'))
                if saved_settings[SETTING_CHECK_DEBUG] == 1:
                    self.debug_checkbox.select()
                else:
                    self.debug_checkbox.deselect()
                if saved_settings[SETTING_CHECK_INSTALL] == 1:
                    self.download_install_checkbox.select()
                else:
                    self.download_install_checkbox.deselect()
                if saved_settings[SETTING_CHECK_P4] == 1:
                    self.perforce_checkbox.select()
                else:
                    self.perforce_checkbox.deselect()
                self.version.delete(0, Tkinter.END)
                self.version.insert(Tkinter.END,saved_settings[SETTING_VERSION])
                self.branch_list.set(saved_settings[SETTING_BRANCH])
                self.perforce_username_value.delete(0, Tkinter.END)
                self.perforce_username_value.insert(Tkinter.END,saved_settings[SETTING_P4_USERNAME])
                self.perforce_password_value.delete(0, Tkinter.END)
                self.perforce_password_value.insert(Tkinter.END,base64.b64decode(saved_settings[SETTING_P4_PASSWORD]))
                self.perforce_workspace_value.set(saved_settings[SETTING_P4_WORKSPACE])
                self.perforce_server_value.set(saved_settings[SETTING_P4_PORT])

                self.version.delete(0, Tkinter.END)
                self.version.insert(Tkinter.END,saved_settings[SETTING_VERSION])
                
        except Exception as exp:
            print "Exception loading settings: {0}".format(exp)
    def save_settings(self):
        saved_settings = {}
        saved_settings[SETTING_CHECK_DEBUG] = self.debug_enabled.get()
        saved_settings[SETTING_CHECK_INSTALL] = self.download_install_enabled.get()
        saved_settings[SETTING_CHECK_P4] = self.perforce_enabled.get()
        saved_settings[SETTING_P4_USERNAME] = self.perforce_username_value.get()
        saved_settings[SETTING_P4_PASSWORD] = base64.b64encode(self.perforce_password_value.get())
        saved_settings[SETTING_P4_WORKSPACE] = self.perforce_workspace_value.get()
        saved_settings[SETTING_BRANCH] = self.branch_list.get()
        saved_settings[SETTING_VERSION] = self.version.get()
        saved_settings[SETTING_P4_PORT] = self.perforce_server_value.get()
        open(setting_path,'w').write(json.dumps(saved_settings))
        
    ## Cleans download thread
    # Should only be called from the thread itself
    def clean_thread(self,thread):
        print "Cleaning Thread"
        if isinstance(thread,DownloadDebugThread):
            print "Debug Thread Finished, Cleaning"
            self.debug_thread = None
            self.debug_progress.reset(thread.stopped,thread.error_message)
        if isinstance(thread,DownloadClientThread):
            print "Download Install Thread Finished, Cleaning"
            self.download_install_thread = None
            self.download_install_progress.reset(thread.stopped,thread.error_message)
        if isinstance(thread,P4SyncThread):
            print "P4 Sync Thread Finished, Cleaning"
            self.p4_sync_thread = None
            self.perforce_progress.reset(thread.stopped,thread.error_message)
        
        self.validate_inputs()
            
def main():
    #validate that this person has access
    try:
        myvalidate = get_bucket().get_key("builds/DL/latest.txt")
    except Exception:
        tkMessageBox.showerror("Cannot Connect","Cannot connect, it is possible your location has no access to the builds. Please contact OriginClientAutomationCore@ea.com for details.")
        return

    root = Tkinter.Tk()
    root.resizable(width=False, height=False)
    root.minsize(width=300,height=500)
    root.grid_columnconfigure(0,weight=1)
    root.columnconfigure(0,weight=1)
    #initalize the downloader
    downloader = ClientDownloader(root)
    #now the downloader is running!
    def cleanup():
        downloader.exit_me = True
        try:
            root.destroy()
        except:
            pass
    root.createcommand('exit',cleanup)
    root.mainloop()
    #the threads watch this, if it becomes true the threads close themselves
    cleanup()
    
if __name__ == '__main__':
    main()
    if debug:
        output_thread.stop_me = True