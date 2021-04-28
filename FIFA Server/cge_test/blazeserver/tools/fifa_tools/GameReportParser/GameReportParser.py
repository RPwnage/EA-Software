#Scripts to parse game reports from Blaze server log
#Alan Poon apoon@ea.com

import os
import os.path
import re
import sys
import fileinput
import string
import xml.etree.ElementTree as ET
import gzip
import datetime

from cStringIO import StringIO

import FIFA15GameReportAttributes
from FIFA15GameReportAttributes import *

import FIFA16GameReportAttributes
from FIFA16GameReportAttributes import *

import FIFA17GameReportAttributes
from FIFA17GameReportAttributes import *

import FIFA18GameReportAttributes
from FIFA18GameReportAttributes import *

import FIFA19GameReportAttributes
from FIFA19GameReportAttributes import *

import FIFA20GameReportAttributes
from FIFA20GameReportAttributes import *

import FIFA21GameReportAttributes
from FIFA21GameReportAttributes import *

from GameReportHelpers import *
     	            
def PrepareCSVHeader(FP):
    FP.write("EventTime,ReportId,NucluesId,PersonaId,SKU,TitleId,GameType,GameTypeRaw,GameTime,EndGameReason\n")

def FastParseXML(platform, title, xmlFile, dirAndFile, outputMode):
    print "PrepareXML "
    #remove all the eof from the raw file     
    xmlString = StringIO()
    xmlTmpString = StringIO()           

    raw = open (xmlFile, "rb")
    
    data = raw.read(1)
    pos=0
    while data != "":
        pos=pos+1
        if(hex(ord(data)) == '0x1a'):            
            xmlString.write("a")
            print hex(ord(data)), pos
        else:
            xmlString.write(data)
        data = raw.read(1)
    raw.close()    
    
    # move the xmlString cursor to the beginning for reading 
    xmlString.seek(0)
    
    #read back from xmlString, and remove the PrivatePlayerReport from it, and write back to xmlFile    
    lines = xmlString.readlines()    
    xmlString.close()
    
    FP = open(dirAndFile.outputDir + "/" + dirAndFile.outputFileName, 'a')
    if(outputMode == "CSV"):
        PrepareCSVHeader(FP)
    
    #remove privateplayerreport node from xml
    lineCount = 0
    skipWrite = 0
    nodeCount = 0
    reportNodeTotal = 0
    preserverPrivateAttribute = 0
    for line in lines:
        if (lineCount == 0):
		    xmlTmpString.write("<rawgamereport>\n"+line)            
        else:
            if(skipWrite == 0):
                if (line.find("<gametype>") != -1):
                    tmp = line.strip("<gametype>")
                    gameType = tmp.strip("</gametype>")                    
                    preserverPrivateAttribute = IsPreservePrivateAttribute(gameType)
                
                if (line.find("<privateattributemap>") != -1):                   
                    if(preserverPrivateAttribute == 0):
                        skipWrite=1
                else:
                    xmlTmpString.write(line)                  
            else:
                if (line.find("</privateattributemap>") != -1):
                  skipWrite=0
        if (line.find("</event>") != -1):
            nodeCount+=1
            reportNodeTotal+=1
            
        # Parse in chunk
        if (nodeCount == 1):            
            xmlTmpString.write("</rawgamereport>")            
            xmlTmpString.seek(0)            
            DoParsing(xmlTmpString, title, platform, FP, outputMode)
            xmlTmpString.truncate(0)         
            xmlTmpString.write("<rawgamereport>\n")
            nodeCount = 0            
        lineCount=lineCount+1
    print "lines: ",lineCount
    
    # Parse the left over
    if(nodeCount > 0 and nodeCount < 500):
        xmlTmpString.write("</rawgamereport>")
        xmlTmpString.seek(0)        
        DoParsing(xmlTmpString, title, platform, FP, outputMode)
	
    FP.close()    
    print "Parsed - " + str(reportNodeTotal)
    xmlTmpString.close()
    
def DoParsing(xmlString, title, platform, FP, outputMode):
    #parse the xmlTmpString
    preparedXmlContents = xmlString.getvalue()
        
    root = ET.fromstring(preparedXmlContents);      
    moduleName = getFIFA15GameReportAttributesModuleName()
    if(title == "FIFA16"):
        moduleName = getFIFA16GameReportAttributesModuleName()
    elif(title == "FIFA17"):
        moduleName = getFIFA17GameReportAttributesModuleName()
    elif(title == "FIFA18"):
        moduleName = getFIFA18GameReportAttributesModuleName()
    elif(title == "FIFA19"):
        moduleName = getFIFA19GameReportAttributesModuleName()
    elif(title == "FIFA20"):
        moduleName = getFIFA20GameReportAttributesModuleName()
    elif(title == "FIFA21"):
        moduleName = getFIFA21GameReportAttributesModuleName()
    
    #print "title: ", title, "moduleName: ", moduleName    
    moduleObject = sys.modules[moduleName]        
    parserClass = getattr(moduleObject, "TitleSpecificParser")
    parserObject = parserClass(platform, outputMode)
    
    parserClass.ParseAndWriteToFile(parserObject, root, FP)
    #print "reportnodes: ", parserObject.numReportNode
    
