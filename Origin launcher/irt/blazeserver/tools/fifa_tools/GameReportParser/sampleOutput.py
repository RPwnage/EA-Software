#!/usr/bin/env python
import sys
import cStringIO
import xml.etree.ElementTree as xml
sys.path.append('./')
import ParseSingleReport
from ParseSingleReport import *

def cleanResult(element):
    result = None
    if element is not None:
        result = element.text
        result = result.strip()
    else:
        result = ""
    return result
def process(val):
    #file_name = os.getenv('map_input_file')
    #file_name='fifa-2015-ps4_blaze_coreSlave28_gamereporting_20150325_215209-134.log'
    file_name ='fifa-2016-ps4_blaze_grSlave7_gamereporting_20150727_235959-999'
    titlePlatformObj = ExtractTitlePlatfromFromFilename(file_name)
    returnval = ("%s") % (ParseReport(val, titlePlatformObj.title, titlePlatformObj.platform))	
    return returnval.strip()

if __name__ == '__main__':
    buff = None
    intext = False
    for line in sys.stdin:        
        if line.find("<event ") != -1:
            intext = True
            buff = cStringIO.StringIO()
            buff.write(line)
        elif line.find("</event>") != -1:
            intext = False
            buff.write(line)
            val = buff.getvalue()
            buff.close()
            buff = None
            print process(val)
        else:
            if intext:
                buff.write(line)
