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

sys.path.append('./')
import FIFA15GameReportAttributes
from FIFA15GameReportAttributes import *

sys.path.append('./')
import FIFA16GameReportAttributes
from FIFA16GameReportAttributes import *

import FIFA17GameReportAttributes
from FIFA17GameReportAttributes import *

from GameReportHelpers import *

class TitlePlatformStruct:
    title = ""
    platform = ""
    
    def __init__(self, title, platform):
        self.title = title
        self.platform = platform

def ParseReport(rawXmlString, title, platform):
    #print "PrepareXML "
    #remove all the eof from the raw file     
    xmlString = StringIO()
    xmlTmpString = StringIO()           
   
    pos=0
    dataLength = len(rawXmlString)
    while (pos < dataLength):               
        data = rawXmlString[pos] 
        if(hex(ord(data)) == '0x1a'):            
            xmlString.write("a")
            #print hex(ord(data)), pos
        else:
            xmlString.write(data)
        pos=pos+1
    
    # move the xmlString cursor to the beginning for reading 
    xmlString.seek(0)
    
    #read back from xmlString, and remove the PrivatePlayerReport from it, and write back to xmlFile    
    lines = xmlString.readlines()    
    xmlString.close()
    
    #remmove privateplayerreport node from xml
    skipWrite = 0        
    for line in lines:        
        if(skipWrite == 0):
            if (line.find("<privateplayerreport>") != -1):
              skipWrite=1
            else:
              xmlTmpString.write(line)                  
        else:
            if (line.find("</privateplayerreport>") != -1):
              skipWrite=0
      
    xmlTmpString.seek(0)     
    #Parse the report after clean up    
    reportString = _ParseReportInternal(xmlTmpString.getvalue(), title, platform)
    xmlTmpString.close()
    return reportString
    
def _ParseReportInternal(xmlString, title, platform):
    #parse the xmlTmpString
    #print "Parsing "        
    root = ET.fromstring(xmlString);      
    moduleName = getFIFA15GameReportAttributesModuleName()
    if(title == "FIFA2016"):
        moduleName = getFIFA16GameReportAttributesModuleName()
    elif(title == "FIFA2017"):
        moduleName = getFIFA17GameReportAttributesModuleName()
        
    moduleObject = sys.modules[moduleName]        
    parserClass = getattr(moduleObject, "TitleSpecificParser")
    parserObject = parserClass(platform)
    
    return parserClass.ParseReport(parserObject, root)

def ExtractTitlePlatfromFromFilename(filename):    
    
    # filename pattern follows fifa-<year>-<platform>_blaze_<slave><index>_gamereporting_<YYMMDD>_<HHmmmss>-<sss>.log
    nameSplitByDash = filename.split("-")
    title = "FIFA" + nameSplitByDash[1]
    nameSplitByUnderscore =  nameSplitByDash[2].split("_")
    platform = nameSplitByUnderscore[0]
    
    #print title + " " + platform
    return TitlePlatformStruct(title, platform)

#
#entry point of the script
#
srcLogDir = sys.argv[1]
filename = sys.argv[2]

titlePlatformObj = ExtractTitlePlatfromFromFilename(filename)

FP = open(srcLogDir + "/" + filename, "rb")
xmlData = FP.read()
print ParseReport(xmlData, titlePlatformObj.title, titlePlatformObj.platform)




       