def ParseGameReports (platform, title, logFiles, dirAndFile, outputMode):
    #print "ParseGameReports"
    lastFetchTimeStamp = dirAndFile.lastFetchTimeStamp
    year = str(datetime.datetime.now().year) 
    logInfo = ""
    for file in logFiles:       
        if(file.endswith(".gz") and file.find(year) != -1):
            xmlFile = dirAndFile.rsyncDir+"/"+string.replace(file, ".gz", "")            
            dirAndFile.PopulateOutputFilename(file)
            
            if not (os.path.exists(xmlFile)):
                #unzip it into raw ascii xml file
                if(os.sys.platform == "win32"):
                    DoCommand0("7z e -o" + dirAndFile.rsyncDir + " " + dirAndFile.rsyncDir+"/" + file)
                else:
                    DoCommand0("gzip -d " + dirAndFile.rsyncDir+"/"+file + " " + dirAndFile.rsyncDir+"/")  
           
            FastParseXML(platform, title, xmlFile, dirAndFile, outputMode)
            #os.remove(xmlFile)
        
            # do compression
            rawTSV = open(dirAndFile.outputDir + "/" + dirAndFile.outputFileName, 'rb')
            compressedTSV = gzip.open(dirAndFile.outputDir + "/" + dirAndFile.compressedOutputFileName, 'wb')
            compressedTSV.writelines(rawTSV)
            compressedTSV.close()
            rawTSV.close()
            #os.remove(dirAndFile.outputDir + "/" + dirAndFile.outputFileName)
            
            # write out the log
            logInfo += os.path.abspath(dirAndFile.outputDir + "/" + dirAndFile.compressedOutputFileName) + "\n"
            
            #update timestamp
            time = ExtractTimeFromFileName(file)
            if(time > lastFetchTimeStamp):
                lastFetchTimeStamp = time
                timeStampFilename = dirAndFile.rsyncDir + "/timeStamp.txt"
                timeStampFP = open (timeStampFilename, 'w')
                timeStampFP.write(str(lastFetchTimeStamp))
                timeStampFP.close()                
    return logInfo
    
class DirectoryAndFile:
    platform=""
    title =""
    rsyncDir=""    
    outputDir=""
    outputFileName=""
    compressedOutputFileName = ""
    logDir=""
    logFileName=""
    logTimeStamp="20140707"
    lastFetchTimeStamp=20100101000000000
    outputMode=""
    
    def __init__(self, platform, title, rsyncDir, outputDir, outputMode="EADP"):
        self.platform=platform
        self.title=title
        self.outputDir = outputDir        
        self.rsyncDir = rsyncDir  
        self.logDir = self.outputDir + "/logs/"
        self.outputMode = outputMode
        CreateDirIfNotExist(self.outputDir)
        #print self.rsyncDir
 
    def PopulateOutputFilename(self, filename):
        year = str(datetime.datetime.now().year)        
        
        if(filename.endswith(".gz") != -1):
            #print filename
            filenamenoExt = string.replace(filename, ".log.gz", "")
            tmp = filenamenoExt.split(year)
            tag = string.replace(tmp[0], "blaze_", "")
            self.logFileName = self.title + "-" + self.platform + "-gamereports-"
            self.outputFileName = year + "_"
            
            suffix = string.replace(tmp[1], "_", "")
            suffix = string.replace(suffix, "-", "")
                        
            length = len(suffix)
            for i in range(0,length):        
                if(i < length-3):
                    if((i % 2) == 0):
                        self.outputFileName += suffix[i]
                    else:
                        self.outputFileName += suffix[i] + "_"
                else:
                    self.outputFileName += suffix[i]                    
            if(outputMode == "CSV"):
                self.outputFileName += ".csv"
            else:
                self.outputFileName += ".tsv"
            self.compressedOutputFileName = self.outputFileName + ".gz"
    
    def GenerateLogFilename(self):
        self.logDir += datetime.datetime.now().strftime('%Y%m%d')
        logTimeStamp = datetime.datetime.now().strftime('%Y%m%d%H%M%S000.complete.log')
        self.logFileName +=logTimeStamp
        CreateDirIfNotExist(self.logDir)
#
#entry point of the script
#
srcLogDir = sys.argv[1]
outputDir = sys.argv[2]
platform = sys.argv[3]
title = sys.argv[4]
outputMode = sys.argv[5]

dirAndFile = DirectoryAndFile(platform, title, srcLogDir, outputDir, outputMode)

# Get the new log files from Blaze
# return the list of new log files
dirAndFile.lastFetchTimeStamp = FindLastFetchLogTime(srcLogDir)
dirAndFile.lastFetchTimeStamp = 201201010000000

print srcLogDir + " " +str(dirAndFile.lastFetchTimeStamp) +"\n"

newLogFiles = LogFilenamesToInterpret(srcLogDir, dirAndFile.lastFetchTimeStamp)

# prepare and parse through the new log files
# output the csv data
count = 0
for newLog in newLogFiles:
    count = count +1

#print "files found: " + str(count)

if (count >0):
    sLogInfo=os.path.abspath(dirAndFile.outputDir) + " ./\n"
    sLogInfo += ParseGameReports(platform, title, newLogFiles, dirAndFile, outputMode)
    
    dirAndFile.GenerateLogFilename()
    logFile = open(dirAndFile.logDir + "/" + dirAndFile.logFileName, 'a')
    logFile.write(sLogInfo)
    logFile.close()



       
